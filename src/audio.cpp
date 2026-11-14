#include "audio.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#elif defined(MARIO_HAS_ALSA)
#include <alsa/asoundlib.h>
#endif

namespace {
constexpr int sampleRate = 22050;
constexpr float pi = 3.14159265358979323846F;
constexpr std::size_t effectCount = 9;

float oscillator(float frequency, float t, bool square) {
    const float phase = std::sin(2.0F * pi * frequency * t);
    return square ? (phase >= 0.0F ? 1.0F : -1.0F) : phase;
}

std::vector<std::int16_t> synthesize(SoundEffect effect) {
    float duration = 0.18F;
    switch (effect) {
    case SoundEffect::Jump: duration = 0.18F; break;
    case SoundEffect::Coin: duration = 0.28F; break;
    case SoundEffect::Brick: duration = 0.12F; break;
    case SoundEffect::Stomp: duration = 0.15F; break;
    case SoundEffect::Power: duration = 0.48F; break;
    case SoundEffect::Fire: duration = 0.16F; break;
    case SoundEffect::Death: duration = 0.85F; break;
    case SoundEffect::Clear: duration = 0.95F; break;
    case SoundEffect::Kick: duration = 0.10F; break;
    }
    const std::size_t count = static_cast<std::size_t>(duration * sampleRate);
    std::vector<std::int16_t> samples(count);
    std::uint32_t noise = 0x12345678U;
    for (std::size_t i = 0; i < count; ++i) {
        const float t = static_cast<float>(i) / sampleRate;
        const float progress = t / duration;
        float value = 0.0F;
        noise = noise * 1664525U + 1013904223U;
        const float random = static_cast<float>((noise >> 8) & 0xFFFFU) / 32768.0F - 1.0F;
        switch (effect) {
        case SoundEffect::Jump:
            value = oscillator(230.0F + 410.0F * progress, t, true) * 0.30F;
            break;
        case SoundEffect::Coin: {
            const float frequency = t < 0.09F ? 988.0F : 1319.0F;
            value = oscillator(frequency, t, false) * 0.38F;
            break;
        }
        case SoundEffect::Brick:
            value = (random * 0.65F + oscillator(95.0F, t, true) * 0.35F) * (1.0F - progress) * 0.42F;
            break;
        case SoundEffect::Stomp:
            value = oscillator(180.0F - 100.0F * progress, t, true) * (1.0F - progress) * 0.35F;
            break;
        case SoundEffect::Power: {
            const float notes[6] = {523, 659, 784, 1047, 1319, 1568};
            const int note = std::min(5, static_cast<int>(t / 0.08F));
            value = oscillator(notes[note], t, false) * 0.32F;
            break;
        }
        case SoundEffect::Fire:
            value = (random * 0.35F + oscillator(650.0F - 400.0F * progress, t, false)) *
                    (1.0F - progress) * 0.30F;
            break;
        case SoundEffect::Death: {
            const float notes[7] = {523, 494, 440, 392, 330, 262, 196};
            const int note = std::min(6, static_cast<int>(t / 0.12F));
            value = oscillator(notes[note], t, note % 2 == 0) * 0.27F;
            break;
        }
        case SoundEffect::Clear: {
            const float notes[8] = {523, 659, 784, 1047, 784, 1047, 1319, 1568};
            const int note = std::min(7, static_cast<int>(t / 0.115F));
            value = oscillator(notes[note], t, false) * 0.32F;
            break;
        }
        case SoundEffect::Kick:
            value = (random * 0.45F + oscillator(130.0F, t, true) * 0.4F) * (1.0F - progress) * 0.35F;
            break;
        }
        const float attack = std::min(1.0F, t / 0.006F);
        const float release = std::min(1.0F, (duration - t) / 0.035F);
        value *= attack * release;
        samples[i] = static_cast<std::int16_t>(std::clamp(value, -0.95F, 0.95F) * 32767.0F);
    }
    return samples;
}
} // namespace

struct Audio::Impl {
    std::array<std::vector<std::int16_t>, effectCount> sounds;
    std::deque<SoundEffect> queue;
    std::mutex mutex;
    std::condition_variable condition;
    std::thread worker;
    std::atomic<bool> running{true};
    std::atomic<bool> muted{false};
    bool ready = false;
#ifdef _WIN32
    HWAVEOUT output = nullptr;
#elif defined(MARIO_HAS_ALSA)
    snd_pcm_t* output = nullptr;
#endif

    Impl() {
        for (std::size_t i = 0; i < effectCount; ++i) sounds[i] = synthesize(static_cast<SoundEffect>(i));
#ifdef _WIN32
        WAVEFORMATEX format{};
        format.wFormatTag = WAVE_FORMAT_PCM;
        format.nChannels = 1;
        format.nSamplesPerSec = sampleRate;
        format.wBitsPerSample = 16;
        format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
        format.nAvgBytesPerSec = sampleRate * format.nBlockAlign;
        ready = waveOutOpen(&output, WAVE_MAPPER, &format, 0, 0, CALLBACK_NULL) == MMSYSERR_NOERROR;
#elif defined(MARIO_HAS_ALSA)
        if (snd_pcm_open(&output, "default", SND_PCM_STREAM_PLAYBACK, 0) >= 0) {
            ready = snd_pcm_set_params(output, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
                                       1, sampleRate, 1, 25000) >= 0;
            if (!ready) {
                snd_pcm_close(output);
                output = nullptr;
            }
        }
#endif
        if (ready) worker = std::thread([this] { run(); });
    }

    ~Impl() {
        running = false;
        condition.notify_all();
        if (worker.joinable()) worker.join();
#ifdef _WIN32
        if (output != nullptr) waveOutClose(output);
#elif defined(MARIO_HAS_ALSA)
        if (output != nullptr) {
            snd_pcm_drop(output);
            snd_pcm_close(output);
        }
#endif
    }

    void enqueue(SoundEffect effect) {
        if (!ready || muted) return;
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (queue.size() > 12) queue.pop_front();
            queue.push_back(effect);
        }
        condition.notify_one();
    }

    void run() {
        while (running) {
            SoundEffect effect = SoundEffect::Jump;
            {
                std::unique_lock<std::mutex> lock(mutex);
                condition.wait(lock, [this] { return !running || !queue.empty(); });
                if (!running) break;
                effect = queue.front();
                queue.pop_front();
            }
            if (!muted) play(sounds[static_cast<std::size_t>(effect)]);
        }
    }

    void play(const std::vector<std::int16_t>& samples) {
#ifdef _WIN32
        WAVEHDR header{};
        header.lpData = reinterpret_cast<LPSTR>(const_cast<std::int16_t*>(samples.data()));
        header.dwBufferLength = static_cast<DWORD>(samples.size() * sizeof(std::int16_t));
        if (waveOutPrepareHeader(output, &header, sizeof(header)) != MMSYSERR_NOERROR) return;
        if (waveOutWrite(output, &header, sizeof(header)) == MMSYSERR_NOERROR) {
            while (running && (header.dwFlags & WHDR_DONE) == 0U) std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        if (!running) waveOutReset(output);
        while ((header.dwFlags & WHDR_DONE) == 0U && running) std::this_thread::yield();
        waveOutUnprepareHeader(output, &header, sizeof(header));
#elif defined(MARIO_HAS_ALSA)
        std::size_t offset = 0;
        while (running && offset < samples.size()) {
            const snd_pcm_sframes_t written = snd_pcm_writei(output, samples.data() + offset, samples.size() - offset);
            if (written == -EPIPE) {
                snd_pcm_prepare(output);
            } else if (written < 0) {
                break;
            } else {
                offset += static_cast<std::size_t>(written);
            }
        }
#else
        (void)samples;
#endif
    }
};

Audio::Audio() : impl_(std::make_unique<Impl>()) {}
Audio::~Audio() = default;
void Audio::play(SoundEffect effect) { impl_->enqueue(effect); }
void Audio::toggleMute() { impl_->muted = !impl_->muted; }
bool Audio::muted() const { return impl_->muted; }
bool Audio::available() const { return impl_->ready; }
