// Self tests and an offscreen screenshot for Stretch Cat Track Effect (plugin classes, no host needed).
// usage: StretchCatTrackSnapshot --selftest        |        StretchCatTrackSnapshot out.png [scale]
#include "../src/trackfx/TrackFxProcessor.h"
#include <cstring>

static float tone(int i, double sr) { const double t = i / sr, tk = std::fmod(t, 0.5); return (float) (0.7 * std::exp(-tk * 9.0) * std::sin(6.2831853 * (60.0 + 90.0 * std::exp(-tk * 30.0)) * tk) + 0.1 * std::sin(6.2831853 * 330.0 * t)); }

// feeds `blocks` blocks of 256 samples through the processor; returns the last block output, and `all` collects every output sample (left)
static void run(TrackFxProcessor& p, int& pos, int blocks, std::vector<float>* all = nullptr, std::vector<float>* in = nullptr, double sr = 48000.0)
{
    juce::AudioBuffer<float> b(2, 256); juce::MidiBuffer m;
    for (int k = 0; k < blocks; ++k)
    {
        for (int i = 0; i < 256; ++i) { const float v = tone(pos + i, sr); b.setSample(0, i, v); b.setSample(1, i, v * 0.9f); if (in) in->push_back(v); }
        p.processBlock(b, m); pos += 256;
        if (all) for (int i = 0; i < 256; ++i) all->push_back(b.getSample(0, i));
    }
}

static int selfTest()
{
    int bad = 0; auto check = [&](bool ok, const char* what) { std::printf("%s: %s\n", ok ? "ok  " : "FAIL", what); if (!ok) ++bad; };
    const double sr = 48000.0;
    auto setp = [](TrackFxProcessor& p, const char* id, float real) { if (auto* q = p.apvts.getParameter(id)) q->setValueNotifyingHost(q->convertTo0to1(real)); };
    TrackFxProcessor p; p.setPlayConfigDetails(2, 2, sr, 256); p.prepareToPlay(sr, 256); int pos = 0;
    check(p.getLatencySamples() == 0, "latency is 0 samples");
    // 1. not held: bit exact dry
    std::vector<float> out, in; run(p, pos, 200, &out, &in, sr);
    bool same = out.size() == in.size(); for (size_t i = 0; same && i < out.size(); ++i) same = out[i] == in[i];
    check(same, "HOLD off: the audio passes bit exactly");
    check(p.writeIndex() >= 50 && p.writeIndex() <= 56, "the 8 second history fills while listening");
    // 2. hold: different from live, not silent, no huge click
    setp(p, "start", 600.f); setp(p, "len", 300.f); setp(p, "speed", 1.f); setp(p, "loop", 1.f); setp(p, "hold", 1.f);
    out.clear(); in.clear(); run(p, pos, 300, &out, &in, sr);
    double eo = 0, ed = 0; float pk = 0; for (size_t i = 3000; i < out.size(); ++i) { eo += (double) out[i] * out[i]; const double d = out[i] - in[i]; ed += d * d; pk = std::max(pk, std::fabs(out[i])); }
    check(eo > 1e-3, "HOLD on: the held part is audible");
    check(ed > 0.05 * eo, "HOLD on: the output is not the live signal any more");
    check(pk < 1.5f, "HOLD on: no runaway level");
    check(p.holding() && p.heldLengthMs() > 250.f && p.heldLengthMs() < 350.f, "the held length follows the Length control");
    check(p.meterL() > 0.f, "the output meter moves");
    // 3. release returns to bit exact dry after the crossfade
    setp(p, "hold", 0.f); out.clear(); in.clear(); run(p, pos, 100, &out, &in, sr);
    bool back = true; for (size_t i = 256 * 20; i < out.size(); ++i) if (out[i] != in[i]) { back = false; break; }
    check(back, "released: bit exact dry again");
    // 4. loop off: plays once, then the live signal returns by itself
    setp(p, "loop", 0.f); setp(p, "len", 200.f); setp(p, "hold", 1.f); out.clear(); in.clear(); run(p, pos, 200, &out, &in, sr);
    bool live = true; for (size_t i = out.size() - 256 * 10; i < out.size(); ++i) if (out[i] != in[i]) { live = false; break; }
    check(live, "loop off: after the one pass the live signal is back");
    setp(p, "hold", 0.f); run(p, pos, 20);
    // 5. state round trip
    setp(p, "len", 1234.f); setp(p, "key", 7.f); setp(p, "mode", 2.f); juce::MemoryBlock mb; p.getStateInformation(mb);
    TrackFxProcessor q; q.setPlayConfigDetails(2, 2, sr, 256); q.prepareToPlay(sr, 256); q.setStateInformation(mb.getData(), (int) mb.getSize());
    auto real = [](TrackFxProcessor& x, const char* id) { return x.apvts.getRawParameterValue(id)->load(); };
    check(std::fabs(real(q, "len") - 1234.f) < 2.f && (int) real(q, "key") == 7 && (int) real(q, "mode") == 2, "state saves and loads");
    // 6. every mode and extreme settings stay finite and bounded
    for (int m = 0; m < 3; ++m) for (float spd : { 0.25f, 4.f }) for (int key : { -24, 24 })
    {
        TrackFxProcessor z; z.setPlayConfigDetails(2, 2, sr, 256); z.prepareToPlay(sr, 256); int zp = 0; run(z, zp, 100);
        setp(z, "mode", (float) m); setp(z, "speed", spd); setp(z, "key", (float) key); setp(z, "depth", 1.f); setp(z, "rand", 1.f); setp(z, "spc", 2.f); setp(z, "len", 4000.f); setp(z, "start", 8000.f); setp(z, "hold", 1.f);
        std::vector<float> o; run(z, zp, 400, &o); bool ok = true; for (float v : o) if (!std::isfinite(v) || std::fabs(v) > 8.f) ok = false;
        if (!ok) std::printf("  mode %d speed %.2f key %d\n", m, spd, key); check(ok, "extreme settings stay finite and bounded");
    }
    return bad;
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc > 1 && std::strcmp(argv[1], "--selftest") == 0) return selfTest() == 0 ? 0 : 1;
    const double sc = argc > 2 ? juce::String(argv[2]).getDoubleValue() : 1.0, sr = 48000.0;
    TrackFxProcessor p; p.setPlayConfigDetails(2, 2, sr, 256); p.prepareToPlay(sr, 256); int pos = 0;
    // 7 s of a drum and chord loop so the history shows a real signal, then hold a part of it
    run(p, pos, (int) (sr * 7.0 / 256.0), nullptr, nullptr, sr);
    if (auto* q = p.apvts.getParameter("hold")) q->setValueNotifyingHost(1.f);
    run(p, pos, 60, nullptr, nullptr, sr);
    std::unique_ptr<juce::AudioProcessorEditor> ed(p.createEditor()); ed->setSize(juce::roundToInt(1400 * sc), juce::roundToInt(830 * sc));
    juce::MessageManager::getInstance()->runDispatchLoopUntil(400);
    juce::Image img = ed->createComponentSnapshot(ed->getLocalBounds(), false, 1.0f);
    juce::File out(argv[1]); out.deleteFile(); juce::FileOutputStream os(out); juce::PNGImageFormat().writeImageToStream(img, os);
    ed.reset(); return 0;
}
