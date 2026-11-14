#pragma once

#include "entity.h"
#include "level.h"

#include <glm/vec4.hpp>

class Enemy : public Entity {
public:
    ~Enemy() override = default;
    virtual bool stompable() const { return true; }
    virtual void stomped(Game& game) = 0;
    virtual void hitByFire(Game& game);
    virtual bool dangerous() const { return alive && collidable; }

    // 怪物等级：生命、移动速度、得分与经验随等级提升。
    void setLevel(int newLevel);
    int levelValue() const { return level_; }
    float speedScale() const { return speedScale_; }
    int xpReward() const;
    virtual void killByStar(Game& game);

protected:
    void tick(float dt);
    bool applyDamage(Game& game, int amount);
    void awardKill(Game& game, int baseScore);
    void defeat(Game& game);
    void drawBadge(Renderer& renderer) const;
    glm::vec4 damageTint() const;

    // 供 Boss 等派生类自定义生命值：普通怪物仍走 setLevel 的等级逻辑，不受影响。
    int hitPoints() const { return hitPoints_; }
    int maxHitPoints() const { return maxHitPoints_; }
    void setHitPoints(int value) { hitPoints_ = value; }
    void setMaxHitPoints(int value) { maxHitPoints_ = value; }

private:
    int level_ = 1;
    int hitPoints_ = 1;
    int maxHitPoints_ = 1;
    float speedScale_ = 1.0F;
    float hitFlash_ = 0.0F;
};

class Goomba final : public Enemy {
public:
    Goomba(float x, float groundY);
    void update(float dt, Game& game) override;
    void draw(Renderer& renderer) const override;
    void stomped(Game& game) override;
    bool dangerous() const override;

private:
    float animationTime_ = 0.0F;
    float squashTime_ = 0.0F;
    bool killAwarded_ = false;
};

enum class KoopaState { Walking, Shell, Sliding };

class Koopa final : public Enemy {
public:
    Koopa(float x, float groundY);
    void update(float dt, Game& game) override;
    void draw(Renderer& renderer) const override;
    void stomped(Game& game) override;
    void hitByFire(Game& game) override;
    bool dangerous() const override;

    KoopaState state() const { return state_; }
    void kick(int direction, Game& game);
    void stopShell();

private:
    KoopaState state_ = KoopaState::Walking;
    float animationTime_ = 0.0F;
    float kickGrace_ = 0.0F;
    bool killAwarded_ = false;
};

class Piranha final : public Enemy {
public:
    Piranha(float centerX, float pipeTopY);
    void update(float dt, Game& game) override;
    void draw(Renderer& renderer) const override;
    void stomped(Game& game) override;
    bool stompable() const override { return false; }

private:
    float pipeTopY_ = 0.0F;
    float extension_ = 0.0F;
    int direction_ = 0;
    float wait_ = 1.2F;
    float animationTime_ = 0.0F;
};

// 飞行乌龟：沿水平巡逻并做正弦上下浮动，踩一次直接消灭。
class Paratroopa final : public Enemy {
public:
    Paratroopa(float x, float groundY);
    void update(float dt, Game& game) override;
    void draw(Renderer& renderer) const override;
    void stomped(Game& game) override;

private:
    float baseY_ = 0.0F;
    float time_ = 0.0F;
};

// 刺刺龟：会巡逻但不可踩踏，只能用火球或滑壳消灭。
class Spiny final : public Enemy {
public:
    Spiny(float x, float groundY);
    void update(float dt, Game& game) override;
    void draw(Renderer& renderer) const override;
    void stomped(Game& game) override;
    bool stompable() const override { return false; }

private:
    float animationTime_ = 0.0F;
};

// 多类型 Boss：爆爆冲锋跳跃、鸟姐定点吐蛋、库巴慢走跳跃并连发火球。
// 生命值明显高于普通怪物，半血后进入狂暴阶段（加速、缩短攻击间隔）。
class Boss : public Enemy {
public:
    Boss(BossType type, float x, float groundY, int level);
    void update(float dt, Game& game) override;
    void draw(Renderer& renderer) const override;
    void stomped(Game& game) override;
    void hitByFire(Game& game) override;
    void killByStar(Game& game) override;

    const char* bossName() const;
    int hp() const { return hitPoints(); }
    int maxHp() const { return maxHitPoints(); }
    BossType type() const { return type_; }

private:
    void takeDamage(Game& game, int amount);
    glm::vec4 bodyColor() const;

    BossType type_;
    float animationTime_ = 0.0F;
    float attackTimer_ = 0.0F;
    float jumpTimer_ = 0.0F;
    float burstTimer_ = 0.0F;
    int fireBurst_ = 0;
};

// Boss 投射物：kind=0 蛋（抛物）、kind=1 火焰（直线连发）。
// 与地形碰撞或寿命耗尽后消失；与玩家的碰撞由 Game 统一处理。
class BossShot final : public Entity {
public:
    BossShot(float x, float y, float vx, float vy, int kind);
    void update(float dt, Game& game) override;
    void draw(Renderer& renderer) const override;
    int kind() const { return kind_; }

private:
    int kind_ = 0;
    float life_ = 0.0F;
    float animationTime_ = 0.0F;
};
