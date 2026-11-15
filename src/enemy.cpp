#include "enemy.h"

#include "audio.h"
#include "config.h"
#include "game.h"
#include "level.h"
#include "player.h"
#include "renderer.h"

#include <algorithm>
#include <cmath>
#include <string>

void Enemy::setLevel(int newLevel) {
    level_ = std::max(1, newLevel);
    maxHitPoints_ = std::min(config::enemyMaxHp,
                             1 + (level_ - 1) / config::enemyHpPerLevels);
    hitPoints_ = maxHitPoints_;
    speedScale_ = std::min(config::enemyMaxSpeedScale,
                           1.0F + static_cast<float>(level_ - 1) * config::enemySpeedPerLevel);
}

int Enemy::xpReward() const {
    return 8 + level_ * 5;
}

void Enemy::tick(float dt) {
    hitFlash_ = std::max(0.0F, hitFlash_ - dt);
}

void Enemy::configureCombat(int baseDamage, float range, float cooldown, bool ranged) {
    combatDamage_ = std::max(1, baseDamage);
    attackRange_ = std::max(24.0F, range);
    attackCooldown_ = std::max(0.25F, cooldown);
    rangedAttack_ = ranged;
    attackTimer_ = 0.4F;
}

int Enemy::combatDamage() const {
    return combatDamage_ + levelValue() * 2;
}

void Enemy::updateCombat(float dt, Game& game) {
    attackTimer_ = std::max(0.0F, attackTimer_ - dt);
    if (!alive || attackTimer_ > 0.0F) return;
    const glm::vec2 playerCenter = game.player().body.position + game.player().body.size * 0.5F;
    const glm::vec2 selfCenter = body.position + body.size * 0.5F;
    const glm::vec2 delta = playerCenter - selfCenter;
    const float distance = std::hypot(delta.x, delta.y);
    if (distance > attackRange_) return;

    const int direction = delta.x >= 0.0F ? 1 : -1;
    const int damage = combatDamage();
    if (rangedAttack_) {
        game.spawnBossShot(selfCenter.x + direction * 16.0F,
                           selfCenter.y,
                           direction * config::bossShotSpeed, 0.0F, 2, damage);
    } else {
        game.spawnBossShot(selfCenter.x + direction * 28.0F,
                           selfCenter.y, 0.0F, 0.0F, 3, damage);
    }
    attackTimer_ = attackCooldown_;
}

void Enemy::hitByWeapon(Game& game, int damage) {
    if (!applyDamage(game, damage)) return;
    defeat(game);
}

bool Enemy::applyDamage(Game& game, int amount) {
    if (!alive || hitPoints_ <= 0) return false;
    hitPoints_ -= std::max(1, amount);
    if (hitPoints_ > 0) {
        hitFlash_ = 0.16F;
        game.addScore(25 * level_);
        game.audio().play(SoundEffect::Brick);
        game.spawnParticles(body.position.x + body.size.x * 0.5F,
                            body.position.y + body.size.y * 0.5F,
                            {1.0F, 0.82F, 0.32F, 1.0F}, 4);
        return false;
    }
    return true;
}

void Enemy::awardKill(Game& game, int baseScore) {
    game.addScore(baseScore + level_ * 60);
    game.addXP(xpReward());
    game.spawnParticles(body.position.x + body.size.x * 0.5F,
                        body.position.y + body.size.y * 0.5F,
                        {1.0F, 0.55F, 0.15F, 1.0F}, 7 + std::min(level_, 5));
}

void Enemy::defeat(Game& game) {
    if (!alive) return;
    alive = false;
    collidable = false;
    awardKill(game, 200);
}

void Enemy::hitByFire(Game& game) {
    if (!applyDamage(game, 1)) return;
    defeat(game);
}

void Enemy::killByStar(Game& game) {
    if (!alive) return;
    hitPoints_ = 1;
    hitByFire(game);
}

glm::vec4 Enemy::damageTint() const {
    return hitFlash_ > 0.0F ? glm::vec4{1.0F, 0.52F, 0.52F, 1.0F}
                            : glm::vec4{1.0F};
}

void Enemy::drawBadge(Renderer& renderer) const {
    if (level_ > 1) {
        renderer.drawText("L" + std::to_string(level_), body.position.x,
                          body.position.y - 21.0F, 12.0F, {1.0F, 0.86F, 0.25F, 1.0F});
    }
    if (hitPoints_ < maxHitPoints_) {
        const float width = std::max(26.0F, body.size.x);
        renderer.drawRect(body.position.x, body.position.y - 9.0F, width, 4.0F,
                          {0.15F, 0.12F, 0.16F, 0.90F});
        const float ratio = static_cast<float>(hitPoints_) / static_cast<float>(maxHitPoints_);
        renderer.drawRect(body.position.x, body.position.y - 9.0F, width * ratio, 4.0F,
                          {1.0F, 0.30F, 0.24F, 1.0F});
    }
}

Goomba::Goomba(float x, float groundY) {
    body.size = {30.0F, 30.0F};
    body.position = {x, groundY - body.size.y};
    body.velocity.x = -config::enemyWalkSpeed;
    configureCombat(config::enemyMeleeDamage, config::enemyAttackRangeMelee,
                    config::enemyMeleeCooldown, false);
}

void Goomba::update(float dt, Game& game) {
    tick(dt);
    animationTime_ += dt;
    if (squashTime_ > 0.0F) {
        squashTime_ -= dt;
        if (squashTime_ <= 0.0F) alive = false;
        return;
    }
    body.velocity.x = facing * config::enemyWalkSpeed * speedScale();
    if (body.onGround && !physics::groundAhead(body, game.level(), facing)) facing *= -1;
    body.velocity.y = std::min(body.velocity.y + config::gravity * dt, config::maxFallSpeed);
    const auto collision = physics::moveBody(body, game.level(), dt);
    if (collision.hitLeft || collision.hitRight) facing *= -1;
    if (body.position.y > config::levelHeightTiles * config::tileSize + 120.0F) alive = false;
    updateCombat(dt, game);
}

void Goomba::draw(Renderer& renderer) const {
    const Sprite sprite = squashTime_ > 0.0F ? Sprite::GoombaFlat
        : (static_cast<int>(animationTime_ * 7.0F) % 2 == 0 ? Sprite::Goomba0 : Sprite::Goomba1);
    renderer.drawSprite(sprite, body.position.x - 3.0F, body.position.y - 3.0F,
                        body.size.x + 6.0F, body.size.y + 6.0F, facing > 0, damageTint());
    if (squashTime_ <= 0.0F) drawBadge(renderer);
}

void Goomba::stomped(Game& game) {
    if (squashTime_ > 0.0F || killAwarded_) return;
    if (!applyDamage(game, 1)) return;
    killAwarded_ = true;
    squashTime_ = 0.45F;
    collidable = false;
    body.velocity = {0.0F, 0.0F};
    awardKill(game, 100);
    game.audio().play(SoundEffect::Stomp);
}

bool Goomba::dangerous() const {
    return alive && collidable && squashTime_ <= 0.0F;
}

Koopa::Koopa(float x, float groundY) {
    body.size = {28.0F, 42.0F};
    body.position = {x, groundY - body.size.y};
    body.velocity.x = -config::enemyWalkSpeed;
    configureCombat(config::enemyMeleeDamage + 2, config::enemyAttackRangeMelee + 4.0F,
                    config::enemyMeleeCooldown + 0.2F, false);
}

void Koopa::update(float dt, Game& game) {
    tick(dt);
    animationTime_ += dt;
    kickGrace_ = std::max(0.0F, kickGrace_ - dt);
    if (state_ == KoopaState::Walking) {
        body.velocity.x = facing * config::enemyWalkSpeed * speedScale();
        if (body.onGround && !physics::groundAhead(body, game.level(), facing)) facing *= -1;
    } else if (state_ == KoopaState::Sliding) {
        body.velocity.x = facing * config::shellSpeed * speedScale();
    } else {
        body.velocity.x = 0.0F;
    }
    body.velocity.y = std::min(body.velocity.y + config::gravity * dt, config::maxFallSpeed);
    const auto collision = physics::moveBody(body, game.level(), dt);
    if (collision.hitLeft || collision.hitRight) {
        facing *= -1;
        if (state_ == KoopaState::Sliding) game.audio().play(SoundEffect::Kick);
    }
    if (body.position.y > config::logicalHeight + 200.0F) alive = false;
    updateCombat(dt, game);
}

void Koopa::draw(Renderer& renderer) const {
    const Sprite sprite = state_ == KoopaState::Walking
        ? (static_cast<int>(animationTime_ * 6.0F) % 2 == 0 ? Sprite::Koopa0 : Sprite::Koopa1)
        : Sprite::Shell;
    const float spin = state_ == KoopaState::Sliding ? std::sin(animationTime_ * 24.0F) * 0.25F + 0.75F : 1.0F;
    renderer.drawSprite(sprite, body.position.x - 3.0F, body.position.y - 3.0F,
                        body.size.x + 6.0F, (body.size.y + 6.0F) * spin, facing > 0,
                        damageTint());
    drawBadge(renderer);
}

void Koopa::stomped(Game& game) {
    const float bottom = body.position.y + body.size.y;
    if (state_ == KoopaState::Walking) {
        if (!applyDamage(game, 1)) return;
        killAwarded_ = true;
        state_ = KoopaState::Shell;
        body.size = {30.0F, 25.0F};
        body.position.y = bottom - body.size.y;
        body.velocity.x = 0.0F;
        awardKill(game, 100);
        game.audio().play(SoundEffect::Stomp);
    } else if (state_ == KoopaState::Sliding) {
        stopShell();
        game.addScore(50);
        game.audio().play(SoundEffect::Stomp);
    } else {
        const int direction = game.player().body.position.x + game.player().body.size.x * 0.5F <
                              body.position.x + body.size.x * 0.5F ? 1 : -1;
        kick(direction, game);
    }
}

void Koopa::kick(int direction, Game& game) {
    const float bottom = body.position.y + body.size.y;
    state_ = KoopaState::Sliding;
    body.size = {30.0F, 25.0F};
    body.position.y = bottom - body.size.y;
    facing = direction >= 0 ? 1 : -1;
    body.velocity.x = facing * config::shellSpeed * speedScale();
    kickGrace_ = 0.20F;
    game.audio().play(SoundEffect::Kick);
}

void Koopa::stopShell() {
    state_ = KoopaState::Shell;
    body.velocity.x = 0.0F;
}

void Koopa::hitByFire(Game& game) {
    Enemy::hitByFire(game);
}

bool Koopa::dangerous() const {
    return alive && collidable && kickGrace_ <= 0.0F;
}

Piranha::Piranha(float centerX, float pipeTopY) : pipeTopY_(pipeTopY) {
    body.size = {28.0F, 46.0F};
    body.position = {centerX - body.size.x * 0.5F, pipeTopY_};
    collidable = false;
    configureCombat(config::enemyRangedDamage + 2, config::enemyAttackRangeRanged,
                    config::enemyRangedCooldown, true);
}

void Piranha::update(float dt, Game& game) {
    tick(dt);
    animationTime_ += dt;
    wait_ -= dt;
    const float playerX = game.player().body.position.x + game.player().body.size.x * 0.5F;
    const float plantX = body.position.x + body.size.x * 0.5F;
    const bool playerNear = std::abs(playerX - plantX) < 105.0F;
    if (direction_ == 0 && wait_ <= 0.0F) {
        if (extension_ <= 0.0F) {
            if (playerNear) wait_ = 0.35F;
            else direction_ = 1;
        } else {
            direction_ = -1;
        }
    }
    if (direction_ != 0) {
        extension_ += direction_ * dt / 0.95F;
        if (extension_ >= 1.0F) {
            extension_ = 1.0F;
            direction_ = 0;
            wait_ = 2.1F;
        } else if (extension_ <= 0.0F) {
            extension_ = 0.0F;
            direction_ = 0;
            wait_ = 1.5F;
        }
    }
    body.position.y = pipeTopY_ - body.size.y * extension_;
    collidable = extension_ > 0.35F;
    if (collidable) updateCombat(dt, game);
}

void Piranha::draw(Renderer& renderer) const {
    if (extension_ <= 0.0F) return;
    const Sprite sprite = static_cast<int>(animationTime_ * 5.0F) % 2 == 0 ? Sprite::Piranha0 : Sprite::Piranha1;
    const float height = (body.size.y + 6.0F) * extension_;
    renderer.drawSprite(sprite, body.position.x - 4.0F, pipeTopY_ - height,
                        body.size.x + 8.0F, height, false, damageTint());
    drawBadge(renderer);
}

void Piranha::stomped(Game& game) {
    (void)game;
}

Paratroopa::Paratroopa(float x, float groundY) {
    body.size = {30.0F, 42.0F};
    body.position = {x, groundY - body.size.y};
    baseY_ = body.position.y;
    body.velocity.x = -96.0F;
    facing = -1;
    configureCombat(config::enemyRangedDamage, config::enemyAttackRangeRanged - 30.0F,
                    config::enemyRangedCooldown + 0.3F, true);
}

void Paratroopa::update(float dt, Game& game) {
    tick(dt);
    time_ += dt;
    body.velocity.x = facing * 96.0F * speedScale();
    body.velocity.y = 0.0F;
    const auto collision = physics::moveBody(body, game.level(), dt);
    if (collision.hitLeft || collision.hitRight) facing *= -1;
    body.position.y = baseY_ + std::sin(time_ * 2.24F) * 58.0F;
    updateCombat(dt, game);
}

void Paratroopa::draw(Renderer& renderer) const {
    const float flap = std::sin(time_ * 16.0F) * 6.0F;
    const glm::vec4 wing{0.96F, 0.96F, 1.0F, 0.95F};
    renderer.drawRect(body.position.x - 8.0F, body.position.y + 10.0F + flap,
                      10.0F, 14.0F, wing);
    renderer.drawRect(body.position.x + body.size.x - 2.0F, body.position.y + 10.0F - flap,
                      10.0F, 14.0F, wing);
    const Sprite sprite = static_cast<int>(time_ * 7.0F) % 2 == 0 ? Sprite::Koopa0 : Sprite::Koopa1;
    glm::vec4 tint = damageTint();
    tint.r *= 1.0F;
    tint.g *= 0.74F;
    tint.b *= 0.66F;
    renderer.drawSprite(sprite, body.position.x - 3.0F, body.position.y - 3.0F,
                        body.size.x + 6.0F, body.size.y + 6.0F, facing > 0, tint);
    drawBadge(renderer);
}

void Paratroopa::stomped(Game& game) {
    if (!applyDamage(game, 1)) return;
    alive = false;
    collidable = false;
    awardKill(game, 200);
    game.audio().play(SoundEffect::Stomp);
}

Spiny::Spiny(float x, float groundY) {
    body.size = {30.0F, 28.0F};
    body.position = {x, groundY - body.size.y};
    body.velocity.x = -config::enemyWalkSpeed;
    configureCombat(config::enemyMeleeDamage + 4, config::enemyAttackRangeMelee,
                    config::enemyMeleeCooldown + 0.4F, false);
}

void Spiny::update(float dt, Game& game) {
    tick(dt);
    animationTime_ += dt;
    body.velocity.x = facing * config::enemyWalkSpeed * speedScale();
    if (body.onGround && !physics::groundAhead(body, game.level(), facing)) facing *= -1;
    body.velocity.y = std::min(body.velocity.y + config::gravity * dt, config::maxFallSpeed);
    const auto collision = physics::moveBody(body, game.level(), dt);
    if (collision.hitLeft || collision.hitRight) facing *= -1;
    if (body.position.y > config::levelHeightTiles * config::tileSize + 120.0F) alive = false;
    updateCombat(dt, game);
}

void Spiny::draw(Renderer& renderer) const {
    glm::vec4 tint = damageTint();
    tint.r *= 1.0F;
    tint.g *= 0.42F;
    tint.b *= 0.32F;
    renderer.drawSprite(Sprite::Shell, body.position.x - 3.0F, body.position.y - 3.0F,
                        body.size.x + 6.0F, body.size.y + 6.0F, facing > 0, tint);
    for (int i = 0; i < 3; ++i) {
        renderer.drawRect(body.position.x + 6.0F + i * 8.0F, body.position.y - 7.0F,
                          4.0F, 8.0F, {0.98F, 0.94F, 0.72F, 1.0F});
    }
    const float eye = std::sin(animationTime_ * 6.0F);
    renderer.drawRect(body.position.x + 8.0F + eye, body.position.y + 10.0F, 4.0F, 4.0F,
                      {0.12F, 0.10F, 0.12F, 1.0F});
    renderer.drawRect(body.position.x + 18.0F - eye, body.position.y + 10.0F, 4.0F, 4.0F,
                      {0.12F, 0.10F, 0.12F, 1.0F});
    drawBadge(renderer);
}

void Spiny::stomped(Game& game) {
    (void)game;
}

Boss::Boss(BossType type, float x, float groundY, int level) : type_(type) {
    setLevel(std::max(1, level));  // 记录等级，沿用等级带来的得分/经验/徽章
    int baseHp = config::bossHpBoomBoom;
    switch (type_) {
    case BossType::BoomBoom:
        baseHp = config::bossHpBoomBoom;
        body.size = {46.0F, 46.0F};
        break;
    case BossType::Birdo:
        baseHp = config::bossHpBirdo;
        body.size = {48.0F, 52.0F};
        break;
    case BossType::Bowser:
        baseHp = config::bossHpBowser;
        body.size = {66.0F, 72.0F};
        break;
    }
    // Boss 生命远高于普通怪物，并随等级增长。
    const int hp = std::min(config::bossMaxHp,
                            baseHp + levelValue() * config::bossHpPerLevel);
    setMaxHitPoints(hp);
    setHitPoints(hp);
    body.position = {x, groundY - body.size.y};
    facing = -1;
    collidable = true;
    attackTimer_ = config::bossAttackInterval;
    jumpTimer_ = 1.2F;
    configureCombat(14, config::enemyAttackRangeRanged + 60.0F,
                    config::bossAttackInterval, true);
}

const char* Boss::bossName() const {
    switch (type_) {
    case BossType::BoomBoom: return "\xE7\x88\x86\xE7\x88\x86";  // 爆爆
    case BossType::Birdo:    return "\xE9\xB8\x9F\xE5\xA7\x90";  // 鸟姐
    case BossType::Bowser:   return "\xE5\xBA\x93\xE5\xB7\xB4";  // 库巴
    }
    return "BOSS";
}

glm::vec4 Boss::bodyColor() const {
    switch (type_) {
    case BossType::BoomBoom: return {0.32F, 0.42F, 0.86F, 1.0F};  // 蓝紫
    case BossType::Birdo:    return {0.96F, 0.48F, 0.68F, 1.0F};  // 粉红
    case BossType::Bowser:   return {0.96F, 0.55F, 0.12F, 1.0F};  // 橙黄
    }
    return {1.0F, 1.0F, 1.0F, 1.0F};
}

void Boss::update(float dt, Game& game) {
    tick(dt);
    animationTime_ += dt;
    attackTimer_ = std::max(0.0F, attackTimer_ - dt);
    jumpTimer_ = std::max(0.0F, jumpTimer_ - dt);
    burstTimer_ = std::max(0.0F, burstTimer_ - dt);

    const bool enraged = hp() * 2 < maxHp();  // 半血以下进入狂暴阶段
    const float speedScale = enraged ? config::bossEnragedSpeedScale : 1.0F;
    const float attackInterval = config::bossAttackInterval *
                                 (enraged ? config::bossEnragedAttackScale : 1.0F);

    const float playerCenterX = game.player().body.position.x +
                                game.player().body.size.x * 0.5F;
    const float centerX = body.position.x + body.size.x * 0.5F;
    const int shotDir = playerCenterX < centerX ? -1 : 1;
    const int shotDamage = combatDamage();

    // 先做地面边缘折返，让本帧速度立即使用修正后的朝向（定点型不参与）。
    if (body.onGround && (type_ == BossType::BoomBoom || type_ == BossType::Bowser) &&
        !physics::groundAhead(body, game.level(), facing)) {
        facing *= -1;
    }

    switch (type_) {
    case BossType::BoomBoom:
        // 冲锋：沿朝向快速移动并周期性跳跃，不使用投射物。
        body.velocity.x = facing * config::bossChargeSpeed * speedScale;
        if (body.onGround && jumpTimer_ <= 0.0F) {
            body.velocity.y = config::bossJumpSpeed;
            body.onGround = false;
            jumpTimer_ = enraged ? 1.2F : 2.0F;
        }
        break;
    case BossType::Birdo:
        // 定点小跳，并朝玩家吐蛋。
        body.velocity.x = 0.0F;
        if (body.onGround && jumpTimer_ <= 0.0F) {
            body.velocity.y = config::bossJumpSpeed * 0.55F;
            body.onGround = false;
            jumpTimer_ = enraged ? 1.3F : 2.1F;
        }
        if (attackTimer_ <= 0.0F) {
            game.spawnBossShot(centerX, body.position.y + body.size.y * 0.35F,
                               shotDir * config::bossShotSpeed, -30.0F, 0, shotDamage);
            attackTimer_ = attackInterval;
            game.audio().play(SoundEffect::Fire);
        }
        break;
    case BossType::Bowser:
        // 慢走 + 跳跃 + 连续火球（扇形散射）。
        body.velocity.x = facing * config::bossWalkSpeed * speedScale;
        if (body.onGround && jumpTimer_ <= 0.0F) {
            body.velocity.y = config::bossJumpSpeed;
            body.onGround = false;
            jumpTimer_ = enraged ? 1.7F : 2.8F;
        }
        if (attackTimer_ <= 0.0F && fireBurst_ == 0) {
            fireBurst_ = config::bossFireBurstCount;
            attackTimer_ = attackInterval;
        }
        if (fireBurst_ > 0 && burstTimer_ <= 0.0F) {
            const float vy = static_cast<float>(fireBurst_ - 2) * 26.0F;
            game.spawnBossShot(centerX, body.position.y + body.size.y * 0.30F,
                               shotDir * config::bossShotSpeed * 1.25F, vy, 1, shotDamage);
            --fireBurst_;
            burstTimer_ = config::bossFireBurstGap;
            game.audio().play(SoundEffect::Fire);
        }
        break;
    }

    body.velocity.y = std::min(body.velocity.y + config::gravity * dt, config::maxFallSpeed);
    const auto collision = physics::moveBody(body, game.level(), dt);
    if (collision.hitLeft || collision.hitRight) facing *= -1;  // 撞墙折返
    if (body.position.y > config::levelHeightTiles * config::tileSize + 160.0F) {
        alive = false;
    }
}

void Boss::takeDamage(Game& game, int amount) {
    if (!applyDamage(game, amount)) return;  // 未致命，仅受击反馈
    awardKill(game, config::bossDefeatScore);  // 大量分数 + 等级奖励
    game.addXP(config::bossDefeatXP);          // 大量经验
    game.spawnParticles(body.position.x + body.size.x * 0.5F,
                        body.position.y + body.size.y * 0.5F,
                        {1.0F, 0.82F, 0.25F, 1.0F}, 20);  // 大量粒子
    game.onBossDefeated(*this);
    alive = false;
    collidable = false;
}

void Boss::stomped(Game& game) {
    takeDamage(game, 1);
    game.audio().play(SoundEffect::Stomp);
}

void Boss::hitByFire(Game& game) {
    takeDamage(game, 1);
}

void Boss::hitByWeapon(Game& game, int damage) {
    takeDamage(game, damage);
}

void Boss::killByStar(Game& game) {
    takeDamage(game, config::bossStarDamage);  // 无敌星只造成一次固定伤害，不秒杀
}

void Boss::draw(Renderer& renderer) const {
    const glm::vec4 tint = damageTint();
    glm::vec4 color = bodyColor();
    color.r *= tint.r;
    color.g *= tint.g;
    color.b *= tint.b;

    const float x = body.position.x;
    const float y = body.position.y;
    const float w = body.size.x;
    const float h = body.size.y;

    // 暗色描边 + 主体色块。
    renderer.drawRect(x - 3.0F, y - 3.0F, w + 6.0F, h + 6.0F,
                      {0.10F, 0.09F, 0.13F, 1.0F});
    renderer.drawRect(x, y, w, h, color);

    // 半血狂暴时叠加脉动红色光环，提示进入第二阶段。
    const bool enraged = hp() * 2 < maxHp();
    if (enraged) {
        const float glow = 0.30F + std::sin(animationTime_ * 12.0F) * 0.15F;
        renderer.drawRect(x - 6.0F, y - 6.0F, w + 12.0F, h + 12.0F,
                          {1.0F, 0.28F, 0.18F, glow});
    }

    // 眼睛：白底 + 黑瞳，随朝向左右移动。
    const float eyeX = x + (facing > 0 ? w * 0.55F : w * 0.22F);
    renderer.drawRect(eyeX, y + h * 0.24F, w * 0.17F, h * 0.18F,
                      {1.0F, 1.0F, 1.0F, 1.0F});
    renderer.drawRect(eyeX + (facing > 0 ? w * 0.09F : 0.0F), y + h * 0.29F,
                      w * 0.07F, h * 0.10F, {0.10F, 0.08F, 0.12F, 1.0F});

    // 用简单图元区分三类 Boss 的外形特征。
    switch (type_) {
    case BossType::BoomBoom:  // 两侧拳头
        renderer.drawRect(x - 9.0F, y + h * 0.42F, 9.0F, 13.0F, color);
        renderer.drawRect(x + w, y + h * 0.42F, 9.0F, 13.0F, color);
        break;
    case BossType::Birdo:  // 前伸的喙
        renderer.drawRect(facing > 0 ? x + w : x - 15.0F, y + h * 0.34F,
                          15.0F, 10.0F, {1.0F, 0.82F, 0.30F, 1.0F});
        break;
    case BossType::Bowser:  // 背刺
        for (int i = 0; i < 3; ++i) {
            renderer.drawRect(x + 8.0F + i * (w / 3.0F), y - 10.0F, 13.0F, 11.0F,
                              {0.92F, 0.93F, 0.96F, 1.0F});
        }
        break;
    }

    drawBadge(renderer);  // 等级徽章与血条
}

BossShot::BossShot(float x, float y, float vx, float vy, int kind, int damage)
    : kind_(kind), damage_(std::max(1, damage)) {
    if (kind_ == 3) body.size = {48.0F, 42.0F};
    else if (kind_ == 2) body.size = {18.0F, 18.0F};
    else body.size = {kind_ == 0 ? 18.0F : 20.0F, 20.0F};
    body.position = {x - body.size.x * 0.5F, y - body.size.y * 0.5F};
    body.velocity = {vx, vy};
    facing = vx >= 0.0F ? 1 : -1;
    collidable = false;  // 与玩家的碰撞由 Game 统一处理
}

void BossShot::update(float dt, Game& game) {
    life_ += dt;
    animationTime_ += dt;
    if (kind_ == 3) {
        // 近战判定只短暂存在，不移动、不参与地形碰撞。
        if (life_ > 0.22F) alive = false;
        return;
    }
    if (kind_ == 0) {
        // 蛋走抛物线；其余投射物保持直线飞行。
        body.velocity.y = std::min(body.velocity.y + config::bossShotGravity * dt,
                                   config::maxFallSpeed);
    }
    const auto collision = physics::moveBody(body, game.level(), dt);
    if (collision.hitLeft || collision.hitRight || collision.hitGround || collision.hitHead ||
        life_ > config::bossShotLife ||
        body.position.y > config::levelHeightTiles * config::tileSize + 80.0F) {
        alive = false;
    }
}

void BossShot::draw(Renderer& renderer) const {
    if (kind_ == 3) {
        const float alpha = std::max(0.0F, 1.0F - life_ / 0.22F);
        renderer.drawRect(body.position.x, body.position.y, body.size.x, body.size.y,
                          {1.0F, 0.22F, 0.16F, 0.55F * alpha});
        renderer.drawRect(body.position.x + 4.0F, body.position.y + 4.0F,
                          body.size.x - 8.0F, body.size.y - 8.0F,
                          {1.0F, 0.68F, 0.22F, 0.45F * alpha});
        return;
    }
    const float pulse = 1.0F + std::sin(animationTime_ * 22.0F) * 0.10F;
    const float size = body.size.x * pulse;
    glm::vec4 tint{1.0F, 0.42F, 0.12F, 1.0F};
    if (kind_ == 0) tint = {1.0F, 0.93F, 0.78F, 1.0F};
    else if (kind_ == 2) tint = {0.72F, 0.45F, 1.0F, 1.0F};
    renderer.drawSprite(Sprite::Fireball,
                        body.position.x + (body.size.x - size) * 0.5F,
                        body.position.y + (body.size.y - size) * 0.5F,
                        size, size, facing < 0, tint);
}
