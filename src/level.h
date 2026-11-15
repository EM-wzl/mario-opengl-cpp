#pragma once

#include <glm/vec2.hpp>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class Renderer;

enum class Tile {
    Empty = 0,
    Ground,
    Solid,
    Brick,
    BrickCoin,
    QuestionCoin,
    QuestionMushroom,
    QuestionFlower,
    QuestionOneUp,
    QuestionStar,
    Used,
    PipeTopLeft,
    PipeTopRight,
    PipeLeft,
    PipeRight,
    CloudPlatform, // 单向平台：可从下方穿过，只能从上方落下站住
    Spring         // 弹簧：落地后自动高弹
};

enum class EnemySpawnType { Goomba, Koopa, Piranha, Paratroopa, Spiny };
enum class LevelTheme { Day, Sunset, Night };
// 三类 Boss：爆爆冲锋跳跃、鸟姐定点吐蛋、库巴慢走跳跃并连发火球。
enum class BossType { BoomBoom, Birdo, Bowser };

struct TilePlacement {
    int x;
    int y;
    Tile tile;
};

struct GroundSegment {
    int first;
    int last; // 含端点
    int top = 15;
};

struct PipePlacement {
    int x;
    int height;
};

struct EnemyPlacement {
    EnemySpawnType type;
    float x; // 瓦片坐标
    float y;
    int level = 1; // 怪物等级；剧情关由关卡序号自动推算
};

struct CoinPlacement {
    float x;
    float y;
};

// Boss 布置：x/y 为瓦片坐标，y 取地面所在行（如 15）。
struct BossPlacement {
    BossType type;
    float x;
    float y;
    int level = 1;
};

// 商店：x/y 为瓦片坐标，默认站在地面所在行（15）。
struct ShopPlacement {
    float x = 0.0F;
    float y = 15.0F;
};

// 箱子：tier 决定稀有度档位，默认站在地面所在行（15）。
struct ChestPlacement {
    float x = 0.0F;
    float y = 15.0F;
    int tier = 1;
};

// 正弦往复平台。axis 决定运动方向，rangeTiles 为离中心点的振幅。
struct MovingPlatformPlacement {
    float x = 0.0F;
    float y = 0.0F;
    int widthTiles = 3;
    glm::vec2 axis{1.0F, 0.0F};
    float rangeTiles = 3.0F;
    float speed = 1.4F;
    float phase = 0.0F;
};

struct LevelDef {
    int widthTiles = 220;
    int world = 1;
    int stage = 1;
    std::string name = "绿丘";
    LevelTheme theme = LevelTheme::Day;
    float timeLimit = 300.0F;
    glm::vec2 playerStart{3.0F, 13.0F};
    float checkpointX = 105.0F;
    float flagX = 210.0F;
    std::vector<GroundSegment> grounds;
    std::vector<TilePlacement> tiles;
    std::vector<PipePlacement> pipes;
    std::vector<EnemyPlacement> enemies;
    std::vector<CoinPlacement> coins;
    std::vector<MovingPlatformPlacement> platforms;
    std::vector<BossPlacement> bosses;
    std::vector<ShopPlacement> shops;
    std::vector<ChestPlacement> chests;
};

enum class TileHitKind { None, Bump, Break, Coin, Mushroom, Flower, OneUp, Star };
struct TileHit {
    TileHitKind kind = TileHitKind::None;
    int x = 0;
    int y = 0;
};

LevelDef makeLevelDefinition(int index);

// 依据层数、种子与难度生成一段可复现的无限世界。
// difficulty: 0=RELAXED, 1=NORMAL, 2=FRENZY。
LevelDef makeEndlessLevelDefinition(int zone, std::uint32_t seed, int difficulty);

// 无渲染依赖的关卡数据自检：检查边界、旗杆、检查点、生成点与平台坐标是否合法。
// 返回空字符串表示通过，否则返回首条错误描述。
std::string validateLevelDefinition(const LevelDef& definition);

class Level {
public:
    explicit Level(int index);
    explicit Level(LevelDef definition);

    Tile tileAt(int x, int y) const;
    bool isSolid(int x, int y) const;
    bool isOneWay(int x, int y) const;
    bool isSolidPixel(float x, float y) const;
    bool isSupportPixel(float x, float y) const;
    TileHit hitTile(int x, int y, bool canBreak);
    void update(float dt);
    void draw(Renderer& renderer, float cameraX, float flagDrop, bool checkpointActive) const;

    int index() const { return index_; }
    int widthTiles() const { return definition_.widthTiles; }
    float worldWidth() const;
    glm::vec2 playerStart() const;
    float checkpointX() const;
    float flagX() const;
    const LevelDef& definition() const { return definition_; }

private:
    void initializeTiles();
    void setTile(int x, int y, Tile tile);
    int gridIndex(int x, int y) const;
    float bumpOffset(int x, int y) const;

    int index_;
    LevelDef definition_;
    std::vector<Tile> tiles_;
    std::unordered_map<int, float> bumps_;
};
