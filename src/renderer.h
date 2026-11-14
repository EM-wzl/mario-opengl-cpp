#pragma once

#include "shader.h"
#include "texture.h"
#include "cnfont.h"

#include "glcore.h"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <string_view>
#include <vector>

class Renderer {
public:
    Renderer();
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void beginFrame(int windowWidth, int windowHeight);
    void endFrame();
    void setCamera(float x, float y = 0.0F);

    void drawSprite(Sprite sprite, float x, float y, float width, float height,
                    bool flipX = false, const glm::vec4& color = {1, 1, 1, 1});
    void drawRect(float x, float y, float width, float height, const glm::vec4& color);
    void drawText(std::string_view text, float x, float y, float size = 18.0F,
                  const glm::vec4& color = {1, 1, 1, 1});
    float textWidth(std::string_view text, float size = 18.0F) const;
    void drawTextCentered(std::string_view text, float y, float size,
                          const glm::vec4& color = {1, 1, 1, 1});

private:
    struct Vertex {
        glm::vec2 position;
        glm::vec2 uv;
        glm::vec4 color;
    };

    // texture = 0 表示使用主图集 atlas_；非 0 时为该批次显式指定的纹理 id。
    void pushQuad(float x, float y, float width, float height, const UV& uv,
                  bool flipX, const glm::vec4& color, GLuint texture = 0);
    void flush();

    std::unique_ptr<Shader> shader_;
    TextureAtlas atlas_;
    ChineseFont chinese_;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint ebo_ = 0;
    // 当前已累积批次所使用的纹理 id；0 表示空批次（flush 后重置）。
    GLuint pendingTexture_ = 0;
    std::vector<Vertex> vertices_;
    std::vector<GLuint> indices_;
};
