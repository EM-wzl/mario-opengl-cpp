#pragma once

#include "entity.h"

enum class ItemType { Coin, Mushroom, Flower, OneUp, Star };

class Item final : public Entity {
public:
    Item(ItemType type, float x, float y);
    void update(float dt, Game& game) override;
    void draw(Renderer& renderer) const override;
    ItemType type() const { return type_; }

private:
    ItemType type_;
    float animationTime_ = 0.0F;
    float age_ = 0.0F;
};

class Fireball final : public Entity {
public:
    Fireball(float x, float y, int direction);
    void update(float dt, Game& game) override;
    void draw(Renderer& renderer) const override;

private:
    float age_ = 0.0F;
};

class PlayerShot final : public Entity {
public:
    PlayerShot(float x, float y, float vx, float vy, int damage, int kind);
    void update(float dt, Game& game) override;
    void draw(Renderer& renderer) const override;
    int damage() const { return damage_; }

private:
    int damage_ = 1;
    int kind_ = 0;  // 0=箭，1=魔法弹
    float life_ = 0.0F;
    float animationTime_ = 0.0F;
};
