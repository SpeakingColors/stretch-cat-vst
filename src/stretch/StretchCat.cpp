#include "StretchCat.h"

using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;
using juce::NormalisableRange;

static const char* kStretchIds[] = { "mode", "tape", "grain", "trans", "smooth", "speed", "tr", "fine", "len", "hostbpm" };

Layout StretchCatProcessor::layout()
{
    Layout l;
    auto f = [&l](const char* id, const char* name, NormalisableRange<float> r, float def) { l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { id, 1 }, name, r, def)); };
    l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "mode", 1 }, "Stretch mode", juce::StringArray { "Beats", "Tones", "Texture", "Re-Pitch", "Complex", "Formant+" }, 4));
    l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "tape", 1 }, "Speed and pitch linked", false));
    f("grain", "Grain", { 0.f, 1.f }, 0.5f); f("trans", "Transients", { 0.f, 1.f }, 0.5f); f("smooth", "Smooth", { 0.f, 1.f }, 0.7f);
    NormalisableRange<float> sp(0.25f, 4.f); sp.setSkewForCentre(1.f); f("speed", "Speed", sp, 1.f);
    l.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID { "tr", 1 }, "Transpose", -24, 24, 0));
    l.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID { "fine", 1 }, "Fine (cents)", -100, 100, 0));
    l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "pb", 1 }, "Playback", juce::StringArray { "Loop", "One shot", "Gate" }, 0));
    l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "len", 1 }, "Length", juce::StringArray { "Auto", "1/16 note", "1/8 note", "1/4 note", "1/2 note", "1 bar", "2 bars", "4 bars", "8 bars", "16 bars", "32 bars" }, 0));
    f("hostbpm", "Fit tempo (BPM)", { 40.f, 240.f }, 120.f);
    f("atk", "Attack", { 0.f, 500.f }, 5.f); f("rel", "Release", { 0.f, 2000.f }, 80.f);
    l.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID { "root", 1 }, "Root note", 24, 96, 60));
    f("gain", "Gain", { -24.f, 12.f }, 0.f);
    return l;
}

StretchCatProcessor::StretchCatProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)), apvts(*this, nullptr, "STATE", layout())
{
    for (auto* id : kStretchIds) apvts.addParameterListener(id, this);
    retired.reserve(64);
    startTimer(120);
}
StretchCatProcessor::~StretchCatProcessor() { stopTimer(); for (auto* id : kStretchIds) apvts.removeParameterListener(id, this); }

void StretchCatProcessor::parameterChanged(const juce::String&, float) { dirty = true; }

void StretchCatProcessor::prepareToPlay(double sr, int) { fs = sr; sampler.prepare(sr); }

bool StretchCatProcessor::loadFile(const juce::File& f)
{
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> r(fm.createReaderFor(f));
    if (r == nullptr || r->lengthInSamples < 64 || r->lengthInSamples > (juce::int64) r->sampleRate * 600) { statusText = "Could not read " + f.getFileName() + " (or longer than 10 minutes)."; return false; }
    juce::AudioBuffer<float> b((int) r->numChannels, (int) r->lengthInSamples); r->read(&b, 0, (int) r->lengthInSamples, 0, true, true);
    srcL.assign(b.getReadPointer(0), b.getReadPointer(0) + b.getNumSamples());
    srcR.assign(b.getReadPointer(b.getNumChannels() > 1 ? 1 : 0), b.getReadPointer(b.getNumChannels() > 1 ? 1 : 0) + b.getNumSamples());
    srcSr = r->sampleRate; fileName = f.getFileName(); lastFile = f; dirty = true;
    statusText = "Loaded " + fileName + ", rendering...";
    return true;
}

void StretchCatProcessor::timerCallback()
{
    {   // follow the host tempo while a length is chosen
        const double hb = hostBpmSeen.load();
        if (hb >= 40.0 && hb <= 240.0 && (int) get("len") != 0 && std::fabs(hb - (double) get("hostbpm")) > 0.01)
            if (auto* pr = apvts.getParameter("hostbpm")) pr->setValueNotifyingHost(pr->convertTo0to1((float) hb));
    }
    if (dirty.exchange(false) && !srcL.empty()) renderNow();
    if (retired.size() > 32) { const juce::SpinLock::ScopedTryLockType tl(handoff); if (tl.isLocked()) retired.erase(retired.begin(), retired.end() - 4); }
}

void StretchCatProcessor::renderNow()
{
    stretchcat::Params p;
    p.mode = (stretchcat::Mode) (int) get("mode"); p.tapeLink = get("tape") > 0.5f;
    p.grain = get("grain"); p.transients = get("trans"); p.smooth = get("smooth"); p.speed = get("speed");
    p.semitones = get("tr") + get("fine") / 100.0;
    {   // Length: fit the whole sample into the chosen number of beats at the fit tempo (host tempo is not read while rendering off the audio thread)
        int cnt = 0; const double* beats = stretchcat::lengthBeats(cnt); const int li = juce::jlimit(0, cnt - 1, (int) get("len"));
        const double fit = stretchcat::fitSpeed((double) srcL.size(), srcSr, get("hostbpm"), beats[li]);
        if (fit > 0.0) p.speed = fit;
    }
    auto out = std::make_shared<stretchcat::Stereo>(stretchcat::render(srcL, srcR, 0, (long long) srcL.size(), p));      // TODO: move to a background thread
    if (out->L.size() < 4) { statusText = "Nothing to play."; return; }
    peaksSrc = makePeaks(srcL, srcR, 1024); peaksRes = makePeaks(out->L, out->R, 1024); resSeconds = (double) out->L.size() / srcSr; ++revision;
    { const juce::SpinLock::ScopedLockType sl(handoff); pending = out; pendingSr = srcSr; hasPending = true; }
    statusText = fileName + ": " + juce::String((double) out->L.size() / srcSr, 2) + " s";
}

void StretchCatProcessor::processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    buf.clear();
    if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition()) if (auto b = pos->getBpm()) hostBpmSeen.store(*b);
    {   // pick up a freshly rendered buffer (the old one is parked, not freed, so no free() on the audio thread)
        const juce::SpinLock::ScopedTryLockType tl(handoff);
        if (tl.isLocked() && hasPending && retired.size() < retired.capacity()) { sampler.setBuffer(pending, pendingSr); retired.push_back(pending); hasPending = false; }
    }
    stretchcat::SamplerParams sp;
    sp.mode = (stretchcat::Playback) (int) get("pb"); sp.attackMs = get("atk"); sp.releaseMs = get("rel"); sp.root = (int) get("root"); sp.gainDb = get("gain");
    const int n = buf.getNumSamples(); int done = 0;
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage(); const int at = juce::jlimit(0, n, meta.samplePosition);
        if (at > done) { sampler.process(sp, buf.getWritePointer(0) + done, buf.getWritePointer(1) + done, at - done); done = at; }
        if (m.isNoteOn()) sampler.noteOn(m.getNoteNumber(), m.getFloatVelocity(), sp);
        else if (m.isNoteOff()) sampler.noteOff(m.getNoteNumber(), sp);
        else if (m.isAllNotesOff() || m.isAllSoundOff()) sampler.allOff();
    }
    if (n > done) sampler.process(sp, buf.getWritePointer(0) + done, buf.getWritePointer(1) + done, n - done);
}

std::vector<float> StretchCatProcessor::makePeaks(const std::vector<float>& l, const std::vector<float>& r, int bins)
{
    std::vector<float> p((size_t) bins, 0.f); const size_t n = l.size(); if (n == 0) return p;
    for (int b = 0; b < bins; ++b)
    {
        const size_t a = n * (size_t) b / (size_t) bins, e = std::max(a + 1, n * (size_t) (b + 1) / (size_t) bins); float m = 0.f;
        for (size_t i = a; i < e && i < n; ++i) m = std::max(m, std::max(std::fabs(l[i]), std::fabs(r.size() == n ? r[i] : l[i])));
        p[(size_t) b] = m;
    }
    return p;
}


void StretchCatProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto st = apvts.copyState(); st.setProperty("file", lastFile.getFullPathName(), nullptr);
    if (auto xml = st.createXml()) copyXmlToBinary(*xml, dest);
}
void StretchCatProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size)) if (xml->hasTagName(apvts.state.getType()))
    {
        auto st = juce::ValueTree::fromXml(*xml); const juce::File f(st.getProperty("file").toString());
        apvts.replaceState(st);
        if (f.existsAsFile()) loadFile(f);
    }
}
