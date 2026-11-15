#pragma once

#include <cstdint>
#include <string>

// 开局模式、角色、难度与成长系统的共享数据结构。
enum class GameMode { Story, Endless };
enum class CharacterType { Mario, Luigi, Toad, Peach, Count };
enum class EndlessDifficulty { Relaxed, Normal, Frenzy, Count };

struct RunOptions {
    GameMode mode = GameMode::Story;
    CharacterType character = CharacterType::Mario;
    EndlessDifficulty difficulty = EndlessDifficulty::Normal;
    int startLevel = 0;
    std::uint32_t seed = 1;
};

// 天赋以乘法/加法修正集中存放，死亡重生与过关时保持不变，仅新开一局重置。
struct TalentState {
    float speed = 1.0F;
    float jump = 1.0F;
    float xp = 1.0F;
    float score = 1.0F;
    float fireCooldown = 1.0F;
    float skillCooldown = 1.0F;
    float stompBounce = 1.0F;
    float magnet = 0.0F;
};

enum class TalentId {
    Swift,
    HighJump,
    Scholar,
    Fortune,
    Pyromancer,
    Focus,
    Magnet,
    Heart,
    Bounce,
    Count
};

const char* modeName(GameMode mode);
const char* characterName(CharacterType character);
const char* skillName(CharacterType character);
const char* skillDescription(CharacterType character);
const char* difficultyName(EndlessDifficulty difficulty);
const char* talentName(TalentId talent);
const char* talentDescription(TalentId talent);

// 三级天赋每次提供不重复的三选一；相同等级与槽位始终给出相同选项，便于复现。
TalentId talentChoice(int level, int slot);
int xpRequired(int level);

// 武器制：四种武器，拳头默认持有；其余需购买，等级可用金币升级。
enum class WeaponType { Fists, Sword, Bow, Wand, Count };
struct WeaponState {
    bool owned[4] = {true, false, false, false};
    int level[4] = {1, 1, 1, 1};
};
const char* weaponName(WeaponType weapon);
int weaponBaseDamage(WeaponType weapon);
float weaponCooldown(WeaponType weapon);
bool weaponRanged(WeaponType weapon);
int weaponPrice(WeaponType weapon);
int weaponUpgradeCost(int currentLevel);
