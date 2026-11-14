#include "texture.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <stdexcept>
#include <string_view>

namespace {
using Pixel = std::array<std::uint8_t, 4>;
constexpr Pixel transparent{0, 0, 0, 0};
constexpr Pixel white{255, 255, 255, 255};
constexpr Pixel black{35, 28, 38, 255};
constexpr Pixel red{222, 49, 45, 255};
constexpr Pixel darkRed{145, 34, 40, 255};
constexpr Pixel blue{45, 93, 190, 255};
constexpr Pixel skin{255, 193, 132, 255};
constexpr Pixel brown{142, 78, 43, 255};
constexpr Pixel darkBrown{86, 45, 36, 255};
constexpr Pixel yellow{255, 205, 55, 255};
constexpr Pixel green{48, 170, 67, 255};
constexpr Pixel darkGreen{25, 105, 52, 255};

struct Painter {
    std::array<Pixel, 256> data{};

    void set(int x, int y, Pixel color) {
        if (x >= 0 && x < 16 && y >= 0 && y < 16) {
            data[static_cast<std::size_t>(y * 16 + x)] = color;
        }
    }
    void rect(int x, int y, int w, int h, Pixel color) {
        for (int yy = y; yy < y + h; ++yy) {
            for (int xx = x; xx < x + w; ++xx) {
                set(xx, yy, color);
            }
        }
    }
    void border(int x, int y, int w, int h, Pixel color) {
        rect(x, y, w, 1, color);
        rect(x, y + h - 1, w, 1, color);
        rect(x, y, 1, h, color);
        rect(x + w - 1, y, 1, h, color);
    }
};

void drawPlayer(Painter& p, bool big, int pose) {
    const Pixel shirt = red;
    p.rect(4, 1, 7, 2, shirt);
    p.rect(3, 3, 10, 1, darkRed);
    p.rect(5, 4, 7, 4, skin);
    p.rect(4, 4, 2, 4, darkBrown);
    p.set(10, 5, black);
    p.rect(9, 7, 4, 1, darkBrown);
    p.rect(3, 8, 10, big ? 4 : 3, shirt);
    p.rect(5, 9, 6, big ? 5 : 4, blue);
    p.set(6, 10, yellow);
    p.set(10, 10, yellow);
    p.rect(1, 8, 3, 3, skin);
    p.rect(12, 8, 3, 3, skin);
    if (pose == 3) { // 跳跃：手脚张开
        p.rect(1, 6, 3, 3, skin);
        p.rect(3, 13, 4, 2, darkBrown);
        p.rect(10, 12, 4, 2, darkBrown);
    } else if (pose == 1) {
        p.rect(3, 13, 5, 2, darkBrown);
        p.rect(10, 12, 4, 3, darkBrown);
    } else if (pose == 2) {
        p.rect(2, 12, 5, 3, darkBrown);
        p.rect(9, 13, 5, 2, darkBrown);
    } else {
        p.rect(3, 13, 4, 2, darkBrown);
        p.rect(9, 13, 4, 2, darkBrown);
    }
}

void drawGoomba(Painter& p, int frame, bool flat) {
    if (flat) {
        p.rect(2, 11, 12, 3, brown);
        p.rect(1, 14, 6, 2, darkBrown);
        p.rect(9, 14, 6, 2, darkBrown);
        p.set(5, 12, white);
        p.set(10, 12, white);
        return;
    }
    p.rect(3, 3, 10, 8, brown);
    p.rect(2, 5, 12, 5, brown);
    p.rect(4, 2, 8, 1, darkBrown);
    p.rect(4, 6, 3, 4, white);
    p.rect(9, 6, 3, 4, white);
    p.set(6 + (frame == 1 ? -1 : 0), 8, black);
    p.set(9 + (frame == 1 ? 1 : 0), 8, black);
    p.rect(4, 10, 8, 2, skin);
    p.rect(1 + (frame == 1 ? 1 : 0), 12, 6, 3, darkBrown);
    p.rect(9, 12 + (frame == 1 ? -1 : 0), 6, 3, darkBrown);
}

void drawKoopa(Painter& p, int frame, bool shellOnly) {
    if (shellOnly) {
        p.rect(2, 5, 12, 9, darkGreen);
        p.rect(3, 4, 10, 2, green);
        p.rect(4, 6, 8, 6, green);
        p.rect(6, 7, 4, 4, yellow);
        p.rect(1, 13, 14, 2, skin);
        return;
    }
    p.rect(8, 1, 5, 6, green);
    p.rect(12, 3, 3, 3, skin);
    p.set(11, 2, black);
    p.rect(3, 5, 9, 9, darkGreen);
    p.rect(4, 6, 7, 6, green);
    p.rect(5, 7, 5, 4, yellow);
    p.rect(1, 12, 4, 3, skin);
    p.rect(8, 12 + frame, 4, 2, skin);
}

Painter paint(Sprite sprite) {
    Painter p;
    switch (sprite) {
    case Sprite::White: p.rect(0, 0, 16, 16, white); break;
    case Sprite::Ground:
    case Sprite::GroundTop:
        p.rect(0, 0, 16, 16, brown);
        p.border(0, 0, 16, 16, darkBrown);
        p.rect(3, 5, 3, 2, darkBrown);
        p.rect(10, 10, 3, 2, darkBrown);
        if (sprite == Sprite::GroundTop) {
            p.rect(0, 0, 16, 4, green);
            p.rect(2, 4, 3, 2, darkGreen);
            p.rect(10, 4, 4, 2, darkGreen);
        }
        break;
    case Sprite::Brick:
        p.rect(0, 0, 16, 16, brown);
        p.border(0, 0, 16, 16, darkBrown);
        p.rect(0, 7, 16, 2, darkBrown);
        p.rect(7, 0, 2, 7, darkBrown);
        p.rect(3, 9, 2, 7, darkBrown);
        p.rect(12, 9, 2, 7, darkBrown);
        p.rect(1, 1, 5, 1, skin);
        break;
    case Sprite::Question:
        p.rect(0, 0, 16, 16, yellow);
        p.border(0, 0, 16, 16, darkBrown);
        p.rect(4, 3, 7, 2, white);
        p.rect(9, 5, 2, 3, white);
        p.rect(7, 7, 3, 3, white);
        p.rect(7, 12, 3, 2, white);
        p.set(2, 2, darkBrown); p.set(13, 2, darkBrown);
        p.set(2, 13, darkBrown); p.set(13, 13, darkBrown);
        break;
    case Sprite::Used:
        p.rect(0, 0, 16, 16, brown);
        p.border(0, 0, 16, 16, darkBrown);
        p.set(3, 3, darkBrown); p.set(12, 3, darkBrown);
        p.set(3, 12, darkBrown); p.set(12, 12, darkBrown);
        break;
    case Sprite::Solid:
        p.rect(0, 0, 16, 16, skin);
        p.border(0, 0, 16, 16, darkBrown);
        p.rect(3, 3, 10, 10, brown);
        p.border(3, 3, 10, 10, darkBrown);
        break;
    case Sprite::PipeTopLeft:
    case Sprite::PipeTopRight:
    case Sprite::PipeLeft:
    case Sprite::PipeRight: {
        const bool top = sprite == Sprite::PipeTopLeft || sprite == Sprite::PipeTopRight;
        const bool left = sprite == Sprite::PipeTopLeft || sprite == Sprite::PipeLeft;
        p.rect(0, 0, 16, 16, green);
        if (left) {
            p.rect(0, 0, 3, 16, darkGreen);
            p.rect(4, 0, 3, 16, Pixel{115, 225, 100, 255});
            if (!top) p.rect(14, 0, 2, 16, darkGreen);
        } else {
            p.rect(13, 0, 3, 16, darkGreen);
            p.rect(3, 0, 3, 16, Pixel{115, 225, 100, 255});
            if (!top) p.rect(0, 0, 2, 16, darkGreen);
        }
        if (top) {
            p.rect(0, 0, 16, 3, darkGreen);
            p.rect(0, 13, 16, 3, darkGreen);
            if (left) p.rect(0, 0, 2, 16, darkGreen);
            else p.rect(14, 0, 2, 16, darkGreen);
        }
        break;
    }
    case Sprite::Cloud:
        p.rect(2, 7, 12, 5, white);
        p.rect(4, 4, 5, 7, white);
        p.rect(8, 6, 5, 6, white);
        p.rect(3, 12, 10, 1, Pixel{193, 230, 255, 255});
        break;
    case Sprite::Hill:
        for (int y = 0; y < 16; ++y) {
            const int half = y / 2 + 1;
            p.rect(8 - half, 15 - y, half * 2, 1, green);
        }
        p.set(5, 10, darkGreen); p.set(10, 7, darkGreen);
        break;
    case Sprite::Bush:
        p.rect(0, 9, 16, 6, green);
        p.rect(2, 5, 5, 8, green);
        p.rect(8, 7, 6, 7, green);
        p.rect(3, 6, 2, 2, Pixel{115, 225, 100, 255});
        break;
    case Sprite::Coin0:
    case Sprite::Coin1:
    case Sprite::Coin2:
    case Sprite::Coin3: {
        const int n = static_cast<int>(sprite) - static_cast<int>(Sprite::Coin0);
        const int widths[4] = {6, 4, 2, 4};
        const int w = widths[n];
        p.rect(8 - w / 2, 2, w, 12, yellow);
        p.border(8 - w / 2, 2, w, 12, darkBrown);
        if (w > 2) p.rect(8 - w / 2 + 1, 4, 1, 7, white);
        break;
    }
    case Sprite::SmallIdle: drawPlayer(p, false, 0); break;
    case Sprite::SmallWalk0: drawPlayer(p, false, 1); break;
    case Sprite::SmallWalk1: drawPlayer(p, false, 2); break;
    case Sprite::SmallJump: drawPlayer(p, false, 3); break;
    case Sprite::BigIdle: drawPlayer(p, true, 0); break;
    case Sprite::BigWalk0: drawPlayer(p, true, 1); break;
    case Sprite::BigWalk1: drawPlayer(p, true, 2); break;
    case Sprite::BigJump: drawPlayer(p, true, 3); break;
    case Sprite::Goomba0: drawGoomba(p, 0, false); break;
    case Sprite::Goomba1: drawGoomba(p, 1, false); break;
    case Sprite::GoombaFlat: drawGoomba(p, 0, true); break;
    case Sprite::Koopa0: drawKoopa(p, 0, false); break;
    case Sprite::Koopa1: drawKoopa(p, 1, false); break;
    case Sprite::Shell: drawKoopa(p, 0, true); break;
    case Sprite::Piranha0:
    case Sprite::Piranha1: {
        const int open = sprite == Sprite::Piranha0 ? 1 : 0;
        p.rect(7, 8, 3, 8, green);
        p.rect(3, 11, 5, 3, darkGreen);
        p.rect(10, 9, 5, 3, green);
        p.rect(3, 1 + open, 10, 5, red);
        p.rect(4, 7 - open, 8, 3, red);
        p.rect(4, 5, 8, 2, white);
        p.set(5, 2 + open, white); p.set(11, 2 + open, white);
        break;
    }
    case Sprite::Mushroom:
        p.rect(2, 3, 12, 7, red);
        p.rect(4, 1, 8, 3, red);
        p.rect(4, 3, 3, 4, white);
        p.rect(10, 5, 3, 3, white);
        p.rect(4, 9, 8, 6, skin);
        p.set(6, 11, black); p.set(10, 11, black);
        break;
    case Sprite::Flower:
        p.rect(7, 8, 3, 8, green);
        p.rect(2, 10, 6, 3, green);
        p.rect(9, 12, 5, 3, green);
        p.rect(3, 1, 10, 8, red);
        p.rect(5, 0, 6, 2, yellow);
        p.rect(5, 3, 6, 5, yellow);
        p.set(6, 5, black); p.set(10, 5, black);
        break;
    case Sprite::Fireball:
        p.rect(4, 4, 8, 8, red);
        p.rect(5, 3, 6, 10, yellow);
        p.rect(6, 5, 5, 5, white);
        break;
    case Sprite::Flag:
        p.rect(1, 1, 13, 9, green);
        p.rect(3, 3, 5, 3, white);
        p.rect(12, 8, 2, 2, darkGreen);
        break;
    case Sprite::Checkpoint:
        p.rect(1, 1, 12, 8, red);
        p.rect(3, 3, 5, 3, white);
        break;
    case Sprite::Star:
        p.rect(6, 0, 4, 16, yellow);
        p.rect(0, 5, 16, 5, yellow);
        p.rect(3, 3, 10, 10, yellow);
        p.rect(2, 11, 4, 4, yellow);
        p.rect(10, 11, 4, 4, yellow);
        p.rect(6, 4, 4, 6, white);
        p.set(6, 7, black); p.set(10, 7, black);
        break;
    case Sprite::CloudPlatform:
        p.rect(0, 7, 16, 5, white);
        p.rect(2, 4, 6, 6, white);
        p.rect(8, 5, 6, 5, white);
        p.rect(1, 12, 14, 2, Pixel{151, 204, 239, 255});
        p.rect(3, 14, 10, 1, Pixel{102, 158, 207, 255});
        break;
    case Sprite::Spring:
        p.rect(2, 1, 12, 3, red);
        p.rect(1, 3, 14, 2, darkRed);
        for (int y = 5; y < 13; y += 3) {
            p.rect(3, y, 10, 1, white);
            p.rect(4, y + 1, 8, 1, black);
        }
        p.rect(1, 13, 14, 3, darkBrown);
        break;
    case Sprite::Count: break;
    }
    return p;
}
} // namespace

TextureAtlas::TextureAtlas() {
    generate();
    generateGlyphs();
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, atlasPixels, atlasPixels, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, pixels_.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

TextureAtlas::~TextureAtlas() {
    if (texture_ != 0) glDeleteTextures(1, &texture_);
}

void TextureAtlas::putTile(int index, const std::array<Pixel, 256>& tile) {
    if (index < 0 || index >= atlasTiles * atlasTiles) {
        throw std::runtime_error("程序化纹理图集空间不足");
    }
    const int originX = (index % atlasTiles) * tilePixels;
    const int originY = (index / atlasTiles) * tilePixels;
    for (int y = 0; y < tilePixels; ++y) {
        for (int x = 0; x < tilePixels; ++x) {
            const Pixel& pixel = tile[static_cast<std::size_t>(y * tilePixels + x)];
            const std::size_t destination = static_cast<std::size_t>(((originY + y) * atlasPixels + originX + x) * 4);
            std::copy(pixel.begin(), pixel.end(), pixels_.begin() + static_cast<std::ptrdiff_t>(destination));
        }
    }
}

void TextureAtlas::generate() {
    pixels_.fill(0);
    for (int i = 0; i < static_cast<int>(Sprite::Count); ++i) {
        putTile(i, paint(static_cast<Sprite>(i)).data);
    }
}

void TextureAtlas::generateGlyphs() {
    using Rows = std::array<std::string_view, 7>;
    const std::unordered_map<char, Rows> font = {
        {'0', {"01110","10001","10011","10101","11001","10001","01110"}},
        {'1', {"00100","01100","00100","00100","00100","00100","01110"}},
        {'2', {"01110","10001","00001","00010","00100","01000","11111"}},
        {'3', {"11110","00001","00001","01110","00001","00001","11110"}},
        {'4', {"00010","00110","01010","10010","11111","00010","00010"}},
        {'5', {"11111","10000","10000","11110","00001","00001","11110"}},
        {'6', {"01110","10000","10000","11110","10001","10001","01110"}},
        {'7', {"11111","00001","00010","00100","01000","01000","01000"}},
        {'8', {"01110","10001","10001","01110","10001","10001","01110"}},
        {'9', {"01110","10001","10001","01111","00001","00001","01110"}},
        {'A', {"01110","10001","10001","11111","10001","10001","10001"}},
        {'B', {"11110","10001","10001","11110","10001","10001","11110"}},
        {'C', {"01111","10000","10000","10000","10000","10000","01111"}},
        {'D', {"11110","10001","10001","10001","10001","10001","11110"}},
        {'E', {"11111","10000","10000","11110","10000","10000","11111"}},
        {'F', {"11111","10000","10000","11110","10000","10000","10000"}},
        {'G', {"01111","10000","10000","10111","10001","10001","01111"}},
        {'H', {"10001","10001","10001","11111","10001","10001","10001"}},
        {'I', {"01110","00100","00100","00100","00100","00100","01110"}},
        {'J', {"00001","00001","00001","00001","10001","10001","01110"}},
        {'K', {"10001","10010","10100","11000","10100","10010","10001"}},
        {'L', {"10000","10000","10000","10000","10000","10000","11111"}},
        {'M', {"10001","11011","10101","10101","10001","10001","10001"}},
        {'N', {"10001","11001","10101","10011","10001","10001","10001"}},
        {'O', {"01110","10001","10001","10001","10001","10001","01110"}},
        {'P', {"11110","10001","10001","11110","10000","10000","10000"}},
        {'Q', {"01110","10001","10001","10001","10101","10010","01101"}},
        {'R', {"11110","10001","10001","11110","10100","10010","10001"}},
        {'S', {"01111","10000","10000","01110","00001","00001","11110"}},
        {'T', {"11111","00100","00100","00100","00100","00100","00100"}},
        {'U', {"10001","10001","10001","10001","10001","10001","01110"}},
        {'V', {"10001","10001","10001","10001","10001","01010","00100"}},
        {'W', {"10001","10001","10001","10101","10101","10101","01010"}},
        {'X', {"10001","10001","01010","00100","01010","10001","10001"}},
        {'Y', {"10001","10001","01010","00100","00100","00100","00100"}},
        {'Z', {"11111","00001","00010","00100","01000","10000","11111"}},
        {'-', {"00000","00000","00000","11111","00000","00000","00000"}},
        {':', {"00000","00100","00100","00000","00100","00100","00000"}},
        {'.', {"00000","00000","00000","00000","00000","00110","00110"}},
        {'/', {"00001","00010","00010","00100","01000","01000","10000"}},
        {'!', {"00100","00100","00100","00100","00100","00000","00100"}},
        {'+', {"00000","00100","00100","11111","00100","00100","00000"}},
        {'<', {"00010","00100","01000","10000","01000","00100","00010"}},
        {'>', {"01000","00100","00010","00001","00010","00100","01000"}}
    };
    for (const auto& [character, rows] : font) {
        Painter p;
        for (int y = 0; y < 7; ++y) {
            for (int x = 0; x < 5; ++x) {
                if (rows[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] == '1') {
                    p.rect(2 + x * 2, 1 + y * 2, 2, 2, white);
                }
            }
        }
        glyphs_[character] = nextFreeTile_;
        putTile(nextFreeTile_++, p.data);
    }
}

void TextureAtlas::bind() const {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_);
}

UV TextureAtlas::uv(Sprite sprite) const {
    const int index = static_cast<int>(sprite);
    const float x = static_cast<float>(index % atlasTiles * tilePixels);
    const float y = static_cast<float>(index / atlasTiles * tilePixels);
    return {x / atlasPixels, y / atlasPixels, (x + tilePixels) / atlasPixels,
            (y + tilePixels) / atlasPixels};
}

UV TextureAtlas::glyphUV(char c) const {
    const char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    const auto it = glyphs_.find(upper);
    if (it == glyphs_.end()) return uv(Sprite::White);
    const int index = it->second;
    const float x = static_cast<float>(index % atlasTiles * tilePixels);
    const float y = static_cast<float>(index / atlasTiles * tilePixels);
    return {x / atlasPixels, y / atlasPixels, (x + tilePixels) / atlasPixels,
            (y + tilePixels) / atlasPixels};
}

bool TextureAtlas::hasGlyph(char c) const {
    const char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return glyphs_.find(upper) != glyphs_.end();
}
