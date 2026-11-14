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
