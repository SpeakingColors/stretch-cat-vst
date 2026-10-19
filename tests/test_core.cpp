// Core tests for Stretch Cat. Pure C++, no JUCE.
#include "../src/core/StretchSampler.h"
#include <cstdio>
#include <random>
using namespace stretchcat;

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { ++fails; std::printf("FAIL %s:%d  ", __FILE__, __LINE__); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)
static const double sr = 48000;

static std::vector<float> tone(double f1, double f2, double secs) {
    std::vector<float> v((size_t)(sr * secs));
    for (size_t i = 0; i < v.size(); ++i) v[i] = 0.3f * (float) std::sin(2 * kPi * f1 * (double) i / sr) + (f2 > 0 ? 0.2f * (float) std::sin(2 * kPi * f2 * (double) i / sr) : 0.f);
    return v;
}
static double zeroCrossHz(const std::vector<float>& v, size_t a, size_t b) { int z = 0; for (size_t i = a + 1; i < b; ++i) if ((v[i - 1] < 0) != (v[i] < 0)) ++z; return 0.5 * z / ((double)(b - a) / sr); }
static double rms(const std::vector<float>& v, size_t a, size_t b) { double s = 0; for (size_t i = a; i < b; ++i) s += (double) v[i] * v[i]; return std::sqrt(s / (double)(b - a)); }
// share of the windowed energy found at the given tones (1 = clean copy, low = smeared)
static double toneShare(const std::vector<float>& v, size_t a, size_t b, const std::vector<double>& freqs) {
    const double N = (double)(b - a); double tot = 0, got = 0;
    for (size_t i = a; i < b; ++i) { const double w = 0.5 - 0.5 * std::cos(2 * kPi * (double)(i - a) / N); tot += w * w * (double) v[i] * v[i]; }
    for (double f : freqs) { double c = 0, s = 0; for (size_t i = a; i < b; ++i) { const double w = 0.5 - 0.5 * std::cos(2 * kPi * (double)(i - a) / N); c += w * v[i] * std::cos(2 * kPi * f * (double) i / sr); s += w * v[i] * std::sin(2 * kPi * f * (double) i / sr); } got += 2.0 * (c * c + s * s) * (0.375 * N) / ((0.5 * N) * (0.5 * N)); }
    return got / std::max(tot, 1e-12);
}
static double avgShare(const std::vector<float>& v, size_t a, size_t b, const std::vector<double>& f) { double s = 0; int c = 0; for (size_t w = a; w + 6000 <= b; w += 6000, ++c) s += toneShare(v, w, w + 6000, f); return s / std::max(1, c); }

static void testSpeed() {
    const auto src = tone(440, 1330, 4.0);
    for (double sp : { 0.5, 0.75, 1.0, 1.5, 2.0 }) {
        Params p; p.speed = sp; auto o = render(src, {}, 0, (long long) src.size(), p);
        const double want = 4.0 / sp, got = (double) o.L.size() / sr;
        CHECK(std::fabs(got - want) < 0.12 * want + 0.1, "speed x%.2f: length %.2f s, wanted %.2f s", sp, got, want);
        const size_t a = (size_t)(0.5 * sr), b = std::min(o.L.size(), (size_t)(want * sr * 0.9));
        const double share = avgShare(o.L, a, b, { 440, 1330 });
        std::printf("speed x%.2f: len %.2f s, tone purity %.3f\n", sp, got, share);
        CHECK(share > 0.85, "speed x%.2f keeps the pitch clean (purity %.3f)", sp, share);
    }
}
static void testPitch() {
    const auto src = tone(500, 0, 4.0);
    for (double st : { -12.0, -5.0, 7.0, 12.0 }) {
        Params p; p.semitones = st; auto o = render(src, {}, 0, (long long) src.size(), p);
        const double want = 500.0 * std::pow(2.0, st / 12.0), got = zeroCrossHz(o.L, (size_t)(0.5 * sr), (size_t)(3.0 * sr));
        CHECK(std::fabs(got - want) < want * 0.02, "pitch %+.0f st: %.1f Hz, wanted %.1f", st, got, want);
        CHECK(std::fabs((double) o.L.size() / sr - 4.0) < 0.2, "pitch %+.0f st keeps the length (%.2f s)", st, (double) o.L.size() / sr);
    }
}
static void testTape() {
    const auto src = tone(500, 0, 2.0);
    Params p; p.mode = Mode::RePitch; p.speed = 2.0; auto o = render(src, {}, 0, (long long) src.size(), p);
    CHECK(std::fabs((double) o.L.size() / sr - 1.0) < 0.05, "re-pitch x2 halves the length (%.2f s)", (double) o.L.size() / sr);
    CHECK(std::fabs(zeroCrossHz(o.L, 4800, 40000) - 1000) < 20, "re-pitch x2 doubles the pitch (%.1f Hz)", zeroCrossHz(o.L, 4800, 40000));
    Params q; q.tapeLink = true; q.speed = 0.5; auto o2 = render(src, {}, 0, (long long) src.size(), q);
    CHECK(std::fabs(zeroCrossHz(o2.L, 9600, 80000) - 250) < 8, "tape link x0.5 halves the pitch (%.1f Hz)", zeroCrossHz(o2.L, 9600, 80000));
}
static void testModes() {
    const auto src = tone(440, 1330, 3.0);
    for (Mode m : { Mode::Beats, Mode::Tones, Mode::Complex, Mode::FormantPlus }) {
        Params p; p.mode = m; p.speed = 0.8; auto o = render(src, {}, 0, (long long) src.size(), p);
        const double share = avgShare(o.L, (size_t) sr / 2, o.L.size() - (size_t) sr / 2, { 440, 1330 });
        CHECK(share > 0.8, "mode %s purity %.3f", modeInfo(m).name, share);
        bool bad = false; for (float v : o.L) if (!std::isfinite(v) || std::fabs(v) > 1.2f) bad = true; CHECK(!bad, "mode %s output finite and bounded", modeInfo(m).name);
    }
    Params p; p.mode = Mode::Texture; auto o = render(src, {}, 0, (long long) src.size(), p);
    CHECK(o.L.size() > 100000 && rms(o.L, 20000, 100000) > 0.05, "texture mode makes sound (rms %.3f)", rms(o.L, 20000, 100000));
}
static void testLanes() {
    const auto src = tone(500, 0, 4.0);
    Params p; p.pitchLane.pts = { { 0.0, 0.5 }, { 0.45, 0.5 }, { 0.55, 1.0 }, { 1.0, 1.0 } };      // neutral, then +12 st
    auto o = render(src, {}, 0, (long long) src.size(), p);
    const double early = zeroCrossHz(o.L, (size_t)(0.5 * sr), (size_t)(1.2 * sr)), late = zeroCrossHz(o.L, (size_t)(2.8 * sr), (size_t)(3.5 * sr));
    CHECK(std::fabs(early - 500) < 15 && std::fabs(late - 1000) < 25, "pitch lane: %.1f Hz then %.1f Hz", early, late);
    Params q; q.speedLane.pts = { { 0.0, 1.0 }, { 1.0, 1.0 } };    // lane max = x4 -> 4 s becomes about 1 s
    auto o2 = render(src, {}, 0, (long long) src.size(), q);
    CHECK(std::fabs((double) o2.L.size() / sr - 1.0) < 0.2, "speed lane x4: %.2f s", (double) o2.L.size() / sr);
    Lane l; l.pts = { { 0, 0 }, { 1, 1 } }; CHECK(std::fabs(l.at(0.25) - 0.25) < 1e-9 && l.at(-1) == 0 && l.at(2) == 1, "lane interpolation");
    CHECK(std::fabs(Params::laneSpeed(0.5) - 1.0) < 1e-9 && std::fabs(Params::lanePitch(0.5)) < 1e-9, "lane centre is neutral");
}
static void testSlices() {
    std::vector<float> v((size_t)(sr * 3), 0.f); std::mt19937 rng(3); std::uniform_real_distribution<float> u(-1.f, 1.f);
    for (double t : { 0.4, 1.0, 1.7, 2.3 }) for (size_t i = 0; i < 6000; ++i) v[(size_t)(t * sr) + i] = 0.8f * std::exp(-(float) i / 1500.f) * u(rng);
    auto s = transientSlices(v, 0, (long long) v.size(), sr);
    CHECK(s.size() == 4, "found %zu transient slices, wanted 4", s.size());
    for (size_t k = 0; k < s.size() && k < 4; ++k) { const double want[4] = { 0.4, 1.0, 1.7, 2.3 }; CHECK(std::fabs(s[k] * 3.0 - want[k]) < 0.02, "slice %zu at %.3f s, wanted %.1f", k, s[k] * 3.0, want[k]); }
}
static void testSampler() {
    auto b = std::make_shared<Stereo>(); b->L = tone(500, 0, 1.0); b->R = b->L;
    Sampler s; s.prepare(sr); s.setBuffer(b, sr); SamplerParams p; p.root = 60; p.attackMs = 1; p.releaseMs = 20;
    { std::vector<float> L(48000, 0.f), R(48000, 0.f); s.noteOn(72, 1.f, p); s.process(p, L.data(), R.data(), 48000);
      CHECK(std::fabs(zeroCrossHz(L, 2000, 40000) - 1000) < 20, "note an octave up plays double frequency (%.1f)", zeroCrossHz(L, 2000, 40000)); }
    { Sampler s2; s2.prepare(sr); s2.setBuffer(b, sr); p.mode = Playback::OneShot; std::vector<float> L(96000, 0.f), R(96000, 0.f);
      s2.noteOn(60, 1.f, p); s2.process(p, L.data(), R.data(), 96000);
      CHECK(rms(L, 60000, 96000) < 1e-4 && s2.activeVoices() == 0, "one shot stops at the end of the buffer"); }
    { Sampler s3; s3.prepare(sr); s3.setBuffer(b, sr); p.mode = Playback::Loop; std::vector<float> L(96000, 0.f), R(96000, 0.f);
      s3.noteOn(60, 1.f, p); s3.process(p, L.data(), R.data(), 48000);
      s3.process(p, L.data() + 48000, R.data() + 48000, 48000);
      CHECK(rms(L, 60000, 96000) > 0.1 && s3.activeVoices() == 1, "loop keeps going");
      s3.noteOff(60, p); std::vector<float> L2(4800, 0.f), R2(4800, 0.f); s3.process(p, L2.data(), R2.data(), 4800);
      CHECK(s3.activeVoices() == 0, "note off releases the voice"); }
    { Sampler s4; s4.prepare(sr); s4.setBuffer(b, sr); p.mode = Playback::Loop; std::vector<float> L(480, 0.f), R(480, 0.f);
      for (int k = 0; k < 12; ++k) s4.noteOn(60 + k, 1.f, p);
      CHECK(s4.activeVoices() == Sampler::kVoices, "voice stealing caps polyphony at %d", Sampler::kVoices);
      s4.process(p, L.data(), R.data(), 480); }
}
int main() {
    testSpeed(); testPitch(); testTape(); testModes(); testLanes(); testSlices(); testSampler();
    if (fails) { std::printf("%d FAILED\n", fails); return 1; }
    std::printf("all tests passed\n"); return 0;
}
