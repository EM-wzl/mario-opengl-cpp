#include "input.h"

#include <GLFW/glfw3.h>

Input::Input(GLFWwindow* window) : window_(window) {}

void Input::poll() {
    previous_ = current_;
    const auto key = [this](int code) {
        return glfwGetKey(window_, code) == GLFW_PRESS;
    };

    current_[static_cast<std::size_t>(Action::Left)] = key(GLFW_KEY_LEFT) || key(GLFW_KEY_A);
    current_[static_cast<std::size_t>(Action::Right)] = key(GLFW_KEY_RIGHT) || key(GLFW_KEY_D);
    current_[static_cast<std::size_t>(Action::Jump)] = key(GLFW_KEY_SPACE) || key(GLFW_KEY_UP) || key(GLFW_KEY_W);
    current_[static_cast<std::size_t>(Action::Run)] = key(GLFW_KEY_LEFT_SHIFT) || key(GLFW_KEY_RIGHT_SHIFT) || key(GLFW_KEY_X);
    current_[static_cast<std::size_t>(Action::Fire)] = key(GLFW_KEY_X) || key(GLFW_KEY_LEFT_SHIFT) || key(GLFW_KEY_RIGHT_SHIFT);
    current_[static_cast<std::size_t>(Action::Pause)] = key(GLFW_KEY_P) || key(GLFW_KEY_ESCAPE);
    current_[static_cast<std::size_t>(Action::Mute)] = key(GLFW_KEY_M);
    current_[static_cast<std::size_t>(Action::Confirm)] = key(GLFW_KEY_ENTER) || key(GLFW_KEY_KP_ENTER);
    current_[static_cast<std::size_t>(Action::MenuUp)] = key(GLFW_KEY_UP) || key(GLFW_KEY_W);
    current_[static_cast<std::size_t>(Action::MenuDown)] = key(GLFW_KEY_DOWN) || key(GLFW_KEY_S);
    current_[static_cast<std::size_t>(Action::Skill)] = key(GLFW_KEY_C) || key(GLFW_KEY_Z) || key(GLFW_KEY_V);
    current_[static_cast<std::size_t>(Action::Save)] = key(GLFW_KEY_F5);
    current_[static_cast<std::size_t>(Action::Load)] = key(GLFW_KEY_F9) || key(GLFW_KEY_L);

    for (std::size_t i = 0; i < actionCount; ++i) {
        // 边沿锁存到物理步消费，避免高渲染帧率下短按被下一次轮询覆盖。
        pressed_[i] = pressed_[i] || (current_[i] && !previous_[i]);
        released_[i] = released_[i] || (!current_[i] && previous_[i]);
    }
}

void Input::clearEdges() {
    pressed_.fill(false);
    released_.fill(false);
}

void Input::setVirtualKey(Action action, bool value) {
    const std::size_t index = static_cast<std::size_t>(action);
    previous_[index] = current_[index];
    current_[index] = value;
    pressed_[index] = pressed_[index] || (value && !previous_[index]);
    released_[index] = released_[index] || (!value && previous_[index]);
}

bool Input::down(Action action) const {
    return current_[static_cast<std::size_t>(action)];
}

bool Input::pressed(Action action) const {
    return pressed_[static_cast<std::size_t>(action)];
}

bool Input::released(Action action) const {
    return released_[static_cast<std::size_t>(action)];
}
