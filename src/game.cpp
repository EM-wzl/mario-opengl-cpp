#include "game.h"

#include "config.h"
#include "renderer.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_map>

namespace {
std::string padded(int value, int width) {
    std::ostringstream stream;
    stream << std::setfill('0') << std::setw(width) << value;
    return stream.str();
}

template <typename T>
void removeDead(std::vector<std::unique_ptr<T>>& entities) {
    entities.erase(std::remove_if(entities.begin(), entities.end(),
                                  [](const auto& entity) { return !entity->alive; }), entities.end());
}

template <typename T>
T parseValue(const std::unordered_map<std::string, std::string>& values,
             const std::string& key, T fallback) {
    const auto it = values.find(key);
    if (it == values.end()) return fallback;
    T result{};
    std::istringstream stream(it->second);
    if (!(stream >> result)) return fallback;
    return result;
}
}  // namespace

Game::Game(Input& input, Audio& audio) : input_(input), audio_(audio) {
    loadHighScore();
    loadLevel(0, false);
    state_ = GameState::Title;
}

std::string Game::savePath(int slot) const {
#ifdef _WIN32
    const char* home = std::getenv("USERPROFILE");
#else
    const char* home = std::getenv("HOME");
#endif
    const int safeSlot = std::clamp(slot, 1, 3);
    return std::string(home != nullptr ? home : ".") + "/.mario_save_" +
           std::to_string(safeSlot) + ".dat";
}

bool Game::readSave(int slot, SaveData& data) const {
    std::ifstream file(savePath(slot));
    if (!file) return false;
    std::unordered_map<std::string, std::string> values;
    std::string line;
    while (std::getline(file, line)) {
        const std::size_t separator = line.find('=');
        if (separator == std::string::npos) continue;
        values[line.substr(0, separator)] = line.substr(separator + 1);
    }
    data.version = parseValue(values, "version", 0);
    if (data.version != 1) return false;
    data.mode = parseValue(values, "mode", 0);
    data.character = parseValue(values, "character", 0);
    data.difficulty = parseValue(values, "difficulty", 1);
    data.seed = parseValue(values, "seed", std::uint32_t{1});
    data.startLevel = parseValue(values, "startLevel", 0);
    data.levelIndex = parseValue(values, "levelIndex", 0);
    data.endlessZone = parseValue(values, "endlessZone", 1);
    data.score = parseValue(values, "score", 0);
    data.coins = parseValue(values, "coins", 0);
    data.hp = parseValue(values, "hp", config::initialMaxHp);
    data.maxHp = parseValue(values, "maxHp", config::initialMaxHp);
    data.playerLevel = parseValue(values, "playerLevel", 1);
    data.xp = parseValue(values, "xp", 0);
    data.power = parseValue(values, "power", 0);
    data.checkpoint = parseValue(values, "checkpoint", 0);
    data.timeLeft = parseValue(values, "timeLeft", 300.0F);
    data.talents.speed = parseValue(values, "talentSpeed", 1.0F);
    data.talents.jump = parseValue(values, "talentJump", 1.0F);
    data.talents.xp = parseValue(values, "talentXp", 1.0F);
    data.talents.score = parseValue(values, "talentScore", 1.0F);
    data.talents.fireCooldown = parseValue(values, "talentFire", 1.0F);
    data.talents.skillCooldown = parseValue(values, "talentSkill", 1.0F);
    data.talents.stompBounce = parseValue(values, "talentBounce", 1.0F);
    data.talents.magnet = parseValue(values, "talentMagnet", 0.0F);
    return true;
}

std::string Game::saveSummary(int slot) const {
    SaveData data;
    if (!readSave(slot, data)) return "空存档";
    return "等级 " + std::to_string(data.playerLevel) + " 体力 " +
           std::to_string(data.hp) + "/" + std::to_string(data.maxHp);
}

bool Game::saveGame(int slot) {
    if (state_ == GameState::Title || state_ == GameState::Dying ||
        state_ == GameState::GameOver) return false;
    const int safeSlot = std::clamp(slot, 1, 3);
    std::ofstream file(savePath(safeSlot), std::ios::trunc);
    if (!file) {
        showSaveMessage("保存失败");
        return false;
    }
    file << std::setprecision(7);
    file << "version=1\n"
         << "mode=" << (endlessMode_ ? 1 : 0) << '\n'
         << "character=" << static_cast<int>(player_.character()) << '\n'
         << "difficulty=" << endlessDifficulty_ << '\n'
         << "seed=" << endlessSeed_ << '\n'
         << "startLevel=" << options_.startLevel << '\n'
         << "levelIndex=" << levelIndex_ << '\n'
         << "endlessZone=" << endlessZone_ << '\n'
         << "score=" << score_ << '\n'
         << "coins=" << coins_ << '\n'
         << "hp=" << hp_ << '\n'
         << "maxHp=" << maxHp_ << '\n'
         << "playerLevel=" << playerLevel_ << '\n'
         << "xp=" << xp_ << '\n'
         << "power=" << static_cast<int>(player_.power) << '\n'
         << "checkpoint=" << (checkpointActive_ ? 1 : 0) << '\n'
         << "timeLeft=" << timeLeft_ << '\n'
         << "talentSpeed=" << player_.talents.speed << '\n'
         << "talentJump=" << player_.talents.jump << '\n'
         << "talentXp=" << player_.talents.xp << '\n'
         << "talentScore=" << player_.talents.score << '\n'
         << "talentFire=" << player_.talents.fireCooldown << '\n'
         << "talentSkill=" << player_.talents.skillCooldown << '\n'
         << "talentBounce=" << player_.talents.stompBounce << '\n'
         << "talentMagnet=" << player_.talents.magnet << '\n';
    showSaveMessage("已保存存档位 " + std::to_string(safeSlot));
    return true;
}

bool Game::loadGame(int slot) {
    SaveData data;
    if (!readSave(slot, data)) {
        showSaveMessage("无存档");
        return false;
    }
    saveSlot_ = std::clamp(slot, 1, 3);
    options_.mode = data.mode == 1 ? GameMode::Endless : GameMode::Story;
    options_.character = static_cast<CharacterType>(std::clamp(data.character, 0, 3));
    options_.difficulty = static_cast<EndlessDifficulty>(std::clamp(data.difficulty, 0, 2));
    options_.seed = data.seed == 0 ? 1U : data.seed;
    options_.startLevel = std::clamp(data.startLevel, 0, config::levelCount - 1);
    endlessMode_ = options_.mode == GameMode::Endless;
    endlessSeed_ = options_.seed;
    endlessDifficulty_ = std::clamp(data.difficulty, 0, 2);
    endlessZone_ = std::max(1, data.endlessZone);
    levelIndex_ = std::clamp(data.levelIndex, 0, config::levelCount - 1);
    score_ = std::max(0, data.score);
    coins_ = std::max(0, data.coins);
    maxHp_ = std::clamp(data.maxHp, 1, config::maxHpCap);
    hp_ = std::clamp(data.hp, 1, maxHp_);
    playerLevel_ = std::max(1, data.playerLevel);
    xp_ = std::max(0, data.xp);
    pendingLevelUps_ = 0;
    player_.resetProgression();
    player_.talents = data.talents;
    player_.setCharacter(options_.character);

    state_ = GameState::Title;
    loadLevel(levelIndex_, data.checkpoint != 0);
    player_.setPower(static_cast<PowerState>(std::clamp(data.power, 0, 2)));
    player_.invincible = config::damageInvincibleTime;
    timeLeft_ = std::max(1.0F, data.timeLeft);
    levelIntroTimer_ = 1.2F;
    state_ = GameState::Playing;
    showSaveMessage("已读取存档位 " + std::to_string(saveSlot_));
    return true;
}

void Game::showSaveMessage(const std::string& message) {
    saveMessage_ = message;
    saveMessageTimer_ = 1.6F;
}

const Boss* Game::activeBoss() const {
    for (const auto& enemy : enemies_) {
        const auto* boss = dynamic_cast<const Boss*>(enemy.get());
        if (boss != nullptr && boss->alive) return boss;
    }
    return nullptr;
}

bool Game::bossAlive() const {
    return activeBoss() != nullptr;
}

void Game::spawnBossShot(float x, float y, float vx, float vy, int kind) {
    if (bossShots_.size() >= 16) return;
    bossShots_.push_back(std::make_unique<BossShot>(x, y, vx, vy, kind));
}

void Game::onBossDefeated(const Boss& boss) {
    (void)boss;
    audio_.play(SoundEffect::Clear);
    showSaveMessage("首领已击败");
    saveGame(saveSlot_);
}

void Game::noteBossLock() {
    bossLockTimer_ = 1.2F;
}

void Game::startRun(const RunOptions& options) {
    options_ = options;
    endlessMode_ = options.mode == GameMode::Endless;
    endlessZone_ = 1;
    endlessSeed_ = options.seed == 0 ? 1U : options.seed;
    endlessDifficulty_ = std::clamp(static_cast<int>(options.difficulty), 0, 2);
    levelIndex_ = std::clamp(options.startLevel, 0, config::levelCount - 1);
    score_ = 0;
    coins_ = 0;
    maxHp_ = config::initialMaxHp;
    hp_ = maxHp_;
    xp_ = 0;
    playerLevel_ = 1;
    pendingLevelUps_ = 0;
    talentCursor_ = 0;
    checkpointActive_ = false;
    player_.resetProgression();
    player_.setCharacter(options.character);
    state_ = GameState::Title;
    loadLevel(levelIndex_, false);
    state_ = GameState::Playing;
    saveGame(saveSlot_);
}

void Game::startAtLevel(int index) {
    RunOptions options;
    options.mode = GameMode::Story;
    options.startLevel = index;
    startRun(options);
}

void Game::startEndlessZone(int zone, std::uint32_t seed, int difficulty,
                            CharacterType character) {
    RunOptions options;
    options.mode = GameMode::Endless;
    options.seed = seed == 0 ? 1U : seed;
    options.difficulty = static_cast<EndlessDifficulty>(std::clamp(difficulty, 0, 2));
    options.character = character;
    startRun(options);
    endlessZone_ = std::clamp(zone, 1, 999);
    loadLevel(levelIndex_, false);
    state_ = GameState::Playing;
    saveGame(saveSlot_);
}

void Game::newGame(int startLevel) {
    RunOptions options = options_;
    options.startLevel = startLevel;
    startRun(options);
}

void Game::loadLevel(int index, bool useCheckpoint) {
    const PowerState carriedPower = player_.power;
    const bool keepPower = state_ == GameState::LevelComplete && !useCheckpoint;

    levelIndex_ = index;
    if (endlessMode_) {
        level_ = std::make_unique<Level>(
            makeEndlessLevelDefinition(endlessZone_, endlessSeed_, endlessDifficulty_));
    } else {
        level_ = std::make_unique<Level>(index);
    }
    items_.clear();
    enemies_.clear();
    fireballs_.clear();
    bossShots_.clear();
    platforms_.clear();
    particles_.clear();
    checkpointActive_ = useCheckpoint;
    timeLeft_ = level_->definition().timeLimit > 0.0F ? level_->definition().timeLimit : config::levelTime;
    fireCooldown_ = 0.0F;
    flagDrop_ = 0.0F;
    levelIntroTimer_ = 2.2F;

    const glm::vec2 start = useCheckpoint
        ? glm::vec2{level_->checkpointX(), 15.0F * config::tileSize}
        : level_->playerStart();
    player_.reset(start);
    if (keepPower) player_.setPower(carriedPower);

    for (const auto& spawn : level_->definition().enemies) {
        const float x = spawn.x * config::tileSize;
        const float y = spawn.y * config::tileSize;
        std::unique_ptr<Enemy> enemy;
        switch (spawn.type) {
        case EnemySpawnType::Goomba: enemy = std::make_unique<Goomba>(x, y); break;
        case EnemySpawnType::Koopa: enemy = std::make_unique<Koopa>(x, y); break;
        case EnemySpawnType::Piranha:
            enemy = std::make_unique<Piranha>(x + config::tileSize * 0.5F, y);
            break;
        case EnemySpawnType::Paratroopa: enemy = std::make_unique<Paratroopa>(x, y); break;
        case EnemySpawnType::Spiny: enemy = std::make_unique<Spiny>(x, y); break;
        }
        if (!enemy) continue;
        enemy->setLevel(spawn.level + (playerLevel_ - 1) / 3);
        enemies_.push_back(std::move(enemy));
    }
    for (const auto& boss : level_->definition().bosses) {
        enemies_.push_back(std::make_unique<Boss>(
            boss.type, boss.x * config::tileSize, boss.y * config::tileSize,
            boss.level + (playerLevel_ - 1) / 3));
    }
    for (const auto& placement : level_->definition().platforms) {
        platforms_.emplace_back(placement);
    }
    for (const auto& coin : level_->definition().coins) {
        items_.push_back(std::make_unique<Item>(ItemType::Coin, coin.x * config::tileSize + 4.0F,
                                                coin.y * config::tileSize + 2.0F));
    }
    updateCamera();
}

void Game::adjustTitle(int delta) {
    switch (titleRow_) {
    case 0:
        options_.mode = options_.mode == GameMode::Story ? GameMode::Endless : GameMode::Story;
        break;
    case 1: {
        const int count = static_cast<int>(CharacterType::Count);
        const int value = static_cast<int>(options_.character);
        options_.character = static_cast<CharacterType>((value + delta + count) % count);
        break;
    }
    case 2:
        if (options_.mode == GameMode::Story) {
            const int count = config::levelCount;
            options_.startLevel = (options_.startLevel + delta + count) % count;
        } else {
            const int count = static_cast<int>(EndlessDifficulty::Count);
            const int value = static_cast<int>(options_.difficulty);
            options_.difficulty = static_cast<EndlessDifficulty>((value + delta + count) % count);
        }
        break;
    case 3: {
        int seed = static_cast<int>(options_.seed) + delta * 7;
        while (seed < 1) seed += 999;
        while (seed > 999) seed -= 999;
        options_.seed = static_cast<std::uint32_t>(seed);
        break;
    }
    default:
        saveSlot_ = (saveSlot_ - 1 + delta + 3) % 3 + 1;
        break;
    }
}

void Game::updateTitle(float dt) {
    updateParticles(dt);
    if (input_.pressed(Action::MenuUp)) titleRow_ = (titleRow_ + 4) % 5;
    if (input_.pressed(Action::MenuDown)) titleRow_ = (titleRow_ + 1) % 5;
    if (input_.pressed(Action::Left)) adjustTitle(-1);
    if (input_.pressed(Action::Right)) adjustTitle(1);
    if (input_.pressed(Action::Confirm)) startRun(options_);
}

void Game::updateLevelUp() {
    if (input_.pressed(Action::Left)) talentCursor_ = (talentCursor_ + 2) % 3;
    if (input_.pressed(Action::Right)) talentCursor_ = (talentCursor_ + 1) % 3;
    if (!input_.pressed(Action::Confirm)) return;
    applyTalent(talentChoices_[static_cast<std::size_t>(talentCursor_)]);
    audio_.play(SoundEffect::Power);
    --pendingLevelUps_;
    if (pendingLevelUps_ > 0) {
        prepareTalentChoices();
    } else {
        state_ = GameState::Playing;
        saveGame(saveSlot_);
    }
}

void Game::prepareTalentChoices() {
    const int levelForChoice = std::max(1, playerLevel_ - pendingLevelUps_ + 1);
    for (int slot = 0; slot < 3; ++slot) {
        talentChoices_[static_cast<std::size_t>(slot)] = talentChoice(levelForChoice, slot);
    }
    talentCursor_ = 0;
}

void Game::applyTalent(TalentId talent) {
    switch (talent) {
    case TalentId::Swift: player_.talents.speed *= 1.08F; break;
    case TalentId::HighJump: player_.talents.jump *= 1.07F; break;
    case TalentId::Scholar: player_.talents.xp *= 1.20F; break;
    case TalentId::Fortune: player_.talents.score *= 1.25F; break;
    case TalentId::Pyromancer:
        player_.talents.fireCooldown = std::max(0.35F, player_.talents.fireCooldown * 0.82F);
        break;
    case TalentId::Focus:
        player_.talents.skillCooldown = std::max(0.40F, player_.talents.skillCooldown * 0.80F);
        break;
    case TalentId::Magnet:
        player_.talents.magnet = std::min(240.0F, player_.talents.magnet + 40.0F);
        break;
    case TalentId::Heart:
        maxHp_ = std::min(config::maxHpCap, maxHp_ + 1);
        heal(1);
        player_.invincible = std::max(player_.invincible, 1.5F);
        break;
    case TalentId::Bounce: player_.talents.stompBounce *= 1.15F; break;
    default: break;
    }
}

void Game::heal(int amount) {
    hp_ = std::min(maxHp_, hp_ + std::max(0, amount));
}

void Game::damagePlayer(int amount, bool ignoreInvincible) {
    if (state_ != GameState::Playing || player_.dead) return;
    if (!ignoreInvincible && (player_.invincible > 0.0F || player_.starTime > 0.0F)) return;
    hp_ -= std::max(1, amount);
    spawnParticles(player_.body.position.x + player_.body.size.x * 0.5F,
                   player_.body.position.y + player_.body.size.y * 0.5F,
                   {1.0F, 0.28F, 0.25F, 1.0F}, 10);
    if (hp_ <= 0) {
        hp_ = 0;
        player_.die(*this);
        return;
    }
    player_.downgrade();
    player_.invincible = config::damageInvincibleTime;
    audio_.play(SoundEffect::Power);
    saveGame(saveSlot_);
}

void Game::fallOut() {
    if (state_ != GameState::Playing || player_.dead) return;
    --hp_;
    if (hp_ <= 0) {
        hp_ = 0;
        player_.die(*this);
        return;
    }
    respawnAfterDamage();
    audio_.play(SoundEffect::Death);
    saveGame(saveSlot_);
}

void Game::respawnAfterDamage() {
    const PowerState currentPower = player_.power;
    const glm::vec2 start = checkpointActive_
        ? glm::vec2{level_->checkpointX(), 15.0F * config::tileSize}
        : level_->playerStart();
    player_.reset(start);
    if (currentPower == PowerState::Fire) player_.setPower(PowerState::Big);
    else if (currentPower == PowerState::Big) player_.setPower(PowerState::Small);
    player_.invincible = config::damageInvincibleTime * 1.2F;
    fireballs_.clear();
    bossShots_.clear();
    updateCamera();
}

void Game::update(float dt) {
    if (input_.pressed(Action::Mute)) audio_.toggleMute();
    saveMessageTimer_ = std::max(0.0F, saveMessageTimer_ - dt);
    bossLockTimer_ = std::max(0.0F, bossLockTimer_ - dt);
    if (input_.pressed(Action::Save) && state_ != GameState::Title &&
        state_ != GameState::Dying && state_ != GameState::GameOver) {
        saveGame(saveSlot_);
    }
    if (input_.pressed(Action::Load) && state_ != GameState::Dying) {
        loadGame(saveSlot_);
    }

    switch (state_) {
    case GameState::Title:
        updateTitle(dt);
        break;
    case GameState::Playing:
        if (input_.pressed(Action::Pause)) state_ = GameState::Paused;
        else updatePlaying(dt);
        break;
    case GameState::Paused:
        if (input_.pressed(Action::Pause) || input_.pressed(Action::Confirm)) state_ = GameState::Playing;
        break;
    case GameState::LevelUp:
        updateLevelUp();
        updateParticles(dt);
        break;
    case GameState::Dying:
        stateTimer_ += dt;
        player_.update(dt, *this);
        updateParticles(dt);
        if (stateTimer_ >= config::deathSequenceTime) {
            saveHighScore();
            state_ = GameState::GameOver;
        }
        break;
    case GameState::LevelComplete:
        stateTimer_ += dt;
        flagDrop_ += 155.0F * dt;
        player_.body.velocity = {0.0F, 0.0F};
        player_.body.position.y = std::min(player_.body.position.y + 110.0F * dt,
                                           15.0F * config::tileSize - player_.body.size.y);
        updateParticles(dt);
        if (stateTimer_ >= config::levelCompleteTime) {
            if (endlessMode_) {
                ++endlessZone_;
                checkpointActive_ = false;
                loadLevel(levelIndex_, false);
                state_ = GameState::Playing;
                saveGame(saveSlot_);
            } else if (levelIndex_ + 1 < config::levelCount) {
                checkpointActive_ = false;
                loadLevel(levelIndex_ + 1, false);
                state_ = GameState::Playing;
                saveGame(saveSlot_);
            } else {
                saveHighScore();
                state_ = GameState::Won;
            }
        }
        break;
    case GameState::GameOver:
    case GameState::Won:
        if (input_.pressed(Action::Confirm)) newGame();
        break;
    }
}

void Game::updatePlaying(float dt) {
    const float previousBottom = player_.body.position.y + player_.body.size.y;
    timeLeft_ = std::max(0.0F, timeLeft_ - dt);
    fireCooldown_ = std::max(0.0F, fireCooldown_ - dt);
    levelIntroTimer_ = std::max(0.0F, levelIntroTimer_ - dt);
    level_->update(dt);
    for (auto& platform : platforms_) platform.update(dt, *this);
    player_.update(dt, *this);
    if (state_ != GameState::Playing) return;
    for (const auto& platform : platforms_) platform.resolvePlayer(*this, previousBottom);
    if (timeLeft_ <= 0.0F) {
        player_.die(*this);
        return;
    }
    for (auto& item : items_) if (item->alive) item->update(dt, *this);
    for (auto& enemy : enemies_) if (enemy->alive) enemy->update(dt, *this);
    for (auto& fireball : fireballs_) if (fireball->alive) fireball->update(dt, *this);
    for (auto& shot : bossShots_) if (shot->alive) shot->update(dt, *this);
    attractCoins(dt);
    handleCollisions(previousBottom);
    updateParticles(dt);
    cleanupEntities();
    updateCamera();
}

void Game::attractCoins(float dt) {
    const float radius = player_.talents.magnet;
    if (radius <= 0.0F) return;
    const glm::vec2 target = player_.body.position + player_.body.size * 0.5F;
    for (auto& item : items_) {
        if (!item->alive || item->type() != ItemType::Coin) continue;
        const glm::vec2 center = item->body.position + item->body.size * 0.5F;
        const glm::vec2 delta = target - center;
        const float distance = std::hypot(delta.x, delta.y);
        if (distance > radius || distance < 0.001F) continue;
        const float step = std::min(distance, config::coinMagnetSpeed * dt);
        item->body.position += delta / distance * step;
    }
}

void Game::handleCollisions(float previousPlayerBottom) {
    for (auto& item : items_) {
        if (!item->alive || !physics::overlaps(player_.body, item->body)) continue;
        item->alive = false;
        switch (item->type()) {
        case ItemType::Coin: addCoin(); break;
        case ItemType::Mushroom:
            player_.setPower(PowerState::Big);
            addScore(1000);
            audio_.play(SoundEffect::Power);
            break;
        case ItemType::Flower:
            player_.setPower(PowerState::Fire);
            addScore(1500);
            audio_.play(SoundEffect::Power);
            break;
        case ItemType::OneUp:
            heal(1);
            addScore(1000);
            audio_.play(SoundEffect::Power);
            spawnParticles(item->body.position.x + 12.0F, item->body.position.y,
                           {0.35F, 1.0F, 0.42F, 1.0F}, 10);
            break;
        case ItemType::Star:
            player_.activateStar(config::starDuration);
            addScore(1000);
            audio_.play(SoundEffect::Power);
            break;
        }
    }

    for (auto& shell : enemies_) {
        auto* koopa = dynamic_cast<Koopa*>(shell.get());
        if (koopa == nullptr || !koopa->alive || koopa->state() != KoopaState::Sliding) continue;
        for (auto& other : enemies_) {
            if (other.get() == koopa || !other->dangerous() || dynamic_cast<Piranha*>(other.get()) != nullptr) continue;
            if (std::abs(other->body.position.x - koopa->body.position.x) > 96.0F) continue;
            if (physics::overlaps(koopa->body, other->body)) {
                other->hitByFire(*this);
                addScore(200);
            }
        }
    }

    for (auto& fireball : fireballs_) {
        if (!fireball->alive) continue;
        for (auto& enemy : enemies_) {
            if (!enemy->dangerous() || std::abs(enemy->body.position.x - fireball->body.position.x) > 96.0F) continue;
            if (!physics::overlaps(fireball->body, enemy->body)) continue;
            enemy->hitByFire(*this);
            fireball->alive = false;
            audio_.play(SoundEffect::Stomp);
            break;
        }
    }

    for (auto& shot : bossShots_) {
        if (!shot->alive || !physics::overlaps(player_.body, shot->body)) continue;
        shot->alive = false;
        damagePlayer(1, false);
        if (player_.dead || state_ != GameState::Playing) break;
    }

    for (auto& enemy : enemies_) {
        if (!enemy->dangerous() || std::abs(enemy->body.position.x - player_.body.position.x) > 128.0F) continue;
        if (!physics::overlaps(player_.body, enemy->body)) continue;
        if (player_.starTime > 0.0F) {
            enemy->killByStar(*this);
            audio_.play(SoundEffect::Stomp);
            continue;
        }
        const bool stomp = enemy->stompable() && previousPlayerBottom <= enemy->body.position.y + 12.0F &&
                           player_.body.velocity.y >= 0.0F;
        if (stomp) {
            enemy->stomped(*this);
            player_.bounce();
            continue;
        }
        auto* koopa = dynamic_cast<Koopa*>(enemy.get());
        if (koopa != nullptr && koopa->state() == KoopaState::Shell) {
            const int direction = player_.body.position.x + player_.body.size.x * 0.5F <
                                  enemy->body.position.x + enemy->body.size.x * 0.5F ? 1 : -1;
            koopa->kick(direction, *this);
            continue;
        }
        player_.hurt(*this);
        if (player_.dead || state_ != GameState::Playing) break;
    }
}

void Game::hitBlock(int x, int y) {
    const TileHit hit = level_->hitTile(x, y, player_.power != PowerState::Small);
    const float centerX = (x + 0.5F) * config::tileSize;
    const float topY = y * config::tileSize;
    switch (hit.kind) {
    case TileHitKind::None: break;
    case TileHitKind::Bump: audio_.play(SoundEffect::Brick); break;
    case TileHitKind::Break:
        addScore(50);
        audio_.play(SoundEffect::Brick);
        spawnParticles(centerX, topY + 16.0F, {0.65F, 0.30F, 0.14F, 1.0F}, 12);
        break;
    case TileHitKind::Coin:
        addCoin();
        spawnParticles(centerX, topY, {1.0F, 0.85F, 0.15F, 1.0F}, 8);
        break;
    case TileHitKind::Mushroom:
        items_.push_back(std::make_unique<Item>(ItemType::Mushroom, x * config::tileSize + 2.0F, topY - 29.0F));
        audio_.play(SoundEffect::Power);
        break;
    case TileHitKind::Flower:
        items_.push_back(std::make_unique<Item>(ItemType::Flower, x * config::tileSize + 2.0F, topY - 29.0F));
        audio_.play(SoundEffect::Power);
        break;
    case TileHitKind::OneUp:
        items_.push_back(std::make_unique<Item>(ItemType::OneUp, x * config::tileSize + 2.0F, topY - 29.0F));
        audio_.play(SoundEffect::Power);
        break;
    case TileHitKind::Star:
        items_.push_back(std::make_unique<Item>(ItemType::Star, x * config::tileSize + 2.0F, topY - 29.0F));
        audio_.play(SoundEffect::Power);
        break;
    }
}

void Game::tryShoot() {
    if (state_ != GameState::Playing || player_.power != PowerState::Fire || fireCooldown_ > 0.0F ||
        fireballs_.size() >= config::maxFireballs) return;
    const float x = player_.body.position.x + player_.body.size.x * 0.5F + player_.facing * 18.0F;
    fireballs_.push_back(std::make_unique<Fireball>(x, player_.body.position.y + 16.0F, player_.facing));
    fireCooldown_ = config::fireCooldown * player_.talents.fireCooldown;
    audio_.play(SoundEffect::Fire);
}

void Game::addScore(int amount) {
    score_ += std::max(1, static_cast<int>(amount * player_.talents.score));
    if (score_ > highScore_) {
        highScore_ = score_;
        saveHighScore();
    }
}

void Game::addXP(int baseAmount) {
    if (baseAmount <= 0) return;
    xp_ += std::max(1, static_cast<int>(baseAmount * player_.talents.xp));
    while (xp_ >= xpRequired(playerLevel_)) {
        xp_ -= xpRequired(playerLevel_);
        ++playerLevel_;
        ++pendingLevelUps_;
    }
    if (pendingLevelUps_ > 0 && state_ == GameState::Playing) {
        prepareTalentChoices();
        state_ = GameState::LevelUp;
        audio_.play(SoundEffect::Power);
        saveGame(saveSlot_);
    }
}

void Game::addCoin() {
    ++coins_;
    if (coins_ >= 100) {
        coins_ -= 100;
        heal(1);
    }
    addScore(200);
    audio_.play(SoundEffect::Coin);
}

void Game::activateCheckpoint() {
    if (checkpointActive_) return;
    checkpointActive_ = true;
    addScore(500);
    saveGame(saveSlot_);
}

void Game::completeLevel() {
    if (state_ != GameState::Playing) return;
    state_ = GameState::LevelComplete;
    stateTimer_ = 0.0F;
    flagDrop_ = 0.0F;
    player_.body.velocity = {0.0F, 0.0F};
    addScore(static_cast<int>(timeLeft_) * 10);
    audio_.play(SoundEffect::Clear);
}

void Game::beginDeath() {
    if (state_ != GameState::Playing) return;
    state_ = GameState::Dying;
    stateTimer_ = 0.0F;
    hp_ = 0;
    audio_.play(SoundEffect::Death);
    saveHighScore();
}

void Game::spawnParticles(float x, float y, const glm::vec4& color, int count) {
    for (int i = 0; i < count; ++i) {
        const float angle = static_cast<float>(i) / static_cast<float>(count) * 6.2831853F;
        const float speed = 90.0F + static_cast<float>((i * 37) % 100);
        particles_.push_back({{x, y}, {std::cos(angle) * speed, std::sin(angle) * speed - 130.0F},
                              color, 0.55F + static_cast<float>(i % 3) * 0.10F, 5.0F + static_cast<float>(i % 3)});
    }
}

void Game::updateParticles(float dt) {
    for (auto& particle : particles_) {
        particle.life -= dt;
        particle.velocity.y += 900.0F * dt;
        particle.position += particle.velocity * dt;
    }
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(),
                                    [](const Particle& p) { return p.life <= 0.0F; }), particles_.end());
}

void Game::cleanupEntities() {
    removeDead(items_);
    removeDead(enemies_);
    removeDead(fireballs_);
    removeDead(bossShots_);
}

void Game::updateCamera() {
    const float screenX = player_.body.position.x - cameraX_;
    if (screenX < config::cameraDeadZoneLeft) cameraX_ = player_.body.position.x - config::cameraDeadZoneLeft;
    if (screenX > config::cameraDeadZoneRight) cameraX_ = player_.body.position.x - config::cameraDeadZoneRight;
    cameraX_ = std::clamp(cameraX_, 0.0F, std::max(0.0F, level_->worldWidth() - config::logicalWidth));
}

void Game::drawBackground(Renderer& renderer) {
    const LevelTheme theme = level_->definition().theme;
    glm::vec4 sky{0.36F, 0.70F, 0.95F, 1.0F};
    glm::vec4 cloudTint{1.0F, 1.0F, 1.0F, 0.95F};
    glm::vec4 hillTint{1.0F};
    glm::vec4 bushTint{1.0F};
    if (theme == LevelTheme::Sunset) {
        sky = {0.94F, 0.56F, 0.38F, 1.0F};
        cloudTint = {1.0F, 0.82F, 0.72F, 0.90F};
        hillTint = {0.86F, 0.58F, 0.44F, 1.0F};
        bushTint = {0.72F, 0.48F, 0.32F, 1.0F};
    } else if (theme == LevelTheme::Night) {
        sky = {0.06F, 0.09F, 0.22F, 1.0F};
        cloudTint = {0.45F, 0.52F, 0.78F, 0.80F};
        hillTint = {0.26F, 0.34F, 0.58F, 1.0F};
        bushTint = {0.18F, 0.28F, 0.42F, 1.0F};
    }

    renderer.setCamera(0.0F);
    renderer.drawRect(0, 0, config::logicalWidth, config::logicalHeight, sky);
    if (theme == LevelTheme::Night) {
        for (int i = 0; i < 46; ++i) {
            const float x = std::fmod(static_cast<float>(i * 137), 930.0F) + 12.0F;
            const float y = 20.0F + std::fmod(static_cast<float>(i * 83), 230.0F);
            const float size = (i % 3 == 0) ? 2.5F : 1.5F;
            renderer.drawRect(x, y, size, size, {0.92F, 0.94F, 1.0F, 0.85F});
        }
    }
    renderer.setCamera(cameraX_ * 0.18F);
    for (int i = 0; i < level_->widthTiles() / 10 + 3; ++i) {
        renderer.drawSprite(Sprite::Cloud, 90.0F + i * 330.0F, 65.0F + (i % 3) * 45.0F,
                            104.0F, 64.0F, false, cloudTint);
        renderer.drawSprite(Sprite::Cloud, 250.0F + i * 510.0F, 150.0F, 72.0F, 45.0F,
                            false, {cloudTint.r, cloudTint.g, cloudTint.b, 0.80F});
    }
    renderer.setCamera(cameraX_ * 0.42F);
    for (int i = 0; i < level_->widthTiles() / 8 + 3; ++i) {
        renderer.drawSprite(Sprite::Hill, 40.0F + i * 290.0F, 320.0F, 190.0F, 160.0F,
                            false, hillTint);
    }
    renderer.setCamera(cameraX_ * 0.70F);
    for (int i = 0; i < level_->widthTiles() / 5 + 3; ++i) {
        renderer.drawSprite(Sprite::Bush, 110.0F + i * 230.0F, 416.0F, 120.0F, 64.0F,
                            false, bushTint);
    }
}

void Game::render(Renderer& renderer) {
    drawBackground(renderer);
    renderer.setCamera(cameraX_);
    level_->draw(renderer, cameraX_, flagDrop_, checkpointActive_);
    for (const auto& platform : platforms_) platform.draw(renderer);
    const auto visible = [this](const Body& body) {
        return body.position.x + body.size.x > cameraX_ - 80.0F &&
               body.position.x < cameraX_ + config::logicalWidth + 80.0F;
    };
    for (const auto& item : items_) if (item->alive && visible(item->body)) item->draw(renderer);
    for (const auto& enemy : enemies_) if (enemy->alive && visible(enemy->body)) enemy->draw(renderer);
    for (const auto& fireball : fireballs_) if (fireball->alive && visible(fireball->body)) fireball->draw(renderer);
    for (const auto& shot : bossShots_) if (shot->alive && visible(shot->body)) shot->draw(renderer);
    if (visible(player_.body) || player_.dead) player_.draw(renderer);
    for (const auto& particle : particles_) {
        renderer.drawRect(particle.position.x, particle.position.y, particle.size, particle.size,
                          {particle.color.r, particle.color.g, particle.color.b,
                           std::min(1.0F, particle.life * 2.0F)});
    }
    renderer.setCamera(0.0F);
    drawHud(renderer);
    drawOverlay(renderer);
}

void Game::drawHud(Renderer& renderer) {
    renderer.drawRect(0, 0, config::logicalWidth, 64, {0.04F, 0.08F, 0.14F, 0.80F});
    const std::string course = endlessMode_
        ? "无尽区域 " + padded(endlessZone_, 2)
        : "世界 " + std::to_string(level_->definition().world) + "-" +
              std::to_string(level_->definition().stage);
    renderer.drawText("分数 " + padded(score_, 6), 18, 8, 18);
    renderer.drawText("金币 " + padded(coins_, 2), 220, 8, 18, {1.0F, 0.86F, 0.25F, 1});
    renderer.drawText(course, 410, 8, 18);
    renderer.drawText("时间 " + padded(static_cast<int>(timeLeft_), 3), 610, 8, 18,
                      timeLeft_ < 60.0F ? glm::vec4{1.0F, 0.35F, 0.25F, 1} : glm::vec4{1.0F});
    renderer.drawText("体力 " + std::to_string(hp_) + "/" + std::to_string(maxHp_), 790, 8, 18,
                      {1.0F, 0.45F, 0.42F, 1});
    for (int i = 0; i < maxHp_; ++i) {
        renderer.drawRect(790.0F + i * 14.0F, 32.0F, 10.0F, 10.0F,
                          i < hp_ ? glm::vec4{0.95F, 0.24F, 0.22F, 1.0F}
                                  : glm::vec4{0.28F, 0.30F, 0.36F, 1.0F});
    }

    const int required = xpRequired(playerLevel_);
    renderer.drawText("等级 " + padded(playerLevel_, 2) + " 经验 " + padded(xp_, 3) + "/" +
                          padded(required, 3),
                      18, 36, 14, {0.72F, 0.92F, 1.0F, 1});
    renderer.drawText("角色 " + std::string(characterName(player_.character())), 250, 36, 14,
                      {0.90F, 0.90F, 1.0F, 1});
    renderer.drawText("技能 " + std::string(skillName(player_.character())) + " " +
                          player_.skillStatus(),
                      420, 36, 14,
                      player_.skillReady() ? glm::vec4{0.48F, 1.0F, 0.58F, 1}
                                           : glm::vec4{0.78F, 0.78F, 0.82F, 1});
    renderer.drawRect(18, 58, 200, 3, {0.18F, 0.22F, 0.30F, 1});
    renderer.drawRect(18, 58, 200.0F * static_cast<float>(xp_) / static_cast<float>(required), 3,
                      {0.35F, 0.85F, 1.0F, 1});

    if (const Boss* boss = activeBoss()) {
        renderer.drawRect(0, 64, config::logicalWidth, 42, {0.10F, 0.03F, 0.06F, 0.78F});
        renderer.drawTextCentered(std::string("首领 ") + boss->bossName() + " 体力 " +
                                      std::to_string(boss->hp()) + "/" + std::to_string(boss->maxHp()),
                                  70, 15, {1.0F, 0.86F, 0.32F, 1});
        renderer.drawRect(230, 92, 500, 7, {0.22F, 0.16F, 0.20F, 1});
        const float ratio = boss->maxHp() > 0
            ? static_cast<float>(boss->hp()) / static_cast<float>(boss->maxHp()) : 0.0F;
        renderer.drawRect(230, 92, 500.0F * ratio, 7, {0.92F, 0.22F, 0.20F, 1});
    }
}

void Game::drawOverlay(Renderer& renderer) {
    if (state_ == GameState::Playing && levelIntroTimer_ > 0.0F) {
        const float alpha = std::min(1.0F, levelIntroTimer_ * 1.4F);
        renderer.drawRect(190, 168, 580, 104, {0.03F, 0.06F, 0.12F, 0.62F * alpha});
        const std::string label = endlessMode_
            ? "无尽区域 " + padded(endlessZone_, 2)
            : "世界 " + std::to_string(level_->definition().world) + "-" +
                  std::to_string(level_->definition().stage);
        renderer.drawTextCentered(label, 186, 30, {1.0F, 0.88F, 0.30F, alpha});
        renderer.drawTextCentered(level_->definition().name, 232, 20, {1.0F, 1.0F, 1.0F, alpha});
    } else if (state_ == GameState::Title) {
        renderer.drawRect(0, 0, config::logicalWidth, config::logicalHeight, {0.03F, 0.06F, 0.12F, 0.72F});
        renderer.drawTextCentered("OPENGL 超级马力欧", 54, 36, {1.0F, 0.82F, 0.20F, 1});
        renderer.drawTextCentered("程序化像素版", 94, 15);
        renderer.drawTextCentered("上下键选择行 左右键修改 L 读取存档 Enter 开始", 126, 13,
                                  {0.72F, 0.82F, 0.92F, 1});
        const auto marker = [this](int row) -> std::string {
            return titleRow_ == row ? "> " : "  ";
        };
        const std::string thirdLabel = options_.mode == GameMode::Story
            ? "剧情关卡 " + std::to_string(options_.startLevel + 1) + "/" +
                  std::to_string(config::levelCount)
            : "难度 " + std::string(difficultyName(options_.difficulty));
        const std::string seedLabel = options_.mode == GameMode::Endless
            ? "种子 " + padded(static_cast<int>(options_.seed), 3)
            : "种子仅无尽模式使用";
        const glm::vec4 selected{1.0F, 0.90F, 0.35F, 1};
        const glm::vec4 normal{0.82F, 0.86F, 0.94F, 1};
        renderer.drawTextCentered(marker(0) + std::string("模式 < ") + modeName(options_.mode) + " >",
                                  158, 17, titleRow_ == 0 ? selected : normal);
        renderer.drawTextCentered(marker(1) + std::string("角色 < ") + characterName(options_.character) + " >",
                                  190, 17, titleRow_ == 1 ? selected : normal);
        renderer.drawTextCentered(marker(2) + "< " + thirdLabel + " >",
                                  222, 17, titleRow_ == 2 ? selected : normal);
        renderer.drawTextCentered(marker(3) + "< " + seedLabel + " >",
                                  254, 17, titleRow_ == 3 ? selected : normal);
        renderer.drawTextCentered(marker(4) + "存档位 " + std::to_string(saveSlot_) + " < " +
                                      saveSummary(saveSlot_) + " >",
                                  286, 17, titleRow_ == 4 ? selected : normal);
        renderer.drawTextCentered(std::string("技能 ") + skillName(options_.character) + " - " +
                                      skillDescription(options_.character),
                                  322, 15, {0.55F, 0.92F, 1.0F, 1});
        renderer.drawTextCentered("最高分 " + padded(highScore_, 6), 352, 17,
                                  {1.0F, 0.86F, 0.25F, 1});
        renderer.drawTextCentered("方向键或 AD 移动 空格或 W 跳跃 C 或 Z 技能", 406, 13);
        renderer.drawTextCentered("F5 保存 F9 或 L 读取 P 暂停 M 静音", 432, 13);
        renderer.drawTextCentered("按 Enter 开始", 480, 19, {1.0F, 0.91F, 0.36F, 1});
    } else if (state_ == GameState::Paused) {
        renderer.drawRect(0, 0, config::logicalWidth, config::logicalHeight, {0, 0, 0, 0.58F});
        renderer.drawTextCentered("已暂停", 220, 42);
        renderer.drawTextCentered("按 P 或 Enter 继续", 290, 18);
    } else if (state_ == GameState::LevelUp) {
        renderer.drawRect(0, 0, config::logicalWidth, config::logicalHeight, {0.02F, 0.05F, 0.12F, 0.84F});
        renderer.drawTextCentered("升级！", 112, 42, {1.0F, 0.86F, 0.25F, 1});
        renderer.drawTextCentered("左右键选择天赋 Enter 确认",
                                  170, 16, {0.78F, 0.86F, 0.95F, 1});
        for (int i = 0; i < 3; ++i) {
            const TalentId talent = talentChoices_[static_cast<std::size_t>(i)];
            const bool chosen = i == talentCursor_;
            renderer.drawRect(135, 210 + i * 66, 690, 50,
                              chosen ? glm::vec4{0.18F, 0.34F, 0.48F, 0.95F}
                                     : glm::vec4{0.08F, 0.12F, 0.19F, 0.90F});
            const std::string prefix = chosen ? "> " : "  ";
            renderer.drawTextCentered(prefix + std::string(talentName(talent)) + " - " +
                                          talentDescription(talent),
                                      224 + i * 66, 18,
                                      chosen ? glm::vec4{1.0F, 0.94F, 0.45F, 1}
                                             : glm::vec4{0.86F, 0.88F, 0.94F, 1});
        }
        renderer.drawTextCentered("等级 " + std::to_string(playerLevel_), 430, 18,
                                  {0.55F, 0.92F, 1.0F, 1});
    } else if (state_ == GameState::LevelComplete) {
        renderer.drawTextCentered(endlessMode_ ? "区域通过！" : "关卡通过！",
                                  80, 30, {1.0F, 0.88F, 0.25F, 1});
    } else if (state_ == GameState::GameOver) {
        renderer.drawRect(0, 0, config::logicalWidth, config::logicalHeight, {0.05F, 0.02F, 0.03F, 0.86F});
        renderer.drawTextCentered("游戏结束", 170, 46, {1.0F, 0.30F, 0.25F, 1});
        renderer.drawTextCentered("最终分数 " + padded(score_, 6), 245, 22);
        renderer.drawTextCentered("角色等级 " + std::to_string(playerLevel_), 280, 18,
                                  {0.55F, 0.92F, 1.0F, 1});
        renderer.drawTextCentered("按 Enter 重新开始 L 读取存档", 335, 20);
    } else if (state_ == GameState::Won) {
        renderer.drawRect(0, 0, config::logicalWidth, config::logicalHeight, {0.03F, 0.08F, 0.13F, 0.80F});
        renderer.drawTextCentered("感谢游玩！", 140, 38, {1.0F, 0.86F, 0.22F, 1});
        renderer.drawTextCentered("你已通关全部八关", 210, 20);
        renderer.drawTextCentered("最终分数 " + padded(score_, 6), 270, 24);
        renderer.drawTextCentered("按 Enter 再玩一次", 340, 18);
    }
    if (bossLockTimer_ > 0.0F) {
        renderer.drawTextCentered("先击败首领才能通过旗杆", 112, 18,
                                  {1.0F, 0.38F, 0.30F, std::min(1.0F, bossLockTimer_ * 2.0F)});
    }
    if (saveMessageTimer_ > 0.0F) {
        renderer.drawTextCentered(saveMessage_, 505, 16,
                                  {0.55F, 1.0F, 0.65F, std::min(1.0F, saveMessageTimer_ * 2.0F)});
    }
}

std::string Game::highScorePath() const {
#ifdef _WIN32
    const char* home = std::getenv("USERPROFILE");
#else
    const char* home = std::getenv("HOME");
#endif
    return std::string(home != nullptr ? home : ".") + "/.mario_highscore";
}

void Game::loadHighScore() {
    std::ifstream file(highScorePath());
    file >> highScore_;
    if (!file) highScore_ = 0;
}

void Game::saveHighScore() const {
    std::ofstream file(highScorePath(), std::ios::trunc);
    if (file) file << highScore_ << '\n';
}
