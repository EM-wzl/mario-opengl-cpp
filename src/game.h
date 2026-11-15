#pragma once

#include "audio.h"
#include "enemy.h"
#include "input.h"
#include "item.h"
#include "level.h"
#include "platform.h"
#include "player.h"
#include "progression.h"

#include <array>
#include <cstdint>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <string>
#include <vector>

class Renderer;

enum class GameState {
    Title,
    Playing,
    Paused,
    Dying,
    LevelComplete,
    GameOver,
    Won,
    LevelUp,
    Shop
};

class Game {
public:
    Game(Input& input, Audio& audio);

    void update(float dt);
    void render(Renderer& renderer);
    GameState state() const { return state_; }

    Input& input() { return input_; }
    Audio& audio() { return audio_; }
    Level& level() { return *level_; }
    Player& player() { return player_; }

    void hitBlock(int x, int y);
    void tryShoot();
    void addScore(int amount);
    void addXP(int baseAmount);
    void addCoin();
    void heal(int amount);
    void activateCheckpoint();
    void completeLevel();
    void beginDeath();
    void damagePlayer(int amount, bool ignoreInvincible = false);
    void fallOut();
    void spawnParticles(float x, float y, const glm::vec4& color, int count = 8);
    void spawnBossShot(float x, float y, float vx, float vy, int kind, int damage = 10);
    void spawnPlayerShot(float x, float y, float vx, float vy, int damage, int kind);
    void onBossDefeated(const Boss& boss);
    void noteBossLock();
    bool bossAlive() const;
    int hp() const { return hp_; }
    int maxHp() const { return maxHp_; }
    int coins() const { return coins_; }

    void performAttack();
    void cycleWeapon();
    WeaponType currentWeapon() const { return currentWeapon_; }
    float weaponCooldown() const { return weaponCooldown_; }
    void interact();
    void openShop();
    void closeShop();
    bool buyShopItem(int index);
    void openChest(int index);

    bool saveGame(int slot);
    bool loadGame(int slot);

    // 开局入口：剧情选关、自定义开局，以及无窗口冒烟测试。
    void startRun(const RunOptions& options);
    void startAtLevel(int index);
    void startEndlessZone(int zone, std::uint32_t seed, int difficulty,
                          CharacterType character);

private:
    struct Particle {
        glm::vec2 position;
        glm::vec2 velocity;
        glm::vec4 color;
        float life = 0.0F;
        float size = 5.0F;
    };

    enum class InteractKind { None, Shop, Chest };
    struct InteractTarget {
        InteractKind kind = InteractKind::None;
        int index = -1;
    };

    struct SaveData {
        int version = 2;
        int mode = 0;
        int character = 0;
        int difficulty = 1;
        std::uint32_t seed = 1;
        int startLevel = 0;
        int levelIndex = 0;
        int endlessZone = 1;
        int score = 0;
        int coins = 0;
        int hp = 100;
        int maxHp = 100;
        int playerLevel = 1;
        int xp = 0;
        int power = 0;
        int checkpoint = 0;
        int currentWeapon = 0;
        float timeLeft = 300.0F;
        TalentState talents{};
        WeaponState weapons{};
    };

    void newGame(int startLevel = 0);
    void loadLevel(int index, bool useCheckpoint);
    void updateTitle(float dt);
    void updateLevelUp();
    void updateShop();
    void adjustTitle(int delta);
    void updatePlaying(float dt);
    void updateParticles(float dt);
    void handleCollisions(float previousPlayerBottom);
    void attractCoins(float dt);
    void prepareTalentChoices();
    void applyTalent(TalentId talent);
    void respawnAfterDamage();
    const Boss* activeBoss() const;
    void showSaveMessage(const std::string& message);
    void showShopMessage(const std::string& message);
    InteractTarget findInteractTarget() const;
    void performMeleeAttack(WeaponType weapon, int damage);
    void updateCombatEffects(float dt);
    std::string savePath(int slot) const;
    bool readSave(int slot, SaveData& data) const;
    std::string saveSummary(int slot) const;
    void updateCamera();
    void cleanupEntities();
    void drawBackground(Renderer& renderer);
    void drawWorldInteractables(Renderer& renderer) const;
    void drawHud(Renderer& renderer);
    void drawOverlay(Renderer& renderer);
    void drawShop(Renderer& renderer) const;
    void drawInteractionPrompt(Renderer& renderer) const;
    void loadHighScore();
    void saveHighScore() const;
    std::string highScorePath() const;

    Input& input_;
    Audio& audio_;
    std::unique_ptr<Level> level_;
    Player player_;
    std::vector<std::unique_ptr<Item>> items_;
    std::vector<std::unique_ptr<Enemy>> enemies_;
    std::vector<std::unique_ptr<Fireball>> fireballs_;
    std::vector<std::unique_ptr<BossShot>> bossShots_;
    std::vector<std::unique_ptr<PlayerShot>> playerShots_;
    std::vector<MovingPlatform> platforms_;
    std::vector<Particle> particles_;
    std::vector<bool> chestOpened_;

    GameState state_ = GameState::Title;
    RunOptions options_{};
    int levelIndex_ = 0;
    int titleRow_ = 0;
    int playerLevel_ = 1;
    int xp_ = 0;
    int pendingLevelUps_ = 0;
    int talentCursor_ = 0;
    std::array<TalentId, 3> talentChoices_{};
    bool endlessMode_ = false;
    int endlessZone_ = 1;
    std::uint32_t endlessSeed_ = 1;
    int endlessDifficulty_ = 1;
    int saveSlot_ = 1;
    float saveMessageTimer_ = 0.0F;
    float bossLockTimer_ = 0.0F;
    std::string saveMessage_;

    WeaponState weapons_{};
    WeaponType currentWeapon_ = WeaponType::Fists;
    float weaponCooldown_ = 0.0F;
    float meleeFlashTimer_ = 0.0F;
    glm::vec2 meleeRectPos_{0.0F};
    glm::vec2 meleeRectSize_{0.0F};
    int shopCursor_ = 0;
    float shopMessageTimer_ = 0.0F;
    std::string shopMessage_;

    int score_ = 0;
    int highScore_ = 0;
    int coins_ = 0;
    int hp_ = 100;
    int maxHp_ = 100;
    float timeLeft_ = 300.0F;
    float stateTimer_ = 0.0F;
    float fireCooldown_ = 0.0F;
    float cameraX_ = 0.0F;
    float flagDrop_ = 0.0F;
    float levelIntroTimer_ = 0.0F;
    bool checkpointActive_ = false;
};
