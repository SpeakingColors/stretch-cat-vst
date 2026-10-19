// Stretch Cat: the time stretch and pitch engine (pure C++, no JUCE). Port of the mockup's engine.
// WSOLA style: windowed grains are laid on the output at a fixed hop; each grain start is nudged (within +-W samples)
// to the position that best matches the tail of the previous grain, so the waves line up and the output does not comb or flutter.
// Speed and pitch are independent (except in Re-Pitch / tape mode), and both can change over time through envelopes.
#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <functional>

namespace stretchcat {

constexpr double kPi = 3.14159265358979323846;

// Mode names as in the mockup. G = grain length, W = alignment search, jit = random start jitter (texture).
enum class Mode { Beats, Tones, Texture, RePitch, Complex, FormantPlus };
struct ModeInfo { int G; int W; double jit; const char* name; };
inline const ModeInfo& modeInfo(Mode m) {
    static const ModeInfo t[] = { { 1024, 128, 0.0, "Beats" }, { 4096, 512, 0.0, "Tones" }, { 2048, 0, 0.45, "Texture" },
                                  { 0, 0, 0.0, "Re-Pitch" }, { 2048, 256, 0.0, "Complex" }, { 3072, 384, 0.0, "Formant+" } };
    return t[(int) m];
}

// A lane: piecewise linear envelope over the selection, points (t 0..1, value 0..1) sorted by t.
struct Lane {
    std::vector<std::pair<double, double>> pts { { 0.0, 0.5 }, { 1.0, 0.5 } };
    double at(double t) const {
        if (pts.empty()) return 0.5;
        if (t <= pts.front().first) return pts.front().second;
        for (size_t i = 1; i < pts.size(); ++i) if (t <= pts[i].first) {
            const auto &a = pts[i - 1], &b = pts[i]; const double d = b.first - a.first; return d <= 0 ? b.second : a.second + (b.second - a.second) * (t - a.first) / d;
        }
        return pts.back().second;
    }
    bool isFlat(double v, double tol = 0.01) const { for (auto& p : pts) if (std::fabs(p.second - v) > tol) return false; return true; }
};

struct Params {
    Mode mode = Mode::Complex;
    bool tapeLink = false;           // speed and pitch are one thing (like a tape or a record)
    double grain = 0.5, transients = 0.5, smooth = 0.7;        // 0..1 as on the knobs
    double speed = 1.0;              // overall, 0.25..4
    double semitones = 0.0;          // transpose + fine already added together
    Lane speedLane, pitchLane;       // lane value 0..1 -> speed 0.25..4 (log) and pitch -12..+12 st; 0.5 is neutral
    static double laneSpeed(double y) { return 0.25 * std::pow(16.0, y); }          // y 0.5 -> 1.0
    static double lanePitch(double y) { return -12.0 + 24.0 * y; }
};

struct Stereo { std::vector<float> L, R; };

inline float hermite(const std::vector<float>& a, double p) {
    const long long n = (long long) a.size(), i = (long long) std::floor(p); const float f = (float)(p - (double) i);
    auto g = [&](long long k) { return (k < 0 || k >= n) ? 0.f : a[(size_t) k]; };
    const float y0 = g(i - 1), y1 = g(i), y2 = g(i + 1), y3 = g(i + 2);
    const float c1 = 0.5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.f * y2 - 0.5f * y3, c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    return ((c3 * f + c2) * f + c1) * f + y1;
}

// Renders source frames [a, b) to a new buffer. The source may be mono (R empty) or stereo.
inline Stereo render(const std::vector<float>& srcL, const std::vector<float>& srcR, long long a, long long b, const Params& p) {
    Stereo out; const long long n = (long long) srcL.size();
    a = std::max(0LL, a); b = std::min(n, b); if (b - a < 16) return out;
    const std::vector<float>& R = srcR.empty() ? srcL : srcR;
    std::vector<float> mono((size_t) n);
    for (long long i = 0; i < n; ++i) mono[(size_t) i] = 0.5f * (srcL[(size_t) i] + R[(size_t) i]);
    const ModeInfo& md = modeInfo(p.mode);
    auto speedAt = [&](double t) { return std::max(0.05, p.speed * Params::laneSpeed(p.speedLane.at(t))); };
    auto pitchAt = [&](double t) { return p.semitones + Params::lanePitch(p.pitchLane.at(t)); };

    if (md.G == 0 || p.tapeLink) {            // tape: one read head, speed and pitch are one thing
        const long long cap = 60LL * 48000;
        double pos = (double) a; long long k = 0;
        while (pos < (double) b && k < cap) {
            const double t = (pos - (double) a) / (double) std::max(1LL, b - a);
            const double rate = speedAt(t) * std::pow(2.0, pitchAt(t) / 12.0); if (rate <= 0.001) break;
            out.L.push_back(hermite(srcL, pos)); out.R.push_back(hermite(R, pos)); pos += rate; ++k;
        }
        return out;
    }
    const int Gs = std::max(256, (int) std::lround(md.G * (0.5 + p.grain) / 2.0) * 2), H = Gs / 2;
    const int W = (int) std::lround(md.W * (0.3 + 1.4 * p.transients));
    std::vector<float> win((size_t) Gs);
    for (int i = 0; i < Gs; ++i) { const double x = (double) i / (Gs - 1); const double e = std::min(1.0, std::min(x, 1.0 - x) / (0.5 * (0.15 + 0.85 * p.smooth))); win[(size_t) i] = (float)(0.5 - 0.5 * std::cos(kPi * e)); }
    // output length estimate at the slowest speed in the lane, capped at 120 s
    const size_t est = (size_t) std::min<double>((double)(b - a) / 0.05 + Gs * 2, 120.0 * 48000.0);
    std::vector<float> oL(est, 0.f), oR(est, 0.f), ws(est, 0.f);
    uint32_t rnd = 12345; auto rd = [&rnd] { rnd = rnd * 1664525u + 1013904223u; return (double) rnd / 4294967296.0; };
    size_t outPos = 0; double pos = (double) a, prev = (double) a - H; bool first = true; int guard = 0;
    while (pos < (double) b && guard++ < 400000) {
        const double t = (pos - (double) a) / (double) std::max(1LL, b - a);
        const double r = std::pow(2.0, pitchAt(t) / 12.0), sp = speedAt(t), len = Gs * r;
        double cand = pos - len / 2.0 + (md.jit > 0 ? (rd() - 0.5) * Gs * md.jit : 0.0);
        if (!first && W > 0) {
            const double tgt = prev + H * r; const int ov = std::min((int) std::lround(H * r), 2048);
            double best = -1e30; int bd = 0;
            for (int d = -W; d <= W; d += 4) {
                const double c0 = cand + d; double s1 = 0, s2 = 0, s3 = 0;
                for (int j = 0; j < ov; j += 6) {
                    const long long ia = (long long) std::llround(c0 + j), ib = (long long) std::llround(tgt + j);
                    if (ia < 0 || ib < 0 || ia >= n || ib >= n) continue;
                    const double va = mono[(size_t) ia], vb = mono[(size_t) ib]; s1 += va * vb; s2 += va * va; s3 += vb * vb;
                }
                const double sc = s1 / std::sqrt(s2 * s3 + 1e-9); if (sc > best) { best = sc; bd = d; }
            }
            cand += bd;
        }
        if (cand < 0) cand = 0;
        prev = cand; first = false;
        if (outPos + (size_t) Gs >= est) break;
        for (int i = 0; i < Gs; ++i) {
            const double sp2 = cand + (double) i * r; if (sp2 >= (double) n - 2) break;
            const float w = win[(size_t) i]; oL[outPos + (size_t) i] += hermite(srcL, sp2) * w; oR[outPos + (size_t) i] += hermite(R, sp2) * w; ws[outPos + (size_t) i] += w;
        }
        outPos += (size_t) H; pos += sp * H;
    }
    const size_t tot = std::min(est, outPos + (size_t) H);
    for (size_t i = 0; i < tot; ++i) if (ws[i] > 0.0001f) { oL[i] /= ws[i]; oR[i] /= ws[i]; }
    out.L.assign(oL.begin(), oL.begin() + (long) tot); out.R.assign(oR.begin(), oR.begin() + (long) tot);
    return out;
}

// Slice marks on the strongest hits, as fractions of the buffer (port of the mockup's "By transient").
inline std::vector<double> transientSlices(const std::vector<float>& L, long long a, long long b, double sr, int maxSlices = 7) {
    const int w = 256; const long long cnt = (b - a) / w; std::vector<float> env((size_t) std::max(0LL, cnt));
    for (long long k = 0; k < cnt; ++k) { double s = 0; for (int i = 0; i < w; ++i) { const float v = L[(size_t)(a + k * w + i)]; s += (double) v * v; } env[(size_t) k] = (float) std::sqrt(s / w); }
    std::vector<std::pair<float, long long>> fl;
    for (long long k = 2; k < cnt; ++k) { const float d = env[(size_t) k] - env[(size_t)(k - 2)]; if (d > 0.04f && env[(size_t) k] > 0.05f) fl.push_back({ d, k }); }
    std::sort(fl.begin(), fl.end(), [](auto& x, auto& y) { return x.first > y.first; });
    std::vector<long long> picks;
    for (auto& f : fl) {
        if ((int) picks.size() >= maxSlices) break;
        bool ok = (double) (f.second * w) > sr * 0.05; for (auto q : picks) if ((double) std::llabs(q - f.second) * w <= sr * 0.09) ok = false;
        if (ok) picks.push_back(f.second);
    }
    std::sort(picks.begin(), picks.end());
    std::vector<double> r; for (auto q : picks) r.push_back((double)(a + q * w) / (double) L.size());
    return r;
}
}
