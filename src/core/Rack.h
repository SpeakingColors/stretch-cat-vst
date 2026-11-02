// Stretch Cat: the FX rack (pure C++): filter, EQ, compressor on the sampler output, plus the live lanes (cutoff, width).
// Every parameter goes through a short linear ramp (10 ms) so a knob move is heard in the same block and never zippers.
// Each section has an On switch that crossfades (10 ms) between the dry and processed signal; off and settled costs nothing.
#pragma once
#include "StretchEngine.h"

namespace stretchcat {

// Linear ramp to a target over `ms`; the value moves from the first sample after the target changes.
class Ramp {
public:
    void prepare(double sampleRate, double ms) { steps = std::max(1.0, ms * 0.001 * sampleRate); }
    void snap(double v) { cur = tgt = v; inc = 0; }
    void set(double v) { if (v == tgt) return; tgt = v; inc = (tgt - cur) / steps; }
    double next() { if (inc == 0) return cur; cur += inc; if ((inc > 0 && cur >= tgt) || (inc < 0 && cur <= tgt)) { cur = tgt; inc = 0; } return cur; }
    bool moving() const { return inc != 0; }
    double target() const { return tgt; }
    double value() const { return cur; }
private:
    double cur = 0, tgt = 0, inc = 0, steps = 480;
};

// 12 dB per octave state variable filter (TPT), stereo.
class Svf {
public:
    enum Type { LP, HP, BP, Notch };
    void reset() { for (auto& c : ic) c[0] = c[1] = 0.0; }
    void setup(double fs, double hz, double q) { hz = std::max(10.0, std::min(0.45 * fs, hz)); const double g = std::tan(kPi * hz / fs), k = 1.0 / std::max(0.2, q); a1 = 1.0 / (1.0 + g * (g + k)); a2 = g * a1; a3 = g * a2; kk = k; }
    float run(int ch, float x, Type t) {
        auto& s = ic[ch]; const double v3 = (double) x - s[1], v1 = a1 * s[0] + a2 * v3, v2 = s[1] + a2 * s[0] + a3 * v3;
        s[0] = 2 * v1 - s[0]; s[1] = 2 * v2 - s[1];
        if (!std::isfinite(s[0]) || !std::isfinite(s[1])) { s[0] = s[1] = 0; return 0.f; }
        switch (t) { case LP: return (float) v2; case BP: return (float) v1; case HP: return (float) ((double) x - kk * v1 - v2); default: return (float) ((double) x - kk * v1); }
    }
private:
    double ic[2][2] = {}, a1 = 0, a2 = 0, a3 = 0, kk = 1.4;
};

struct Biquad {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z[2][2] = {};
    void reset() { for (auto& c : z) c[0] = c[1] = 0; }
    float run(int ch, float x) { const double y = b0 * x + z[ch][0]; z[ch][0] = b1 * x - a1 * y + z[ch][1]; z[ch][1] = b2 * x - a2 * y; if (!std::isfinite(y)) { reset(); return 0.f; } return (float) y; }
    // RBJ cookbook
    void lowShelf(double fs, double f, double db) { const double A = std::pow(10.0, db / 40), w = 2 * kPi * f / fs, c = std::cos(w), s = std::sin(w), al = s / 2 * std::sqrt(2.0), sq = 2 * std::sqrt(A) * al;
        set((A * ((A + 1) - (A - 1) * c + sq)), (2 * A * ((A - 1) - (A + 1) * c)), (A * ((A + 1) - (A - 1) * c - sq)), ((A + 1) + (A - 1) * c + sq), (-2 * ((A - 1) + (A + 1) * c)), ((A + 1) + (A - 1) * c - sq)); }
    void highShelf(double fs, double f, double db) { const double A = std::pow(10.0, db / 40), w = 2 * kPi * f / fs, c = std::cos(w), s = std::sin(w), al = s / 2 * std::sqrt(2.0), sq = 2 * std::sqrt(A) * al;
        set((A * ((A + 1) + (A - 1) * c + sq)), (-2 * A * ((A - 1) + (A + 1) * c)), (A * ((A + 1) + (A - 1) * c - sq)), ((A + 1) - (A - 1) * c + sq), (2 * ((A - 1) - (A + 1) * c)), ((A + 1) - (A - 1) * c - sq)); }
    void peak(double fs, double f, double db, double q) { const double A = std::pow(10.0, db / 40), w = 2 * kPi * f / fs, c = std::cos(w), al = std::sin(w) / (2 * q);
        set(1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A); }
private:
    void set(double B0, double B1, double B2, double A0, double A1, double A2) { b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0; }
};

struct RackParams {
    bool filterOn = true; int filterType = 0; double cutoff = 16000, reso = 0.7;
    bool eqOn = true; double eqLow = 0, eqMid = 0, eqHigh = 0;
    bool compOn = false; double thresh = -18, ratio = 3, attackMs = 10, releaseMs = 120;
};

class Rack {
public:
    static constexpr int kBlock = 8;               // coefficients are refreshed every 8 samples while a ramp moves
    void prepare(double sampleRate) {
        fs = sampleRate; for (Ramp* r : { &cut, &q, &lo, &mi, &hi, &fMix, &eMix, &cMix, &thr, &rat }) r->prepare(fs, 10.0);
        cut.snap(16000); q.snap(0.7); lo.snap(0); mi.snap(0); hi.snap(0); fMix.snap(1); eMix.snap(1); cMix.snap(0); thr.snap(-18); rat.snap(3);
        svf.reset(); for (auto& b : eq) b.reset(); grDb = 0; gainDb = 0; first = true;
    }
    void setParams(const RackParams& p) {
        P = p; if (first) { cut.snap(p.cutoff); q.snap(p.reso); lo.snap(p.eqLow); mi.snap(p.eqMid); hi.snap(p.eqHigh); fMix.snap(p.filterOn ? 1 : 0); eMix.snap(p.eqOn ? 1 : 0); cMix.snap(p.compOn ? 1 : 0); thr.snap(p.thresh); rat.snap(p.ratio); first = false; }
        cut.set(p.cutoff); q.set(p.reso); lo.set(p.eqLow); mi.set(p.eqMid); hi.set(p.eqHigh); fMix.set(p.filterOn ? 1 : 0); eMix.set(p.eqOn ? 1 : 0); cMix.set(p.compOn ? 1 : 0); thr.set(p.thresh); rat.set(p.ratio);
        const double a = std::max(0.1, p.attackMs) * 0.001 * fs, r = std::max(1.0, p.releaseMs) * 0.001 * fs; aCoef = std::exp(-1.0 / a); rCoef = std::exp(-1.0 / r);
    }
    void process(float* L, float* R, int n) {
        const bool fAct = fMix.value() > 0 || fMix.target() > 0, eAct = eMix.value() > 0 || eMix.target() > 0, cAct = cMix.value() > 0 || cMix.target() > 0;
        if (!fAct) svf.reset();
        if (!eAct) for (auto& b : eq) b.reset();
        if (!cAct) grDb = 0;
        for (int i = 0; i < n; ++i) {
            if (fAct) { const double fm = fMix.next(), c = cut.next(), qq = q.next(); if ((i % kBlock) == 0) svf.setup(fs, c, qq);
                if (fm > 0) { const Svf::Type t = (Svf::Type) P.filterType; const float l = svf.run(0, L[i], t), r = svf.run(1, R[i], t); L[i] += (float) fm * (l - L[i]); R[i] += (float) fm * (r - R[i]); } }
            if (eAct) { const double em = eMix.next(); const double gl = lo.next(), gm = mi.next(), gh = hi.next();
                if ((i % kBlock) == 0) { eq[0].lowShelf(fs, 200, gl); eq[1].peak(fs, 1000, gm, 0.8); eq[2].highShelf(fs, 5000, gh); }
                float l = L[i], r = R[i]; for (auto& b : eq) { l = b.run(0, l); r = b.run(1, r); } L[i] += (float) em * (l - L[i]); R[i] += (float) em * (r - R[i]); }
            if (cAct) { const double cm = cMix.next(), th = thr.next(), ra = rat.next();
                const double lvl = std::max(std::fabs((double) L[i]), std::fabs((double) R[i])), db = 20 * std::log10(std::max(1e-6, lvl));
                const double over = db - th, knee = 6.0; double gr;                                     // gain reduction in dB (<= 0), soft knee
                if (over <= -knee / 2) gr = 0; else if (over >= knee / 2) gr = -over * (1 - 1 / ra); else { const double x = over + knee / 2; gr = -(1 - 1 / ra) * x * x / (2 * knee); }
                const double c = gr < grDb ? aCoef : rCoef; grDb = c * grDb + (1 - c) * gr;
                const double makeup = std::min(12.0, 0.5 * (-th) * (1 - 1 / ra));                      // half of the full make-up so loud material does not jump
                const float g = (float) std::pow(10.0, (grDb + makeup) / 20.0); L[i] += (float) cm * (L[i] * g - L[i]); R[i] += (float) cm * (R[i] * g - R[i]); }
        }
    }
    double gainReductionDb() const { return grDb; }
private:
    double fs = 44100, grDb = 0, gainDb = 0, aCoef = 0.99, rCoef = 0.999; bool first = true; RackParams P;
    Ramp cut, q, lo, mi, hi, fMix, eMix, cMix, thr, rat; Svf svf; Biquad eq[3];
};

// A lane copied into a small table the audio thread can read without locks (message thread writes, audio thread reads).
struct LaneTable {
    static constexpr int N = 128;
    std::atomic<float> v[N]; std::atomic<bool> active { false };      // active: the lane is not flat
    double neutral = 0.5;
    explicit LaneTable(double flat = 0.5) : neutral(flat) { for (auto& x : v) x.store((float) flat, std::memory_order_relaxed); }
    void set(const Lane& l) { bool act = false; for (int i = 0; i < N; ++i) { const float y = (float) l.at((double) i / (N - 1)); v[i].store(y, std::memory_order_relaxed); if (std::fabs(y - neutral) > 0.01) act = true; } active.store(act, std::memory_order_relaxed); }
    float at(double t) const { t = std::max(0.0, std::min(1.0, t)) * (N - 1); const int i = (int) t, j = std::min(N - 1, i + 1); const float f = (float) (t - i); return v[i].load(std::memory_order_relaxed) * (1 - f) + v[j].load(std::memory_order_relaxed) * f; }
};
struct LiveLanes {
    LaneTable cutoff { 1.0 }, width { 0.5 };
    static double cutoffHz(double y) { return 100.0 * std::pow(160.0, y); }       // y 1 -> 16 kHz (open), as in the mockup
    static double widthOf(double y) { return 2.0 * y; }                           // 0..200 percent, 0.5 -> 100
};
}
