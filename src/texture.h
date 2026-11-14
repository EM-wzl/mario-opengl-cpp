#pragma once

#include "glcore.h"
#include <array>
#include <cstdint>
#include <unordered_map>

// 图集中的每一项都是 16×16 像素，绘制时可整数倍放大。
enum class Sprite {
    White = 0,
    Ground,
    GroundTop,
    Brick,
    Question,
    Used,
    Solid,
    PipeTopLeft,
    PipeTopRight,
    PipeLeft,
    PipeRight,
    Cloud,
    Hill,
    Bush,
    Coin0,
    Coin1,
    Coin2,
    Coin3,
    SmallIdle,
    SmallWalk0,
    SmallWalk1,
    SmallJump,
    BigIdle,
    BigWalk0,
    BigWalk1,
    BigJump,
    Goomba0,
    Goomba1,
    GoombaFlat,
    Koopa0,
    Koopa1,
    Shell,
    Piranha0,
    Piranha1,
    Mushroom,
    Flower,
    Fireball,
    Flag,
    Checkpoint,
    Star,
    CloudPlatform,
    Spring,
    Count
};

struct UV {
    float u0 = 0.0F;
    float v0 = 0.0F;
    float u1 = 1.0F;
    float v1 = 1.0F;
};

class TextureAtlas {
public:
    TextureAtlas();
    ~TextureAtlas();
    TextureAtlas(const TextureAtlas&) = delete;
    TextureAtlas& operator=(const TextureAtlas&) = delete;

    void bind() const;
    GLuint textureId() const { return texture_; }
    UV uv(Sprite sprite) const;
    UV glyphUV(char c) const;
    bool hasGlyph(char c) const;

private:
    static constexpr int tilePixels = 16;
    static constexpr int atlasTiles = 16;
    static constexpr int atlasPixels = tilePixels * atlasTiles;

    using Pixel = std::array<std::uint8_t, 4>;
    void putTile(int index, const std::array<Pixel, tilePixels * tilePixels>& tile);
    void generate();
    void generateGlyphs();

    GLuint texture_ = 0;
    std::array<std::uint8_t, atlasPixels * atlasPixels * 4> pixels_{};
    std::unordered_map<char, int> glyphs_;
    int nextFreeTile_ = static_cast<int>(Sprite::Count);
};
