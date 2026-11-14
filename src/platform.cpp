#include "platform.h"

#include "config.h"
#include "game.h"
#include "player.h"
#include "renderer.h"

#include <algorithm>
#include <cmath>

MovingPlatform::MovingPlatform(const MovingPlatformPlacement& placement)
    : origin_{placement.x * config::tileSize, placement.y * config::tileSize},
      axis_{placement.axis},
      range_(placement.rangeTiles * config::tileSize),
      speed_(placement.speed),
      phase_(placement.phase) {
    body.size = {placement.widthTiles * config::tileSize, 14.0F};
    body.position = origin_ + axis_ * (std::sin(phase_) * range_);
}

void MovingPlatform::update(float dt, Game& game) {
    time_ += dt;
    const glm::vec2 previous = body.position;
    body.position = origin_ + axis_ * (std::sin(time_ * speed_ + phase_) * range_);
    const glm::vec2 delta = body.position - previous;

    // 若玩家上一物理步正站在平台上，先随平台一起位移，再执行玩家自身物理。
    Player& player = game.player();
    if (player.dead) return;
    const Body& p = player.body;
    const float bottom = p.position.y + p.size.y;
    const bool above = bottom >= previous.y - 5.0F && bottom <= previous.y + 9.0F;
    const bool horizontal = p.position.x + p.size.x > previous.x + 2.0F &&
                            p.position.x < previous.x + body.size.x - 2.0F;
    if (above && horizontal && p.velocity.y >= -20.0F) {
        player.body.position += delta;
        player.body.onGround = true;
    }
}

void MovingPlatform::resolvePlayer(Game& game, float previousPlayerBottom) const {
    Player& player = game.player();
    if (player.dead || player.body.velocity.y < 0.0F) return;
    Body& p = player.body;
    const float top = body.position.y;
    const float bottom = p.position.y + p.size.y;
    const bool horizontal = p.position.x + p.size.x > body.position.x + 2.0F &&
                            p.position.x < body.position.x + body.size.x - 2.0F;
    if (!horizontal) return;

    // 仅当上一帧脚底不低于平台顶面附近时才着陆，避免从下方被吸附。
    if (previousPlayerBottom <= top + 10.0F && bottom >= top && bottom <= top + 30.0F) {
        p.position.y = top - p.size.y - 0.001F;
        p.velocity.y = 0.0F;
        p.onGround = true;
    }
}

void MovingPlatform::draw(Renderer& renderer) const {
    renderer.drawRect(body.position.x, body.position.y, body.size.x, body.size.y,
                      {0.32F, 0.40F, 0.52F, 1.0F});
    renderer.drawRect(body.position.x, body.position.y, body.size.x, 4.0F,
                      {0.82F, 0.88F, 0.95F, 1.0F});
    renderer.drawRect(body.position.x, body.position.y + body.size.y - 3.0F,
                      body.size.x, 3.0F, {0.15F, 0.19F, 0.28F, 1.0F});
    for (float x = body.position.x + 10.0F; x < body.position.x + body.size.x; x += 24.0F) {
        renderer.drawRect(x, body.position.y + 6.0F, 4.0F, 4.0F, {0.95F, 0.72F, 0.22F, 1.0F});
    }
}
