#include "physics.h"

#include "config.h"
#include "level.h"

#include <algorithm>
#include <cmath>

namespace physics {
bool overlaps(const Body& a, const Body& b) {
    return overlaps(a.position, a.size, b.position, b.size);
}

bool overlaps(const glm::vec2& aPosition, const glm::vec2& aSize,
              const glm::vec2& bPosition, const glm::vec2& bSize) {
    return aPosition.x < bPosition.x + bSize.x && aPosition.x + aSize.x > bPosition.x &&
           aPosition.y < bPosition.y + bSize.y && aPosition.y + aSize.y > bPosition.y;
}

CollisionResult moveBody(Body& body, const Level& level, float dt) {
    CollisionResult result;
    constexpr float epsilon = 0.001F;
    const float tile = config::tileSize;

    body.position.x += body.velocity.x * dt;
    if (body.velocity.x > 0.0F) {
        const int tx = static_cast<int>(std::floor((body.position.x + body.size.x) / tile));
        const int firstY = static_cast<int>(std::floor((body.position.y + 2.0F) / tile));
        const int lastY = static_cast<int>(std::floor((body.position.y + body.size.y - 2.0F) / tile));
        for (int ty = firstY; ty <= lastY; ++ty) {
            if (level.isSolid(tx, ty)) {
                body.position.x = tx * tile - body.size.x - epsilon;
                body.velocity.x = 0.0F;
                result.hitRight = true;
                break;
            }
        }
    } else if (body.velocity.x < 0.0F) {
        const int tx = static_cast<int>(std::floor(body.position.x / tile));
        const int firstY = static_cast<int>(std::floor((body.position.y + 2.0F) / tile));
        const int lastY = static_cast<int>(std::floor((body.position.y + body.size.y - 2.0F) / tile));
        for (int ty = firstY; ty <= lastY; ++ty) {
            if (level.isSolid(tx, ty)) {
                body.position.x = (tx + 1) * tile + epsilon;
                body.velocity.x = 0.0F;
                result.hitLeft = true;
                break;
            }
        }
    }

    const float previousBottom = body.position.y + body.size.y;
    body.position.y += body.velocity.y * dt;
    body.onGround = false;
    const int firstX = static_cast<int>(std::floor((body.position.x + 2.0F) / tile));
    const int lastX = static_cast<int>(std::floor((body.position.x + body.size.x - 2.0F) / tile));
    if (body.velocity.y > 0.0F) {
        const int ty = static_cast<int>(std::floor((body.position.y + body.size.y) / tile));
        for (int tx = firstX; tx <= lastX; ++tx) {
            // 单向平台只在“从上方落下”时支撑；实心砖则始终碰撞。
            const bool landingOnOneWay = level.isOneWay(tx, ty) && previousBottom <= ty * tile + 4.0F;
            if (level.isSolid(tx, ty) || landingOnOneWay) {
                body.position.y = ty * tile - body.size.y - epsilon;
                body.velocity.y = 0.0F;
                body.onGround = true;
                result.hitGround = true;
                break;
            }
        }
    } else if (body.velocity.y < 0.0F) {
        const int ty = static_cast<int>(std::floor(body.position.y / tile));
        for (int tx = firstX; tx <= lastX; ++tx) {
            if (level.isSolid(tx, ty)) {
                body.position.y = (ty + 1) * tile + epsilon;
                body.velocity.y = 0.0F;
                result.hitHead = true;
                result.headTiles.emplace_back(tx, ty);
            }
        }
    }
    return result;
}

bool groundAhead(const Body& body, const Level& level, int direction) {
    const float probeX = direction > 0 ? body.position.x + body.size.x + 3.0F
                                       : body.position.x - 3.0F;
    const float probeY = body.position.y + body.size.y + 5.0F;
    return level.isSupportPixel(probeX, probeY);
}
} // namespace physics
