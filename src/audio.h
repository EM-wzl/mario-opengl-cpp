#pragma once

#include <memory>

enum class SoundEffect { Jump, Coin, Brick, Stomp, Power, Fire, Death, Clear, Kick };

class Audio {
public:
    Audio();
    ~Audio();
    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;

    void play(SoundEffect effect);
    void toggleMute();
    bool muted() const;
    bool available() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
