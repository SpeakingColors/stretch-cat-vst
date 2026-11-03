// Renders the real editor offscreen to a PNG so it can be compared with the mockup, and runs self tests that need the plugin classes.
// usage: StretchCatSnapshot out.png [scale] [dialog none|settings|about|guide|presets|save] [lane 0..4] [pad 0..7 or -1]
//        StretchCatSnapshot --selftest [tests/fixtures/state-v1.xml]
#include "../src/stretch/StretchCat.h"

void stretchCatEditorTestHook(juce::AudioProcessorEditor*, const juce::String& dialog, int lane);

// A short drum and chord loop written as a wav, so the screenshots show a real sound and the self test has something to render.
static juce::File makeTestWav()
{
    auto f = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("stretchcat-test-loop.wav"); f.deleteFile();
    const double sr = 44100.0; const int n = (int) (sr * 3.0); juce::AudioBuffer<float> b(2, n); juce::Random r(11);
    for (int i = 0; i < n; ++i)
    {
        const double t = i / sr, tk = std::fmod(t, 0.75), th = std::fmod(t + 0.375, 0.75);
        float v = (float) (0.8 * std::exp(-tk * 12.0) * std::sin(6.2831853 * (50.0 + 110.0 * std::exp(-tk * 35.0)) * tk));
        v += (float) (0.25 * std::exp(-th * 40.0) * (r.nextFloat() * 2.f - 1.f));
        v += 0.1f * (float) (std::sin(6.2831853 * 220.0 * t) + 0.7 * std::sin(6.2831853 * 277.18 * t) + 0.5 * std::sin(6.2831853 * 329.63 * t));
        b.setSample(0, i, v); b.setSample(1, i, v * 0.95f);
    }
    juce::WavAudioFormat wav; if (auto os = f.createOutputStream()) if (std::unique_ptr<juce::AudioFormatWriter> w { wav.createWriterFor(os.get(), sr, 2, 16, {}, 0) }) { os.release(); w->writeFromAudioSampleBuffer(b, 0, n); }
    return f;
}

static void waitRender(StretchCatProcessor& p, int maxMs = 8000)
{
    for (int t = 0; t < maxMs / 20; ++t) { juce::MessageManager::getInstance()->runDispatchLoopUntil(20); if (!p.rendering() && p.resultLength() > 0) break; }
}

static void runBlocks(StretchCatProcessor& p, int blocks, juce::MidiBuffer midi = {})
{
    juce::AudioBuffer<float> buf(2, 512);
    for (int i = 0; i < blocks; ++i) { buf.clear(); juce::MidiBuffer m; if (i == 0) m = midi; p.processBlock(buf, m); juce::MessageManager::getInstance()->runDispatchLoopUntil(5); }
}

static int fixtureTest(const juce::File& f)
{
    int bad = 0; auto check = [&](bool ok, const char* what) { std::printf("%s: %s\n", ok ? "ok  " : "FAIL", what); if (!ok) ++bad; };
    StretchCatProcessor p; auto xml = juce::XmlDocument::parse(f); check(xml != nullptr, "v1 fixture parses"); if (!xml) return 1;
    juce::MemoryBlock mb; juce::AudioProcessor::copyXmlToBinary(*xml, mb); p.setStateInformation(mb.getData(), (int) mb.getSize());
    auto real = [&p](const char* id) { return p.apvts.getRawParameterValue(id)->load(); };
    check((int) real("mode") == 1 && (int) real("tape") == 1 && std::abs(real("grain") - 0.3f) < 0.01f && std::abs(real("speed") - 0.5f) < 0.01f, "stretch values from the v1 fixture");
    check((int) real("tr") == 7 && (int) real("fine") == -20 && (int) real("pb") == 2 && (int) real("len") == 5 && std::abs(real("hostbpm") - 140.f) < 0.1f, "pitch, playback, length from the v1 fixture");
    check(std::abs(real("atk") - 12.f) < 0.1f && std::abs(real("rel") - 300.f) < 0.5f && (int) real("root") == 48 && std::abs(real("gain") + 3.f) < 0.05f, "envelope, root and gain from the v1 fixture");
    check(real("fo") < 0.5f && real("eo") < 0.5f && real("co") < 0.5f && real("fmo") < 0.5f, "rack sections are off for a v1 state");
    check(p.getLane('p').pts.size() == 2 && p.getLane('c').pts.size() == 2, "lanes default to a flat line for a v1 state");
    return bad;
}

static int selfTest(const juce::File& fixture)
{
    int bad = 0; auto check = [&](bool ok, const char* what) { std::printf("%s: %s\n", ok ? "ok  " : "FAIL", what); if (!ok) ++bad; };
    StretchCatProcessor a, b; a.prepareToPlay(44100.0, 512);
    auto set = [](StretchCatProcessor& p, const char* id, float real) { p.setReal(id, real); };
    auto real = [](StretchCatProcessor& p, const char* id) { return p.apvts.getRawParameterValue(id)->load(); };
    check(std::abs(real(a, "speed") - 1.f) < 1e-4f && (int) real(a, "root") == 60 && real(a, "fo") < 0.5f, "defaults");
    // every choice control at every item, last one included
    for (const char* id : { "mode", "pb", "len", "ft" })
    {
        auto* c = dynamic_cast<juce::AudioParameterChoice*>(a.apvts.getParameter(id)); bool ok = c != nullptr;
        for (int i = 0; ok && i < c->choices.size(); ++i) { set(a, id, (float) i); ok = (int) real(a, id) == i; }
        check(ok, (juce::String("choice ") + id + " reads back at every item").toRawUTF8());
    }
    check(dynamic_cast<juce::AudioParameterChoice*>(a.apvts.getParameter("pb"))->choices.size() == 4, "playback has the Slices to keys item");
    // load a sound, render in the background, slice, and play a pad
    const auto wav = makeTestWav(); check(a.loadFile(wav), "test sound loads");
    waitRender(a); check(a.resultLength() > 0 && a.resultSeconds() > 2.9, "background render finished (length about 3 s)");
    a.slicesEven(); check(a.slices().count() == 8, "even slices make 8 slices");
    // state round trip with lanes, slices and rack values
    set(a, "fo", 1.f); set(a, "fc", 900.f); set(a, "fms", 4.f); set(a, "tr", 5.f);
    stretchcat::Lane l; l.pts = { { 0.0, 0.5 }, { 0.5, 0.75 }, { 1.0, 0.5 } }; a.setLane('p', l);
    juce::MemoryBlock mb; a.getStateInformation(mb); b.setStateInformation(mb.getData(), (int) mb.getSize());
    check(real(b, "fo") > 0.5f && std::abs(real(b, "fc") - 900.f) < 1.f && std::abs(real(b, "fms") - 4.f) < 0.05f && (int) real(b, "tr") == 5, "parameters survive a state round trip");
    check(b.getLane('p').pts.size() == 3 && b.slices().count() == 8 && b.hasSource(), "lanes, slices and the sound survive a state round trip");
    b.prepareToPlay(44100.0, 512); waitRender(b, 20000);
    // audio comes out, and a pad makes sound
 b.triggerSlice(2, true); float peak = 0.f; juce::AudioBuffer<float> buf(2, 512);
    for (int i = 0; i < 40; ++i) { buf.clear(); juce::MidiBuffer m; b.processBlock(buf, m); peak = std::max(peak, buf.getMagnitude(0, 512)); juce::MessageManager::getInstance()->runDispatchLoopUntil(5); }
    std::printf("info: pad peak %.5f, rendering %d, result length %d, slices %d\n", (double) peak, (int) b.rendering(), (int) b.resultLength(), (int) b.slices().count());
    check(peak > 0.01f && std::isfinite(peak), "a slice pad plays sound");
    check(b.getLatencySamples() == 0, "reported latency is exactly 0 samples");
    { std::unique_ptr<juce::AudioProcessorEditor> ed(b.createEditor()); ed->setSize(700, 470); ed->setSize(2800, 1880); check(ed->getWidth() == 2800, "editor creates and resizes"); }
    if (fixture != juce::File()) bad += fixtureTest(fixture);
    std::printf(bad == 0 ? "ALL SELF TESTS PASSED\n" : "SELF TESTS FAILED\n");
    return bad;
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc > 1 && juce::String(argv[1]) == "--selftest") return selfTest(argc > 2 ? juce::File(argv[2]) : juce::File()) == 0 ? 0 : 1;
    StretchCatProcessor proc; proc.prepareToPlay(44100.0, 512);
    const double sc = argc > 2 ? juce::String(argv[2]).getDoubleValue() : 1.0;
    const juce::String dialog = argc > 3 ? juce::String(argv[3]) : juce::String("none"); const int lane = argc > 4 ? juce::String(argv[4]).getIntValue() : -1;
    const int pad = argc > 5 ? juce::String(argv[5]).getIntValue() : -1;
    proc.loadFile(makeTestWav()); waitRender(proc); proc.slicesEven();
    proc.setReal("fo", 1.f); proc.setReal("fc", 3500.f);
    std::unique_ptr<juce::AudioProcessorEditor> ed(proc.createEditor());
    ed->setSize(juce::roundToInt(1400 * sc), juce::roundToInt(940 * sc));
    if (pad >= 0) proc.triggerSlice(pad, true); else proc.triggerPlay(true);
    waitRender(proc); runBlocks(proc, 60);
    stretchCatEditorTestHook(ed.get(), dialog, lane);
    juce::MessageManager::getInstance()->runDispatchLoopUntil(300);
    juce::Image img = ed->createComponentSnapshot(ed->getLocalBounds(), false, 1.0f);
    juce::File out(argv[1]); out.deleteFile(); juce::FileOutputStream os(out); juce::PNGImageFormat().writeImageToStream(img, os);
    ed.reset(); return 0;
}
