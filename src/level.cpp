#include "level.h"

#include "config.h"
#include "renderer.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <utility>

namespace {
void row(LevelDef& def, int first, int last, int y, Tile tile) {
    for (int x = first; x <= last; ++x) def.tiles.push_back({x, y, tile});
}

void coinRow(LevelDef& def, int first, int last, float y) {
    for (int x = first; x <= last; ++x) def.coins.push_back({static_cast<float>(x), y});
}

void assignEnemyLevels(LevelDef& def, int baseLevel) {
    for (auto& enemy : def.enemies) {
        const int depthBonus = static_cast<int>(enemy.x / static_cast<float>(def.widthTiles) * 4.0F);
        enemy.level = std::max(1, baseLevel + depthBonus);
    }
}

std::string twoDigits(int value) {
    return value < 10 ? "0" + std::to_string(value) : std::to_string(value);
}

// 在旗杆前布置一个 Boss：补足连续地面保证脚下支撑，并清掉会与 Boss 重叠的管道与杂兵。
void placeBoss(LevelDef& def, BossType type, int level) {
    const float x = def.flagX - 20.0F;
    const int tx = static_cast<int>(x);
    const int arenaFirst = std::max(0, tx - 8);
    // 一段连续实体地面：既支撑 Boss，也连到旗杆，避免落在坑里。
    def.grounds.push_back({arenaFirst, def.widthTiles - 1});
    const int clearFirst = tx - 2;
    const int clearLast = tx + 4;
    def.pipes.erase(std::remove_if(def.pipes.begin(), def.pipes.end(),
                                   [clearFirst, clearLast](const PipePlacement& p) {
                                       return p.x >= clearFirst && p.x <= clearLast;
                                   }), def.pipes.end());
    def.enemies.erase(std::remove_if(def.enemies.begin(), def.enemies.end(),
                                     [clearFirst, clearLast](const EnemyPlacement& e) {
                                         return e.x >= static_cast<float>(clearFirst) &&
                                                e.x <= static_cast<float>(clearLast);
                                     }), def.enemies.end());
    def.bosses.push_back({type, x, 15.0F, level});
}

LevelDef firstLevel() {
    LevelDef d;
    d.widthTiles = 224;
    d.world = 1;
    d.stage = 1;
    d.name = "绿丘";
    d.playerStart = {3.0F, 15.0F};
    d.checkpointX = 108.0F;
    d.flagX = 214.0F;
    d.grounds = {{0, 69}, {73, 140}, {146, 223}};

    // 教学顺序：金币 -> 单个栗宝宝 -> 问号块/蘑菇 -> 管道 -> 坑洞。
    coinRow(d, 8, 12, 11.0F);
    d.tiles.push_back({16, 14 - 4, Tile::QuestionCoin});
    d.tiles.push_back({20, 10, Tile::BrickCoin});
    d.tiles.push_back({21, 10, Tile::QuestionMushroom});
    d.tiles.push_back({22, 10, Tile::Brick});
    d.tiles.push_back({23, 10, Tile::QuestionCoin});
    d.tiles.push_back({24, 10, Tile::Brick});
    d.tiles.push_back({22, 6, Tile::QuestionCoin});
    d.pipes = {{34, 2}, {47, 3}, {61, 2}, {91, 3}, {126, 2}, {170, 3}};
    d.enemies = {
        {EnemySpawnType::Goomba, 27, 15}, {EnemySpawnType::Goomba, 43, 15},
        {EnemySpawnType::Goomba, 56, 15}, {EnemySpawnType::Goomba, 79, 15},
        {EnemySpawnType::Goomba, 84, 15}, {EnemySpawnType::Goomba, 116, 15},
        {EnemySpawnType::Goomba, 133, 15}, {EnemySpawnType::Goomba, 157, 15},
        {EnemySpawnType::Goomba, 181, 15}, {EnemySpawnType::Goomba, 184, 15}
    };
    row(d, 77, 81, 11, Tile::Brick);
    d.tiles[ d.tiles.size() - 3 ].tile = Tile::QuestionCoin;
    coinRow(d, 77, 81, 8.0F);
    row(d, 99, 103, 12, Tile::Solid);
    row(d, 100, 103, 11, Tile::Solid);
    coinRow(d, 99, 103, 8.0F);
    row(d, 113, 117, 10, Tile::Brick);
    d.tiles.push_back({115, 10, Tile::QuestionMushroom});
    coinRow(d, 113, 117, 7.0F);
    // 坑洞上方的金币提示跳跃弧线。
    d.coins.insert(d.coins.end(), {{70, 11}, {71, 10}, {72, 11}, {142, 11}, {143, 10}, {144, 10}, {145, 11}});
    row(d, 151, 154, 11, Tile::Solid);
    row(d, 155, 158, 9, Tile::Solid);
    row(d, 159, 162, 7, Tile::Solid);
    coinRow(d, 151, 162, 5.0F);
    row(d, 177, 181, 11, Tile::Brick);
    d.tiles.push_back({179, 11, Tile::QuestionCoin});
    row(d, 190, 196, 12, Tile::Solid);
    row(d, 191, 196, 11, Tile::Solid);
    row(d, 192, 196, 10, Tile::Solid);
    coinRow(d, 198, 202, 12.0F);
    return d;
}

LevelDef secondLevel() {
    LevelDef d;
    d.widthTiles = 272;
    d.world = 1;
    d.stage = 2;
    d.name = "管道之路";
    d.theme = LevelTheme::Sunset;
    d.playerStart = {3.0F, 15.0F};
    d.checkpointX = 132.0F;
    d.flagX = 262.0F;
    d.grounds = {{0, 44}, {50, 94}, {101, 149}, {157, 209}, {216, 271}};
    d.pipes = {{25, 2}, {39, 3}, {63, 3}, {86, 2}, {118, 3}, {143, 2}, {181, 3}, {226, 2}};
    d.enemies = {
        {EnemySpawnType::Goomba, 18, 15}, {EnemySpawnType::Koopa, 32, 15},
        {EnemySpawnType::Piranha, 25.5, 13}, {EnemySpawnType::Piranha, 39.5, 12},
        {EnemySpawnType::Goomba, 55, 15}, {EnemySpawnType::Koopa, 72, 15},
        {EnemySpawnType::Piranha, 63.5, 12}, {EnemySpawnType::Goomba, 82, 15},
        {EnemySpawnType::Piranha, 86.5, 13}, {EnemySpawnType::Koopa, 108, 15},
        {EnemySpawnType::Piranha, 118.5, 12}, {EnemySpawnType::Goomba, 126, 15},
        {EnemySpawnType::Koopa, 139, 15}, {EnemySpawnType::Piranha, 143.5, 13},
        {EnemySpawnType::Goomba, 164, 15}, {EnemySpawnType::Goomba, 168, 15},
        {EnemySpawnType::Koopa, 176, 15}, {EnemySpawnType::Piranha, 181.5, 12},
        {EnemySpawnType::Koopa, 198, 15}, {EnemySpawnType::Goomba, 222, 15},
        {EnemySpawnType::Piranha, 226.5, 13}, {EnemySpawnType::Koopa, 240, 15}
    };
    d.tiles.push_back({13, 10, Tile::QuestionMushroom});
    d.tiles.push_back({14, 10, Tile::BrickCoin});
    d.tiles.push_back({15, 10, Tile::QuestionFlower});
    d.tiles.push_back({16, 10, Tile::Brick});
    coinRow(d, 9, 13, 12.0F);
    row(d, 30, 34, 11, Tile::Brick);
    d.tiles.push_back({32, 11, Tile::QuestionCoin});
    row(d, 53, 57, 10, Tile::Solid);
    coinRow(d, 53, 57, 7.0F);
    d.tiles.push_back({69, 11, Tile::QuestionFlower});
    d.tiles.push_back({70, 11, Tile::Brick});
    d.tiles.push_back({71, 11, Tile::QuestionCoin});
    row(d, 75, 79, 7, Tile::Brick);
    coinRow(d, 75, 79, 4.0F);
    d.coins.insert(d.coins.end(), {{45, 11}, {46, 9}, {47, 8}, {48, 9}, {49, 11},
                                  {95, 11}, {96, 9}, {97, 8}, {98, 8}, {99, 9}, {100, 11}});
    row(d, 104, 110, 11, Tile::Brick);
    d.tiles.push_back({107, 11, Tile::QuestionMushroom});
    row(d, 122, 126, 9, Tile::Solid);
    coinRow(d, 122, 126, 6.0F);
    d.tiles.push_back({135, 11, Tile::QuestionFlower});
    d.tiles.push_back({136, 11, Tile::Brick});
    d.tiles.push_back({137, 11, Tile::QuestionCoin});
    d.coins.insert(d.coins.end(), {{150, 11}, {151, 9}, {152, 8}, {153, 8}, {154, 9}, {155, 11},
                                  {210, 11}, {211, 9}, {212, 8}, {213, 8}, {214, 9}, {215, 11}});
    row(d, 160, 164, 11, Tile::Solid);
    row(d, 170, 175, 9, Tile::Brick);
    d.tiles.push_back({172, 9, Tile::QuestionCoin});
    row(d, 187, 193, 12, Tile::Solid);
    row(d, 188, 193, 11, Tile::Solid);
    row(d, 189, 193, 10, Tile::Solid);
    row(d, 230, 236, 10, Tile::Brick);
    d.tiles.push_back({233, 10, Tile::QuestionMushroom});
    coinRow(d, 244, 250, 12.0F);
    return d;
}

void cloudRow(LevelDef& def, int first, int last, int y) {
    row(def, first, last, y, Tile::CloudPlatform);
}

void spring(LevelDef& def, int x) {
    def.tiles.push_back({x, 14, Tile::Spring});
}

LevelDef thirdLevel() {
    LevelDef d;
    d.widthTiles = 288;
    d.world = 1;
    d.stage = 3;
    d.name = "弹簧峡谷";
    d.checkpointX = 140.0F;
    d.flagX = 278.0F;
    d.timeLimit = 330.0F;
    d.playerStart = {3.0F, 15.0F};
    d.grounds = {{0, 52}, {60, 98}, {107, 151}, {160, 207}, {216, 287}};
    d.pipes = {{30, 2}, {83, 3}, {132, 2}, {188, 3}, {244, 2}};

    // 弹簧与云平台交替出现：可走稳妥的云路，也可用弹簧走高空捷径。
    spring(d, 51);
    spring(d, 97);
    spring(d, 150);
    spring(d, 206);
    cloudRow(d, 54, 56, 11);
    cloudRow(d, 57, 59, 9);
    cloudRow(d, 100, 102, 10);
    cloudRow(d, 104, 106, 8);
    cloudRow(d, 153, 155, 11);
    cloudRow(d, 157, 159, 9);
    cloudRow(d, 209, 211, 10);
    cloudRow(d, 213, 215, 8);
    d.platforms = {
        {102.0F, 12.0F, 3, {1.0F, 0.0F}, 2.5F, 1.6F, 0.0F},
        {155.0F, 11.0F, 3, {0.0F, 1.0F}, 2.0F, 1.5F, 0.0F},
        {211.0F, 12.0F, 3, {1.0F, 0.0F}, 2.0F, 1.8F, 1.0F}
    };
    d.tiles.push_back({18, 10, Tile::QuestionMushroom});
    d.tiles.push_back({19, 10, Tile::BrickCoin});
    d.tiles.push_back({20, 10, Tile::QuestionCoin});
    row(d, 67, 72, 10, Tile::Brick);
    d.tiles.push_back({69, 10, Tile::QuestionStar});
    cloudRow(d, 113, 120, 10);
    d.tiles.push_back({116, 7, Tile::QuestionOneUp});
    d.tiles.push_back({117, 7, Tile::Brick});
    row(d, 166, 172, 11, Tile::Solid);
    row(d, 168, 172, 10, Tile::Solid);
    row(d, 224, 230, 10, Tile::Brick);
    d.tiles.push_back({227, 10, Tile::QuestionFlower});
    coinRow(d, 8, 13, 12.0F);
    coinRow(d, 54, 59, 6.0F);
    coinRow(d, 100, 106, 5.0F);
    coinRow(d, 113, 120, 7.0F);
    coinRow(d, 153, 159, 6.0F);
    coinRow(d, 209, 215, 5.0F);
    coinRow(d, 258, 266, 12.0F);
    d.enemies = {
        {EnemySpawnType::Goomba, 25, 15}, {EnemySpawnType::Koopa, 40, 15},
        {EnemySpawnType::Paratroopa, 56, 10}, {EnemySpawnType::Spiny, 70, 15},
        {EnemySpawnType::Piranha, 83.5F, 12}, {EnemySpawnType::Paratroopa, 103, 9},
        {EnemySpawnType::Goomba, 124, 15}, {EnemySpawnType::Spiny, 143, 15},
        {EnemySpawnType::Paratroopa, 156, 9}, {EnemySpawnType::Koopa, 177, 15},
        {EnemySpawnType::Piranha, 188.5F, 12}, {EnemySpawnType::Spiny, 200, 15},
        {EnemySpawnType::Paratroopa, 212, 8}, {EnemySpawnType::Koopa, 232, 15},
        {EnemySpawnType::Piranha, 244.5F, 13}, {EnemySpawnType::Spiny, 263, 15}
    };
    return d;
}

LevelDef fourthLevel() {
    LevelDef d;
    d.widthTiles = 304;
    d.world = 1;
    d.stage = 4;
    d.name = "移动要塞";
    d.theme = LevelTheme::Night;
    d.timeLimit = 350.0F;
    d.checkpointX = 148.0F;
    d.flagX = 294.0F;
    d.playerStart = {3.0F, 15.0F};
    d.grounds = {{0, 42}, {52, 88}, {99, 135}, {146, 190}, {201, 244}, {254, 303}};
    d.pipes = {{21, 3}, {68, 2}, {119, 3}, {176, 2}, {227, 3}, {275, 2}};
    d.platforms = {
        {45.0F, 11.0F, 3, {1.0F, 0.0F}, 3.0F, 1.4F, 0.0F},
        {92.0F, 10.0F, 3, {0.0F, 1.0F}, 2.5F, 1.6F, 0.5F},
        {139.0F, 11.0F, 3, {1.0F, 0.0F}, 3.0F, 1.8F, 1.0F},
        {194.0F, 10.0F, 3, {0.0F, 1.0F}, 2.5F, 1.7F, 0.0F},
        {247.0F, 11.0F, 3, {1.0F, 0.0F}, 2.5F, 2.0F, 0.0F}
    };
    spring(d, 41);
    spring(d, 134);
    cloudRow(d, 47, 49, 7);
    cloudRow(d, 91, 95, 7);
    cloudRow(d, 140, 143, 7);
    cloudRow(d, 194, 198, 6);
    cloudRow(d, 247, 251, 7);
    row(d, 29, 34, 10, Tile::Brick);
    d.tiles.push_back({31, 10, Tile::QuestionMushroom});
    d.tiles.push_back({74, 10, Tile::QuestionStar});
    d.tiles.push_back({75, 10, Tile::BrickCoin});
    d.tiles.push_back({76, 10, Tile::QuestionCoin});
    row(d, 105, 111, 11, Tile::Solid);
    row(d, 107, 111, 10, Tile::Solid);
    row(d, 158, 164, 10, Tile::Brick);
    d.tiles.push_back({160, 10, Tile::QuestionFlower});
    d.tiles.push_back({173, 8, Tile::QuestionOneUp});
    cloudRow(d, 171, 175, 11);
    row(d, 211, 217, 11, Tile::Brick);
    d.tiles.push_back({214, 11, Tile::QuestionStar});
    row(d, 263, 269, 10, Tile::Solid);
    coinRow(d, 44, 50, 5.0F);
    coinRow(d, 90, 96, 4.0F);
    coinRow(d, 137, 144, 5.0F);
    coinRow(d, 192, 199, 3.0F);
    coinRow(d, 245, 252, 5.0F);
    coinRow(d, 281, 287, 12.0F);
    d.enemies = {
        {EnemySpawnType::Spiny, 15, 15}, {EnemySpawnType::Piranha, 21.5F, 12},
        {EnemySpawnType::Paratroopa, 46, 9}, {EnemySpawnType::Koopa, 60, 15},
        {EnemySpawnType::Piranha, 68.5F, 13}, {EnemySpawnType::Spiny, 81, 15},
        {EnemySpawnType::Paratroopa, 93, 8}, {EnemySpawnType::Goomba, 113, 15},
        {EnemySpawnType::Piranha, 119.5F, 12}, {EnemySpawnType::Spiny, 129, 15},
        {EnemySpawnType::Paratroopa, 140, 8}, {EnemySpawnType::Koopa, 156, 15},
        {EnemySpawnType::Spiny, 169, 15}, {EnemySpawnType::Piranha, 176.5F, 13},
        {EnemySpawnType::Paratroopa, 195, 7}, {EnemySpawnType::Spiny, 210, 15},
        {EnemySpawnType::Piranha, 227.5F, 12}, {EnemySpawnType::Koopa, 237, 15},
        {EnemySpawnType::Paratroopa, 248, 8}, {EnemySpawnType::Spiny, 264, 15},
        {EnemySpawnType::Piranha, 275.5F, 13}, {EnemySpawnType::Koopa, 286, 15}
    };
    return d;
}

LevelDef fifthLevel() {
    LevelDef d;
    d.widthTiles = 320;
    d.world = 2;
    d.stage = 1;
    d.name = "天空之桥";
    d.theme = LevelTheme::Sunset;
    d.timeLimit = 360.0F;
    d.checkpointX = 158.0F;
    d.flagX = 310.0F;
    d.playerStart = {3.0F, 15.0F};
    d.grounds = {{0, 30}, {44, 64}, {80, 100}, {118, 142}, {160, 182},
                 {202, 226}, {244, 268}, {286, 319}};
    d.pipes = {{53, 2}, {91, 3}, {132, 2}, {214, 3}, {297, 2}};
    // 空中关卡以单向平台和移动平台为主，实体地面只是短暂落脚点。
    cloudRow(d, 32, 35, 11);
    cloudRow(d, 37, 40, 9);
    cloudRow(d, 42, 43, 11);
    cloudRow(d, 66, 69, 10);
    cloudRow(d, 72, 75, 8);
    cloudRow(d, 78, 79, 10);
    cloudRow(d, 102, 105, 11);
    cloudRow(d, 108, 111, 8);
    cloudRow(d, 115, 117, 10);
    cloudRow(d, 144, 147, 10);
    cloudRow(d, 151, 154, 7);
    cloudRow(d, 158, 159, 10);
    cloudRow(d, 184, 187, 11);
    cloudRow(d, 191, 194, 8);
    cloudRow(d, 200, 201, 10);
    cloudRow(d, 228, 231, 11);
    cloudRow(d, 235, 238, 8);
    cloudRow(d, 242, 243, 10);
    cloudRow(d, 270, 273, 11);
    cloudRow(d, 277, 280, 8);
    cloudRow(d, 284, 285, 10);
    d.platforms = {
        {36.0F, 12.0F, 3, {0.0F, 1.0F}, 2.5F, 1.6F, 0.0F},
        {70.0F, 11.0F, 3, {1.0F, 0.0F}, 3.0F, 1.5F, 0.5F},
        {106.0F, 12.0F, 3, {0.0F, 1.0F}, 3.0F, 1.7F, 0.0F},
        {149.0F, 11.0F, 3, {1.0F, 0.0F}, 3.0F, 1.8F, 0.0F},
        {189.0F, 12.0F, 3, {0.0F, 1.0F}, 3.0F, 1.8F, 0.5F},
        {233.0F, 11.0F, 3, {1.0F, 0.0F}, 3.0F, 1.9F, 0.0F},
        {275.0F, 12.0F, 3, {0.0F, 1.0F}, 3.0F, 2.0F, 0.0F}
    };
    spring(d, 63);
    spring(d, 181);
    d.tiles.push_back({17, 10, Tile::QuestionMushroom});
    d.tiles.push_back({58, 9, Tile::QuestionStar});
    cloudRow(d, 56, 60, 12);
    d.tiles.push_back({95, 8, Tile::QuestionOneUp});
    cloudRow(d, 93, 97, 11);
    d.tiles.push_back({126, 10, Tile::QuestionFlower});
    d.tiles.push_back({172, 9, Tile::QuestionStar});
    cloudRow(d, 170, 174, 12);
    d.tiles.push_back({257, 10, Tile::QuestionFlower});
    for (int x = 32; x <= 285; x += 16) coinRow(d, x, std::min(x + 4, 285), 5.0F + (x % 3));
    d.enemies = {
        {EnemySpawnType::Koopa, 22, 15}, {EnemySpawnType::Paratroopa, 36, 8},
        {EnemySpawnType::Piranha, 53.5F, 13}, {EnemySpawnType::Paratroopa, 70, 7},
        {EnemySpawnType::Spiny, 86, 15}, {EnemySpawnType::Piranha, 91.5F, 12},
        {EnemySpawnType::Paratroopa, 108, 7}, {EnemySpawnType::Koopa, 126, 15},
        {EnemySpawnType::Piranha, 132.5F, 13}, {EnemySpawnType::Paratroopa, 150, 6},
        {EnemySpawnType::Spiny, 169, 15}, {EnemySpawnType::Paratroopa, 190, 7},
        {EnemySpawnType::Koopa, 209, 15}, {EnemySpawnType::Piranha, 214.5F, 12},
        {EnemySpawnType::Paratroopa, 234, 7}, {EnemySpawnType::Spiny, 254, 15},
        {EnemySpawnType::Paratroopa, 276, 7}, {EnemySpawnType::Koopa, 295, 15},
        {EnemySpawnType::Piranha, 297.5F, 13}
    };
    return d;
}

LevelDef sixthLevel() {
    LevelDef d;
    d.widthTiles = 336;
    d.world = 2;
    d.stage = 2;
    d.name = "最终试炼";
    d.theme = LevelTheme::Night;
    d.timeLimit = 400.0F;
    d.checkpointX = 164.0F;
    d.flagX = 326.0F;
    d.playerStart = {3.0F, 15.0F};
    d.grounds = {{0, 55}, {63, 108}, {116, 165}, {174, 224}, {233, 278}, {287, 335}};
    d.pipes = {{28, 3}, {82, 2}, {139, 3}, {196, 2}, {252, 3}, {306, 2}};
    d.platforms = {
        {58.0F, 11.0F, 3, {1.0F, 0.0F}, 2.5F, 1.8F, 0.0F},
        {111.0F, 10.0F, 3, {0.0F, 1.0F}, 2.5F, 1.8F, 0.5F},
        {168.0F, 11.0F, 3, {1.0F, 0.0F}, 3.0F, 2.0F, 0.0F},
        {227.0F, 10.0F, 3, {0.0F, 1.0F}, 3.0F, 1.9F, 0.0F},
        {281.0F, 11.0F, 3, {1.0F, 0.0F}, 3.0F, 2.1F, 0.0F}
    };
    spring(d, 54);
    spring(d, 107);
    spring(d, 223);
    spring(d, 277);
    cloudRow(d, 57, 60, 7);
    cloudRow(d, 110, 114, 7);
    cloudRow(d, 167, 172, 7);
    cloudRow(d, 226, 231, 6);
    cloudRow(d, 280, 285, 7);
    row(d, 15, 20, 10, Tile::Brick);
    d.tiles.push_back({17, 10, Tile::QuestionMushroom});
    d.tiles.push_back({18, 10, Tile::QuestionStar});
    row(d, 38, 45, 11, Tile::Solid);
    row(d, 40, 45, 10, Tile::Solid);
    row(d, 69, 75, 10, Tile::Brick);
    d.tiles.push_back({72, 10, Tile::QuestionFlower});
    d.tiles.push_back({94, 8, Tile::QuestionOneUp});
    cloudRow(d, 92, 96, 11);
    row(d, 122, 129, 11, Tile::Solid);
    row(d, 124, 129, 10, Tile::Solid);
    d.tiles.push_back({151, 10, Tile::QuestionStar});
    row(d, 181, 188, 10, Tile::Brick);
    d.tiles.push_back({184, 10, Tile::QuestionFlower});
    row(d, 207, 214, 11, Tile::Solid);
    row(d, 209, 214, 10, Tile::Solid);
    d.tiles.push_back({241, 9, Tile::QuestionOneUp});
    cloudRow(d, 239, 243, 12);
    row(d, 261, 268, 10, Tile::Brick);
    d.tiles.push_back({264, 10, Tile::QuestionStar});
    row(d, 294, 301, 11, Tile::Solid);
    row(d, 296, 301, 10, Tile::Solid);
    for (int x = 56; x <= 285; x += 19) coinRow(d, x, std::min(x + 4, 286), 4.0F + (x % 3));
    coinRow(d, 312, 319, 12.0F);
    d.enemies = {
        {EnemySpawnType::Goomba, 12, 15}, {EnemySpawnType::Spiny, 25, 15},
        {EnemySpawnType::Piranha, 28.5F, 12}, {EnemySpawnType::Paratroopa, 48, 8},
        {EnemySpawnType::Paratroopa, 59, 7}, {EnemySpawnType::Koopa, 70, 15},
        {EnemySpawnType::Piranha, 82.5F, 13}, {EnemySpawnType::Spiny, 98, 15},
        {EnemySpawnType::Paratroopa, 112, 7}, {EnemySpawnType::Koopa, 126, 15},
        {EnemySpawnType::Piranha, 139.5F, 12}, {EnemySpawnType::Spiny, 157, 15},
        {EnemySpawnType::Paratroopa, 169, 7}, {EnemySpawnType::Spiny, 184, 15},
        {EnemySpawnType::Piranha, 196.5F, 13}, {EnemySpawnType::Koopa, 215, 15},
        {EnemySpawnType::Paratroopa, 228, 6}, {EnemySpawnType::Spiny, 244, 15},
        {EnemySpawnType::Piranha, 252.5F, 12}, {EnemySpawnType::Koopa, 270, 15},
        {EnemySpawnType::Paratroopa, 282, 7}, {EnemySpawnType::Spiny, 296, 15},
        {EnemySpawnType::Piranha, 306.5F, 13}, {EnemySpawnType::Koopa, 316, 15}
    };
    return d;
}

LevelDef seventhLevel() {
    LevelDef d;
    d.widthTiles = 312;
    d.world = 2;
    d.stage = 3;
    d.name = "齿轮洞窟";
    d.theme = LevelTheme::Sunset;
    d.timeLimit = 370.0F;
    d.checkpointX = 154.0F;
    d.flagX = 302.0F;
    d.playerStart = {3.0F, 15.0F};
    d.grounds = {{0, 48}, {57, 96}, {105, 150}, {159, 205}, {214, 260}, {269, 311}};
    d.pipes = {{22, 3}, {78, 2}, {132, 3}, {188, 2}, {241, 3}, {288, 2}};
    d.platforms = {
        {51.0F, 11.0F, 3, {1.0F, 0.0F}, 2.5F, 1.9F, 0.0F},
        {99.0F, 10.0F, 3, {0.0F, 1.0F}, 2.5F, 1.8F, 0.5F},
        {153.0F, 11.0F, 3, {1.0F, 0.0F}, 2.5F, 2.0F, 0.0F},
        {208.0F, 10.0F, 3, {0.0F, 1.0F}, 2.5F, 1.8F, 0.0F},
        {263.0F, 11.0F, 3, {1.0F, 0.0F}, 2.5F, 2.1F, 0.5F}
    };
    spring(d, 47);
    spring(d, 149);
    spring(d, 259);
    cloudRow(d, 50, 54, 7);
    cloudRow(d, 98, 102, 7);
    cloudRow(d, 152, 156, 6);
    cloudRow(d, 207, 211, 7);
    cloudRow(d, 262, 266, 7);
    row(d, 12, 17, 10, Tile::Brick);
    d.tiles.push_back({14, 10, Tile::QuestionStar});
    row(d, 64, 70, 10, Tile::Solid);
    d.tiles.push_back({67, 9, Tile::QuestionOneUp});
    row(d, 84, 90, 11, Tile::Brick);
    d.tiles.push_back({87, 11, Tile::QuestionFlower});
    row(d, 114, 120, 10, Tile::Solid);
    row(d, 116, 120, 9, Tile::Solid);
    row(d, 170, 176, 11, Tile::Brick);
    d.tiles.push_back({173, 11, Tile::QuestionMushroom});
    row(d, 222, 228, 10, Tile::Solid);
    d.tiles.push_back({225, 8, Tile::QuestionStar});
    row(d, 276, 282, 11, Tile::Brick);
    d.tiles.push_back({279, 11, Tile::QuestionFlower});
    for (int x = 49; x <= 266; x += 18) coinRow(d, x, std::min(x + 4, 267), 4.0F + x % 3);
    coinRow(d, 292, 297, 12.0F);
    d.enemies = {
        {EnemySpawnType::Spiny, 16, 15}, {EnemySpawnType::Piranha, 22.5F, 12},
        {EnemySpawnType::Paratroopa, 51, 8}, {EnemySpawnType::Koopa, 65, 15},
        {EnemySpawnType::Piranha, 78.5F, 13}, {EnemySpawnType::Spiny, 91, 15},
        {EnemySpawnType::Paratroopa, 100, 7}, {EnemySpawnType::Koopa, 112, 15},
        {EnemySpawnType::Piranha, 132.5F, 12}, {EnemySpawnType::Spiny, 145, 15},
        {EnemySpawnType::Paratroopa, 154, 7}, {EnemySpawnType::Koopa, 168, 15},
        {EnemySpawnType::Piranha, 188.5F, 13}, {EnemySpawnType::Spiny, 200, 15},
        {EnemySpawnType::Paratroopa, 209, 7}, {EnemySpawnType::Koopa, 220, 15},
        {EnemySpawnType::Piranha, 241.5F, 12}, {EnemySpawnType::Spiny, 255, 15},
        {EnemySpawnType::Paratroopa, 264, 7}, {EnemySpawnType::Koopa, 276, 15},
        {EnemySpawnType::Piranha, 288.5F, 13}, {EnemySpawnType::Spiny, 296, 15}
    };
    return d;
}

LevelDef eighthLevel() {
    LevelDef d;
    d.widthTiles = 344;
    d.world = 2;
    d.stage = 4;
    d.name = "星夜登峰";
    d.theme = LevelTheme::Night;
    d.timeLimit = 400.0F;
    d.checkpointX = 168.0F;
    d.flagX = 334.0F;
    d.playerStart = {3.0F, 15.0F};
    d.grounds = {{0, 52}, {61, 104}, {113, 160}, {170, 218}, {227, 278}, {288, 343}};
    d.pipes = {{27, 2}, {83, 3}, {140, 2}, {196, 3}, {252, 2}, {312, 3}};
    d.platforms = {
        {55.0F, 11.0F, 3, {0.0F, 1.0F}, 2.5F, 1.8F, 0.0F},
        {107.0F, 10.0F, 3, {1.0F, 0.0F}, 3.0F, 2.0F, 0.5F},
        {163.0F, 11.0F, 3, {0.0F, 1.0F}, 3.0F, 1.9F, 0.0F},
        {221.0F, 10.0F, 3, {1.0F, 0.0F}, 3.0F, 2.2F, 0.0F},
        {281.0F, 11.0F, 3, {0.0F, 1.0F}, 3.0F, 2.0F, 0.5F}
    };
    spring(d, 51);
    spring(d, 159);
    spring(d, 277);
    cloudRow(d, 54, 58, 6);
    cloudRow(d, 106, 110, 7);
    cloudRow(d, 162, 167, 6);
    cloudRow(d, 220, 224, 7);
    cloudRow(d, 280, 285, 6);
    row(d, 10, 16, 10, Tile::Brick);
    d.tiles.push_back({13, 10, Tile::QuestionMushroom});
    d.tiles.push_back({14, 10, Tile::QuestionStar});
    row(d, 34, 42, 11, Tile::Solid);
    row(d, 36, 42, 10, Tile::Solid);
    row(d, 68, 74, 10, Tile::Brick);
    d.tiles.push_back({71, 10, Tile::QuestionFlower});
    cloudRow(d, 88, 92, 11);
    d.tiles.push_back({90, 8, Tile::QuestionOneUp});
    row(d, 120, 128, 11, Tile::Solid);
    row(d, 122, 128, 10, Tile::Solid);
    d.tiles.push_back({148, 9, Tile::QuestionStar});
    row(d, 178, 186, 10, Tile::Brick);
    d.tiles.push_back({182, 10, Tile::QuestionFlower});
    row(d, 204, 212, 11, Tile::Solid);
    row(d, 206, 212, 10, Tile::Solid);
    cloudRow(d, 238, 242, 12);
    d.tiles.push_back({240, 9, Tile::QuestionOneUp});
    row(d, 260, 268, 10, Tile::Brick);
    d.tiles.push_back({264, 10, Tile::QuestionStar});
    row(d, 300, 308, 11, Tile::Solid);
    row(d, 302, 308, 10, Tile::Solid);
    for (int x = 53; x <= 285; x += 17) coinRow(d, x, std::min(x + 4, 286), 4.0F + x % 3);
    coinRow(d, 320, 327, 12.0F);
    d.enemies = {
        {EnemySpawnType::Goomba, 12, 15}, {EnemySpawnType::Spiny, 24, 15},
        {EnemySpawnType::Piranha, 27.5F, 13}, {EnemySpawnType::Paratroopa, 55, 7},
        {EnemySpawnType::Koopa, 69, 15}, {EnemySpawnType::Piranha, 83.5F, 12},
        {EnemySpawnType::Spiny, 98, 15}, {EnemySpawnType::Paratroopa, 108, 7},
        {EnemySpawnType::Koopa, 124, 15}, {EnemySpawnType::Piranha, 140.5F, 13},
        {EnemySpawnType::Spiny, 154, 15}, {EnemySpawnType::Paratroopa, 164, 6},
        {EnemySpawnType::Koopa, 180, 15}, {EnemySpawnType::Piranha, 196.5F, 12},
        {EnemySpawnType::Spiny, 212, 15}, {EnemySpawnType::Paratroopa, 222, 7},
        {EnemySpawnType::Koopa, 234, 15}, {EnemySpawnType::Piranha, 252.5F, 13},
        {EnemySpawnType::Spiny, 270, 15}, {EnemySpawnType::Paratroopa, 282, 6},
        {EnemySpawnType::Koopa, 296, 15}, {EnemySpawnType::Piranha, 312.5F, 12},
        {EnemySpawnType::Spiny, 324, 15}, {EnemySpawnType::Koopa, 330, 15}
    };
    return d;
}
} // namespace

LevelDef makeEndlessLevelDefinition(int zone, std::uint32_t seed, int difficulty) {
    const int safeZone = std::max(1, zone);
    const int safeDifficulty = std::clamp(difficulty, 0, 2);
    std::mt19937 rng(seed * 73129U + static_cast<std::uint32_t>(safeZone) * 104729U +
                     static_cast<std::uint32_t>(safeDifficulty) * 17U);
    auto randomInt = [&rng](int low, int high) {
        std::uniform_int_distribution<int> distribution(low, high);
        return distribution(rng);
    };

    LevelDef d;
    d.widthTiles = 220 + std::min(80, safeZone * 8 + safeDifficulty * 10);
    d.world = 9;
    d.stage = safeZone;
    d.name = "无尽区域 " + twoDigits(safeZone);
    d.theme = static_cast<LevelTheme>((safeZone + safeDifficulty) % 3);
    d.timeLimit = 300.0F + static_cast<float>(std::min(150, safeZone * 10)) +
                  (safeDifficulty == 0 ? 40.0F : 0.0F);
    d.playerStart = {3.0F, 15.0F};
    d.checkpointX = static_cast<float>(d.widthTiles / 2);
    d.flagX = static_cast<float>(d.widthTiles - 10);

    std::vector<GroundSegment> playable;
    int cursor = 0;
    while (cursor < d.widthTiles - 1) {
        const int length = randomInt(14, 28);
        const int last = std::min(cursor + length - 1, d.widthTiles - 1);
        const GroundSegment segment{cursor, last};
        d.grounds.push_back(segment);
        playable.push_back(segment);
        if (last >= d.widthTiles - 1) break;

        const int gap = std::min(9, randomInt(3, 5 + safeDifficulty) +
                                       std::min(3, safeZone / 3));
        const int gapStart = last + 1;
        const int gapEnd = std::min(gapStart + gap - 1, d.widthTiles - 1);
        if (gapEnd >= d.widthTiles - 1) break;

        const int middle = (gapStart + gapEnd) / 2;
        const int cloudLeft = std::max(gapStart + 1, middle - 2);
        const int cloudRight = std::min(gapEnd - 1, middle + 2);
        if (cloudLeft <= cloudRight) cloudRow(d, cloudLeft, cloudRight, 10);
        if (gap >= 6) cloudRow(d, gapStart + 1, std::min(gapStart + 3, gapEnd - 1), 7);
        if (gapStart + 1 <= gapEnd - 1) coinRow(d, gapStart + 1, gapEnd - 1, 5.0F);
        if (randomInt(0, 1) == 0) {
            d.platforms.push_back({static_cast<float>(gapStart + 1), 12.0F, 3,
                                   {1.0F, 0.0F}, static_cast<float>(std::max(1, gap / 2 - 1)),
                                   1.4F + safeDifficulty * 0.2F,
                                   static_cast<float>(randomInt(0, 6)) * 0.3F});
        }
        cursor = gapEnd + 1;
    }
    // 强制保证中点检查点和终点旗杆下方都有安全地面。
    const int middle = d.widthTiles / 2;
    d.grounds.push_back({middle - 8, middle + 8});
    d.grounds.push_back({d.widthTiles - 24, d.widthTiles - 1});

    const Tile questionTiles[] = {Tile::QuestionCoin, Tile::QuestionMushroom,
                                  Tile::QuestionFlower, Tile::QuestionOneUp,
                                  Tile::QuestionStar};
    for (const auto& segment : playable) {
        const int length = segment.last - segment.first + 1;
        if (length < 12) continue;
        const int blockX = segment.first + randomInt(3, std::max(3, length - 7));
        if (blockX + 3 < segment.last) {
            row(d, blockX, blockX + 3, 10, Tile::Brick);
            d.tiles.push_back({blockX + 1, 10, questionTiles[randomInt(0, 4)]});
            coinRow(d, blockX, blockX + 3, 7.0F);
        }
        if (randomInt(0, 2) == 0 && segment.last < d.widthTiles - 12) {
            spring(d, segment.last - 1);
        }
        if (randomInt(0, 2) == 0 && segment.first + 10 < segment.last) {
            const int pipeX = segment.first + randomInt(6, length - 5);
            const int height = 2 + randomInt(0, 1);
            d.pipes.push_back({pipeX, height});
            const int level = 1 + safeZone + safeDifficulty * 2 + randomInt(0, 2);
            d.enemies.push_back({EnemySpawnType::Piranha, static_cast<float>(pipeX) + 0.5F,
                                 static_cast<float>(15 - height), level});
        }
        const int enemyCount = 1 + randomInt(0, 2) + (safeDifficulty == 2 ? 1 : 0);
        for (int i = 0; i < enemyCount; ++i) {
            const int x = segment.first + randomInt(3, std::max(3, length - 3));
            const EnemySpawnType type = static_cast<EnemySpawnType>(randomInt(0, 4));
            const int level = 1 + safeZone + safeDifficulty * 2 + randomInt(0, 2) +
                              static_cast<int>(x / static_cast<float>(d.widthTiles) * 2.0F);
            if (type == EnemySpawnType::Piranha) {
                if (x + 2 >= segment.last) continue;
                const int height = 2 + randomInt(0, 1);
                d.pipes.push_back({x, height});
                d.enemies.push_back({EnemySpawnType::Piranha, static_cast<float>(x) + 0.5F,
                                     static_cast<float>(15 - height), level});
            } else if (type == EnemySpawnType::Paratroopa) {
                d.enemies.push_back({type, static_cast<float>(x),
                                     static_cast<float>(7 + randomInt(0, 3)), level});
            } else {
                d.enemies.push_back({type, static_cast<float>(x), 15.0F, level});
            }
        }
    }
    // 每个区域末尾生成一个 Boss，类型按 zone 循环，等级随 zone/difficulty 增长。
    const BossType bossType = static_cast<BossType>((safeZone - 1) % 3);
    const int bossLevel = 2 + safeZone + safeDifficulty * 2;
    placeBoss(d, bossType, bossLevel);
    return d;
}

LevelDef makeLevelDefinition(int index) {
    LevelDef d;
    switch (index) {
    case 0: d = firstLevel(); break;
    case 1: d = secondLevel(); break;
    case 2: d = thirdLevel(); break;
    case 3: d = fourthLevel(); break;
    case 4: d = fifthLevel(); break;
    case 5: d = sixthLevel(); break;
    case 6: d = seventhLevel(); break;
    default: d = eighthLevel(); break;
    }
    // 1-4、2-2、2-4 各布置一个不同类型的 Boss。
    switch (index) {
    case 3: placeBoss(d, BossType::BoomBoom, 3); break;
    case 5: placeBoss(d, BossType::Birdo, 5); break;
    case 7: placeBoss(d, BossType::Bowser, 7); break;
    default: break;
    }
    assignEnemyLevels(d, 1 + std::clamp(index, 0, config::levelCount - 1));
    return d;
}

std::string validateLevelDefinition(const LevelDef& d) {
    const auto fail = [](const std::string& message) { return message; };
    if (d.widthTiles < 60) return fail("level too short");
    if (d.flagX <= 0.0F || d.flagX >= static_cast<float>(d.widthTiles - 2)) return fail("flag out of bounds");
    if (d.checkpointX <= 0.0F || d.checkpointX >= d.flagX) return fail("checkpoint out of order");
    if (d.playerStart.x < 0.0F || d.playerStart.x >= d.flagX) return fail("player start out of bounds");
    if (d.grounds.empty()) return fail("no ground segments");
    for (const auto& g : d.grounds) {
        if (g.first < 0 || g.last >= d.widthTiles || g.first > g.last) return fail("ground segment invalid");
        if (g.top < 1 || g.top >= config::levelHeightTiles) return fail("ground top invalid");
    }
    for (const auto& t : d.tiles) {
        if (t.x < 0 || t.x >= d.widthTiles || t.y < 0 || t.y >= config::levelHeightTiles) {
            return fail("tile out of bounds");
        }
    }
    for (const auto& p : d.pipes) {
        if (p.x < 0 || p.x + 1 >= d.widthTiles || p.height < 1 || p.height > 4) return fail("pipe invalid");
    }
    for (const auto& e : d.enemies) {
        if (e.x < 0.0F || e.x >= static_cast<float>(d.widthTiles)) return fail("enemy out of bounds");
        if (e.y < 1.0F || e.y > static_cast<float>(config::levelHeightTiles)) return fail("enemy height invalid");
        if (e.level < 1 || e.level > 99) return fail("enemy level invalid");
        if (e.type == EnemySpawnType::Piranha) continue;
        // 除飞行乌龟外，地面敌人脚下应有支撑，避免出生即坠落。
        if (e.type == EnemySpawnType::Paratroopa) continue;
        const int tx = static_cast<int>(std::floor(e.x));
        const int ty = static_cast<int>(std::floor(e.y));
        if (!d.grounds.empty()) {
            bool supported = false;
            for (const auto& g : d.grounds) {
                if (tx >= g.first && tx <= g.last && ty <= g.top) supported = true;
            }
            if (!supported) return fail("ground enemy has no support");
        }
    }
    for (const auto& b : d.bosses) {
        if (b.x < 0.0F || b.x >= static_cast<float>(d.widthTiles)) return fail("boss out of bounds");
        if (b.y < 1.0F || b.y > static_cast<float>(config::levelHeightTiles)) return fail("boss height invalid");
        if (b.level < 1 || b.level > 99) return fail("boss level invalid");
        const int tx = static_cast<int>(std::floor(b.x));
        const int ty = static_cast<int>(std::floor(b.y));
        // Boss 体型较大，脚下必须有实体地面支撑，避免出生即坠落。
        bool supported = false;
        for (const auto& g : d.grounds) {
            if (tx >= g.first && tx <= g.last && ty <= g.top) { supported = true; break; }
        }
        if (!supported) return fail("boss has no ground support");
    }
    for (const auto& c : d.coins) {
        if (c.x < 0.0F || c.x >= static_cast<float>(d.widthTiles)) return fail("coin out of bounds");
        if (c.y < 1.0F || c.y > static_cast<float>(config::levelHeightTiles)) return fail("coin height invalid");
    }
    for (const auto& p : d.platforms) {
        if (p.x < 0.0F || p.x >= static_cast<float>(d.widthTiles)) return fail("platform out of bounds");
        if (p.y < 1.0F || p.y > static_cast<float>(config::levelHeightTiles)) return fail("platform height invalid");
        if (p.widthTiles < 1 || p.widthTiles > 8) return fail("platform width invalid");
        if (p.rangeTiles < 0.0F || p.speed <= 0.0F) return fail("platform motion invalid");
    }
    return {};
}

Level::Level(int index) : Level(makeLevelDefinition(index)) {
    index_ = index;
}

Level::Level(LevelDef definition) : index_(-1), definition_(std::move(definition)) {
    initializeTiles();
}

void Level::initializeTiles() {
    tiles_.resize(static_cast<std::size_t>(definition_.widthTiles * config::levelHeightTiles), Tile::Empty);
    for (const auto& ground : definition_.grounds) {
        for (int x = ground.first; x <= ground.last; ++x) {
            for (int y = ground.top; y < config::levelHeightTiles; ++y) setTile(x, y, Tile::Ground);
        }
    }
    for (const auto& placement : definition_.tiles) setTile(placement.x, placement.y, placement.tile);
    for (const auto& pipe : definition_.pipes) {
        const int top = 15 - pipe.height;
        setTile(pipe.x, top, Tile::PipeTopLeft);
        setTile(pipe.x + 1, top, Tile::PipeTopRight);
        for (int y = top + 1; y < 15; ++y) {
            setTile(pipe.x, y, Tile::PipeLeft);
            setTile(pipe.x + 1, y, Tile::PipeRight);
        }
    }
}

int Level::gridIndex(int x, int y) const {
    return y * definition_.widthTiles + x;
}

void Level::setTile(int x, int y, Tile tile) {
    if (x < 0 || x >= definition_.widthTiles || y < 0 || y >= config::levelHeightTiles) return;
    tiles_[static_cast<std::size_t>(gridIndex(x, y))] = tile;
}

Tile Level::tileAt(int x, int y) const {
    if (x < 0 || x >= definition_.widthTiles) return Tile::Solid; // 关卡左右边界
    if (y < 0 || y >= config::levelHeightTiles) return Tile::Empty;
    return tiles_[static_cast<std::size_t>(gridIndex(x, y))];
}

bool Level::isSolid(int x, int y) const {
    const Tile tile = tileAt(x, y);
    return tile != Tile::Empty && tile != Tile::CloudPlatform;
}

bool Level::isOneWay(int x, int y) const {
    return tileAt(x, y) == Tile::CloudPlatform;
}

bool Level::isSolidPixel(float x, float y) const {
    return isSolid(static_cast<int>(std::floor(x / config::tileSize)),
                   static_cast<int>(std::floor(y / config::tileSize)));
}

bool Level::isSupportPixel(float x, float y) const {
    const int tx = static_cast<int>(std::floor(x / config::tileSize));
    const int ty = static_cast<int>(std::floor(y / config::tileSize));
    return isSolid(tx, ty) || isOneWay(tx, ty);
}

TileHit Level::hitTile(int x, int y, bool canBreak) {
    const Tile tile = tileAt(x, y);
    TileHit hit;
    hit.x = x;
    hit.y = y;
    switch (tile) {
    case Tile::Brick:
        hit.kind = canBreak ? TileHitKind::Break : TileHitKind::Bump;
        if (canBreak) setTile(x, y, Tile::Empty);
        break;
    case Tile::BrickCoin:
        hit.kind = TileHitKind::Coin;
        setTile(x, y, Tile::Used);
        break;
    case Tile::QuestionCoin:
        hit.kind = TileHitKind::Coin;
        setTile(x, y, Tile::Used);
        break;
    case Tile::QuestionMushroom:
        hit.kind = TileHitKind::Mushroom;
        setTile(x, y, Tile::Used);
        break;
    case Tile::QuestionFlower:
        hit.kind = TileHitKind::Flower;
        setTile(x, y, Tile::Used);
        break;
    case Tile::QuestionOneUp:
        hit.kind = TileHitKind::OneUp;
        setTile(x, y, Tile::Used);
        break;
    case Tile::QuestionStar:
        hit.kind = TileHitKind::Star;
        setTile(x, y, Tile::Used);
        break;
    default:
        return {TileHitKind::None, x, y};
    }
    bumps_[gridIndex(x, y)] = 0.22F;
    return hit;
}

void Level::update(float dt) {
    for (auto it = bumps_.begin(); it != bumps_.end();) {
        it->second -= dt;
        if (it->second <= 0.0F) it = bumps_.erase(it);
        else ++it;
    }
}

float Level::bumpOffset(int x, int y) const {
    const auto it = bumps_.find(gridIndex(x, y));
    if (it == bumps_.end()) return 0.0F;
    const float progress = 1.0F - it->second / 0.22F;
    return -std::sin(progress * 3.1415926F) * 9.0F;
}

void Level::draw(Renderer& renderer, float cameraX, float flagDrop, bool checkpointActive) const {
    const int first = std::max(0, static_cast<int>(cameraX / config::tileSize) - 2);
    const int last = std::min(definition_.widthTiles - 1, first + config::logicalWidth / 32 + 4);
    for (int y = 0; y < config::levelHeightTiles; ++y) {
        for (int x = first; x <= last; ++x) {
            const Tile tile = tileAt(x, y);
            if (tile == Tile::Empty) continue;
            Sprite sprite = Sprite::Solid;
            switch (tile) {
            case Tile::Ground: sprite = isSolid(x, y - 1) ? Sprite::Ground : Sprite::GroundTop; break;
            case Tile::Solid: sprite = Sprite::Solid; break;
            case Tile::Brick:
            case Tile::BrickCoin: sprite = Sprite::Brick; break;
            case Tile::QuestionCoin:
            case Tile::QuestionMushroom:
            case Tile::QuestionFlower:
            case Tile::QuestionOneUp:
            case Tile::QuestionStar: sprite = Sprite::Question; break;
            case Tile::Used: sprite = Sprite::Used; break;
            case Tile::PipeTopLeft: sprite = Sprite::PipeTopLeft; break;
            case Tile::PipeTopRight: sprite = Sprite::PipeTopRight; break;
            case Tile::PipeLeft: sprite = Sprite::PipeLeft; break;
            case Tile::PipeRight: sprite = Sprite::PipeRight; break;
            case Tile::CloudPlatform: sprite = Sprite::CloudPlatform; break;
            case Tile::Spring: sprite = Sprite::Spring; break;
            case Tile::Empty: break;
            }
            renderer.drawSprite(sprite, x * config::tileSize,
                                y * config::tileSize + bumpOffset(x, y), config::tileSize, config::tileSize);
        }
    }

    // 中点旗与终点旗杆都是程序化图元，不使用图片。
    const float checkpoint = checkpointX();
    renderer.drawRect(checkpoint + 14, 336, 4, 144, {0.85F, 0.85F, 0.85F, 1});
    renderer.drawSprite(Sprite::Checkpoint, checkpoint + 18, 344, 40, 28, false,
                        checkpointActive ? glm::vec4{1, 1, 1, 1} : glm::vec4{0.55F, 0.55F, 0.55F, 1});
    const float poleX = flagX() + 15;
    renderer.drawRect(poleX, 128, 5, 352, {0.90F, 0.95F, 0.86F, 1});
    renderer.drawRect(poleX - 5, 120, 15, 12, {1.0F, 0.82F, 0.20F, 1});
    renderer.drawSprite(Sprite::Flag, poleX - 43, 140 + std::min(flagDrop, 270.0F), 42, 30, true);

    const float castleX = flagX() + 4 * config::tileSize;
    renderer.drawRect(castleX, 384, 128, 96, {0.58F, 0.28F, 0.18F, 1});
    renderer.drawRect(castleX + 40, 416, 42, 64, {0.15F, 0.10F, 0.12F, 1});
    for (int i = 0; i < 4; ++i) {
        renderer.drawRect(castleX + i * 32, 368, 20, 20, {0.58F, 0.28F, 0.18F, 1});
    }
    renderer.drawRect(castleX + 12, 400, 18, 20, {0.12F, 0.16F, 0.20F, 1});
    renderer.drawRect(castleX + 98, 400, 18, 20, {0.12F, 0.16F, 0.20F, 1});
}

float Level::worldWidth() const { return definition_.widthTiles * config::tileSize; }
glm::vec2 Level::playerStart() const { return definition_.playerStart * config::tileSize; }
float Level::checkpointX() const { return definition_.checkpointX * config::tileSize; }
float Level::flagX() const { return definition_.flagX * config::tileSize; }
