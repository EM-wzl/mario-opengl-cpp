#pragma once

#include "level.h"
#include "physics.h"

class Game;
class Renderer;

// 往复移动平台。平台可载人移动，玩家也能从空中落到其上。
class MovingPlatform {
public:
    explicit MovingPlatform(const MovingPlatformPlacement& placement);

    void update(float dt, Game& game);
    void resolvePlayer(Game& game, float previousPlayerBottom) const;
    void draw(Renderer& renderer) const;

    Body body;

private:
    glm::vec2 origin_{0.0F};
    glm::vec2 axis_{1.0F, 0.0F};
    float range_ = 96.0F;
    float speed_ = 1.4F;
    float phase_ = 0.0F;
    float time_ = 0.0F;
};
