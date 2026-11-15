#include "progression.h"

#include <array>
#include <cstddef>

namespace {
constexpr int talentCount = static_cast<int>(TalentId::Count);

struct TalentInfo {
    const char* name;
    const char* description;
};

constexpr std::array<TalentInfo, talentCount> talents{{
    {"迅捷", "移速 +8"},
    {"高跳", "跳跃 +7"},
    {"学者", "经验 +20"},
    {"财富", "分数 +25"},
    {"火球专精", "火球冷却 -18"},
    {"专注", "技能冷却 -20"},
    {"金币磁吸", "拾取范围 +40"},
    {"额外体力", "体力上限 +1"},
    {"踏跃", "踩踏反弹 +15"}
}};

constexpr int weaponCount = static_cast<int>(WeaponType::Count);

struct WeaponInfo {
    const char* name;
    int baseDamage;
    float cooldown;
    bool ranged;
    int price;
};

constexpr std::array<WeaponInfo, weaponCount> weapons{{
    {"拳头", 6, 0.35F, false, 0},
    {"木剑", 18, 0.45F, false, 30},
    {"短弓", 14, 0.55F, true, 45},
    {"火杖", 22, 0.70F, true, 70}
}};

const WeaponInfo& weaponInfo(WeaponType weapon) {
    const int index = static_cast<int>(weapon);
    return index >= 0 && index < weaponCount
        ? weapons[static_cast<std::size_t>(index)] : weapons[0];
}
}  // namespace

const char* modeName(GameMode mode) {
    return mode == GameMode::Story ? "剧情" : "无尽";
}

const char* characterName(CharacterType character) {
    switch (character) {
    case CharacterType::Mario: return "马力欧";
    case CharacterType::Luigi: return "路易吉";
    case CharacterType::Toad: return "奇诺比奥";
    case CharacterType::Peach: return "碧姬";
    default: return "英雄";
    }
}

const char* skillName(CharacterType character) {
    switch (character) {
    case CharacterType::Mario: return "护盾";
    case CharacterType::Luigi: return "二段跳";
    case CharacterType::Toad: return "冲刺";
    case CharacterType::Peach: return "星之庇护";
    default: return "技能";
    }
}

const char* skillDescription(CharacterType character) {
    switch (character) {
    case CharacterType::Mario: return "短暂无敌";
    case CharacterType::Luigi: return "空中额外跳跃";
    case CharacterType::Toad: return "短距高速冲刺";
    case CharacterType::Peach: return "短时星之力";
    default: return "特殊能力";
    }
}

const char* difficultyName(EndlessDifficulty difficulty) {
    switch (difficulty) {
    case EndlessDifficulty::Relaxed: return "轻松";
    case EndlessDifficulty::Normal: return "普通";
    case EndlessDifficulty::Frenzy: return "狂热";
    default: return "普通";
    }
}

const char* talentName(TalentId talent) {
    const int index = static_cast<int>(talent);
    return index >= 0 && index < talentCount ? talents[static_cast<std::size_t>(index)].name : "天赋";
}

const char* talentDescription(TalentId talent) {
    const int index = static_cast<int>(talent);
    return index >= 0 && index < talentCount ? talents[static_cast<std::size_t>(index)].description : "加成";
}

TalentId talentChoice(int level, int slot) {
    const int normalizedLevel = level > 0 ? level : 1;
    const int normalizedSlot = ((slot % 3) + 3) % 3;
    const int index = (normalizedLevel * 3 + normalizedSlot * 4 + normalizedLevel / 3) % talentCount;
    return static_cast<TalentId>(index);
}

int xpRequired(int level) {
    return 90 + 65 * (level > 0 ? level - 1 : 0);
}

const char* weaponName(WeaponType weapon) {
    return weaponInfo(weapon).name;
}

int weaponBaseDamage(WeaponType weapon) {
    return weaponInfo(weapon).baseDamage;
}

float weaponCooldown(WeaponType weapon) {
    return weaponInfo(weapon).cooldown;
}

bool weaponRanged(WeaponType weapon) {
    return weaponInfo(weapon).ranged;
}

int weaponPrice(WeaponType weapon) {
    return weaponInfo(weapon).price;
}

int weaponUpgradeCost(int currentLevel) {
    return 40 * currentLevel;
}
