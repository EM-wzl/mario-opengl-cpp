#include "renderer.h"

#include "config.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <glm/gtc/matrix_transform.hpp>

namespace {
constexpr const char* vertexShader = R"GLSL(#version 330 core
layout (location = 0) in vec2 aPosition;
layout (location = 1) in vec2 aUV;
layout (location = 2) in vec4 aColor;
uniform mat4 uViewProjection;
out vec2 vUV;
out vec4 vColor;
void main() {
    vUV = aUV;
    vColor = aColor;
    gl_Position = uViewProjection * vec4(aPosition, 0.0, 1.0);
}
)GLSL";

constexpr const char* fragmentShader = R"GLSL(#version 330 core
in vec2 vUV;
in vec4 vColor;
uniform sampler2D uAtlas;
out vec4 FragColor;
void main() {
    vec4 color = texture(uAtlas, vUV) * vColor;
    if (color.a < 0.03) discard;
    FragColor = color;
}
)GLSL";

// 文本推进系数：与原有 ASCII 位图字体保持一致。
constexpr float kSpaceAdvance = 0.65F;
constexpr float kAsciiAdvance = 0.75F;
// 中文按约 1.0 倍字号推进并绘制，与 ASCII 混排时宽度对齐。
constexpr float kCjkAdvance = 1.0F;
constexpr float kCjkScale = 1.0F;

struct Decoded {
    std::uint32_t codepoint;
    bool ascii;
};

// 从 text[i] 解析一个 UTF-8 码点并把 i 前进到下一个字符。
// 非法或不完整序列按单个字节跳过，保证不越界、不死循环。
Decoded decodeUtf8(std::string_view text, std::size_t& i) {
    const std::size_t length = text.size();
    const auto lead = static_cast<unsigned>(static_cast<unsigned char>(text[i]));
    if (lead < 0x80U) {
        ++i;
        return {static_cast<std::uint32_t>(lead), true};
    }
    const auto isContinuation = [&](std::size_t k) -> bool {
        return i + k < length &&
               (static_cast<unsigned>(static_cast<unsigned char>(text[i + k])) & 0xC0U) == 0x80U;
    };
    if ((lead & 0xE0U) == 0xC0U && isContinuation(1)) {
        const std::uint32_t cp =
            (static_cast<std::uint32_t>(lead & 0x1FU) << 6) |
            static_cast<std::uint32_t>(static_cast<unsigned char>(text[i + 1]) & 0x3FU);
        i += 2;
        return {cp, false};
    }
    if ((lead & 0xF0U) == 0xE0U && isContinuation(1) && isContinuation(2)) {
        const std::uint32_t cp =
            (static_cast<std::uint32_t>(lead & 0x0FU) << 12) |
            (static_cast<std::uint32_t>(static_cast<unsigned char>(text[i + 1]) & 0x3FU) << 6) |
            static_cast<std::uint32_t>(static_cast<unsigned char>(text[i + 2]) & 0x3FU);
        i += 3;
        return {cp, false};
    }
    if ((lead & 0xF8U) == 0xF0U && isContinuation(1) && isContinuation(2) && isContinuation(3)) {
        const std::uint32_t cp =
            (static_cast<std::uint32_t>(lead & 0x07U) << 18) |
            (static_cast<std::uint32_t>(static_cast<unsigned char>(text[i + 1]) & 0x3FU) << 12) |
            (static_cast<std::uint32_t>(static_cast<unsigned char>(text[i + 2]) & 0x3FU) << 6) |
            static_cast<std::uint32_t>(static_cast<unsigned char>(text[i + 3]) & 0x3FU);
        i += 4;
        return {cp, false};
    }
    // 非法/不完整：按单字节跳过，当作 ASCII 处理（查不到字形会被安全跳过）。
    ++i;
    return {static_cast<std::uint32_t>(lead), true};
}
}

Renderer::Renderer() {
    shader_ = std::make_unique<Shader>(vertexShader, fragmentShader);
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    const auto stride = static_cast<GLsizei>(sizeof(Vertex));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, uv)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, color)));
    glBindVertexArray(0);

    shader_->use();
    glUniform1i(glGetUniformLocation(shader_->id(), "uAtlas"), 0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    vertices_.reserve(4096);
    indices_.reserve(6144);
}

Renderer::~Renderer() {
    if (ebo_ != 0) glDeleteBuffers(1, &ebo_);
    if (vbo_ != 0) glDeleteBuffers(1, &vbo_);
    if (vao_ != 0) glDeleteVertexArrays(1, &vao_);
}

void Renderer::beginFrame(int windowWidth, int windowHeight) {
    flush();
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, windowWidth, windowHeight);
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);

    // 以最大整数无关的等比视口适配窗口，多余区域保留黑边。
    const float scale = std::min(static_cast<float>(windowWidth) / config::logicalWidth,
                                 static_cast<float>(windowHeight) / config::logicalHeight);
    const int viewportWidth = static_cast<int>(config::logicalWidth * scale);
    const int viewportHeight = static_cast<int>(config::logicalHeight * scale);
    const int viewportX = (windowWidth - viewportWidth) / 2;
    const int viewportY = (windowHeight - viewportHeight) / 2;
    glViewport(viewportX, viewportY, viewportWidth, viewportHeight);
    glEnable(GL_SCISSOR_TEST);
    glScissor(viewportX, viewportY, viewportWidth, viewportHeight);
    glClearColor(0.36F, 0.70F, 0.95F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
    setCamera(0.0F);
}

void Renderer::endFrame() {
    flush();
}

void Renderer::setCamera(float x, float y) {
    flush();
    const glm::mat4 projection = glm::ortho(0.0F, static_cast<float>(config::logicalWidth),
                                            static_cast<float>(config::logicalHeight), 0.0F);
    const glm::mat4 view = glm::translate(glm::mat4(1.0F), glm::vec3(-x, -y, 0.0F));
    shader_->use();
    shader_->setMat4("uViewProjection", projection * view);
}

void Renderer::pushQuad(float x, float y, float width, float height, const UV& uv,
                        bool flipX, const glm::vec4& color, GLuint texture) {
    // 0 表示使用主图集，归一化为真实纹理 id 后再与当前批次比较。
    const GLuint resolved = (texture == 0) ? atlas_.textureId() : texture;
    // 纹理切换或容量不足时先提交当前批次；flush 会清空并重置 pendingTexture_，
    // 故无论是否发生 flush，下面都会把本 quad 的纹理重新设为当前批次纹理。
    if (!vertices_.empty() && resolved != pendingTexture_) flush();
    if (vertices_.size() + 4 > 24576) flush();
    pendingTexture_ = resolved;
    const GLuint base = static_cast<GLuint>(vertices_.size());
    const float leftUV = flipX ? uv.u1 : uv.u0;
    const float rightUV = flipX ? uv.u0 : uv.u1;
    vertices_.push_back({{x, y}, {leftUV, uv.v0}, color});
    vertices_.push_back({{x + width, y}, {rightUV, uv.v0}, color});
    vertices_.push_back({{x + width, y + height}, {rightUV, uv.v1}, color});
    vertices_.push_back({{x, y + height}, {leftUV, uv.v1}, color});
    indices_.insert(indices_.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
}

void Renderer::drawSprite(Sprite sprite, float x, float y, float width, float height,
                          bool flipX, const glm::vec4& color) {
    pushQuad(x, y, width, height, atlas_.uv(sprite), flipX, color);
}

void Renderer::drawRect(float x, float y, float width, float height, const glm::vec4& color) {
    pushQuad(x, y, width, height, atlas_.uv(Sprite::White), false, color);
}

void Renderer::drawText(std::string_view text, float x, float y, float size,
                        const glm::vec4& color) {
    float cursor = x;
    std::size_t i = 0;
    while (i < text.size()) {
        const Decoded decoded = decodeUtf8(text, i);
        if (decoded.ascii) {
            // ASCII 字符沿用内置位图字体与原有间距。
            const char character = static_cast<char>(decoded.codepoint);
            if (character == ' ') {
                cursor += size * kSpaceAdvance;
                continue;
            }
            if (atlas_.hasGlyph(character)) {
                pushQuad(cursor, y, size, size, atlas_.glyphUV(character), false, color);
            }
            cursor += size * kAsciiAdvance;
        } else if (chinese_.available()) {
            // 非 ASCII 码点（中文等）：用动态中文图集绘制，宽度约 1.0 倍字号。
            pushQuad(cursor, y, size * kCjkScale, size, chinese_.glyph(decoded.codepoint),
                     false, color, chinese_.textureId());
            cursor += size * kCjkAdvance;
        } else {
            // 平台不支持中文：跳过字形但仍推进光标，保持与 textWidth 一致。
            cursor += size * kCjkAdvance;
        }
    }
}

float Renderer::textWidth(std::string_view text, float size) const {
    float width = 0.0F;
    std::size_t i = 0;
    while (i < text.size()) {
        const Decoded decoded = decodeUtf8(text, i);
        if (decoded.ascii) {
            width += size * (static_cast<char>(decoded.codepoint) == ' ' ? kSpaceAdvance : kAsciiAdvance);
        } else {
            width += size * kCjkAdvance;
        }
    }
    return width;
}

void Renderer::drawTextCentered(std::string_view text, float y, float size,
                                const glm::vec4& color) {
    drawText(text, (config::logicalWidth - textWidth(text, size)) * 0.5F, y, size, color);
}

void Renderer::flush() {
    if (vertices_.empty()) {
        pendingTexture_ = 0;
        return;
    }
    // 按当前批次纹理显式绑定；0 仅是空批次哨兵，正常绘制时不会为 0。
    const GLuint texture = (pendingTexture_ != 0) ? pendingTexture_ : atlas_.textureId();
    shader_->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices_.size() * sizeof(Vertex)),
                 vertices_.data(), GL_DYNAMIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices_.size() * sizeof(GLuint)),
                 indices_.data(), GL_DYNAMIC_DRAW);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    vertices_.clear();
    indices_.clear();
    pendingTexture_ = 0;
}
