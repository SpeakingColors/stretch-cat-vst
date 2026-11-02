// Stretch Cat: the sampler part. Plays the rendered (stretched) buffer from MIDI notes. Pure C++.
// A note repitches the rendered buffer like a tape sampler (root note = original pitch). Playback: loop, one shot, or gate.
// Slice voices play one slice of the render at its original pitch. When a new render arrives (swapTo) the old one fades out
// over 20 ms while the new one fades in and every voice carries on at the same place in the music. Live lanes (cutoff, width)
// are applied per voice at the voice's place in the render, so they react at once and need no new render.
#pragma once
#include "StretchEngine.h"
#include "Slices.h"
#include "Rack.h"
#include <array>
#include <memory>

namespace stretchcat {

enum class Playback { Loop, OneShot, Gate };

// One finished render, as the audio thread sees it. Written by the render worker, then only read.
struct Rendered {
    Stereo audio; double sr = 44100; uint64_t id = 0;
    std::vector<long long> sliceBounds;               // slice count + 1 positions in `audio`
    std::array<float, 256> tCurve {};                 // source time 0..1 (over the selection) at each 1/255 of the render
    double renderMs = 0;
    long long length() const { return (long long) audio.L.size(); }
    double tAt(double frac) const { const double x = std::max(0.0, std::min(1.0, frac)) * 255.0; const int i = (int) x, j = std::min(255, i + 1); const float f = (float) (x - i); return (double) (tCurve[(size_t) i] * (1 - f) + tCurve[(size_t) j] * f); }
};

struct SamplerParams { Playback mode = Playback::Loop; double attackMs = 5, releaseMs = 80; int root = 60; float gainDb = 0.f; };

class Sampler {
public:
    static constexpr int kVoices = 8;
    void prepare(double sampleRate) { fs = sampleRate; for (auto& v : voices) v = Voice(); cur = old = nullptr; xf = 0; ended = false; }
    void setLanes(const LiveLanes* l) { lanes = l; }
    bool crossfading() const { return xf > 0.f; }
    bool takeCrossfadeEnded() { const bool e = ended; ended = false; return e; }
    int sliceCount() const { return cur ? (int) cur->sliceBounds.size() - 1 : 0; }
    const Rendered* current() const { return cur; }

    // Switch to a newly finished render. The pointer must stay valid until takeCrossfadeEnded() reports true (and while it is current).
    void swapTo(const Rendered* r) {
        if (!r) return;
        if (!cur) { cur = r; return; }
        old = cur; cur = r; xf = 1.f; ended = false;
        const double newLen = (double) std::max<long long>(1, cur->length());
        for (auto& v : voices) {
            if (!v.active) continue;
            v.posOld = v.pos; v.loOld = v.lo; v.hiOld = v.hi;
            if (v.slice >= 0 && v.slice < sliceCount()) { v.lo = (double) cur->sliceBounds[(size_t) v.slice]; v.hi = (double) cur->sliceBounds[(size_t) v.slice + 1]; }
            else if (v.slice >= 0) { v.releasing = true; v.lo = 0; v.hi = newLen; }
            else { v.lo = 0; v.hi = newLen; }
            const double rel = (v.posOld - v.loOld) / std::max(1.0, v.hiOld - v.loOld); v.pos = v.lo + std::max(0.0, std::min(1.0, rel)) * (v.hi - v.lo);
        }
    }

    void noteOn(int note, float vel, const SamplerParams& p) {
        Voice* v = start(note, vel); if (!v) return;
        v->slice = -1; v->lo = 0; v->hi = (double) cur->length(); v->ratio = std::pow(2.0, (double)(note - p.root) / 12.0) * cur->sr / fs;
    }
    void sliceOn(int slice, float vel) {
        if (!cur || slice < 0 || slice >= sliceCount()) return;
        Voice* v = start(1000 + slice, vel); if (!v) return;
        v->slice = slice; v->lo = (double) cur->sliceBounds[(size_t) slice]; v->hi = (double) cur->sliceBounds[(size_t) slice + 1]; v->pos = v->lo; v->ratio = cur->sr / fs;
    }
    void noteOff(int note, const SamplerParams& p) { for (auto& v : voices) if (v.active && v.key == note && !v.releasing && p.mode != Playback::OneShot) v.releasing = true; }
    void sliceOff(int slice, const SamplerParams& p) { noteOff(1000 + slice, p); }
    void allOff() { for (auto& v : voices) v.releasing = true; }
    int activeVoices() const { int c = 0; for (auto& v : voices) c += v.active ? 1 : 0; return c; }
    // fraction 0..1 of the render where the newest voice is playing (for the editor's play head), or -1
    float newestPosition() const { const Voice* b = nullptr; for (auto& v : voices) if (v.active && (!b || v.age < b->age)) b = &v; return (b && cur) ? (float) (b->pos / std::max<long long>(1, cur->length())) : -1.f; }

    // Adds into outL/outR.
    void process(const SamplerParams& p, float* outL, float* outR, int n) {
        if (!cur) return;
        const float g = std::pow(10.f, p.gainDb / 20.f);
        const float aInc = 1.f / (float) std::max(1.0, p.attackMs * 0.001 * fs), rDec = 1.f / (float) std::max(1.0, p.releaseMs * 0.001 * fs);
        const float xfStep = 1.f / (float) std::max(1.0, 0.020 * fs);
        const bool useCut = lanes && lanes->cutoff.active.load(std::memory_order_relaxed), useWid = lanes && lanes->width.active.load(std::memory_order_relaxed);
        const long long len = cur->length(); const bool blendOld = xf > 0.f && old; const float xf0 = xf;
        for (auto& v : voices) {
            if (!v.active) continue;
            const bool slice = v.slice >= 0; const double span = std::max(1.0, v.hi - v.lo - 1);
            for (int i = 0; i < n; ++i) {
                if (v.releasing) { v.env -= rDec; if (v.env <= 0.f) { v.active = false; break; } }
                else if (v.env < 1.f) v.env = std::min(1.f, v.env + aInc);
                if (v.pos >= v.hi - 1) {
                    if (p.mode == Playback::Loop || (p.mode == Playback::Gate && !slice)) v.pos = v.lo + std::fmod(v.pos - v.lo, span);
                    else { v.active = false; break; }
                }
                float a = hermite(cur->audio.L, v.pos), b = hermite(cur->audio.R, v.pos);
                if (blendOld) {
                    float oa = 0.f, ob = 0.f; if (v.posOld < v.hiOld - 1) { oa = hermite(old->audio.L, v.posOld); ob = hermite(old->audio.R, v.posOld); }
                    const float f = std::max(0.f, xf0 - xfStep * (float) (i + 1)); a = a * (1.f - f) + oa * f; b = b * (1.f - f) + ob * f; v.posOld += v.ratio;
                }
                // the stop of a one shot or a slice is faded over 64 samples so it never clicks
                const float edge = (float) std::min(1.0, (v.hi - 1 - v.pos) / 64.0);
                if (useCut || useWid) {
                    if ((v.ctr++ & 15) == 0) { const double t = cur->tAt(v.pos / (double) std::max<long long>(1, len)); v.cutTarget = useCut ? LiveLanes::cutoffHz(lanes->cutoff.at(t)) : 20000.0; v.widTarget = useWid ? LiveLanes::widthOf(lanes->width.at(t)) : 1.0; }
                    v.cut += 0.05 * (v.cutTarget - v.cut); v.wid += 0.05 * (v.widTarget - v.wid);
                    if (useCut) { if ((v.ctr & 15) == 1) v.f.setup(fs, v.cut, 0.707); a = v.f.run(0, a, Svf::LP); b = v.f.run(1, b, Svf::LP); }
                    if (useWid) { const float m = 0.5f * (a + b), s = 0.5f * (a - b) * (float) v.wid; a = m + s; b = m - s; }
                }
                const float k = v.env * v.vel * g * edge;
                outL[i] += a * k; outR[i] += b * k; v.pos += v.ratio;
            }
            ++v.age;
        }
        if (xf > 0.f) { xf = std::max(0.f, xf0 - xfStep * (float) n); if (xf <= 0.f) { old = nullptr; ended = true; } }
    }
private:
    struct Voice {
        bool active = false, releasing = false; int key = 0, slice = -1, age = 0, ctr = 0; float vel = 0, env = 0;
        double pos = 0, posOld = 0, ratio = 1, lo = 0, hi = 0, loOld = 0, hiOld = 0, cut = 20000, cutTarget = 20000, wid = 1, widTarget = 1; Svf f;
    };
    Voice* start(int key, float vel) {
        if (!cur || cur->length() < 4) return nullptr;
        Voice* v = nullptr; for (auto& x : voices) if (!x.active) { v = &x; break; }
        if (!v) { v = &voices[0]; for (auto& x : voices) if (x.age > v->age) v = &x; }       // steal the oldest
        *v = Voice(); v->active = true; v->key = key; v->vel = vel; v->pos = 0; v->env = 0; v->f.reset();
        return v;
    }
    std::array<Voice, kVoices> voices;
    const Rendered* cur = nullptr; const Rendered* old = nullptr; double fs = 44100; float xf = 0.f; bool ended = false; const LiveLanes* lanes = nullptr;
};
}
