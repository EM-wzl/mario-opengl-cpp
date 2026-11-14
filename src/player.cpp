#include "player.h"

#include "audio.h"
#include "config.h"
#include "game.h"
#include "input.h"
#include "level.h"
#include "renderer.h"

#include <algorithm>
#include <cmath>

namespace {
float approach(float value, float target, float amount) {
    if (value < target) return std::min(value + amount, target);
    return std::max(value - amount, target);
}

float characterSpeedScale(CharacterType character) {
    return character == CharacterType::Toad ? config::toadSpeedScale : 1.0F;
}

float characterJumpScale(CharacterType character) {
    return character == CharacterType::Luigi ? config::luigiJumpScale : 1.0F;
}

float characterHoldGravityScale(CharacterType character) {
    return character == CharacterType::Peach ? config::peachHoldGravityScale : 1.0F;
}
}  // namespace

Player::Player() {
    body.size = {24.0F, 30.0F};
}

void Player::reset(const glm::vec2& bottomPosition) {
    power = PowerState::Small;
    body.size = {24.0F, 30.0F};
    body.position = {bottomPosition.x, bottomPosition.y - body.size.y};
    body.velocity = {0.0F, 0.0F};
    body.onGround = false;
    facing = 1;
    dead = false;
    invincible = 0.0F;
    starTime = 0.0F;
    skillCooldown = 0.0F;
    dashTime_ = 0.0F;
    airSkillUsed_ = false;
    coyote_ = jumpBuffer_ = jumpHeld_ = animationTime_ = 0.0F;
}

void Player::resetProgression() {
    talents = TalentState{};
    skillCooldown = 0.0F;
    dashTime_ = 0.0F;
    airSkillUsed_ = false;
}

void Player::setCharacter(CharacterType type) {
    character_ = type;
}

void Player::setPower(PowerState newPower) {
    const float bottom = body.position.y + body.size.y;
    power = newPower;
    body.size = power == PowerState::Small ? glm::vec2{24.0F, 30.0F} : glm::vec2{26.0F, 50.0F};
    body.position.y = bottom - body.size.y;
}

void Player::downgrade() {
    if (power == PowerState::Fire) setPower(PowerState::Big);
    else if (power == PowerState::Big) setPower(PowerState::Small);
}

void Player::update(float dt, Game& game) {
    if (dead) {
        body.velocity.y = std::min(body.velocity.y + config::gravity * dt, config::maxFallSpeed);
        body.position += body.velocity * dt;
        return;
    }

    invincible = std::max(0.0F, invincible - dt);
    starTime = std::max(0.0F, starTime - dt);
    skillCooldown = std::max(0.0F, skillCooldown - dt);
    dashTime_ = std::max(0.0F, dashTime_ - dt);
    animationTime_ += dt;
    jumpBuffer_ = std::max(0.0F, jumpBuffer_ - dt);
    coyote_ = body.onGround ? config::coyoteTime : std::max(0.0F, coyote_ - dt);

    Input& input = game.input();
    if (input.pressed(Action::Jump)) jumpBuffer_ = config::jumpBufferTime;
    if (input.pressed(Action::Skill)) useSkill(game);

    const float direction = (input.down(Action::Right) ? 1.0F : 0.0F) -
                            (input.down(Action::Left) ? 1.0F : 0.0F);
    const float speedScale = talents.speed * characterSpeedScale(character_);
    if (dashTime_ > 0.0F) {
        body.velocity.x = facing * config::runSpeed * config::dashSpeedScale * speedScale;
    } else if (direction != 0.0F) {
        facing = direction > 0.0F ? 1 : -1;
        const float maximum = (input.down(Action::Run) ? config::runSpeed : config::walkSpeed) * speedScale;
        const float acceleration = (body.onGround ? config::groundAcceleration : config::airAcceleration) * speedScale;
        body.velocity.x = approach(body.velocity.x, direction * maximum, acceleration * dt);
    } else {
        const float friction = body.onGround ? config::groundFriction : config::airFriction;
        body.velocity.x = approach(body.velocity.x, 0.0F, friction * dt);
    }

    if (jumpBuffer_ > 0.0F && coyote_ > 0.0F) {
        body.velocity.y = config::jumpSpeed * talents.jump * characterJumpScale(character_);
        body.onGround = false;
        coyote_ = 0.0F;
        jumpBuffer_ = 0.0F;
        jumpHeld_ = 0.0F;
        game.audio().play(SoundEffect::Jump);
    }
    // 上升时提前松键，立即削减速度，形成短跳。
    if (input.released(Action::Jump) && body.velocity.y < 0.0F) {
        body.velocity.y *= config::jumpCutMultiplier;
        jumpHeld_ = config::jumpHoldTime;
    }
    const bool holdingJump = input.down(Action::Jump) && body.velocity.y < 0.0F &&
                             jumpHeld_ < config::jumpHoldTime;
    if (holdingJump) jumpHeld_ += dt;
    const float gravityScale = holdingJump
        ? config::jumpHoldGravityScale * characterHoldGravityScale(character_)
        : 1.0F;
    body.velocity.y = std::min(body.velocity.y + config::gravity * gravityScale * dt,
                               config::maxFallSpeed);

    const CollisionResult collision = physics::moveBody(body, game.level(), dt);
    if (body.onGround) airSkillUsed_ = false;
    if (collision.hitHead) {
        for (const auto& [x, y] : collision.headTiles) game.hitBlock(x, y);
    }
    if (collision.hitGround) {
        const int tx = static_cast<int>(std::floor((body.position.x + body.size.x * 0.5F) / config::tileSize));
        const int ty = static_cast<int>(std::floor((body.position.y + body.size.y + 2.0F) / config::tileSize));
        if (game.level().tileAt(tx, ty) == Tile::Spring) {
            body.velocity.y = config::springJumpSpeed * talents.jump * characterJumpScale(character_);
            body.onGround = false;
            airSkillUsed_ = false;
            jumpHeld_ = 0.0F;
            game.audio().play(SoundEffect::Jump);
            game.spawnParticles(body.position.x + body.size.x * 0.5F,
                                body.position.y + body.size.y, {1.0F, 0.85F, 0.25F, 1.0F}, 6);
        }
    }
    if (input.pressed(Action::Fire)) game.tryShoot();

    if (body.position.y > config::levelHeightTiles * config::tileSize + 100.0F) {
        game.fallOut();
        return;
    }
    if (!game.level().definition().grounds.empty() && body.position.x >= game.level().checkpointX()) {
        game.activateCheckpoint();
    }
    if (body.position.x + body.size.x >= game.level().flagX() + 10.0F) {
        if (game.bossAlive()) game.noteBossLock();
        else game.completeLevel();
    }
}

void Player::draw(Renderer& renderer) const {
    if (invincible > 0.0F && static_cast<int>(invincible * 18.0F) % 2 == 0) return;
    Sprite sprite;
    if (power == PowerState::Small) {
        if (!body.onGround || dead) sprite = Sprite::SmallJump;
        else if (std::abs(body.velocity.x) > 20.0F) {
            sprite = static_cast<int>(animationTime_ * 12.0F) % 2 == 0 ? Sprite::SmallWalk0 : Sprite::SmallWalk1;
        } else sprite = Sprite::SmallIdle;
    } else {
        if (!body.onGround || dead) sprite = Sprite::BigJump;
        else if (std::abs(body.velocity.x) > 20.0F) {
            sprite = static_cast<int>(animationTime_ * 12.0F) % 2 == 0 ? Sprite::BigWalk0 : Sprite::BigWalk1;
        } else sprite = Sprite::BigIdle;
    }

    glm::vec4 tint{1.0F};
    if (power == PowerState::Fire) tint = {1.0F, 0.88F, 0.62F, 1.0F};
    else if (character_ == CharacterType::Luigi) tint = {0.74F, 1.0F, 0.78F, 1.0F};
    else if (character_ == CharacterType::Toad) tint = {0.70F, 0.86F, 1.0F, 1.0F};
    else if (character_ == CharacterType::Peach) tint = {1.0F, 0.78F, 0.90F, 1.0F};
    if (starTime > 0.0F) {
        const float phase = animationTime_ * 18.0F;
        tint = {0.75F + std::sin(phase) * 0.25F,
                0.75F + std::sin(phase + 2.1F) * 0.25F,
                0.75F + std::sin(phase + 4.2F) * 0.25F, 1.0F};
    }
    if (invincible > 0.0F) {
        renderer.drawRect(body.position.x - 7.0F, body.position.y - 6.0F,
                          body.size.x + 14.0F, body.size.y + 12.0F,
                          {0.35F, 0.82F, 1.0F, 0.28F});
    }
    renderer.drawSprite(sprite, body.position.x - 4.0F, body.position.y - 3.0F,
                        body.size.x + 8.0F, body.size.y + 6.0F, facing < 0, tint);
}

void Player::hurt(Game& game) {
    if (dead || invincible > 0.0F || starTime > 0.0F) return;
    game.damagePlayer(1, false);
}

void Player::die(Game& game) {
    if (dead) return;
    dead = true;
    body.velocity = {0.0F, -690.0F};
    body.onGround = false;
    invincible = 0.0F;
    game.beginDeath();
}

void Player::bounce() {
    body.velocity.y = config::stompBounceSpeed * talents.stompBounce;
    body.onGround = false;
}

void Player::activateStar(float duration) {
    starTime = std::max(starTime, duration);
    invincible = 0.0F;
}

void Player::useSkill(Game& game) {
    if (dead) return;
    const float cooldownScale = talents.skillCooldown;
    switch (character_) {
    case CharacterType::Mario:
        if (skillCooldown > 0.0F) return;
        invincible = std::max(invincible, config::shieldDuration);
        skillCooldown = config::shieldCooldown * cooldownScale;
        game.audio().play(SoundEffect::Power);
        break;
    case CharacterType::Luigi:
        if (body.onGround || airSkillUsed_) return;
        airSkillUsed_ = true;
        body.velocity.y = config::jumpSpeed * 0.92F * talents.jump * characterJumpScale(character_);
        body.onGround = false;
        coyote_ = 0.0F;
        jumpBuffer_ = 0.0F;
        jumpHeld_ = 0.0F;
        game.audio().play(SoundEffect::Jump);
        break;
    case CharacterType::Toad:
        if (skillCooldown > 0.0F) return;
        dashTime_ = config::dashDuration;
        skillCooldown = config::dashCooldown * cooldownScale;
        game.audio().play(SoundEffect::Kick);
        break;
    case CharacterType::Peach:
        if (skillCooldown > 0.0F) return;
        activateStar(config::veilDuration);
        skillCooldown = config::veilCooldown * cooldownScale;
        game.audio().play(SoundEffect::Power);
        break;
    default:
        return;
    }
    game.spawnParticles(body.position.x + body.size.x * 0.5F,
                        body.position.y + body.size.y * 0.5F,
                        {0.62F, 0.88F, 1.0F, 1.0F}, 8);
}

bool Player::skillReady() const {
    if (character_ == CharacterType::Luigi) return !body.onGround && !airSkillUsed_;
    return skillCooldown <= 0.0F;
}

std::string Player::skillStatus() const {
    if (character_ == CharacterType::Luigi) {
        if (body.onGround) return "落地";
        return airSkillUsed_ ? "已用" : "就绪";
    }
    if (skillCooldown <= 0.0F) return "就绪";
    return "冷却 " + std::to_string(static_cast<int>(std::ceil(skillCooldown)));
}
