#include "item.h"

#include "config.h"
#include "game.h"
#include "level.h"
#include "renderer.h"

#include <algorithm>
#include <cmath>

Item::Item(ItemType type, float x, float y) : type_(type) {
    body.size = type == ItemType::Coin ? glm::vec2{24.0F, 28.0F} : glm::vec2{28.0F, 28.0F};
    body.position = {x, y};
    facing = 1;
    if (type == ItemType::Mushroom || type == ItemType::OneUp) body.velocity.x = 105.0F;
    if (type == ItemType::Star) body.velocity.x = 145.0F;
}

void Item::update(float dt, Game& game) {
    animationTime_ += dt;
    age_ += dt;
    if (type_ == ItemType::Coin || type_ == ItemType::Flower) return;
    const float speed = type_ == ItemType::Star ? 145.0F : 105.0F;
    body.velocity.x = facing * speed;
    body.velocity.y = std::min(body.velocity.y + config::gravity * dt, config::maxFallSpeed);
    const auto collision = physics::moveBody(body, game.level(), dt);
    if (collision.hitLeft || collision.hitRight) facing *= -1;
    // 星星会持续弹跳，1UP 与红蘑菇则沿地面滑动。
    if (type_ == ItemType::Star && collision.hitGround) body.velocity.y = -520.0F;
    if (body.position.y > config::levelHeightTiles * config::tileSize + 100.0F) alive = false;
}

void Item::draw(Renderer& renderer) const {
    if (type_ == ItemType::Coin) {
        const int frame = static_cast<int>(animationTime_ * 10.0F) % 4;
        renderer.drawSprite(static_cast<Sprite>(static_cast<int>(Sprite::Coin0) + frame),
                            body.position.x, body.position.y, body.size.x, body.size.y);
    } else {
        Sprite sprite = Sprite::Mushroom;
        glm::vec4 tint{1.0F};
        if (type_ == ItemType::Flower) sprite = Sprite::Flower;
        if (type_ == ItemType::Star) sprite = Sprite::Star;
        if (type_ == ItemType::OneUp) tint = {0.35F, 1.0F, 0.42F, 1.0F};
        const float bob = type_ == ItemType::Flower ? std::sin(animationTime_ * 4.0F) * 2.0F : 0.0F;
        const float pulse = type_ == ItemType::Star ? 1.0F + std::sin(animationTime_ * 14.0F) * 0.12F : 1.0F;
        renderer.drawSprite(sprite, body.position.x - 2.0F, body.position.y - 2.0F + bob,
                            (body.size.x + 4.0F) * pulse, (body.size.y + 4.0F) * pulse, false, tint);
    }
}

Fireball::Fireball(float x, float y, int direction) {
    body.size = {16.0F, 16.0F};
    body.position = {x, y};
    facing = direction >= 0 ? 1 : -1;
    body.velocity.x = facing * config::fireballSpeed;
    body.velocity.y = 120.0F;
}

void Fireball::update(float dt, Game& game) {
    age_ += dt;
    body.velocity.x = facing * config::fireballSpeed;
    body.velocity.y = std::min(body.velocity.y + config::gravity * 0.75F * dt, config::maxFallSpeed);
    const auto collision = physics::moveBody(body, game.level(), dt);
    if (collision.hitGround) body.velocity.y = config::fireballBounce;
    if (collision.hitLeft || collision.hitRight || age_ > 3.5F ||
        body.position.y > config::levelHeightTiles * config::tileSize + 80.0F) {
        alive = false;
    }
}

void Fireball::draw(Renderer& renderer) const {
    const float pulse = 1.0F + std::sin(age_ * 30.0F) * 0.12F;
    const float size = body.size.x * pulse;
    renderer.drawSprite(Sprite::Fireball, body.position.x + (body.size.x - size) * 0.5F,
                        body.position.y + (body.size.y - size) * 0.5F, size, size);
}

PlayerShot::PlayerShot(float x, float y, float vx, float vy, int damage, int kind)
    : damage_(damage), kind_(kind) {
    body.size = {kind_ == 0 ? 20.0F : 18.0F, 18.0F};
    body.position = {x - body.size.x * 0.5F, y - body.size.y * 0.5F};
    body.velocity = {vx, vy};
    facing = vx >= 0.0F ? 1 : -1;
    collidable = false;  // 与敌人的碰撞由 Game 统一处理
}

void PlayerShot::update(float dt, Game& game) {
    life_ += dt;
    animationTime_ += dt;
    // 直线飞行：不施加重力，命中任意地形或寿命耗尽即消失。
    const auto collision = physics::moveBody(body, game.level(), dt);
    if (collision.hitLeft || collision.hitRight || collision.hitGround || collision.hitHead ||
        life_ > config::playerShotLife ||
        body.position.y > config::levelHeightTiles * config::tileSize + 80.0F) {
        alive = false;
    }
}

void PlayerShot::draw(Renderer& renderer) const {
    const float pulse = 1.0F + std::sin(animationTime_ * 26.0F) * 0.10F;
    const float size = body.size.x * pulse;
    const glm::vec4 tint = kind_ == 0
        ? glm::vec4{0.62F, 0.84F, 1.0F, 1.0F}    // 箭：蓝白
        : glm::vec4{0.72F, 0.40F, 0.96F, 1.0F};  // 魔法弹：紫
    renderer.drawSprite(Sprite::Fireball,
                        body.position.x + (body.size.x - size) * 0.5F,
                        body.position.y + (body.size.y - size) * 0.5F,
                        size, size, facing < 0, tint);
}
