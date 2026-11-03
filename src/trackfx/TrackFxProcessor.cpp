#include "TrackFxProcessor.h"
#include "TrackFxEditorFactory.h"
using namespace juce;

static NormalisableRange<float> logRange(float lo, float hi) { NormalisableRange<float> r(lo, hi); r.setSkewForCentre(std::sqrt(lo * hi)); return r; }

AudioProcessorValueTreeState::ParameterLayout TrackFxProcessor::layout()
{
    AudioProcessorValueTreeState::ParameterLayout l;
    auto ms = [](const char* id, const char* name, NormalisableRange<float> r, float def, int dec) {
        return std::make_unique<AudioParameterFloat>(ParameterID { id, 1 }, name, r, def, AudioParameterFloatAttributes().withStringFromValueFunction([dec](float v, int) { return String(v, dec) + " ms"; })); };
    l.add(std::make_unique<AudioParameterBool>(ParameterID { "hold", 1 }, "Hold", false));
    l.add(std::make_unique<AudioParameterBool>(ParameterID { "loop", 1 }, "Loop", true));
    l.add(ms("start", "Start", NormalisableRange<float>(0.f, 8000.f), 1000.f, 0));
    l.add(ms("len", "Length", logRange(20.f, 4000.f), 500.f, 0));
    l.add(std::make_unique<AudioParameterFloat>(ParameterID { "speed", 1 }, "Speed", logRange(0.25f, 4.f), 1.f, AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) { return "x" + String(v, 2); })));
    l.add(std::make_unique<AudioParameterInt>(ParameterID { "key", 1 }, "Key to (Pitch)", -24, 24, 0, AudioParameterIntAttributes().withStringFromValueFunction([](int v, int) { return String(v > 0 ? "+" : "") + String(v) + " st"; })));
    l.add(ms("atk", "Attack", logRange(1.f, 500.f), 10.f, 0));
    l.add(ms("hld", "Grain hold", logRange(1.f, 1000.f), 60.f, 0));
    l.add(ms("spc", "Grain spacing", logRange(2.f, 500.f), 40.f, 0));
    l.add(ms("wav", "Wave spacing", NormalisableRange<float>(0.f, 100.f), 0.f, 1));
    l.add(std::make_unique<AudioParameterFloat>(ParameterID { "pan", 1 }, "Pan", NormalisableRange<float>(-1.f, 1.f), 0.f));
    l.add(std::make_unique<AudioParameterFloat>(ParameterID { "depth", 1 }, "Depth", NormalisableRange<float>(0.f, 1.f), 0.f));
    l.add(std::make_unique<AudioParameterFloat>(ParameterID { "rand", 1 }, "Rand", NormalisableRange<float>(0.f, 1.f), 0.f));
    l.add(std::make_unique<AudioParameterChoice>(ParameterID { "mode", 1 }, "Transient mode", StringArray { "Off", "Use regions", "Detect" }, 0));
    l.add(std::make_unique<AudioParameterFloat>(ParameterID { "amt", 1 }, "Hold amount", NormalisableRange<float>(0.05f, 1.f), 1.f));
    return l;
}

TrackFxProcessor::TrackFxProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", AudioChannelSet::stereo(), true).withOutput("Output", AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", layout())
{
    for (auto& c : cols) c.store(0.f);
}

void TrackFxProcessor::prepareToPlay(double sampleRate, int)
{
    fs = sampleRate; fx.prepare(sampleRate, 12.0);       // 12 s ring: the start can reach 8 s back and the part can be 4 s long
    // "Use regions": there are no slice markers in this plugin, so the held part is cut into 8 equal regions
    const double fr[7] = { 1.0 / 8, 2.0 / 8, 3.0 / 8, 4.0 / 8, 5.0 / 8, 6.0 / 8, 7.0 / 8 }; fx.setRegions(fr, 7);
    lastHold = false; colSamples = sampleRate * kHistorySeconds / (double) kCols; colPos = 0; colPeak = 0.f;
    for (auto& c : cols) c.store(0.f);
    wcol.store(0); heldNow.store(false);
}

void TrackFxProcessor::processBlock(AudioBuffer<float>& b, MidiBuffer&)
{
    ScopedNoDenormals nd; const int n = b.getNumSamples(); if (b.getNumChannels() < 2 || n == 0) return;
    float* L = b.getWritePointer(0); float* R = b.getWritePointer(1);
    // history of the incoming track for the window (before the effect touches it)
    for (int i = 0; i < n; ++i)
    {
        colPeak = std::max(colPeak, std::max(std::fabs(L[i]), std::fabs(R[i])));
        if ((colPos += 1.0) >= colSamples) { colPos -= colSamples; const int w = wcol.load(std::memory_order_relaxed); cols[(size_t) (w % kCols)].store(colPeak, std::memory_order_relaxed); wcol.store(w + 1, std::memory_order_release); colPeak = 0.f; }
    }
    stretchcat::TrackParams p;
    p.startMs = get("start"); p.lengthMs = get("len"); p.loop = get("loop") > 0.5f; p.keySemis = get("key"); p.speed = get("speed");
    p.attackMs = get("atk"); p.holdMs = get("hld"); p.spacingMs = get("spc"); p.waveSpacingMs = get("wav");
    p.pan = get("pan"); p.depth = get("depth"); p.rand = get("rand");
    const int m = juce::roundToInt(get("mode")); p.trMode = m == 1 ? stretchcat::TrMode::Regions : m == 2 ? stretchcat::TrMode::Detect : stretchcat::TrMode::Off;
    p.holdAmount = get("amt");
    fx.setParams(p);
    const bool h = get("hold") > 0.5f; if (h != lastHold) { fx.setHold(h); lastHold = h; }     // applies at the start of the block
    fx.process(L, R, n);
    heldNow.store(fx.isHeld()); playMs.store((float) fx.playheadMs()); heldMs.store((float) fx.heldLengthMs());
    float pl = 0.f, pr = 0.f; for (int i = 0; i < n; ++i) { pl = std::max(pl, std::fabs(L[i])); pr = std::max(pr, std::fabs(R[i])); }
    mL.store(pl); mR.store(pr);
}

void TrackFxProcessor::getStateInformation(MemoryBlock& dest)
{
    auto s = apvts.copyState(); s.setProperty("version", kStateVersion, nullptr);
    if (auto xml = s.createXml()) copyXmlToBinary(*xml, dest);
}
void TrackFxProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size)) if (xml->hasTagName(apvts.state.getType())) apvts.replaceState(ValueTree::fromXml(*xml));
}
AudioProcessorEditor* TrackFxProcessor::createEditor() { return makeTrackFxEditor(*this); }
