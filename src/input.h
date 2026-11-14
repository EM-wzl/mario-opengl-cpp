#pragma once

#include <array>

struct GLFWwindow;

enum class Action {
    Left = 0,
    Right,
    Jump,
    Run,
    Fire,
    Pause,
    Mute,
    Confirm,
    MenuUp,
    MenuDown,
    Skill,
    Save,
    Load,
    Count
};

class Input {
public:
    explicit Input(GLFWwindow* window);

    // 每个渲染帧轮询一次；pressed/released 在首个物理步后清除，避免重复跳跃。
    void poll();
    void clearEdges();
    // 供无窗口冒烟测试注入一次虚拟按键；正常游戏不会调用。
    void setVirtualKey(Action action, bool value);
    bool down(Action action) const;
    bool pressed(Action action) const;
    bool released(Action action) const;

private:
    static constexpr std::size_t actionCount = static_cast<std::size_t>(Action::Count);
    GLFWwindow* window_;
    std::array<bool, actionCount> current_{};
    std::array<bool, actionCount> previous_{};
    std::array<bool, actionCount> pressed_{};
    std::array<bool, actionCount> released_{};
};
