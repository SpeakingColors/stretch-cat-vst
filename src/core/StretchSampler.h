// Stretch Cat: the sampler part. Plays the rendered (stretched) buffer from MIDI notes. Pure C++.
// A note repitches the rendered buffer like a tape sampler (root note = original pitch). Playback: loop, one shot, or gate.
#pragma once
#include "StretchEngine.h"
#include <array>
#include <memory>

namespace stretchcat {

enum class Playback { Loop, OneShot, Gate };

struct SamplerParams { Playback mode = Playback::Loop; double attackMs = 5, releaseMs = 80; int root = 60; float gainDb = 0.f; };

class Sampler {
public:
    static constexpr int kVoices = 8;
    void prepare(double sampleRate) { fs = sampleRate; for (auto& v : voices) v = Voice(); }
    // The buffer is shared with the audio thread; swap it in whole. Rendered at bufferSr.
    void setBuffer(std::shared_ptr<const Stereo> b, double bufferSr) { buf = std::move(b); bufSr = bufferSr; }

    void noteOn(int note, float vel, const SamplerParams& p) {
        if (!buf || buf->L.size() < 4) return;
        Voice* v = nullptr; for (auto& x : voices) if (!x.active) { v = &x; break; }
        if (!v) { v = &voices[0]; for (auto& x : voices) if (x.age > v->age) v = &x; }       // steal the oldest
        v->active = true; v->note = note; v->vel = vel; v->pos = 0; v->env = 0; v->releasing = false; v->age = 0;
        v->ratio = std::pow(2.0, (double)(note - p.root) / 12.0) * bufSr / fs;
    }
    void noteOff(int note, const SamplerParams& p) {
        for (auto& v : voices) if (v.active && v.note == note && !v.releasing && p.mode != Playback::OneShot) v.releasing = true;
    }
    void allOff() { for (auto& v : voices) v.releasing = true; }
    int activeVoices() const { int c = 0; for (auto& v : voices) c += v.active ? 1 : 0; return c; }

    // Adds into outL/outR.
    void process(const SamplerParams& p, float* outL, float* outR, int n) {
        if (!buf) return;
        const double len = (double) buf->L.size(); const float g = std::pow(10.f, p.gainDb / 20.f);
        const float aInc = 1.f / (float) std::max(1.0, p.attackMs * 0.001 * fs), rDec = 1.f / (float) std::max(1.0, p.releaseMs * 0.001 * fs);
        for (auto& v : voices) {
            if (!v.active) continue;
            for (int i = 0; i < n; ++i) {
                if (v.releasing) { v.env -= rDec; if (v.env <= 0.f) { v.active = false; break; } }
                else if (v.env < 1.f) v.env = std::min(1.f, v.env + aInc);
                if (v.pos >= len - 1) {
                    if (p.mode == Playback::Loop || p.mode == Playback::Gate) { v.pos = std::fmod(v.pos, len - 1); }
                    else { v.active = false; break; }
                }
                const float a = hermite(buf->L, v.pos), b = hermite(buf->R, v.pos), k = v.env * v.vel * g;
                outL[i] += a * k; outR[i] += b * k; v.pos += v.ratio;
            }
            ++v.age;
        }
    }
private:
    struct Voice { bool active = false, releasing = false; int note = 0; float vel = 0, env = 0; double pos = 0, ratio = 1; int age = 0; };
    std::array<Voice, kVoices> voices;
    std::shared_ptr<const Stereo> buf; double bufSr = 44100, fs = 44100;
};
}
