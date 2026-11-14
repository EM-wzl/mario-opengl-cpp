#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "cnfont.h"

#include <cstddef>
#include <vector>

#ifdef _WIN32
#include <array>
#include <cstring>
#endif

namespace {
// 不可用时返回的安全占位 UV：零面积，采样结果透明，会被片段着色器丢弃。
constexpr UV kPlaceholderUV{0.0F, 0.0F, 0.0F, 0.0F};
}  // namespace

ChineseFont::ChineseFont() {
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // 初始清零：整张动态图集分配为全 0（完全透明），未使用单元不会显示内容。
    const std::vector<GLubyte> blank(
        static_cast<std::size_t>(atlasPixels) * static_cast<std::size_t>(atlasPixels) * 4U, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, atlasPixels, atlasPixels, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, blank.data());
    glBindTexture(GL_TEXTURE_2D, 0);
#ifdef _WIN32
    available_ = true;
#else
    available_ = false;
#endif
}

ChineseFont::~ChineseFont() {
    if (texture_ != 0) glDeleteTextures(1, &texture_);
}

UV ChineseFont::glyph(std::uint32_t codepoint) {
    if (!available_) return kPlaceholderUV;
    const auto cached = cache_.find(codepoint);
    if (cached != cache_.end()) return cached->second;
    if (nextSlot_ >= maxGlyphs) return kPlaceholderUV;  // 图集已用尽
    const int slot = nextSlot_;
    if (!rasterize(codepoint, slot)) return kPlaceholderUV;
    const UV uv = slotUV(slot);
    cache_.emplace(codepoint, uv);
    ++nextSlot_;
    return uv;
}

UV ChineseFont::slotUV(int slot) const noexcept {
    const int col = slot % cells;
    const int row = slot / cells;
    const float x = static_cast<float>(col * cellPixels);
    const float y = static_cast<float>(row * cellPixels);
    const float extent = static_cast<float>(atlasPixels);
    const float edge = static_cast<float>(cellPixels);
    return {x / extent, y / extent, (x + edge) / extent, (y + edge) / extent};
}

#ifdef _WIN32
bool ChineseFont::rasterize(std::uint32_t codepoint, int slot) {
    // 32x32、32bpp 的 top-down DIB（biHeight 为负 => 第一行位于左上角）。
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = static_cast<DWORD>(sizeof(BITMAPINFOHEADER));
    bmi.bmiHeader.biWidth = cellPixels;
    bmi.bmiHeader.biHeight = -cellPixels;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC screenDC = GetDC(nullptr);
    if (screenDC == nullptr) return false;

    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(screenDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (dib == nullptr || bits == nullptr) {
        if (dib != nullptr) DeleteObject(dib);
        ReleaseDC(nullptr, screenDC);
        return false;
    }
    // CreateDIBSection 不保证清零，显式清成透明黑。
    std::memset(bits, 0, static_cast<std::size_t>(cellPixels) * cellPixels * 4U);

    HDC memDC = CreateCompatibleDC(screenDC);
    if (memDC == nullptr) {
        DeleteObject(dib);
        ReleaseDC(nullptr, screenDC);
        return false;
    }
    HGDIOBJ oldBitmap = SelectObject(memDC, dib);

    // 使用系统字体（微软雅黑）；字体缺失时 GDI 字体映射器会按 DEFAULT_CHARSET
    // 自动回退到含中文字形的字体。字号略小于单元，给四周留出边距。
    HFONT font = CreateFontW(-28, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                             L"Microsoft YaHei");
    HGDIOBJ oldFont = (font != nullptr) ? SelectObject(memDC, font) : nullptr;
    SetBkMode(memDC, TRANSPARENT);
    SetTextColor(memDC, RGB(255, 255, 255));

    // 码点转 UTF-16：BMP 用 1 个 wchar_t，辅助平面用代理对。
    wchar_t text[3] = {};
    int length = 0;
    if (codepoint <= 0xFFFFU) {
        text[0] = static_cast<wchar_t>(codepoint);
        length = 1;
    } else {
        const std::uint32_t adjusted = codepoint - 0x10000U;
        text[0] = static_cast<wchar_t>(0xD800U + (adjusted >> 10));
        text[1] = static_cast<wchar_t>(0xDC00U + (adjusted & 0x3FFU));
        length = 2;
    }

    RECT rect{0, 0, cellPixels, cellPixels};
    DrawTextW(memDC, text, length, &rect,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    // GDI 在 32bpp DIB 上写的是 BGRA，但 alpha 通道不可靠；文字为白色，
    // 因此用 RGB 灰度作为 alpha，输出白字透明底的 RGBA 像素。
    std::array<std::uint8_t, cellPixels * cellPixels * 4> pixels{};
    const auto* src = static_cast<const std::uint8_t*>(bits);
    for (int i = 0; i < cellPixels * cellPixels; ++i) {
        const std::size_t offset = static_cast<std::size_t>(i) * 4U;
        const unsigned b = src[offset + 0];
        const unsigned g = src[offset + 1];
        const unsigned r = src[offset + 2];
        const auto gray = static_cast<std::uint8_t>((r * 77U + g * 150U + b * 29U) / 256U);
        pixels[offset + 0] = 255;
        pixels[offset + 1] = 255;
        pixels[offset + 2] = 255;
        pixels[offset + 3] = gray;
    }

    const int col = slot % cells;
    const int row = slot / cells;
    glBindTexture(GL_TEXTURE_2D, texture_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // glTexSubImage2D 是 OpenGL 1.1 核心函数，<GL/gl.h> 已声明、opengl32 直接导出，
    // 直接调用即可，无需经 glcore 的 wglGetProcAddress 加载。
    glTexSubImage2D(GL_TEXTURE_2D, 0, col * cellPixels, row * cellPixels,
                    cellPixels, cellPixels, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    // 还原并释放 GDI 对象。
    if (oldFont != nullptr) SelectObject(memDC, oldFont);
    SelectObject(memDC, oldBitmap);
    if (font != nullptr) DeleteObject(font);
    DeleteObject(dib);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screenDC);
    return true;
}
#else
bool ChineseFont::rasterize(std::uint32_t, int) {
    // 非 Windows 平台不生成字形，调用方通过 available() 跳过中文。
    return false;
}
#endif
