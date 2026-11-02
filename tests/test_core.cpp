// Core tests for Stretch Cat. Pure C++, no JUCE.
#include "../src/core/RenderWorker.h"
#include <thread>
#include <chrono>
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
static std::unique_ptr<Rendered> mkR(const std::vector<float>& v, double rate = sr) {
    auto r = std::make_unique<Rendered>(); r->audio.L = v; r->audio.R = v; r->sr = rate; r->sliceBounds = { 0, (long long) v.size() };
    for (int i = 0; i < 256; ++i) r->tCurve[(size_t) i] = (float) i / 255.f;
    return r;
}
static void testSampler() {
    auto b = mkR(tone(500, 0, 1.0));
    Sampler s; s.prepare(sr); s.swapTo(b.get()); SamplerParams p; p.root = 60; p.attackMs = 1; p.releaseMs = 20;
    { std::vector<float> L(48000, 0.f), R(48000, 0.f); s.noteOn(72, 1.f, p); s.process(p, L.data(), R.data(), 48000);
      CHECK(std::fabs(zeroCrossHz(L, 2000, 40000) - 1000) < 20, "note an octave up plays double frequency (%.1f)", zeroCrossHz(L, 2000, 40000)); }
    { Sampler s2; s2.prepare(sr); s2.swapTo(b.get()); p.mode = Playback::OneShot; std::vector<float> L(96000, 0.f), R(96000, 0.f);
      s2.noteOn(60, 1.f, p); s2.process(p, L.data(), R.data(), 96000);
      CHECK(rms(L, 60000, 96000) < 1e-4 && s2.activeVoices() == 0, "one shot stops at the end of the buffer"); }
    { Sampler s3; s3.prepare(sr); s3.swapTo(b.get()); p.mode = Playback::Loop; std::vector<float> L(96000, 0.f), R(96000, 0.f);
      s3.noteOn(60, 1.f, p); s3.process(p, L.data(), R.data(), 48000);
      s3.process(p, L.data() + 48000, R.data() + 48000, 48000);
      CHECK(rms(L, 60000, 96000) > 0.1 && s3.activeVoices() == 1, "loop keeps going");
      s3.noteOff(60, p); std::vector<float> L2(4800, 0.f), R2(4800, 0.f); s3.process(p, L2.data(), R2.data(), 4800);
      CHECK(s3.activeVoices() == 0, "note off releases the voice"); }
    { Sampler s4; s4.prepare(sr); s4.swapTo(b.get()); p.mode = Playback::Loop; std::vector<float> L(480, 0.f), R(480, 0.f);
      for (int k = 0; k < 12; ++k) s4.noteOn(60 + k, 1.f, p);
      CHECK(s4.activeVoices() == Sampler::kVoices, "voice stealing caps polyphony at %d", Sampler::kVoices);
      s4.process(p, L.data(), R.data(), 480); }
}
static void testLength() {
    CHECK(std::fabs(fitSpeed(48000 * 2.0, 48000, 120, 4.0) - 1.0) < 1e-9, "2 s at 120 BPM is exactly 1 bar: speed 1");
    CHECK(std::fabs(fitSpeed(48000 * 4.0, 48000, 120, 4.0) - 2.0) < 1e-9, "4 s into 1 bar at 120 BPM: speed 2");
    CHECK(std::fabs(fitSpeed(48000 * 2.0, 48000, 120, 16.0) - 0.25) < 1e-9, "2 s into 4 bars at 120 BPM: speed 0.25");
    CHECK(std::fabs(fitSpeed(44100 * 1.0, 44100, 60, 1.0) - 1.0) < 1e-9, "1 s at 60 BPM is one beat");
    CHECK(fitSpeed(1000, 48000, 120, 0.0) == 0.0 && fitSpeed(1000, 48000, 0, 4.0) == 0.0, "Auto or bad tempo gives no fit");
    // end to end: the rendered length matches the chosen length
    std::vector<float> v = tone(300, 0, 2.0); Params p; p.mode = Mode::Complex; p.speed = fitSpeed((double) v.size(), sr, 120, 4.0) * 2.0;
    auto o = render(v, v, 0, (long long) v.size(), p);
    CHECK(std::fabs((double) o.L.size() / sr - 1.0) < 0.15, "2 s fitted to half a bar at 120 BPM renders about 1.0 s (%.3f)", (double) o.L.size() / sr);
}

static double maxJump(const std::vector<float>& v, size_t a, size_t b) { double m = 0; for (size_t i = a + 1; i < b; ++i) m = std::max(m, (double) std::fabs(v[i] - v[i - 1])); return m; }
static bool finiteAll(const std::vector<float>& v) { for (float x : v) if (!std::isfinite(x)) return false; return true; }

// ---------- 1. background render: bus, worker, crossfade ----------
static void testBusAndWorker() {
    auto src = std::make_shared<Source>(); src->L = tone(440, 1330, 6.0); src->R = src->L; src->sr = sr;
    ResultBus bus; auto worker = std::make_unique<RenderWorker>(bus);
    std::atomic<int> onAudio { 0 }; const auto audioId = std::this_thread::get_id(); std::thread::id workerId = worker->threadId();
    worker->onRenderStart = [&] { if (std::this_thread::get_id() == audioId) ++onAudio; };
    CHECK(workerId != audioId, "the worker is its own thread");
    Request rq; rq.src = src; rq.p.mode = Mode::Complex; rq.p.speed = 0.75;
    const auto t0 = std::chrono::steady_clock::now(); worker->post(rq);
    ResultBus::AudioHandle h; int blocks = 0; bool got = false;
    while (std::chrono::steady_clock::now() - t0 < std::chrono::seconds(20)) { ++blocks; if (bus.poll(h)) { got = true; break; } std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    CHECK(got, "a posted render is published");
    std::printf("render of 6 s source on the worker: %.0f ms from post to published (audio thread polled %d times meanwhile)\n", ms, blocks);
    CHECK(onAudio == 0, "the render never ran on the audio thread");
    if (got) CHECK(bus.slot(h.cur).length() > (long long) (6.0 / 0.75 * sr * 0.9), "published render has the stretched length (%lld)", bus.slot(h.cur).length());
    // latest wins: post many requests quickly, only the last one must end up published, and the audio side must keep adopting without tearing
    ResultBus::AudioHandle h2 = h; double lastSpeed = 0;
    for (int k = 0; k < 8; ++k) { rq.p.speed = 0.6 + 0.1 * k; lastSpeed = rq.p.speed; worker->post(rq); std::this_thread::sleep_for(std::chrono::milliseconds(5)); }
    int adopted = 0; const auto t1 = std::chrono::steady_clock::now(); long long lastLen = 0;
    while (std::chrono::steady_clock::now() - t1 < std::chrono::seconds(30)) { if (bus.poll(h2)) { ++adopted; lastLen = bus.slot(h2.cur).length(); bus.releasePrev(h2); } if (worker->idle() && adopted > 0) { if (bus.poll(h2)) { lastLen = bus.slot(h2.cur).length(); bus.releasePrev(h2); } break; } std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    const double wantLen = 6.0 / lastSpeed * sr;
    CHECK(std::fabs((double) lastLen - wantLen) < 0.12 * wantLen + 4800, "latest request wins (length %lld, wanted about %.0f)", lastLen, wantLen);
    CHECK(worker->rendersDone() >= 2, "renders completed: %llu", (unsigned long long) worker->rendersDone());
    // a slices-only change reuses the audio (no new stretch)
    Request rs = rq; rs.slices.even(0, 1, 4); worker->post(rs);
    while (!worker->idle()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    UiResult u; bool ui = false; while (worker->fetchUi(u)) ui = true;
    CHECK(ui && u.sliceFrac.size() == 5 && u.sliceFrac.front() == 0.0 && std::fabs(u.sliceFrac.back() - 1.0) < 1e-9, "slice edges reported for the editor (%zu)", u.sliceFrac.size());
    CHECK(u.ms < 20000, "ui info carries the render time");
    worker.reset();
}
// The audio thread keeps playing while renders are posted from another thread; no clicks, no NaN, no data race (run under tsan too).
static void testSwapWhilePlaying() {
    auto src = std::make_shared<Source>(); src->L = tone(500, 0, 3.0); src->R = src->L; src->sr = sr;
    ResultBus bus; RenderWorker worker(bus); Request rq; rq.src = src; rq.p.mode = Mode::Tones; worker.post(rq);
    ResultBus::AudioHandle h; Sampler smp; smp.prepare(sr); SamplerParams sp; sp.attackMs = 1; sp.root = 60; sp.releaseMs = 30; sp.mode = Playback::Loop;
    while (!bus.poll(h)) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    smp.swapTo(&bus.slot(h.cur)); smp.noteOn(60, 1.f, sp);
    std::atomic<bool> stopPoster { false }; std::thread poster([&] { int k = 0; while (!stopPoster) { Request r = rq; r.p.speed = 0.8 + 0.05 * (k++ % 6); worker.post(r); std::this_thread::sleep_for(std::chrono::milliseconds(60)); } });
    std::vector<float> out; int swaps = 0; const int B = 128;
    for (int blk = 0; blk < 48000 * 4 / B; ++blk) {
        if (!smp.crossfading()) { if (bus.poll(h)) { smp.swapTo(&bus.slot(h.cur)); ++swaps; } }
        std::vector<float> L(B, 0.f), R(B, 0.f); smp.process(sp, L.data(), R.data(), B);
        if (smp.takeCrossfadeEnded()) bus.releasePrev(h);
        out.insert(out.end(), L.begin(), L.end());
        std::this_thread::sleep_for(std::chrono::microseconds(900));
    }
    stopPoster = true; poster.join();
    CHECK(swaps >= 3, "buffers were swapped while playing (%d swaps)", swaps);
    CHECK(finiteAll(out), "swapping while playing gives finite output");
    // a 500 Hz sine at 0.3 never steps by more than 2*pi*500/48000*0.3 = 0.02 per sample; loop seams and swaps must stay under 0.2
    const double mj = maxJump(out, 2000, out.size()); CHECK(mj < 0.2, "no click when swapping or looping (largest step %.3f)", mj);
}
static void testCrossfade() {
    auto A = mkR(tone(500, 0, 1.0)), B = mkR(tone(1000, 0, 1.0));
    Sampler s; s.prepare(sr); SamplerParams p; p.attackMs = 1; p.mode = Playback::Loop; s.swapTo(A.get()); s.noteOn(60, 1.f, p);
    std::vector<float> L(48000, 0.f), R(48000, 0.f); s.process(p, L.data(), R.data(), 4800);
    s.swapTo(B.get()); CHECK(s.crossfading(), "crossfade starts on swap");
    s.process(p, L.data() + 4800, R.data() + 4800, 43200);
    CHECK(!s.crossfading() && s.takeCrossfadeEnded(), "crossfade ends");
    CHECK(std::fabs(zeroCrossHz(L, 1000, 4000) - 500) < 15, "before the swap the old render plays (%.0f Hz)", zeroCrossHz(L, 1000, 4000));
    CHECK(std::fabs(zeroCrossHz(L, 8000, 40000) - 1000) < 25, "after the swap the new render plays (%.0f Hz)", zeroCrossHz(L, 8000, 40000));
    // the swap is complete 20 ms (960 samples) after it started
    CHECK(rms(L, 4800 + 1100, 4800 + 3000) > 0.15, "new render is at full level right after the fade");
    CHECK(maxJump(L, 4800 + 960, 40000) < 0.15, "after the fade there is no step");
}
// ---------- 2. slices ----------
static void testSliceSet() {
    SliceSet s; s.even(0.0, 1.0); CHECK(s.count() == 8 && std::fabs(s.marks[3] - 0.5) < 1e-9, "even slices: %d", s.count());
    CHECK(s.add(0.51, 0, 1) == -1, "a marker too close to another is refused");
    SliceSet t; CHECK(t.add(0.3, 0, 1) == 0 && t.add(0.1, 0, 1) == 0 && t.marks[1] == 0.3, "markers stay sorted");
    t.move(0, 0.5, 0, 1); CHECK(t.marks[0] < 0.3, "a marker cannot pass its neighbour (%.3f)", t.marks[0]);
    t.remove(0); CHECK(t.count() == 2, "remove");
    SliceSet u; for (int i = 1; i <= 12; ++i) u.add(i * 0.07, 0, 1); CHECK(u.count() == SliceSet::kMax, "at most %d slices", SliceSet::kMax);
    SliceSet r = SliceSet::fromString(s.toString()); CHECK(r.marks.size() == s.marks.size() && std::fabs(r.marks[2] - s.marks[2]) < 1e-9, "string round trip");
    CHECK(SliceSet::fromString("junk,0.5,,nan,2,-1,0.5").marks.size() == 1, "bad text is cleaned");
    // the mapping follows speed: slices of a source stretched to half speed land twice as late
    std::vector<std::pair<double, double>> map = { { 0, 0 }, { 2000, 1000 }, { 4000, 2000 } };
    CHECK(std::fabs(sourceToOut(map, 500) - 1000) < 1e-9 && std::fabs(outToSource(map, 3000) - 1500) < 1e-9, "source and output positions map both ways");
    SliceSet q; q.marks = { 0.25, 0.5 }; auto bd = sliceBounds(q, map, 2000.0, 4000); CHECK(bd.size() == 3 + 1 && bd[1] == 1000 && bd[2] == 2000 && bd[0] == 0 && bd[3] == 4000, "slice bounds in the render");
}
static void testSlicePlay() {
    // four 0.25 s blocks of different tones; slice i must play only block i
    std::vector<float> v; const double fr[4] = { 300, 600, 900, 1200 }; for (int b = 0; b < 4; ++b) { auto t = tone(fr[b], 0, 0.25); v.insert(v.end(), t.begin(), t.end()); }
    auto R = mkR(v); const long long q = (long long) (sr * 0.25); R->sliceBounds = { 0, q, 2 * q, 3 * q, 4 * q };
    Sampler s; s.prepare(sr); s.swapTo(R.get()); SamplerParams p; p.attackMs = 1; p.mode = Playback::OneShot;
    CHECK(s.sliceCount() == 4, "four slices");
    for (int i = 0; i < 4; ++i) {
        std::vector<float> L(24000, 0.f), Rr(24000, 0.f); s.sliceOn(i, 1.f); s.process(p, L.data(), Rr.data(), 24000);
        const double hz = zeroCrossHz(L, 1000, 11000); CHECK(std::fabs(hz - fr[i]) < fr[i] * 0.03, "slice %d plays its own block (%.0f Hz, wanted %.0f)", i, hz, fr[i]);
        CHECK(rms(L, 12500, 24000) < 1e-4 && s.activeVoices() == 0, "slice %d stops at its end", i);
    }
    s.sliceOn(9, 1.f); CHECK(s.activeVoices() == 0, "a pad with no slice does nothing");
    // a slice keeps playing its own block after a new render with different lengths arrives
    auto R2 = mkR(v); R2->audio.L.resize(v.size() * 2); R2->audio.R = R2->audio.L; const long long q2 = 2 * q; for (long long i = 0; i < (long long) R2->audio.L.size(); ++i) R2->audio.L[(size_t) i] = v[(size_t) (i / 2)]; R2->audio.R = R2->audio.L; R2->sliceBounds = { 0, q2, 2 * q2, 3 * q2, 4 * q2 };
    Sampler s2; s2.prepare(sr); s2.swapTo(R.get()); p.mode = Playback::Loop; s2.sliceOn(1, 1.f); std::vector<float> L(48000, 0.f), Rr(48000, 0.f); s2.process(p, L.data(), Rr.data(), 2000); s2.swapTo(R2.get()); s2.process(p, L.data() + 2000, Rr.data() + 2000, 46000);
    CHECK(std::fabs(zeroCrossHz(L, 6000, 40000) - 300) < 20, "slice voice follows the new render: slice 1 (600 Hz) is now twice as long, so 300 Hz (%.0f Hz)", zeroCrossHz(L, 6000, 40000));
}
static void testSlicesFromRender() {
    // clicks at 0.5 s and 1.5 s of a 2 s source, rendered at half speed: the slice between them must start about 1.0 s into the 4 s result
    std::vector<float> v((size_t) (sr * 2), 0.f); v[(size_t) (sr * 0.5)] = 0.9f; v[(size_t) (sr * 1.5)] = 0.9f;
    for (size_t i = 0; i < v.size(); ++i) v[i] += 0.05f * (float) std::sin(2 * kPi * 400.0 * (double) i / sr);
    Request rq; auto src = std::make_shared<Source>(); src->L = v; src->R = v; src->sr = sr; rq.src = src; rq.p.speed = 0.5; rq.slices.marks = { 0.25, 0.75 };
    Rendered r; CHECK(renderAll(rq, r, nullptr), "render ok"); finishRendered(rq, r);
    CHECK(r.sliceBounds.size() == 4, "3 slices");
    if (r.sliceBounds.size() == 4) CHECK(std::fabs((double) r.sliceBounds[1] / sr - 1.0) < 0.15 && std::fabs((double) r.sliceBounds[2] / sr - 3.0) < 0.2, "slice edges follow the stretch (%.2f s, %.2f s)", (double) r.sliceBounds[1] / sr, (double) r.sliceBounds[2] / sr);
    CHECK(std::fabs(r.tCurve[128] - 0.5) < 0.05 && r.tCurve[0] < 0.02 && r.tCurve[255] > 0.98, "source time curve is 0 .. 1 (mid %.3f)", r.tCurve[128]);
}
// ---------- 3. lanes, live ----------
static void testLiveLanes() {
    auto R = mkR(tone(6000, 0, 1.0)); LiveLanes lanes; Sampler s; s.prepare(sr); s.setLanes(&lanes); s.swapTo(R.get()); SamplerParams p; p.attackMs = 1; p.mode = Playback::Loop;
    auto level = [&](double y, int w) { Lane l; l.pts = { { 0, y }, { 1, y } }; lanes.cutoff.set(l); Sampler q; q.prepare(sr); q.setLanes(&lanes); q.swapTo(R.get()); std::vector<float> L(24000, 0.f), Rr(24000, 0.f); q.noteOn(60, 1.f, p); q.process(p, L.data(), Rr.data(), 24000); (void) w; return rms(L, 8000, 24000); };
    const double open = level(1.0, 0), closed = level(0.1, 0);     // y 0.1 -> about 100 * 160^0.1 = 166 Hz
    std::printf("cutoff lane: 6 kHz tone, open %.3f, closed %.5f\n", open, closed);
    CHECK(open > 0.1 && closed < open * 0.03, "cutoff lane closes the filter live (%.4f vs %.4f)", closed, open);
    // width lane 0 -> mono
    std::vector<float> a = tone(500, 0, 1.0), b = tone(700, 0, 1.0); auto W = mkR(a); W->audio.R = b;
    Lane wl; wl.pts = { { 0, 0.0 }, { 1, 0.0 } }; lanes.width.set(wl); lanes.cutoff.set(Lane { { { 0, 1.0 }, { 1, 1.0 } } });
    Sampler q; q.prepare(sr); q.setLanes(&lanes); q.swapTo(W.get()); std::vector<float> L(12000, 0.f), Rr(12000, 0.f); q.noteOn(60, 1.f, p); q.process(p, L.data(), Rr.data(), 12000);
    double d = 0; for (size_t i = 6000; i < 12000; ++i) d = std::max(d, (double) std::fabs(L[i] - Rr[i])); CHECK(d < 0.05, "width lane at 0 makes the voice mono once settled (%.4f)", d);
    lanes.width.set(Lane());   // flat again: stereo untouched
    Sampler q2; q2.prepare(sr); q2.setLanes(&lanes); q2.swapTo(W.get()); std::vector<float> L2(12000, 0.f), R2(12000, 0.f); q2.noteOn(60, 1.f, p); q2.process(p, L2.data(), R2.data(), 12000);
    d = 0; for (size_t i = 6000; i < 12000; ++i) d = std::max(d, (double) std::fabs(L2[i] - R2[i])); CHECK(d > 0.3, "flat width lane leaves stereo alone");
    // the lane follows the position in the render: close the filter in the second half only
    auto H = mkR(tone(6000, 0, 2.0)); Lane half; half.pts = { { 0, 1.0 }, { 0.45, 1.0 }, { 0.55, 0.1 }, { 1.0, 0.1 } }; lanes.cutoff.set(half); lanes.width.set(Lane());
    Sampler q3; q3.prepare(sr); q3.setLanes(&lanes); q3.swapTo(H.get()); p.mode = Playback::OneShot; std::vector<float> L3(96000, 0.f), R3(96000, 0.f); q3.noteOn(60, 1.f, p); q3.process(p, L3.data(), R3.data(), 96000);
    CHECK(rms(L3, 10000, 40000) > 0.1 && rms(L3, 70000, 94000) < 0.02, "cutoff lane over time: open first half (%.3f), closed second (%.4f)", rms(L3, 10000, 40000), rms(L3, 70000, 94000));
}
// ---------- 4. rack ----------
static std::vector<float> runRack(Rack& r, const std::vector<float>& in, int block, std::vector<float>* outR = nullptr) {
    std::vector<float> L = in, R = in; for (size_t i = 0; i < L.size(); i += (size_t) block) r.process(L.data() + i, R.data() + i, (int) std::min<size_t>((size_t) block, L.size() - i)); if (outR) *outR = R; return L;
}
static void testRack() {
    Rack r; r.prepare(sr); RackParams p; p.filterOn = false; p.eqOn = false; p.compOn = false; r.setParams(p);
    auto in = tone(1000, 0, 0.5); auto o = runRack(r, in, 100); bool same = true; for (size_t i = 0; i < in.size(); ++i) if (o[i] != in[i]) same = false; CHECK(same, "rack with everything off is bit exact");
    { Rack f; f.prepare(sr); RackParams q; q.filterOn = true; q.filterType = 0; q.cutoff = 1000; q.eqOn = false; f.setParams(q);
      auto hi = tone(8000, 0, 0.5), lo = tone(100, 0, 0.5); auto oh = runRack(f, hi, 256);
      Rack f2; f2.prepare(sr); f2.setParams(q); auto ol = runRack(f2, lo, 256);
      const double dbH = 20 * std::log10(rms(oh, 10000, 24000) / rms(hi, 10000, 24000)), dbL = 20 * std::log10(rms(ol, 10000, 24000) / rms(lo, 10000, 24000));
      CHECK(dbH < -30 && std::fabs(dbL) < 1.0, "low-pass at 1 kHz: 8 kHz %.1f dB, 100 Hz %.1f dB", dbH, dbL);
      for (int t = 1; t < 4; ++t) { Rack g; g.prepare(sr); RackParams z = q; z.filterType = t; g.setParams(z); auto og = runRack(g, tone(1000, 4000, 0.5), 256); CHECK(finiteAll(og), "filter type %d is finite", t); } }
    { Rack e; e.prepare(sr); RackParams q; q.filterOn = false; q.eqOn = true; q.eqLow = 12; e.setParams(q); auto lo = tone(50, 0, 0.5), mid = tone(2000, 0, 0.5);
      auto ol = runRack(e, lo, 256); Rack e2; e2.prepare(sr); e2.setParams(q); auto om = runRack(e2, mid, 256);
      const double dbL = 20 * std::log10(rms(ol, 10000, 24000) / rms(lo, 10000, 24000)), dbM = 20 * std::log10(rms(om, 10000, 24000) / rms(mid, 10000, 24000));
      CHECK(dbL > 9 && dbL < 13 && std::fabs(dbM) < 1.0, "low shelf +12 dB: 50 Hz %.1f dB, 2 kHz %.1f dB", dbL, dbM);
      RackParams m = q; m.eqLow = 0; m.eqMid = -12; Rack e3; e3.prepare(sr); e3.setParams(m); auto k = tone(1000, 0, 0.5); auto ok = runRack(e3, k, 256); const double dbK = 20 * std::log10(rms(ok, 10000, 24000) / rms(k, 10000, 24000)); CHECK(std::fabs(dbK + 12) < 1.0, "mid -12 dB at 1 kHz: %.1f dB", dbK);
      RackParams h = q; h.eqLow = 0; h.eqHigh = 12; Rack e4; e4.prepare(sr); e4.setParams(h); auto k2 = tone(10000, 0, 0.5); auto o2 = runRack(e4, k2, 256); const double dbH = 20 * std::log10(rms(o2, 10000, 24000) / rms(k2, 10000, 24000)); CHECK(dbH > 9 && dbH < 13, "high shelf +12 dB at 10 kHz: %.1f dB", dbH); }
    { std::vector<float> loud(24000), quiet(24000); for (size_t i = 0; i < 24000; ++i) { loud[i] = 0.8f * (float) std::sin(2 * kPi * 300.0 * (double) i / sr); quiet[i] = 0.02f * (float) std::sin(2 * kPi * 300.0 * (double) i / sr); }
      RackParams q; q.filterOn = false; q.eqOn = false; q.compOn = true; q.thresh = -18; q.ratio = 8; q.attackMs = 2; q.releaseMs = 100; q.eqLow = 0;
      Rack c1; c1.prepare(sr); c1.setParams(q); auto ol = runRack(c1, loud, 128); Rack c2; c2.prepare(sr); c2.setParams(q); auto oq = runRack(c2, quiet, 128);
      const double gl = 20 * std::log10(rms(ol, 12000, 24000) / rms(loud, 12000, 24000)), gq = 20 * std::log10(rms(oq, 12000, 24000) / rms(quiet, 12000, 24000));
      std::printf("compressor: loud tone %.1f dB, quiet tone %.1f dB (includes auto make-up)\n", gl, gq);
      CHECK(gl < gq - 5 && finiteAll(ol), "compressor squeezes the loud tone more than the quiet one"); CHECK(c1.gainReductionDb() < -5, "gain reduction is reported (%.1f dB)", c1.gainReductionDb()); }
}
// a parameter step is heard in the same block, and does not zipper
static void testRackResponsiveness() {
    const int B = 64; Rack r; r.prepare(sr); RackParams p; p.filterOn = true; p.cutoff = 16000; p.eqOn = false; r.setParams(p);
    auto in = tone(5000, 0, 1.0); std::vector<float> L = in, R = in; size_t pos = 0;
    for (int k = 0; k < 20; ++k, pos += B) r.process(L.data() + pos, R.data() + pos, B);
    const size_t stepAt = pos; p.cutoff = 300; r.setParams(p); size_t firstChange = 0;
    std::vector<float> ref = in; { Rack r2; r2.prepare(sr); r2.setParams(RackParams { true, 0, 16000, 0.7, false, 0, 0, 0 }); std::vector<float> a = in, b = in; r2.process(a.data(), b.data(), (int) in.size()); ref = a; }
    for (int k = 0; k < 40; ++k, pos += B) r.process(L.data() + pos, R.data() + pos, B);
    for (size_t i = stepAt; i < pos; ++i) if (std::fabs(L[i] - ref[i]) > 0.01f) { firstChange = i - stepAt; break; }
    std::printf("knob to sound: cutoff step is audible after %zu samples (%.2f ms), one block is %d samples\n", firstChange, 1000.0 * (double) firstChange / sr, B);
    CHECK(firstChange > 0 && firstChange < (size_t) B, "a rack knob change is heard within one buffer (%zu samples)", firstChange);
    CHECK(maxJump(L, stepAt, pos) < 0.9, "no zipper step when the cutoff jumps (largest step %.3f)", maxJump(L, stepAt, pos));
    // on/off switch crossfades: no click
    Rack c; c.prepare(sr); RackParams q; q.filterOn = false; q.eqOn = false; q.compOn = false; c.setParams(q); auto t2 = tone(500, 0, 0.5); std::vector<float> L2 = t2, R2 = t2;
    c.process(L2.data(), R2.data(), 6000); q.compOn = true; q.thresh = -40; q.ratio = 10; c.setParams(q); c.process(L2.data() + 6000, R2.data() + 6000, 18000);
    CHECK(maxJump(L2, 5000, 24000) < 0.1, "switching the compressor on does not click (%.3f)", maxJump(L2, 5000, 24000));
}
// ---------- 5. formant ----------
static double centroid(const std::vector<float>& v, size_t from, size_t to, double maxHz) {
    Fft f(8192); std::vector<double> re(8192), im(8192); double num = 0, den = 0;
    for (size_t st = from; st + 8192 < to; st += 8192) { for (int i = 0; i < 8192; ++i) { re[(size_t) i] = v[st + (size_t) i] * (0.5 - 0.5 * std::cos(2 * kPi * i / 8192)); im[(size_t) i] = 0; } f.run(re, im, false);
        for (int k = 1; k < 4096; ++k) { const double m = re[(size_t) k] * re[(size_t) k] + im[(size_t) k] * im[(size_t) k], hz = k * sr / 8192; if (hz < maxHz) { num += hz * m; den += m; } } }
    return num / std::max(den, 1e-30);
}
static std::vector<float> vowel(double f0, double secs) {
    const size_t n = (size_t) (sr * secs); std::vector<double> x(n, 0.0); for (double i = 0; i < (double) n; i += sr / f0) x[(size_t) i] = 1;
    auto res = [&](double fc, double bw) { const double r = std::exp(-kPi * bw / sr), c = -r * r, b = 2 * r * std::cos(2 * kPi * fc / sr); std::vector<double> y(n); double y1 = 0, y2 = 0; for (size_t i = 0; i < n; ++i) { const double v = x[i] * (1 - r) + b * y1 + c * y2; y2 = y1; y1 = v; y[i] = v; } return y; };
    auto a = res(700, 80), b = res(1200, 100); std::vector<float> o(n); for (size_t i = 0; i < n; ++i) o[i] = (float) (a[i] + 0.7 * b[i]); return o;
}
static void testFormant() {
    auto v = vowel(120, 2.0); Stereo s; s.L = v; s.R = v; const double c0 = centroid(v, 20000, v.size() - 8000, 6000);
    { Stereo up = s; bool ok = applyFormantShift(up, sr, [](double) { return 7.0; }); const double c1 = centroid(up.L, 20000, v.size() - 8000, 6000);
      std::printf("formant +7 st: spectral centroid %.0f -> %.0f Hz (x%.2f)\n", c0, c1, c1 / c0);
      CHECK(ok && c1 / c0 > 1.25 && c1 / c0 < 1.8, "formant +7 st moves the spectral envelope up (x%.2f)", c1 / c0);
      // pitch unchanged: the energy stays on multiples of 120 Hz
      std::vector<double> hs; for (int k = 1; k < 30; ++k) hs.push_back(120.0 * k); const double sh = avgShare(up.L, 20000, up.L.size() - 8000, hs), sh0 = avgShare(v, 20000, v.size() - 8000, hs);
      CHECK(sh > 0.85 && sh > sh0 - 0.1, "formant shift keeps the pitch (harmonic share %.2f, source %.2f)", sh, sh0);
      CHECK(finiteAll(up.L) && up.L.size() == v.size(), "formant output finite, same length"); }
    { Stereo dn = s; applyFormantShift(dn, sr, [](double) { return -7.0; }); const double c1 = centroid(dn.L, 20000, v.size() - 8000, 6000); CHECK(c1 / c0 < 0.85 && c1 / c0 > 0.45, "formant -7 st moves it down (x%.2f)", c1 / c0); }
    { Stereo z = s; applyFormantShift(z, sr, [](double) { return 0.0; }); bool same = true; for (size_t i = 0; i < v.size(); ++i) if (z.L[i] != v[i]) same = false; CHECK(same, "formant 0 is bit exact"); }
    { Stereo z = s; std::atomic<bool> c { true }; CHECK(!applyFormantShift(z, sr, [](double) { return 5.0; }, &c) && z.L == v, "a cancelled formant pass leaves the audio alone"); }
    { Stereo z = s; applyFormantShift(z, sr, [](double f) { return f < 0.5 ? 0.0 : 8.0; }); const double ca = centroid(z.L, 20000, 40000, 6000), cb = centroid(z.L, 60000, v.size() - 8000, 6000); CHECK(cb / ca > 1.2, "the shift can change over time (x%.2f)", cb / ca); }
    // end to end: Formant+ mode with a +7 st transpose keeps the formants, the plain mode lets them move with the pitch
    auto src = std::make_shared<Source>(); src->L = v; src->R = v; src->sr = sr; Request rq; rq.src = src; rq.p.mode = Mode::Complex; rq.p.semitones = 7;
    Rendered plain, keep; renderAll(rq, plain, nullptr); rq.p.mode = Mode::FormantPlus; renderAll(rq, keep, nullptr);
    const double cp = centroid(plain.audio.L, 20000, plain.audio.L.size() - 8000, 6000), ck = centroid(keep.audio.L, 20000, keep.audio.L.size() - 8000, 6000);
    std::printf("transpose +7: plain centroid %.0f Hz, Formant+ %.0f Hz (source %.0f Hz)\n", cp, ck, c0);
    CHECK(ck < cp * 0.85, "Formant+ keeps the formants down while plain transpose lets them rise (%.0f vs %.0f)", ck, cp);
    Params q; CHECK(!q.needsFormant(), "default params need no formant pass"); q.formantOn = true; q.formantSemis = 3; CHECK(q.needsFormant(), "shift asks for the formant pass");
}
int main() {
    testSpeed(); testPitch(); testTape(); testModes(); testLanes(); testSlices(); testSampler(); testLength();
    testBusAndWorker(); testSwapWhilePlaying(); testCrossfade(); testSliceSet(); testSlicePlay(); testSlicesFromRender(); testLiveLanes(); testRack(); testRackResponsiveness(); testFormant();
    if (fails) { std::printf("%d FAILED\n", fails); return 1; }
    std::printf("all tests passed\n"); return 0;
}
