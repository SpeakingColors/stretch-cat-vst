// Stretch Cat: the background render (pure C++, std::thread). The audio thread never renders and never waits:
//  * the message thread posts a Request (latest one wins; a render in progress is cancelled when a newer request arrives),
//  * the worker thread renders into a free slot of the ResultBus and publishes it with one atomic store,
//  * the audio thread polls the bus (wait free), crossfades to the new slot and later tells the bus the old one is free.
// A slot is only reused when it is not published and not held by the audio thread (hold pointers, checked twice on pickup).
#pragma once
#include "StretchSampler.h"
#include "Formant.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace stretchcat {

class ResultBus {
public:
    static constexpr int N = 5;                       // published, current, previous, candidate, and one being written
    struct AudioHandle { int cur = -1, prev = -1; };
    Rendered& slot(int i) { return slots[(size_t) i]; }
    int publishedIndex() const { return published.load(); }
    // worker: a slot nobody can be reading. -1 when none (cannot happen with N = 5, kept for safety)
    int acquireFree() const {
        const int pub = published.load();
        for (int i = 0; i < N; ++i) { if (i == pub) continue; if (i == hold[0].load() || i == hold[1].load() || i == hold[2].load()) continue; return i; }
        return -1;
    }
    void publish(int i) { published.store(i); }
    // audio thread: true when a new slot was adopted; the previous current slot is kept in h.prev until releasePrev
    bool poll(AudioHandle& h) {
        const int p = published.load(); if (p < 0 || p == h.cur) return false;
        hold[2].store(p); if (published.load() != p) { hold[2].store(-1); return false; }      // the worker moved on: try again next block
        h.prev = h.cur; hold[1].store(h.prev); h.cur = p; hold[0].store(p); hold[2].store(-1); return true;
    }
    void releasePrev(AudioHandle& h) { h.prev = -1; hold[1].store(-1); }
private:
    std::array<Rendered, N> slots; std::atomic<int> published { -1 }; std::atomic<int> hold[3] { { -1 }, { -1 }, { -1 } };
};

struct Source { std::vector<float> L, R; double sr = 44100; };
struct Request { std::shared_ptr<const Source> src; Params p; double selA = 0.0, selB = 1.0; SliceSet slices; };

// What the editor needs to draw the result (message thread only).
struct UiResult {
    uint64_t id = 0; std::vector<float> peaks; double seconds = 0, ms = 0; std::vector<double> sliceFrac;   // slice edges as fractions of the render
    std::vector<std::pair<double, double>> map; long long length = 0;
};

inline std::vector<float> makePeaks(const std::vector<float>& l, const std::vector<float>& r, int bins) {
    std::vector<float> p((size_t) bins, 0.f); const size_t n = l.size(); if (n == 0) return p;
    for (int b = 0; b < bins; ++b) {
        const size_t a = n * (size_t) b / (size_t) bins, e = std::max(a + 1, n * (size_t) (b + 1) / (size_t) bins); float m = 0.f;
        for (size_t i = a; i < e && i < n; ++i) m = std::max(m, std::max(std::fabs(l[i]), std::fabs(r.size() == n ? r[i] : l[i])));
        p[(size_t) b] = m;
    }
    return p;
}

// The whole render: stretch, then the formant shift when it is needed. Returns false when cancelled.
inline bool renderAll(const Request& rq, Rendered& out, const std::atomic<bool>* cancel) {
    const auto t0 = std::chrono::steady_clock::now(); const Source& s = *rq.src; const long long n = (long long) s.L.size();
    const long long a = (long long) (rq.selA * (double) n), b = (long long) (rq.selB * (double) n);
    out.audio = render(s.L, s.R, a, b, rq.p, cancel);
    if (cancel && cancel->load()) return false;
    out.sr = s.sr;
    if (out.audio.L.size() >= 4 && rq.p.needsFormant()) {
        const auto& map = out.audio.map;
        auto semis = [&](double frac) { const double o = frac * (double) out.audio.L.size(); return rq.p.formantAt(std::max(0.0, std::min(1.0, (outToSource(map, o) - (double) a) / (double) std::max(1LL, b - a)))); };
        if (!applyFormantShift(out.audio, s.sr, semis, cancel)) return false;
    }
    out.renderMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    return true;
}
// Slice edges and the source-time curve for a finished render.
inline void finishRendered(const Request& rq, Rendered& r) {
    const long long n = (long long) rq.src->L.size(), a = (long long) (rq.selA * (double) n), b = (long long) (rq.selB * (double) n);
    r.sliceBounds = sliceBounds(rq.slices, r.audio.map, (double) n, r.length());
    for (int i = 0; i < 256; ++i) r.tCurve[(size_t) i] = (float) std::max(0.0, std::min(1.0, (outToSource(r.audio.map, (double) i / 255.0 * (double) r.length()) - (double) a) / (double) std::max(1LL, b - a)));
}

class RenderWorker {
public:
    explicit RenderWorker(ResultBus& b) : bus(b), th([this] { loop(); }) {}
    ~RenderWorker() { { std::lock_guard<std::mutex> l(m); quit = true; cancel.store(true); } cv.notify_all(); th.join(); }
    // message thread. Never blocks for long (a mutex that only the worker's short bookkeeping also takes).
    void post(const Request& r) { { std::lock_guard<std::mutex> l(m); pending = r; has = true; cancel.store(true); } cv.notify_all(); }
    bool fetchUi(UiResult& u) { std::lock_guard<std::mutex> l(m); if (!uiNew) return false; u = ui; uiNew = false; return true; }
    std::thread::id threadId() const { return th.get_id(); }
    bool idle() const { std::lock_guard<std::mutex> l(m); return !has && !busy; }
    uint64_t rendersDone() const { return done.load(); }
    std::function<void()> onRenderStart;              // test hook, called on the worker thread
private:
    void loop() {
        for (;;) {
            Request rq;
            { std::unique_lock<std::mutex> l(m); cv.wait(l, [this] { return has || quit; }); if (quit) return; rq = std::move(pending); has = false; cancel.store(false); busy = true; }
            if (onRenderStart) onRenderStart();
            int idx = bus.acquireFree(); while (idx < 0) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); idx = bus.acquireFree(); }
            Rendered& slot = bus.slot(idx); bool ok = true;
            const bool sameStretch = lastValid && lastSrc == rq.src && lastP == rq.p && lastA == rq.selA && lastB == rq.selB && bus.publishedIndex() >= 0;
            if (sameStretch) {                         // only the slices changed: copy the audio of the published render instead of stretching again
                const Rendered& pub = bus.slot(bus.publishedIndex()); slot.audio = pub.audio; slot.sr = pub.sr; slot.renderMs = pub.renderMs;
            } else ok = renderAll(rq, slot, &cancel);
            if (ok && slot.audio.L.size() >= 4) {
                finishRendered(rq, slot); slot.id = ++counter;
                lastValid = true; lastSrc = rq.src; lastP = rq.p; lastA = rq.selA; lastB = rq.selB;
                UiResult u; u.id = slot.id; u.peaks = makePeaks(slot.audio.L, slot.audio.R, 1024); u.seconds = (double) slot.length() / slot.sr; u.ms = slot.renderMs; u.map = slot.audio.map; u.length = slot.length();
                for (long long bnd : slot.sliceBounds) u.sliceFrac.push_back((double) bnd / (double) slot.length());
                bus.publish(idx);
                { std::lock_guard<std::mutex> l(m); ui = std::move(u); uiNew = true; }
            }
            done.fetch_add(1);
            { std::lock_guard<std::mutex> l(m); busy = false; }
        }
    }
    ResultBus& bus; mutable std::mutex m; std::condition_variable cv; Request pending; bool has = false, quit = false, busy = false, uiNew = false; UiResult ui;
    std::atomic<bool> cancel { false }; std::atomic<uint64_t> done { 0 };
    uint64_t counter = 0; bool lastValid = false; std::shared_ptr<const Source> lastSrc; Params lastP; double lastA = 0, lastB = 1;
    std::thread th;
};
}
