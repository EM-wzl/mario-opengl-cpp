#pragma once

// cnfont.h — 运行时生成中文字符位图的动态纹理图集。
// 不依赖任何外部图片或字体文件：Windows 下用系统 GDI（CreateFontW /
// DrawTextW / CreateDIBSection）把系统字体（微软雅黑等）光栅化为白字透明底的
// 32x32 RGBA 位图，按需经 glTexSubImage2D 上传到一张 1024x1024 的 GL_RGBA8 纹理。
// 非 Windows 平台 available() 返回 false，调用方据此跳过中文，不会崩溃。
#include "texture.h"  // 复用 UV 结构与 GL 类型

#include <cstdint>
#include <unordered_map>

class ChineseFont {
public:
    ChineseFont();
    ~ChineseFont();
    ChineseFont(const ChineseFont&) = delete;
    ChineseFont& operator=(const ChineseFont&) = delete;

    // 当前平台是否支持生成中文字形（仅 Windows 且 GDI 可用时为 true）。
    bool available() const noexcept { return available_; }

    // 动态图集纹理 id，供 renderer 在绘制中文时显式绑定。
    GLuint textureId() const noexcept { return texture_; }

    // 返回码点对应的图集 UV；首次调用时生成并缓存字形。
    // 平台不支持或图集已用尽时返回安全占位 UV（绘制后会被着色器丢弃）。
    UV glyph(std::uint32_t codepoint);

private:
    // 把一个码点光栅化到指定单元并上传到纹理，成功返回 true。
    bool rasterize(std::uint32_t codepoint, int slot);
    UV slotUV(int slot) const noexcept;

    static constexpr int atlasPixels = 1024;                // 纹理边长
    static constexpr int cellPixels = 32;                   // 单元边长
    static constexpr int cells = atlasPixels / cellPixels;  // 每行/列单元数 32
    static constexpr int maxGlyphs = cells * cells;         // 最多 1024 个字形

    GLuint texture_ = 0;
    bool available_ = false;
    int nextSlot_ = 0;
    std::unordered_map<std::uint32_t, UV> cache_;
};
