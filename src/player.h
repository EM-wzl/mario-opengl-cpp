#pragma once

#include "physics.h"
#include "progression.h"

#include <string>

class Game;
class Renderer;

enum class PowerState { Small, Big, Fire };

class Player {
public:
    Player();

    void reset(const glm::vec2& bottomPosition);
    void resetProgression();
    void setCharacter(CharacterType type);
    CharacterType character() const { return character_; }

    void update(float dt, Game& game);
    void draw(Renderer& renderer) const;
    void setPower(PowerState power);
    void downgrade();
    void hurt(Game& game);
    void die(Game& game);
    void bounce();
    void activateStar(float duration);
    void useSkill(Game& game);
    bool skillReady() const;
    std::string skillStatus() const;

    Body body;
    PowerState power = PowerState::Small;
    int facing = 1;
    bool dead = false;
    float invincible = 0.0F;
    float starTime = 0.0F;
    float skillCooldown = 0.0F;
    TalentState talents;

private:
    CharacterType character_ = CharacterType::Mario;
    float coyote_ = 0.0F;
    float jumpBuffer_ = 0.0F;
    float jumpHeld_ = 0.0F;
    float animationTime_ = 0.0F;
    float dashTime_ = 0.0F;
    bool airSkillUsed_ = false;
};
