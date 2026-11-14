#pragma once

#include <glm/vec2.hpp>
#include <utility>
#include <vector>

class Level;

struct Body {
    glm::vec2 position{0.0F};
    glm::vec2 size{24.0F, 30.0F};
    glm::vec2 velocity{0.0F};
    bool onGround = false;
};

struct CollisionResult {
    bool hitLeft = false;
    bool hitRight = false;
    bool hitHead = false;
    bool hitGround = false;
    std::vector<std::pair<int, int>> headTiles;
};

namespace physics {
bool overlaps(const Body& a, const Body& b);
bool overlaps(const glm::vec2& aPosition, const glm::vec2& aSize,
              const glm::vec2& bPosition, const glm::vec2& bSize);

// 分轴解算：先 X 后 Y；只查询 Body 覆盖的少量瓦片，避免遍历整张地图。
CollisionResult moveBody(Body& body, const Level& level, float dt);
bool groundAhead(const Body& body, const Level& level, int direction);
}
