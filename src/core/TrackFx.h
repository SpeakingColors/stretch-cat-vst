// Track effect core (pure C++, no JUCE): listens to the incoming audio in a ring buffer, HOLD freezes that moment and a grain
// scheduler repeats the held part at an adjustable speed. Everything is allocated in prepare(); process() never allocates or locks.
// Latency: exactly 0 samples (it only ever looks at the past). Dry passes bit exactly while not held.
#pragma once
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <atomic>

namespace stretchcat
{
enum class TrMode { Off, Regions, Detect };

struct TrackParams
{
    // Time
    double startMs = 500.0;      // where the held part begins, measured back from the moment HOLD was pressed (0 = now)
    double lengthMs = 400.0;     // length of the held part
    bool loop = true;            // repeat the held part; off = play it once, then return to the dry signal
    double keySemis = 0.0;       // Key to (Pitch): transposition of the grains in semitones (resampling, so speed of the grain changes too)
    double speed = 1.0;          // playhead speed through the held part, 0.25 .. 4 (grain pitch is not changed by it)
    // Grains
    double attackMs = 10.0;      // fade in and fade out time of every grain
    double holdMs = 60.0;        // time a grain stays at full level between its fades
    double spacingMs = 40.0;     // time between the starts of two grains
    double waveSpacingMs = 0.0;  // extra offset added to the read position of each next grain (smears or stacks copies of the wave)
    // Effects
    double pan = 0.0;            // -1 .. 1, centre of the grains
    double depth = 0.0;          // 0 .. 1, how far grains are spread left and right and varied in level
    double rand = 0.0;           // 0 .. 1, random timing and read-position jitter of the grains
    // Transients
    TrMode trMode = TrMode::Off;
    double holdAmount = 1.0;     // 0.05 .. 1, share of each transient region that is repeated (small = only the hit)
    bool operator==(const TrackParams& o) const {
        return startMs == o.startMs && lengthMs == o.lengthMs && loop == o.loop && keySemis == o.keySemis && speed == o.speed && attackMs == o.attackMs && holdMs == o.holdMs
            && spacingMs == o.spacingMs && waveSpacingMs == o.waveSpacingMs && pan == o.pan && depth == o.depth && rand == o.rand && trMode == o.trMode && holdAmount == o.holdAmount; }
};

class TrackFx
{
public:
    static constexpr int kMaxGrains = 48, kMaxMarks = 256;

    void prepare(double sampleRate, double ringSeconds = 10.0)
    {
        sr = sampleRate; cap = (size_t) (sr * ringSeconds); ringL.assign(cap, 0.f); ringR.assign(cap, 0.f); wr = 0; total = 0; held = false; xf = 0.f; xfTarget = 0.f;
        marks.assign(kMaxMarks, 0); nMarks = 0; fast = slow = 0.f; lastOnset = -1000000; for (auto& g : grains) g.on = false;
        heldMarks.assign(kMaxMarks, 0); nHeld = 0; rngState = 0x9e3779b9u; setSmoothing();
    }
    void setParams(const TrackParams& p) { prm = p; }
    const TrackParams& params() const { return prm; }
    int latencySamples() const { return 0; }
    bool isHeld() const { return held; }
    double playheadMs() const { return playhead / sr * 1000.0; }                 // inside the held part, for the waveform view
    double heldLengthMs() const { return (double) heldLen / sr * 1000.0; }
    size_t ringSize() const { return cap; }
    int regionCount() const { return nHeld; }
    long long regionStart(int i) const { return rel(heldMarks[(size_t) i]); }   // sample offset inside the held part, for the waveform view
    // Use regions: start points of the regions as fractions (0..1) of the held part, e.g. from the slice markers (max 16).
    void setRegions(const double* fractions, int n) { nUser = std::min(16, std::max(0, n)); for (int i = 0; i < nUser; ++i) userFr[i] = fractions[i]; if (held) applyUserRegions(); }
    float heldSample(long long i) const { return ringL[(size_t) wrapIdx(heldStart + i)]; }   // left channel, for the waveform view
    // Peak of the held part in nBins bins (no allocation: the caller owns out). Message thread only, while held.
    void heldPeaks(float* out, int nBins) const { for (int b = 0; b < nBins; ++b) { const long long a = heldLen * b / nBins, e = std::max(a + 1, heldLen * (b + 1) / nBins); float m = 0; for (long long i = a; i < e; i += std::max<long long>(1, (e - a) / 32)) m = std::max(m, std::fabs(heldSample(i))); out[b] = m; } }

    // HOLD pressed or released (audio thread, or posted to it).
    void setHold(bool on)
    {
        if (on == held) return;
        if (on)
        {
            heldLen = std::max<long long>(64, (long long) (prm.lengthMs * 0.001 * sr)); const long long back = (long long) (prm.startMs * 0.001 * sr);
            heldLen = std::min<long long>(heldLen, (long long) cap - 1); const long long avail = std::min<long long>((long long) cap - heldLen - 1, total);
            long long startBack = std::min<long long>(std::max<long long>(back, heldLen), avail > heldLen ? avail : heldLen);   // distance from "now" back to the start of the held part
            startBack = std::min<long long>(startBack, (long long) cap - 2);
            heldLen = std::min<long long>(heldLen, startBack);
            heldStart = (long long) wr - startBack; if (heldStart < 0) heldStart += (long long) cap;
            nHeld = 0; const long long startAbs = (long long) total - startBack;
            for (int i = 0; i < nMarks; ++i) { const long long m = marks[(size_t) i]; if (m >= startAbs && m < startAbs + heldLen) { heldMarks[(size_t) nHeld++] = heldStart + (m - startAbs); if (heldMarks[(size_t) nHeld - 1] >= (long long) cap) heldMarks[(size_t) nHeld - 1] -= (long long) cap; } }
            std::sort(heldMarks.begin(), heldMarks.begin() + nHeld, [&](long long a, long long b) { return rel(a) < rel(b); });
            if (prm.trMode == TrMode::Regions) applyUserRegions();
            playhead = 0; nextSpawn = 0; waveCount = 0; finished = false; for (auto& g : grains) g.on = false;
        }
        held = on; xfTarget = on ? 1.f : 0.f;
    }

    // In place, stereo. Not held: the ring keeps listening and the dry signal passes untouched.
    void process(float* L, float* R, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            const float dl = L[i], dr = R[i];
            if (!held) capture(dl, dr);
            if (xf < xfTarget) xf = std::min(xfTarget, xf + xfStep); else if (xf > xfTarget) xf = std::max(xfTarget, xf - xfStep);
            if (xf <= 0.f && !held) continue;                                    // bit exact dry
            float gl = 0.f, gr = 0.f; if (held || xf > 0.f) renderGrains(gl, gr);
            // equal power crossfade between dry and grains; when "loop" is off and the part has played, the wet side fades out by itself
            const float wet = std::sin(xf * 1.5707963f), dry = std::cos(xf * 1.5707963f);
            L[i] = dl * dry + gl * wet; R[i] = dr * dry + gr * wet;
        }
    }

private:
    struct Grain { bool on = false; double pos = 0, inc = 1; long long age = 0, len = 0, att = 0; float gL = 1.f, gR = 1.f; };
    double sr = 48000; size_t cap = 0; std::vector<float> ringL, ringR; size_t wr = 0; long long total = 0;
    bool held = false, finished = false; float xf = 0.f, xfTarget = 0.f, xfStep = 0.002f;
    long long heldStart = 0, heldLen = 0; double playhead = 0; double nextSpawn = 0; long long waveCount = 0;
    TrackParams prm; Grain grains[kMaxGrains];
    std::vector<long long> marks, heldMarks; int nMarks = 0, nHeld = 0; float fast = 0, slow = 0; long long lastOnset = -1000000;
    uint32_t rngState = 1; double userFr[16] {}; int nUser = 0;

    void applyUserRegions() { nHeld = 0; for (int i = 0; i < nUser; ++i) { const long long o = (long long) (userFr[i] * (double) heldLen); if (o > 0 && o < heldLen) heldMarks[(size_t) nHeld++] = wrapIdx(heldStart + o); } std::sort(heldMarks.begin(), heldMarks.begin() + nHeld, [&](long long a, long long b) { return rel(a) < rel(b); }); }
    void setSmoothing() { xfStep = (float) (1.0 / (0.010 * sr)); }   // 10 ms crossfade between dry and grains
    long long wrapIdx(long long i) const { const long long c = (long long) cap; i %= c; return i < 0 ? i + c : i; }
    long long rel(long long a) const { long long r = a - heldStart; if (r < 0) r += (long long) cap; return r; }
    float rnd() { rngState = rngState * 1664525u + 1013904223u; return (float) (rngState >> 8) * (1.f / 16777216.f); }   // 0 .. 1
    void capture(float l, float r)
    {
        ringL[wr] = l; ringR[wr] = r; wr = wr + 1 == cap ? 0 : wr + 1; ++total;
        const float a = std::fabs(0.5f * (l + r)); fast += (a - fast) * 0.02f; slow += (a - slow) * 0.0007f;       // onset detector for Detect mode
        if (fast > 0.02f && fast > slow * 2.4f && total - lastOnset > (long long) (0.06 * sr)) { lastOnset = total; if (nMarks == kMaxMarks) { std::copy(marks.begin() + 1, marks.end(), marks.begin()); --nMarks; } marks[(size_t) nMarks++] = total; }
    }
    // Region the playhead is currently in: [a, b) inside the held part. Off = the whole held part.
    void region(double ph, double& a, double& b) const
    {
        a = 0; b = (double) heldLen;
        if (prm.trMode == TrMode::Off || nHeld == 0) return;
        double s = 0; for (int i = 0; i < nHeld; ++i) { const double m = (double) rel(heldMarks[(size_t) i]); if (m <= ph) s = m; else { b = m; break; } }
        a = s;                                                                    // regions start at the held part's start and at each transient
    }
    void spawn()
    {
        Grain* g = nullptr; for (auto& x : grains) if (!x.on) { g = &x; break; } if (!g) { double oldest = -1; for (auto& x : grains) if ((double) x.age > oldest) { oldest = (double) x.age; g = &x; } }
        double a, b; region(playhead, a, b); const double regLen = std::max(64.0, b - a);
        const double r = prm.rand;
        // Hold amount (only with Use regions / Detect): the read position scans just the first part of each region, so a small amount repeats the hit itself
        const double amt = prm.trMode == TrMode::Off ? 1.0 : std::min(1.0, std::max(0.05, prm.holdAmount));
        double start = a + (playhead - a) * amt + (double) waveCount * prm.waveSpacingMs * 0.001 * sr + (rnd() - 0.5) * 2.0 * r * 0.02 * sr;   // up to +-20 ms of jitter
        start = a + std::fmod(std::fmod(start - a, regLen) + regLen, regLen);
        g->att = std::max<long long>(1, (long long) (prm.attackMs * 0.001 * sr)); g->len = 2 * g->att + std::max<long long>(1, (long long) (prm.holdMs * 0.001 * sr));
        g->pos = start; g->inc = std::pow(2.0, prm.keySemis / 12.0); g->age = 0; g->on = true; ++waveCount;
        const float sp = (float) prm.depth * (rnd() * 2.f - 1.f); const float p = std::min(1.f, std::max(-1.f, (float) prm.pan + sp));
        const float lvl = 1.f - 0.5f * (float) prm.depth * rnd(); g->gL = lvl * std::cos((p * 0.5f + 0.5f) * 1.5707963f) * 1.4142135f; g->gR = lvl * std::sin((p * 0.5f + 0.5f) * 1.5707963f) * 1.4142135f;
    }
    void renderGrains(float& oL, float& oR)
    {
        const double spacing = std::max(1.0, prm.spacingMs * 0.001 * sr);
        if (!finished)
        {
            nextSpawn -= 1.0;
            if (nextSpawn <= 0.0) { spawn(); nextSpawn += spacing * (1.0 + (rnd() - 0.5) * prm.rand * 0.5); }
            playhead += prm.speed;
            if (playhead >= (double) heldLen) { if (prm.loop) playhead = std::fmod(playhead, (double) heldLen); else { playhead = (double) heldLen; finished = true; } }
        }
        const double overlap = std::max(1.0, (double) (2 * (long long) (prm.attackMs * 0.001 * sr) + (long long) (prm.holdMs * 0.001 * sr)) / spacing); const float norm = (float) (1.0 / std::sqrt(overlap));
        double a, b; region(std::min(playhead, (double) heldLen - 1.0), a, b);
        float sl = 0.f, sr_ = 0.f; bool any = false;
        for (auto& g : grains)
        {
            if (!g.on) continue;
            any = true;
            const double len = (double) heldLen; double p = g.pos; p = std::fmod(p, len); if (p < 0) p += len;
            const long long i0 = (long long) p; const float f = (float) (p - (double) i0); const long long i1 = i0 + 1 >= heldLen ? 0 : i0 + 1;
            const long long im1 = i0 == 0 ? heldLen - 1 : i0 - 1, i2 = i1 + 1 >= heldLen ? 0 : i1 + 1;
            auto rd = [&](const std::vector<float>& v, long long k) { return v[(size_t) wrapIdx(heldStart + k)]; };
            auto cubic = [&](const std::vector<float>& v) { const float y0 = rd(v, im1), y1 = rd(v, i0), y2 = rd(v, i1), y3 = rd(v, i2); return y1 + 0.5f * f * (y2 - y0 + f * (2.f * y0 - 5.f * y1 + 4.f * y2 - y3 + f * (3.f * (y1 - y2) + y3 - y0))); };
            float env; if (g.age < g.att) env = 0.5f - 0.5f * std::cos(3.14159265f * (float) g.age / (float) g.att); else if (g.age >= g.len - g.att) env = 0.5f - 0.5f * std::cos(3.14159265f * (float) (g.len - g.age) / (float) g.att); else env = 1.f;
            sl += cubic(ringL) * env * g.gL; sr_ += cubic(ringR) * env * g.gR;
            g.pos += g.inc; ++g.age; if (g.age >= g.len) g.on = false;
        }
        // fade the wet side out once a one-shot hold has finished and the grains died (back to dry, no click)
        if (finished && !any && xfTarget > 0.f) { xfTarget = 0.f; held = false; }
        oL = sl * norm; oR = sr_ * norm;
    }
};
}
