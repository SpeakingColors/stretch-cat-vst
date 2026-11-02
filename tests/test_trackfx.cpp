// Tests for the Track effect core. Pure C++, no JUCE.
#include "../src/core/TrackFx.h"
#include <cstdio>
#include <cstdlib>
#include <new>
using namespace stretchcat;

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { ++fails; std::printf("FAIL %s:%d  ", __FILE__, __LINE__); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)
static const double sr = 48000, kPi2 = 6.283185307179586;

#if !defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__)
#define COUNT_ALLOC 1
static std::atomic<long> gAllocs { 0 }; static std::atomic<bool> gCount { false };
void* operator new(std::size_t n) { if (gCount) ++gAllocs; if (void* p = std::malloc(n ? n : 1)) return p; throw std::bad_alloc(); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
#endif

static double goertzel(const std::vector<float>& v, size_t a, size_t b, double f) {
    double c = 0, s = 0; for (size_t i = a; i < b; ++i) { c += v[i] * std::cos(kPi2 * f * (double) i / sr); s += v[i] * std::sin(kPi2 * f * (double) i / sr); }
    return 2.0 * std::sqrt(c * c + s * s) / (double) (b - a);
}
static double maxJump(const std::vector<float>& v, size_t a, size_t b) { double m = 0; for (size_t i = a + 1; i < b; ++i) m = std::max(m, (double) std::fabs(v[i] - v[i - 1])); return m; }
static double peak(const std::vector<float>& v, size_t a, size_t b) { double m = 0; for (size_t i = a; i < b; ++i) m = std::max(m, (double) std::fabs(v[i])); return m; }
// run n samples of a generator through the effect in blocks of 256; returns the left output
template <class Gen> static std::vector<float> run(TrackFx& fx, size_t from, size_t n, Gen gen) {
    std::vector<float> out; out.reserve(n); std::vector<float> L(256), R(256);
    for (size_t pos = from; pos < from + n; pos += 256) { const size_t m = std::min<size_t>(256, from + n - pos); for (size_t i = 0; i < m; ++i) L[i] = R[i] = gen(pos + i); fx.process(L.data(), R.data(), (int) m); out.insert(out.end(), L.begin(), L.begin() + (long) m); }
    return out;
}
static TrackParams base() { TrackParams p; p.startMs = 600; p.lengthMs = 300; p.loop = true; p.attackMs = 10; p.holdMs = 40; p.spacingMs = 30; return p; }
// 440 Hz for the first second, 1 kHz afterwards: what is held tells which part was captured
static float twoTone(size_t i) { return i < (size_t) sr ? 0.4f * (float) std::sin(kPi2 * 440.0 * (double) i / sr) : 0.4f * (float) std::sin(kPi2 * 1000.0 * (double) i / sr); }

static void testDryIsBitExact() {
    TrackFx fx; fx.prepare(sr); fx.setParams(base()); std::vector<float> in; for (size_t i = 0; i < 20000; ++i) in.push_back(twoTone(i));
    auto out = run(fx, 0, in.size(), twoTone); bool same = true; for (size_t i = 0; i < in.size(); ++i) if (out[i] != in[i]) same = false;
    CHECK(same, "not held: output is bit exact the input");
    CHECK(fx.latencySamples() == 0, "latency is reported as exactly 0 samples");
}
static void testCaptureAndFreeze() {
    TrackFx fx; fx.prepare(sr, 1.0); auto p = base(); p.startMs = 500; p.lengthMs = 200; fx.setParams(p);   // 1 s ring: later input would overwrite the held part if capture did not stop
    auto ramp = [](size_t i) { return (float) (i % 100000) * 1e-5f; };                       // sample i has value i*1e-5, so the held content is checkable
    run(fx, 0, 48000, ramp); fx.setHold(true);
    // the held part starts 500 ms before "now" and is 200 ms long
    const long long startIdx = 48000 - 24000, len = 9600;
    CHECK(std::llabs((long long) (fx.heldLengthMs() * 48.0 + 0.5) - len) <= 1, "held length is %.1f ms", fx.heldLengthMs());
    CHECK(std::fabs(fx.heldSample(0) - ramp((size_t) startIdx)) < 1e-6 && std::fabs(fx.heldSample(len - 1) - ramp((size_t) (startIdx + len - 1))) < 1e-6, "held part is the right stretch of the past (start %.4f)", fx.heldSample(0));
    // input after HOLD must not enter the held part
    run(fx, 48000, 60000, [](size_t) { return 0.9f; });
    CHECK(std::fabs(fx.heldSample(0) - ramp((size_t) startIdx)) < 1e-6 && std::fabs(fx.heldSample(len - 1) - ramp((size_t) (startIdx + len - 1))) < 1e-6, "freeze stops capture: held part unchanged by later input");
    // ring wraps correctly: a hold taken after the ring has wrapped twice
    TrackFx g; g.prepare(sr, 1.0); g.setParams(p); auto cnt = [](size_t i) { return (float) (i % 7919) * 1e-4f; }; run(g, 0, 150000, cnt); g.setHold(true);
    const long long sIdx = 150000 - (long long) (500 * 48 > 47999 ? 47000 : 500 * 48);
    CHECK(g.ringSize() == 48000 && std::fabs(g.heldSample(0) - cnt((size_t) sIdx)) < 1e-6, "hold after the ring has wrapped reads the right samples (%.5f vs %.5f)", g.heldSample(0), cnt((size_t) sIdx));
}
static void testRepeatsHeldPart() {
    TrackFx fx; fx.prepare(sr); fx.setParams(base());                                     // held part = 600..300 ms back = inside the 440 Hz second? now = 1.5 s, so 0.9 .. 1.2 s: mixed. use start 1000
    auto p = base(); p.startMs = 1100; p.lengthMs = 300; fx.setParams(p);                  // now = 1.5 s: held part 0.4 .. 0.7 s = 440 Hz
    run(fx, 0, 72000, twoTone); fx.setHold(true);
    auto out = run(fx, 72000, 48000, twoTone);                                              // input is 1 kHz meanwhile
    const size_t a = 4000, b = out.size();
    const double g440 = goertzel(out, a, b, 440.0), g1k = goertzel(out, a, b, 1000.0);
    CHECK(g440 > 0.15 && g1k < 0.02, "hold repeats the captured 440 Hz and ignores the live 1 kHz (440: %.3f, 1k: %.4f)", g440, g1k);
    // release returns to the dry signal
    fx.setHold(false); auto back = run(fx, 120000, 24000, twoTone);
    CHECK(goertzel(back, 6000, 24000, 1000.0) > 0.35 && goertzel(back, 6000, 24000, 440.0) < 0.02, "after release the live signal is back");
}
static void testLoopAndOneShot() {
    auto p = base(); p.startMs = 1100; p.lengthMs = 200; p.speed = 1.0;
    { TrackFx fx; fx.prepare(sr); p.loop = true; fx.setParams(p); run(fx, 0, 72000, twoTone); fx.setHold(true); auto o = run(fx, 72000, 96000, twoTone);
      CHECK(peak(o, 60000, 96000) > 0.15, "loop on: still repeating after 5 times the held length (peak %.3f)", peak(o, 60000, 96000)); }
    { TrackFx fx; fx.prepare(sr); p.loop = false; fx.setParams(p); run(fx, 0, 72000, twoTone); fx.setHold(true); auto o = run(fx, 72000, 96000, twoTone);
      // one pass: the wet part ends, then the live 1 kHz returns (so check that 440 stops and the held part ended)
      CHECK(goertzel(o, 40000, 96000, 440.0) < 0.02 && goertzel(o, 40000, 96000, 1000.0) > 0.3, "loop off: plays the part once, then the live signal returns"); CHECK(!fx.isHeld(), "loop off: hold releases itself after the single pass"); }
}
static void testSpeed() {
    for (double sp : { 0.5, 1.0, 2.0 }) {
        TrackFx fx; fx.prepare(sr); auto p = base(); p.startMs = 1100; p.lengthMs = 1000; p.speed = sp; p.loop = false; fx.setParams(p); run(fx, 0, 72000, twoTone); fx.setHold(true);
        run(fx, 72000, 9600, twoTone);                                                       // 200 ms
        CHECK(std::fabs(fx.playheadMs() - 200.0 * sp) < 4.0, "speed x%.1f: playhead moved %.1f ms in 200 ms (want %.0f)", sp, fx.playheadMs(), 200.0 * sp);
    }
}
static void testPitch() {
    for (double st : { 12.0, -12.0, 7.0 }) {
        TrackFx fx; fx.prepare(sr); auto p = base(); p.startMs = 1100; p.lengthMs = 300; p.keySemis = st; p.attackMs = 15; p.holdMs = 60; p.spacingMs = 100;   // grains do not overlap, so the pitch can be read cleanly
         fx.setParams(p); run(fx, 0, 72000, twoTone); fx.setHold(true);
        auto o = run(fx, 72000, 48000, [](size_t) { return 0.f; }); const double want = 440.0 * std::pow(2.0, st / 12.0);
        double best = 0, bf = 0; for (double f = 200; f < 1000; f += 2.0) { const double g = goertzel(o, 4000, 40000, f); if (g > best) { best = g; bf = f; } }
        CHECK(std::fabs(bf - want) < 0.03 * want, "key %+.0f st: peak at %.1f Hz, wanted %.1f Hz", st, bf, want);
    }
}
static void testNoClicks() {
    TrackFx fx; fx.prepare(sr); auto p = base(); p.startMs = 1100; p.lengthMs = 300; p.rand = 0.4; p.depth = 0.5; fx.setParams(p); run(fx, 0, 72000, twoTone);
    auto sine = [](size_t i) { return 0.4f * (float) std::sin(kPi2 * 440.0 * (double) i / sr); };
    fx.setHold(true); auto a = run(fx, 72000, 48000, sine); fx.setHold(false); auto b = run(fx, 120000, 24000, sine);
    // a 440 Hz sine at 0.4 steps at most 0.023 per sample; grain edges, hold on and release must stay in that region (pan spread adds a little)
    CHECK(maxJump(a, 0, a.size()) < 0.12, "no clicks while holding (largest step %.4f)", maxJump(a, 0, a.size()));
    CHECK(maxJump(b, 0, b.size()) < 0.12, "no click at release (largest step %.4f)", maxJump(b, 0, b.size()));
    bool finite = true; for (float v : a) if (!std::isfinite(v)) finite = false; CHECK(finite, "output stays finite");
    CHECK(peak(a, 0, a.size()) < 1.0, "level stays sane while holding (peak %.3f)", peak(a, 0, a.size()));
    // parameter changes while holding are smooth (grains carry their own envelopes; a pan jump only reaches new grains)
    fx.setHold(true); auto c1 = run(fx, 144000, 10000, sine); auto q = p; q.pan = 1.0; q.spacingMs = 15; q.attackMs = 25; fx.setParams(q); auto c2 = run(fx, 154000, 20000, sine);
    CHECK(maxJump(c2, 0, c2.size()) < 0.15, "changing pan, spacing and attack while holding makes no click (%.4f)", maxJump(c2, 0, c2.size()));
    (void) c1;
}
static void testPanAndDepth() {
    TrackFx fx; fx.prepare(sr); auto p = base(); p.startMs = 1100; p.lengthMs = 300; p.pan = -1.0; fx.setParams(p); run(fx, 0, 72000, twoTone); fx.setHold(true);
    std::vector<float> L(4800), R(4800); for (size_t i = 0; i < 4800; ++i) L[i] = R[i] = 0.f; double eL = 0, eR = 0;
    for (int k = 0; k < 6; ++k) { fx.process(L.data(), R.data(), 4800); if (k > 1) for (size_t i = 0; i < 4800; ++i) { eL += L[i] * L[i]; eR += R[i] * R[i]; } }
    CHECK(eL > 20.0 * eR, "pan -1 puts the grains on the left (L %.3f, R %.5f)", eL, eR);
}
static void testDetect() {
    // clicks (short noise bursts) every 400 ms; Detect should find them in the held part
    auto clicks = [](size_t i) { const size_t k = i % 19200; static uint32_t s = 5; s = s * 1664525u + 1013904223u; return k < 300 ? 0.8f * ((float) (s >> 9) / 4194304.f - 1.f) * (1.f - (float) k / 300.f) : 0.f; };
    TrackFx fx; fx.prepare(sr); auto p = base(); p.startMs = 2000; p.lengthMs = 1600; p.trMode = TrMode::Detect; fx.setParams(p); run(fx, 0, 96000, clicks); fx.setHold(true);
    CHECK(fx.regionCount() >= 3 && fx.regionCount() <= 5, "Detect finds the transients in the held part (%d found, 4 hits)", fx.regionCount());
    if (fx.regionCount() > 0) { const long long a = fx.regionStart(0); CHECK(std::llabs(a % 19200) < 900 || std::llabs(a % 19200 - 19200) < 900, "first detected hit sits at a click (offset %lld)", a); }
    // Use regions: markers given as fractions
    TrackFx g; g.prepare(sr); p.trMode = TrMode::Regions; g.setParams(p); const double fr[3] = { 0.25, 0.5, 0.75 }; g.setRegions(fr, 3); run(g, 0, 96000, clicks); g.setHold(true);
    CHECK(g.regionCount() == 3 && std::llabs(g.regionStart(1) - (long long) (0.5 * 76800)) < 4, "Use regions takes the given markers (%d)", g.regionCount());
    // Hold amount: small amount repeats only the start of each region (the playhead region shrinks, so grains read less audio)
    auto q = p; q.holdAmount = 0.1; g.setParams(q); auto o = run(g, 96000, 48000, [](size_t) { return 0.f; }); CHECK(peak(o, 4000, o.size()) > 0.02, "Hold amount 10%% still produces sound");
}
static void testNoAllocation() {
#ifdef COUNT_ALLOC
    TrackFx fx; fx.prepare(sr); auto p = base(); p.rand = 0.5; p.depth = 0.5; p.trMode = TrMode::Detect; fx.setParams(p); std::vector<float> L(512, 0.1f), R(512, 0.1f);
    for (int i = 0; i < 100; ++i) fx.process(L.data(), R.data(), 512);
    gAllocs = 0; gCount = true;
    fx.setHold(true); for (int i = 0; i < 400; ++i) fx.process(L.data(), R.data(), 512); fx.setHold(false); for (int i = 0; i < 50; ++i) fx.process(L.data(), R.data(), 512);
    gCount = false; CHECK(gAllocs == 0, "no memory allocation in process() or setHold() (%ld allocations)", (long) gAllocs.load());
#else
    std::printf("allocation test skipped under sanitizers (it replaces operator new)\n");
#endif
}

int main() {
    testDryIsBitExact(); testCaptureAndFreeze(); testRepeatsHeldPart(); testLoopAndOneShot(); testSpeed(); testPitch(); testNoClicks(); testPanAndDepth(); testDetect(); testNoAllocation();
    if (fails) { std::printf("%d FAILED\n", fails); return 1; }
    std::printf("trackfx: all tests passed\n"); return 0;
}
