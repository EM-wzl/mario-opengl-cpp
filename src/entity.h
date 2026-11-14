#pragma once

#include "physics.h"

class Game;
class Renderer;

class Entity {
public:
    virtual ~Entity() = default;
    virtual void update(float dt, Game& game) = 0;
    virtual void draw(Renderer& renderer) const = 0;

    Body body;
    bool alive = true;
    bool collidable = true;
    int facing = -1;
};
