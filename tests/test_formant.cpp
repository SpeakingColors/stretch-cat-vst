// Tests for the formant shift (src/core/Formant.h). Pure C++, no JUCE.
// A synthetic "vowel" (harmonics under a known formant envelope) is shifted and the harmonic amplitudes are compared with the
// envelope moved by the requested number of semitones, while the harmonic frequencies (the pitch) must not move.
#include "../src/core/Formant.h"
#include <cstdio>
#include <random>
using namespace stretchcat;

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { ++fails; std::printf("FAIL %s:%d  ", __FILE__, __LINE__); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)
static const double sr = 48000, P2 = 6.283185307179586;

// Known envelope: three two-pole resonances (the classic formant model) and a gentle tilt. bwScale widens them for high pitches.
struct Env { double F[3] = { 650, 1250, 2700 }, B[3] = { 90, 110, 180 }; double bw = 1.0;
    double at(double f) const { double a = 1; for (int i = 0; i < 3; ++i) { const double x = (f * f - F[i] * F[i]) / (f * B[i] * bw); a *= 1.0 / std::sqrt(1.0 + x * x); } return a * std::pow(std::max(f, 50.0) / 500.0, -0.5); } };

static std::vector<float> vowel(double f0, const Env& e, double secs, double level = 0.1) {
    std::vector<float> v((size_t) (sr * secs), 0.f);
    for (int k = 1; k * f0 < 0.45 * sr; ++k) { const double a = e.at(k * f0), ph = 1.3 * k * k; for (size_t i = 0; i < v.size(); ++i) v[i] += (float) (level * a * std::sin(P2 * k * f0 * (double) i / sr + ph)); }
    return v;
}
// amplitude at one frequency, Hann window over [a,b)
static double amp(const std::vector<float>& v, size_t a, size_t b, double f) {
    const double N = (double) (b - a); double c = 0, s = 0, w0 = 0;
    for (size_t i = a; i < b; ++i) { const double w = 0.5 - 0.5 * std::cos(P2 * (double) (i - a) / N); c += w * v[i] * std::cos(P2 * f * (double) i / sr); s += w * v[i] * std::sin(P2 * f * (double) i / sr); w0 += w; }
    return 2.0 * std::sqrt(c * c + s * s) / w0;
}
struct Res { double rmsDb, pitchShare, maxDb; int n; };
// shift the vowel by st semitones and compare the harmonic amplitudes with the envelope moved by st
static Res vowelTest(double f0, double st, double bw) {
    Env e; e.bw = bw; auto in = vowel(f0, e, 1.6); Stereo s; s.L = in; s.R = in; applyFormantShift(s, sr, [st](double) { return st; });
    const double ratio = std::pow(2.0, st / 12.0); const size_t a = (size_t) (0.45 * sr), b = (size_t) (1.15 * sr);
    double top = 0; for (int k = 1; k * f0 < 6000; ++k) top = std::max(top, e.at(k * f0));
    std::vector<double> d;
    for (int k = 1; k * f0 < 6000; ++k) {
        const double want = e.at(k * f0 / ratio), got = amp(s.L, a, b, k * f0), wi = e.at(k * f0);
        if (want < top * 0.03 || wi < top * 0.03) continue;            // harmonics 30 dB under the strongest are not judged
        d.push_back(20 * std::log10((got + 1e-12) / want));
    }
    // pitch share: the energy at the harmonics of f0 against all the energy (what is left over is inharmonic: a moved pitch or smear)
    double hs = 0; for (int k = 1; k * f0 < 0.45 * sr; ++k) { const double q = amp(s.L, a, b, k * f0); hs += 0.5 * q * q; }
    double all = 0; for (size_t i = a; i < b; ++i) all += (double) s.L[i] * s.L[i]; all /= (double) (b - a);
    std::sort(d.begin(), d.end()); const double med = d.empty() ? 0 : d[d.size() / 2]; double ss = 0, mx = 0; for (double x : d) { ss += (x - med) * (x - med); mx = std::max(mx, std::fabs(x - med)); }
    return { d.empty() ? 99 : std::sqrt(ss / (double) d.size()), std::min(1.0, hs / std::max(all, 1e-18)), mx, (int) d.size() };
}

static void testVowels() {
    // tolerances: RMS error of the harmonic amplitudes against the moved envelope, after removing the overall gain
    // The formants are widened with the pitch (bandwidth scale = f0 / 90 Hz): a formant narrower than the harmonic spacing lies between the
    // harmonics, is not in the signal, and no estimator could move it.
    struct C { double f0, bw, tol; } cases[] = { { 100, 1.1, 2.0 }, { 150, 1.7, 2.0 }, { 220, 2.4, 2.0 }, { 330, 3.7, 3.5 }, { 440, 4.9, 3.5 }, { 600, 6.7, 5.5 }, { 800, 8.9, 5.5 }, { 1000, 11, 5.0 } };   // limits = what is measured plus a margin; the old fixed lifter gave 10 to 30 dB above 600 Hz
    for (auto c : cases) for (double st : { -5.0, 3.0, 7.0 }) {
        const auto r = vowelTest(c.f0, st, c.bw);
        std::printf("vowel f0 %4.0f Hz  shift %+.0f st: envelope error %.2f dB rms (max %.1f dB, %d harmonics), pitch share %.3f\n", c.f0, st, r.rmsDb, r.maxDb, r.n, r.pitchShare);
        CHECK(r.n >= 3, "f0 %.0f: enough harmonics judged (%d)", c.f0, r.n);
        CHECK(r.rmsDb < c.tol, "f0 %.0f Hz shift %+.0f st: envelope moves by the requested amount (%.2f dB rms, limit %.1f)", c.f0, st, r.rmsDb, c.tol);
        CHECK(r.pitchShare > 0.995, "f0 %.0f Hz shift %+.0f st: the harmonics stay where they were (pitch share %.3f)", c.f0, st, r.pitchShare);
    }
}

static void testNoiseAndTransients() {
    std::mt19937 g(3); std::normal_distribution<float> nd(0.f, 0.2f); Stereo n; n.L.resize((size_t) (sr * 3)); for (auto& x : n.L) x = nd(g); n.R = n.L;
    const auto in = n.L; applyFormantShift(n, sr, [](double) { return 6.0; });
    auto rmsOf = [](const std::vector<float>& v, size_t a, size_t b) { double s = 0; for (size_t i = a; i < b; ++i) s += (double) v[i] * v[i]; return std::sqrt(s / (double) (b - a)); };
    const double r0 = rmsOf(in, 20000, 120000), r1 = rmsOf(n.L, 20000, 120000); const double lvl = 20 * std::log10(r1 / r0);
    // band balance of white noise stays flat: compare the level in 4 bands
    double bands[4] = {}; const double edges[5] = { 300, 1200, 3000, 6000, 12000 };
    for (int b = 0; b < 4; ++b) { double e = 0; int c = 0; for (double f = edges[b]; f < edges[b + 1]; f += 23.0, ++c) { const double q = amp(n.L, 20000, 120000, f); e += q * q; } bands[b] = 10 * std::log10(e / c); }
    double lo = 1e9, hi = -1e9; for (double x : bands) { lo = std::min(lo, x); hi = std::max(hi, x); }
    std::printf("white noise +6 st: level %+.2f dB, band spread %.2f dB\n", lvl, hi - lo);
    CHECK(std::fabs(lvl) < 2.0, "noise level stays within 2 dB (%.2f)", lvl);
    CHECK(hi - lo < 3.0, "white noise stays white, band levels within 3 dB (%.2f)", hi - lo);
    bool fin = true; for (float x : n.L) if (!std::isfinite(x)) fin = false; CHECK(fin, "noise output finite");
    // transients: a click train must not smear in time (energy must stay near the clicks) or ring
    Stereo c; c.L.assign((size_t) (sr * 2), 0.f); for (size_t i = 24000; i < c.L.size(); i += 12000) c.L[i] = 0.8f; c.R = c.L; applyFormantShift(c, sr, [](double) { return 5.0; });
    double nearE = 0, allE = 0; for (size_t i = 0; i < c.L.size(); ++i) { const double e = (double) c.L[i] * c.L[i]; allE += e; const size_t m = i % 12000; if (m < 700 || m > 12000 - 700) nearE += e; }
    double pk = 0; for (float x : c.L) pk = std::max(pk, (double) std::fabs(x));
    std::printf("click train +5 st: %.1f percent of the energy within 15 ms of a click, peak %.2f\n", 100 * nearE / allE, pk);
    CHECK(nearE / allE > 0.9, "transients stay in time (%.3f of the energy near the clicks)", nearE / allE);
    CHECK(pk < 1.6, "clicks do not grow into big peaks (%.2f)", pk);
    // silence and DC stay silent
    Stereo z; z.L.assign(20000, 0.f); z.R = z.L; applyFormantShift(z, sr, [](double) { return 7.0; }); bool zero = true; for (float x : z.L) if (!(std::fabs(x) < 1e-6f)) zero = false; CHECK(zero, "silence stays silent");
}

static void testTimeVarying() {
    // a formant shift that glides while a vowel sounds must not click: compare the largest sample step with the unshifted vowel
    auto in = vowel(180, [] { Env e; e.bw = 2.0; return e; }(), 2.5); Stereo s; s.L = in; s.R = in;
    applyFormantShift(s, sr, [](double f) { return -8.0 + 16.0 * f; });
    double m0 = 0, m1 = 0; for (size_t i = 5000; i + 1 < in.size() - 5000; ++i) { m0 = std::max(m0, (double) std::fabs(in[i + 1] - in[i])); m1 = std::max(m1, (double) std::fabs(s.L[i + 1] - s.L[i])); }
    std::printf("gliding shift -8..+8 st: largest sample step %.4f (unshifted vowel %.4f)\n", m1, m0);
    CHECK(m1 < 2.2 * m0, "a gliding shift makes no clicks (%.4f vs %.4f)", m1, m0);
}

static void testBasics() {
    auto in = vowel(220, Env(), 1.0); Stereo s; s.L = in; s.R = in; applyFormantShift(s, sr, [](double) { return 0.0; }); CHECK(s.L == in, "0 st is bit exact");
    Stereo t; t.L = in; t.R = in; std::atomic<bool> c { true }; CHECK(!applyFormantShift(t, sr, [](double) { return 4.0; }, &c) && t.L == in, "a cancelled pass leaves the audio alone");
    // limits: +-12 st stays finite and bounded
    for (double st : { -12.0, 12.0 }) { Stereo u; u.L = in; u.R = in; applyFormantShift(u, sr, [st](double) { return st; }); double pk = 0; bool fin = true; for (float x : u.L) { pk = std::max(pk, (double) std::fabs(x)); if (!std::isfinite(x)) fin = false; } CHECK(fin && pk < 1.0, "%+.0f st is finite and bounded (peak %.2f)", st, pk); }
    CHECK(formantLatencySamples() == 0, "the formant pass is applied to the offline render, it adds no latency");
}

int main() {
    testBasics(); testVowels(); testNoiseAndTransients(); testTimeVarying();
    if (fails) { std::printf("%d FAILED\n", fails); return 1; }
    std::printf("formant: all tests passed\n"); return 0;
}
