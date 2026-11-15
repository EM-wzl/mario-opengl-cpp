#pragma once

#include <cstddef>

// 所有常用玩法与显示参数集中在这里，单位均为逻辑像素和秒。
namespace config {
inline constexpr int logicalWidth = 960;
inline constexpr int logicalHeight = 540;
inline constexpr float tileSize = 32.0F;
inline constexpr int levelHeightTiles = 17;
inline constexpr float fixedStep = 1.0F / 60.0F;
inline constexpr float maxFrameTime = 0.25F;

inline constexpr float gravity = 2200.0F;
inline constexpr float maxFallSpeed = 900.0F;
inline constexpr float walkSpeed = 260.0F;
inline constexpr float runSpeed = 380.0F;
inline constexpr float groundAcceleration = 1800.0F;
inline constexpr float airAcceleration = 1080.0F;
inline constexpr float groundFriction = 1800.0F;
inline constexpr float airFriction = 240.0F;
inline constexpr float jumpSpeed = -720.0F;
inline constexpr float jumpHoldTime = 0.25F;
inline constexpr float jumpHoldGravityScale = 0.45F;
inline constexpr float jumpCutMultiplier = 0.45F;
inline constexpr float coyoteTime = 0.10F;
inline constexpr float jumpBufferTime = 0.10F;
inline constexpr float stompBounceSpeed = -450.0F;
inline constexpr float springJumpSpeed = -1080.0F;
inline constexpr float starDuration = 9.0F;

inline constexpr float playerInvincibleTime = 1.6F;
inline constexpr float fireCooldown = 0.35F;
inline constexpr std::size_t maxFireballs = 2;
inline constexpr float fireballSpeed = 390.0F;
inline constexpr float fireballBounce = -390.0F;
inline constexpr float shellSpeed = 520.0F;
inline constexpr float enemyWalkSpeed = 72.0F;

inline constexpr float levelTime = 300.0F;
inline constexpr int levelCount = 8;
// 100 血制：生命按点数结算（旧版为 5 格心），伤害、治疗与掉落均为具体数值。
inline constexpr int initialMaxHp = 100;
inline constexpr int maxHpCap = 200;
inline constexpr float damageInvincibleTime = 0.75F;

// 角色被动与主动技能参数。
inline constexpr float luigiJumpScale = 1.14F;
inline constexpr float toadSpeedScale = 1.13F;
inline constexpr float peachHoldGravityScale = 0.72F;
inline constexpr float shieldDuration = 2.2F;
inline constexpr float shieldCooldown = 8.0F;
inline constexpr float dashDuration = 0.26F;
inline constexpr float dashCooldown = 2.8F;
inline constexpr float dashSpeedScale = 1.85F;
inline constexpr float veilDuration = 3.6F;
inline constexpr float veilCooldown = 10.0F;

// 敌人等级：每 3 级增加 1 点生命，移动速度随等级小幅提升。
inline constexpr int enemyHpPerLevels = 3;
inline constexpr int enemyMaxHp = 6;
inline constexpr float enemySpeedPerLevel = 0.065F;
inline constexpr float enemyMaxSpeedScale = 1.80F;
inline constexpr float coinMagnetSpeed = 420.0F;
inline constexpr float deathSequenceTime = 2.25F;
inline constexpr float levelCompleteTime = 2.8F;
inline constexpr float cameraDeadZoneLeft = logicalWidth / 3.0F;
inline constexpr float cameraDeadZoneRight = logicalWidth / 2.0F;

// 多类型 Boss：生命、移动、攻击与击败奖励参数集中管理，避免魔法数字散落。
inline constexpr int bossHpBoomBoom = 10;        // 爆爆基础生命
inline constexpr int bossHpBirdo = 9;            // 鸟姐基础生命
inline constexpr int bossHpBowser = 14;          // 库巴基础生命
inline constexpr int bossHpPerLevel = 2;         // 每级额外生命
inline constexpr int bossMaxHp = 48;             // Boss 生命上限
inline constexpr int bossStarDamage = 3;         // 无敌星对 Boss 单次伤害
inline constexpr int bossDefeatScore = 1500;     // 击败 Boss 基础分
inline constexpr int bossDefeatXP = 120;         // 击败 Boss 额外经验
inline constexpr float bossChargeSpeed = 168.0F;      // 爆爆冲锋速度
inline constexpr float bossWalkSpeed = 58.0F;         // 库巴慢走速度
inline constexpr float bossEnragedSpeedScale = 1.5F;  // 半血后移动加速
inline constexpr float bossJumpSpeed = -560.0F;       // Boss 起跳速度
inline constexpr float bossAttackInterval = 1.7F;     // 攻击间隔
inline constexpr float bossEnragedAttackScale = 0.6F; // 半血后攻击间隔缩短
inline constexpr int bossFireBurstCount = 3;          // 库巴连续火球数量
inline constexpr float bossFireBurstGap = 0.28F;      // 连发间隔
inline constexpr float bossShotSpeed = 210.0F;        // 投射物水平速度
inline constexpr float bossShotGravity = 520.0F;      // 蛋的抛物重力
inline constexpr float bossShotLife = 4.0F;           // 投射物寿命

// 武器制战斗：玩家近战/远程、敌人攻击、掉落与交互范围。
inline constexpr float playerShotSpeed = 520.0F;
inline constexpr float playerShotLife = 2.5F;
inline constexpr int enemyMeleeDamage = 8;
inline constexpr int enemyRangedDamage = 7;
inline constexpr float enemyAttackRangeMelee = 64.0F;
inline constexpr float enemyAttackRangeRanged = 280.0F;
inline constexpr float enemyMeleeCooldown = 1.4F;
inline constexpr float enemyRangedCooldown = 2.1F;
inline constexpr int fallDamage = 20;
inline constexpr int potionHeal = 30;
inline constexpr int chestHeal = 20;
inline constexpr int chestCoinReward = 25;
inline constexpr float shopInteractRange = 72.0F;
inline constexpr float chestInteractRange = 60.0F;
inline constexpr float weaponMeleeReachFists = 46.0F;
inline constexpr float weaponMeleeReachSword = 76.0F;
inline constexpr float weaponMeleeHeight = 42.0F;
}
