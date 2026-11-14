#include "audio.h"
#include "config.h"
#include "game.h"
#include "input.h"
#include "renderer.h"

#include "glcore.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <exception>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    const bool smokeTest = argc > 1 && std::string(argv[1]) == "--smoke";

    // 无窗口自检模式：验证所有关卡数据，构建流水线可直接调用。
    if (argc > 1 && std::string(argv[1]) == "--validate") {
        int failures = 0;
        for (int i = 0; i < config::levelCount; ++i) {
            const LevelDef definition = makeLevelDefinition(i);
            const std::string error = validateLevelDefinition(definition);
            std::cout << "LEVEL " << definition.world << "-" << definition.stage << " "
                      << definition.name << " (" << definition.widthTiles << " tiles, "
                      << definition.bosses.size() << " bosses): "
                      << (error.empty() ? "OK" : error) << '\n';
            if (!error.empty()) ++failures;
            if ((i == 3 || i == 5 || i == 7) && definition.bosses.empty()) ++failures;
        }
        // 覆盖不同层数、种子、难度下的程序化无限世界生成结果。
        const std::uint32_t seeds[] = {1U, 77U, 4096U};
        const int zones[] = {1, 4, 9};
        for (int difficulty = 0; difficulty < 3; ++difficulty) {
            for (int zone : zones) {
                const LevelDef definition = makeEndlessLevelDefinition(zone, seeds[difficulty], difficulty);
                const std::string error = validateLevelDefinition(definition);
                std::cout << "ENDLESS Z" << zone << " D" << difficulty << " S"
                          << seeds[difficulty] << " (" << definition.widthTiles << " tiles, "
                          << definition.bosses.size() << " bosses): "
                          << (error.empty() ? "OK" : error) << '\n';
                if (!error.empty() || definition.bosses.empty()) ++failures;
            }
        }
        return failures == 0 ? 0 : 1;
    }

    if (glfwInit() == GLFW_FALSE) {
        std::cerr << "GLFW 初始化失败。\n";
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    if (smokeTest) glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(config::logicalWidth, config::logicalHeight,
                                          "OpenGL 超级马力欧 - 程序化像素版", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "无法创建 OpenGL 3.3 窗口。\n";
        glfwTerminate();
        return 1;
    }

    try {
        glfwMakeContextCurrent(window);
        if (!loadGL()) {
            throw std::runtime_error("OpenGL 函数加载失败（上下文或驱动不受支持）");
        }
        // 清空可能存在的历史 GL 错误状态。
        while (glGetError() != GL_NO_ERROR) {}
        glfwSwapInterval(smokeTest ? 0 : 1);

        {
            Renderer renderer;
            Input input(window);
            Audio audio;
            Game game(input, audio);
            std::cout << "操作：方向键或 A/D 移动，Space/W/上跳跃，Shift/X 加速与发射，C/Z 角色技能，F5 保存，F9/L 读取，P 暂停，M 静音。\n";
            std::cout << "标题界面：上下选择行，左右调整模式/角色/关卡、难度、种子与存档位，Enter 开始，L 读取存档。\n";
            if (!audio.available()) std::cout << "未检测到可用音频后端，游戏将以静音模式运行。\n";

            if (smokeTest) {
                // 逐关加载并模拟：覆盖关卡构建、怪物等级、平台移动、成长界面与完整绘制。
                int glErrors = 0;
                auto simulate = [&](int frames) {
                    for (int frame = 0; frame < frames; ++frame) {
                        input.poll();
                        game.update(config::fixedStep);
                        renderer.beginFrame(config::logicalWidth, config::logicalHeight);
                        game.render(renderer);
                        renderer.endFrame();
                        glfwSwapBuffers(window);
                    }
                    while (glGetError() != GL_NO_ERROR) ++glErrors;
                };
                auto tap = [&](Action action) {
                    input.setVirtualKey(action, true);
                    game.update(config::fixedStep);
                    input.clearEdges();
                    input.setVirtualKey(action, false);
                    game.update(config::fixedStep);
                    input.clearEdges();
                };

                // 标题菜单：上下换列、左右改值，验证开局选项状态机。
                tap(Action::MenuDown);
                tap(Action::Right);
                tap(Action::MenuUp);
                tap(Action::Left);
                tap(Action::MenuDown);
                tap(Action::MenuDown);
                tap(Action::Right);
                std::cout << "SMOKE MENU OK\n";

                for (int level = 0; level < config::levelCount; ++level) {
                    game.startAtLevel(level);
                    simulate(120);
                    const LevelDef& def = game.level().definition();
                    std::cout << "SMOKE LEVEL " << def.world << "-" << def.stage << " "
                              << def.name << " OK\n";
                }

                const std::uint32_t seeds[] = {7U, 128U, 999U};
                for (int difficulty = 0; difficulty < 3; ++difficulty) {
                    game.startEndlessZone(1 + difficulty * 3, seeds[difficulty], difficulty,
                                          static_cast<CharacterType>(difficulty));
                    simulate(90);
                    std::cout << "SMOKE ENDLESS D" << difficulty << " Z"
                              << game.level().definition().stage << " OK\n";
                }

                // 触达旗杆后，验证无限世界真的切换到下一层而不是结束。
                game.startEndlessZone(1, 77U, 1, CharacterType::Mario);
                const int previousZone = game.level().definition().stage;
                game.completeLevel();
                simulate(180);
                const int nextZone = game.level().definition().stage;
                if (game.state() != GameState::Playing || nextZone != previousZone + 1) ++glErrors;
                std::cout << "SMOKE ENDLESS ADVANCE Z" << nextZone << " OK\n";

                // 强制触发一次升级，验证天赋三选一界面的选择、确认与绘制。
                game.startAtLevel(0);
                game.addXP(xpRequired(1) + 10);
                if (game.state() != GameState::LevelUp) ++glErrors;
                simulate(30);
                tap(Action::Right);
                tap(Action::Confirm);
                if (game.state() != GameState::Playing) ++glErrors;
                simulate(30);
                std::cout << "SMOKE LEVEL UP OK\n";

                // 四个角色的主动技能分别执行一次，覆盖各自分支。
                for (int character = 0; character < static_cast<int>(CharacterType::Count); ++character) {
                    RunOptions options;
                    options.character = static_cast<CharacterType>(character);
                    options.startLevel = 0;
                    game.startRun(options);
                    game.player().body.position.y -= 80.0F;
                    game.player().body.onGround = false;
                    game.player().useSkill(game);
                    simulate(30);
                    std::cout << "SMOKE SKILL " << characterName(options.character) << " OK\n";
                }

                // 血量、存档、读取：验证 HP 从 5 降到 4 后写入并恢复。
                game.startAtLevel(0);
                const int initialHp = game.hp();
                game.damagePlayer(1, true);
                if (game.hp() != initialHp - 1) ++glErrors;
                if (!game.saveGame(2)) ++glErrors;
                if (!game.loadGame(2)) ++glErrors;
                if (game.state() != GameState::Playing || game.hp() != initialHp - 1) ++glErrors;
                simulate(30);
                std::cout << "SMOKE HP SAVE LOAD OK\n";

                // Boss 关卡：验证 Boss 存在、投射物生成、Boss 血条与旗杆锁定。
                game.startAtLevel(3);
                if (!game.bossAlive()) ++glErrors;
                game.spawnBossShot(game.player().body.position.x + 120.0F,
                                   game.player().body.position.y, -160.0F, 0.0F, 0);
                simulate(120);
                if (!game.bossAlive()) ++glErrors;
                std::cout << "SMOKE BOSS OK\n";

                std::cout << (glErrors == 0 ? "SMOKE PASS\n" : "SMOKE GL ERRORS\n");
                return glErrors == 0 ? 0 : 1;
            }

            double previousTime = glfwGetTime();
            double accumulator = 0.0;
            while (glfwWindowShouldClose(window) == GLFW_FALSE) {
                glfwPollEvents();
                input.poll();
                const double now = glfwGetTime();
                accumulator += std::min(now - previousTime, static_cast<double>(config::maxFrameTime));
                previousTime = now;

                bool firstPhysicsStep = true;
                while (accumulator >= config::fixedStep) {
                    game.update(config::fixedStep);
                    accumulator -= config::fixedStep;
                    if (firstPhysicsStep) {
                        input.clearEdges();
                        firstPhysicsStep = false;
                    }
                }

                int width = 0;
                int height = 0;
                glfwGetFramebufferSize(window, &width, &height);
                if (width > 0 && height > 0) {
                    renderer.beginFrame(width, height);
                    game.render(renderer);
                    renderer.endFrame();
                    glfwSwapBuffers(window);
                }
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "运行错误: " << error.what() << '\n';
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
