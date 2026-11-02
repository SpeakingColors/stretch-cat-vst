#include "StretchCat.h"

using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;
using juce::NormalisableRange;

// Parameters that need a new stretch render when they change.
static const char* kRenderIds[] = { "mode", "tape", "grain", "trans", "smooth", "speed", "tr", "fine", "len", "hostbpm", "sela", "selb", "fmo", "fms", "fmk" };

Layout StretchCatProcessor::layout()
{
    Layout l;
    auto f = [&l](const char* id, const char* name, NormalisableRange<float> r, float def, int ver = 1) { l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { id, ver }, name, r, def)); };
    auto b = [&l](const char* id, const char* name, bool def) { l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { id, 2 }, name, def)); };
    // ---- version 1 parameters: ids and order never change (saved projects and automation) ----
    l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "mode", 1 }, "Stretch mode", juce::StringArray { "Beats", "Tones", "Texture", "Re-Pitch", "Complex", "Formant+" }, 4));
    l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "tape", 1 }, "Speed and pitch linked", false));
    f("grain", "Grain", { 0.f, 1.f }, 0.5f); f("trans", "Transients", { 0.f, 1.f }, 0.5f); f("smooth", "Smooth", { 0.f, 1.f }, 0.7f);
    NormalisableRange<float> sp(0.25f, 4.f); sp.setSkewForCentre(1.f); f("speed", "Speed", sp, 1.f);
    l.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID { "tr", 1 }, "Transpose", -24, 24, 0));
    l.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID { "fine", 1 }, "Fine (cents)", -100, 100, 0));
    l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "pb", 1 }, "Playback", juce::StringArray { "Loop", "One shot", "Gate", "Slices to keys" }, 0));   // item 3 is new in 0.2
    l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "len", 1 }, "Length", juce::StringArray { "Auto", "1/16 note", "1/8 note", "1/4 note", "1/2 note", "1 bar", "2 bars", "4 bars", "8 bars", "16 bars", "32 bars" }, 0));
    f("hostbpm", "Fit tempo (BPM)", { 40.f, 240.f }, 120.f);
    f("atk", "Attack", { 0.f, 500.f }, 5.f); f("rel", "Release", { 0.f, 2000.f }, 80.f);
    l.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID { "root", 1 }, "Root note", 24, 96, 60));
    f("gain", "Gain", { -24.f, 12.f }, 0.f);
    // ---- new in 0.2 (state version 2): old projects load with these defaults, which leave the sound unchanged ----
    f("sela", "Selection start", { 0.f, 1.f }, 0.f, 2); f("selb", "Selection end", { 0.f, 1.f }, 1.f, 2);
    b("fo", "Filter on", false);
    l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "ft", 2 }, "Filter type", juce::StringArray { "Low-pass", "High-pass", "Band-pass", "Notch" }, 0));
    NormalisableRange<float> fc(40.f, 16000.f); fc.setSkewForCentre(800.f); f("fc", "Filter cutoff", fc, 16000.f, 2);
    NormalisableRange<float> fr(0.5f, 12.f); fr.setSkewForCentre(2.f); f("fr", "Filter resonance", fr, 0.7f, 2);
    b("eo", "EQ on", false); f("elo", "EQ low", { -18.f, 18.f }, 0.f, 2); f("emi", "EQ mid", { -18.f, 18.f }, 0.f, 2); f("ehi", "EQ high", { -18.f, 18.f }, 0.f, 2);
    b("co", "Compressor on", false); f("cth", "Comp threshold", { -60.f, 0.f }, -18.f, 2);
    NormalisableRange<float> cr(1.f, 20.f); cr.setSkewForCentre(4.f); f("crt", "Comp ratio", cr, 3.f, 2);
    NormalisableRange<float> ca(1.f, 100.f); ca.setSkewForCentre(10.f); f("cat", "Comp attack", ca, 10.f, 2);
    NormalisableRange<float> crl(20.f, 1000.f); crl.setSkewForCentre(150.f); f("crl", "Comp release", crl, 120.f, 2);
    b("fmo", "Formant on", false); f("fms", "Formant shift", { -12.f, 12.f }, 0.f, 2); b("fmk", "Formant keep", true);
    return l;
}

StretchCatProcessor::StretchCatProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)), apvts(*this, nullptr, "STATE", layout())
{
    for (auto* id : kRenderIds) apvts.addParameterListener(id, this);
    sampler.setLanes(&live);
    for (auto& l : lanes) l.pts = { { 0.0, 0.5 }, { 1.0, 0.5 } };
    lanes[2].pts = { { 0.0, 1.0 }, { 1.0, 1.0 } };          // cutoff lane: open
    updateLiveLanes();
    startTimer(20);
}
StretchCatProcessor::~StretchCatProcessor() { stopTimer(); for (auto* id : kRenderIds) apvts.removeParameterListener(id, this); }

void StretchCatProcessor::parameterChanged(const juce::String&, float) { dirty = true; }       // may be called from any thread: only sets a flag

void StretchCatProcessor::prepareToPlay(double sr, int)
{
    fs = sr; sampler.prepare(sr); rack.prepare(sr); handle = {};
    // a fresh sampler has no buffer: pick up the newest published render at once
    if (bus.publishedIndex() >= 0 && bus.poll(handle)) { sampler.swapTo(&bus.slot(handle.cur)); }
}

// ------------------------------------------------------------------ source

void StretchCatProcessor::applySource(std::shared_ptr<const stretchcat::Source> s)
{
    source = std::move(s); peaksSrc = stretchcat::makePeaks(source->L, source->R, 1024); ++revision; dirty = true;
}

bool StretchCatProcessor::loadFile(const juce::File& f)
{
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> r(fm.createReaderFor(f));
    if (r == nullptr || r->lengthInSamples < 64 || r->lengthInSamples > (juce::int64) r->sampleRate * 600) return false;
    juce::AudioBuffer<float> b((int) r->numChannels, (int) r->lengthInSamples); r->read(&b, 0, (int) r->lengthInSamples, 0, true, true);
    auto s = std::make_shared<stretchcat::Source>();
    s->L.assign(b.getReadPointer(0), b.getReadPointer(0) + b.getNumSamples());
    s->R.assign(b.getReadPointer(b.getNumChannels() > 1 ? 1 : 0), b.getReadPointer(b.getNumChannels() > 1 ? 1 : 0) + b.getNumSamples());
    s->sr = r->sampleRate; fileName = f.getFileName(); lastFile = f; original = s;
    auto edited = std::make_shared<stretchcat::Source>(*s);
    if (srcNorm) { float pk = 0.f; for (size_t i = 0; i < edited->L.size(); ++i) pk = std::max(pk, std::max(std::fabs(edited->L[i]), std::fabs(edited->R[i]))); if (pk > 1e-6f) for (size_t i = 0; i < edited->L.size(); ++i) { edited->L[i] *= 0.98f / pk; edited->R[i] *= 0.98f / pk; } }
    if (srcRev) { std::reverse(edited->L.begin(), edited->L.end()); std::reverse(edited->R.begin(), edited->R.end()); }
    applySource(edited);
    if (sliceSet.marks.empty()) sliceSet.even(selA(), selB());
    return true;
}
void StretchCatProcessor::normalizeSource()
{
    if (!source) return;
    auto e = std::make_shared<stretchcat::Source>(*source); float pk = 0.f;
    for (size_t i = 0; i < e->L.size(); ++i)
        pk = std::max(pk, std::max(std::fabs(e->L[i]), std::fabs(e->R[i])));
    if (pk < 1e-6f) return;
    const float g = 0.98f / pk;
    for (size_t i = 0; i < e->L.size(); ++i) { e->L[i] *= g; e->R[i] *= g; }
    srcNorm = true; applySource(e);
}
void StretchCatProcessor::reverseSource()
{
    if (!source) return;
    auto e = std::make_shared<stretchcat::Source>(*source); std::reverse(e->L.begin(), e->L.end()); std::reverse(e->R.begin(), e->R.end());
    srcRev = !srcRev;
    // the selection and the slice marks flip with the audio
    const double a = selA(), bb = selB(); stretchcat::SliceSet s; for (auto it = sliceSet.marks.rbegin(); it != sliceSet.marks.rend(); ++it) s.marks.push_back(1.0 - *it);
    sliceSet = s; setReal("sela", (float) (1.0 - bb)); setReal("selb", (float) (1.0 - a)); applySource(e);
}
void StretchCatProcessor::resetSource() { if (!original) return; srcNorm = srcRev = false; applySource(original); }

void StretchCatProcessor::setSelection(double a, double b)
{
    a = juce::jlimit(0.0, 0.98, a); b = juce::jlimit(a + 0.02, 1.0, b);
    setReal("sela", (float) a); setReal("selb", (float) b); sliceSet.sanitise(a, b); dirty = true;
}
void StretchCatProcessor::setSlices(const stretchcat::SliceSet& s) { sliceSet = s; sliceSet.sanitise(selA(), selB()); dirty = true; }
void StretchCatProcessor::slicesEven() { sliceSet.even(selA(), selB()); dirty = true; }
void StretchCatProcessor::slicesByTransient() { if (source) { sliceSet.byTransient(source->L, selA(), selB(), source->sr); dirty = true; } }

// ------------------------------------------------------------------ lanes

stretchcat::Lane StretchCatProcessor::getLane(char id) const { return lanes[laneIndex(id)]; }
void StretchCatProcessor::updateLiveLanes() { live.cutoff.set(lanes[2]); live.width.set(lanes[4]); }
void StretchCatProcessor::setLane(char id, const stretchcat::Lane& l)
{
    lanes[laneIndex(id)] = l; updateLiveLanes();
    if (id == 'p' || id == 's' || id == 'f') dirty = true;      // cutoff and width act live on the audio thread, no render needed
}
void StretchCatProcessor::saveLanesToState()
{
    for (char c : { 'p', 's', 'c', 'f', 'w' })
    {
        juce::StringArray pts; for (auto& p : lanes[laneIndex(c)].pts) pts.add(juce::String(p.first, 5) + ":" + juce::String(p.second, 5));
        apvts.state.setProperty(juce::String("ln_") + c, pts.joinIntoString(","), nullptr);
    }
}
static stretchcat::Lane parseLane(const juce::String& s, double flat)
{
    stretchcat::Lane l; l.pts.clear();
    for (auto& t : juce::StringArray::fromTokens(s, ",", "")) { auto kv = juce::StringArray::fromTokens(t, ":", ""); if (kv.size() == 2) { const double x = kv[0].getDoubleValue(), y = kv[1].getDoubleValue(); if (std::isfinite(x) && std::isfinite(y)) l.pts.push_back({ juce::jlimit(0.0, 1.0, x), juce::jlimit(0.0, 1.0, y) }); } }
    std::sort(l.pts.begin(), l.pts.end()); if (l.pts.size() < 2) l.pts = { { 0.0, flat }, { 1.0, flat } };
    return l;
}

// ------------------------------------------------------------------ render requests

void StretchCatProcessor::requestRender()
{
    if (!source) return;
    stretchcat::Request rq; rq.src = source; auto& p = rq.p;
    p.mode = (stretchcat::Mode) (int) get("mode"); p.tapeLink = get("tape") > 0.5f;
    p.grain = get("grain"); p.transients = get("trans"); p.smooth = get("smooth"); p.speed = get("speed");
    p.semitones = get("tr") + get("fine") / 100.0; p.speedLane = lanes[1]; p.pitchLane = lanes[0];
    p.formantOn = get("fmo") > 0.5f; p.formantSemis = get("fms"); p.formantKeep = get("fmk") > 0.5f; p.formantLane = lanes[3];
    rq.selA = selA(); rq.selB = std::max(selB(), rq.selA + 0.02); rq.slices = sliceSet;
    {   // Length: fit the selection into the chosen number of beats at the fit tempo (the host tempo is not read on the render thread)
        int cnt = 0; const double* beats = stretchcat::lengthBeats(cnt); const int li = juce::jlimit(0, cnt - 1, (int) get("len"));
        const double fit = stretchcat::fitSpeed((double) source->L.size() * (rq.selB - rq.selA), source->sr, get("hostbpm"), beats[li]);
        if (fit > 0.0) p.speed = fit;
    }
    worker.post(rq); pendingRender = true;
}

void StretchCatProcessor::timerCallback()
{
    {   // follow the host tempo while a length is chosen
        const double hb = hostBpmSeen.load();
        if (hb >= 40.0 && hb <= 240.0 && (int) get("len") != 0 && std::fabs(hb - (double) get("hostbpm")) > 0.01)
            if (auto* pr = apvts.getParameter("hostbpm")) pr->setValueNotifyingHost(pr->convertTo0to1((float) hb));
    }
    if (dirty.exchange(false)) requestRender();
    stretchcat::UiResult u; bool got = false; while (worker.fetchUi(u)) got = true;
    if (got) { ui = std::move(u); ++revision; }
    if (pendingRender && worker.idle()) pendingRender = false;
}

// ------------------------------------------------------------------ pads and presets

void StretchCatProcessor::triggerSlice(int index, bool on)
{
    int s1, s2, b1, b2; padFifo.prepareToWrite(1, s1, b1, s2, b2);
    if (b1 > 0) { padBuf[s1] = (index + 1) * 2 + (on ? 1 : 0); padFifo.finishedWrite(1); }
}
void StretchCatProcessor::triggerPlay(bool on) { triggerSlice(-1, on); }

juce::ValueTree StretchCatProcessor::capturePreset()
{
    auto st = apvts.copyState(); juce::ValueTree p("PRESET");
    for (auto c : st) { const auto id = c.getProperty("id").toString(); if (id == "sela" || id == "selb") continue; p.appendChild(c.createCopy(), nullptr); }
    for (char c : { 'p', 's', 'c', 'f', 'w' }) { juce::StringArray pts; for (auto& q : lanes[laneIndex(c)].pts) pts.add(juce::String(q.first, 5) + ":" + juce::String(q.second, 5)); p.setProperty(juce::String("ln_") + c, pts.joinIntoString(","), nullptr); }
    return p;
}
void StretchCatProcessor::applyPreset(const juce::ValueTree& p)
{
    if (!p.isValid()) return;
    for (auto c : p)
    {
        const auto id = c.getProperty("id").toString(); if (id == "sela" || id == "selb") continue;
        if (auto* prm = apvts.getParameter(id)) prm->setValueNotifyingHost(prm->convertTo0to1((float) c.getProperty("value")));
    }
    for (char c : { 'p', 's', 'c', 'f', 'w' }) { const auto k = juce::String("ln_") + c; lanes[laneIndex(c)] = p.hasProperty(k) ? parseLane(p.getProperty(k).toString(), c == 'c' ? 1.0 : 0.5) : parseLane({}, c == 'c' ? 1.0 : 0.5); }
    updateLiveLanes(); dirty = true;
}

// ------------------------------------------------------------------ audio

void StretchCatProcessor::processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    buf.clear();
    if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition()) if (auto b = pos->getBpm()) hostBpmSeen.store(*b);
    // New render ready? Never while a crossfade is running (the old one is still needed for it). Wait free.
    if (!sampler.crossfading() && bus.poll(handle)) sampler.swapTo(&bus.slot(handle.cur));
    stretchcat::SamplerParams sp;
    sp.mode = (stretchcat::Playback) juce::jmin(2, (int) get("pb")); sp.attackMs = get("atk"); sp.releaseMs = get("rel"); sp.root = (int) get("root"); sp.gainDb = get("gain");
    const bool keys = (int) get("pb") == 3;                        // Slices to keys: notes root, root+1 ... play slice 1, 2 ...
    const int n = buf.getNumSamples(); int done = 0; float* L = buf.getWritePointer(0); float* R = buf.getWritePointer(1);
    auto padNote = [&](int code)
    {
        const int idx = code / 2 - 1, on = code & 1;
        if (idx < 0) { if (on) sampler.noteOn(sp.root, 1.f, sp); else sampler.noteOff(sp.root, sp); }
        else if (on) sampler.sliceOn(idx, 1.f); else sampler.sliceOff(idx, sp);
    };
    {   // pads from the editor
        int s1, b1, s2, b2; padFifo.prepareToRead(padFifo.getNumReady(), s1, b1, s2, b2);
        for (int i = 0; i < b1; ++i) padNote(padBuf[s1 + i]);
        for (int i = 0; i < b2; ++i) padNote(padBuf[s2 + i]);
        padFifo.finishedRead(b1 + b2);
    }
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage(); const int at = juce::jlimit(0, n, meta.samplePosition);
        if (at > done) { sampler.process(sp, L + done, R + done, at - done); done = at; }
        const int slice = m.getNoteNumber() - sp.root;
        if (m.isNoteOn()) { if (keys && slice >= 0 && slice < sampler.sliceCount()) sampler.sliceOn(slice, m.getFloatVelocity()); else if (!keys) sampler.noteOn(m.getNoteNumber(), m.getFloatVelocity(), sp); }
        else if (m.isNoteOff()) { if (keys && slice >= 0 && slice < sampler.sliceCount()) sampler.sliceOff(slice, sp); else if (!keys) sampler.noteOff(m.getNoteNumber(), sp); }
        else if (m.isAllNotesOff() || m.isAllSoundOff()) sampler.allOff();
    }
    if (n > done) sampler.process(sp, L + done, R + done, n - done);
    if (sampler.takeCrossfadeEnded()) bus.releasePrev(handle);
    // rack: smoothed parameters (10 ms ramps inside), the same block
    stretchcat::RackParams rp;
    rp.filterOn = get("fo") > 0.5f; rp.filterType = (int) get("ft"); rp.cutoff = get("fc"); rp.reso = get("fr");
    rp.eqOn = get("eo") > 0.5f; rp.eqLow = get("elo"); rp.eqMid = get("emi"); rp.eqHigh = get("ehi");
    rp.compOn = get("co") > 0.5f; rp.thresh = get("cth"); rp.ratio = get("crt"); rp.attackMs = get("cat"); rp.releaseMs = get("crl");
    rack.setParams(rp); rack.process(L, R, n);
    float pl = 0.f, pr = 0.f;
    for (int i = 0; i < n; ++i) { pl = std::max(pl, std::fabs(L[i])); pr = std::max(pr, std::fabs(R[i])); }
    mL.store(pl); mR.store(pr); grDb.store((float) rack.gainReductionDb()); playPos.store(sampler.newestPosition());
}

// ------------------------------------------------------------------ state

void StretchCatProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    saveLanesToState();
    auto st = apvts.copyState(); st.setProperty("ver", kStateVersion, nullptr); st.setProperty("file", lastFile.getFullPathName(), nullptr);
    st.setProperty("slices", juce::String(sliceSet.toString()), nullptr); st.setProperty("srcNorm", srcNorm, nullptr); st.setProperty("srcRev", srcRev, nullptr);
    if (auto xml = st.createXml()) copyXmlToBinary(*xml, dest);
}
void StretchCatProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size)) if (xml->hasTagName(apvts.state.getType()))
    {
        auto st = juce::ValueTree::fromXml(*xml);
        // state version 1 (v0.1) had no rack, lanes, selection or slices: the new parameters simply keep their defaults
        const juce::File f(st.getProperty("file").toString());
        apvts.replaceState(st);
        for (char c : { 'p', 's', 'c', 'f', 'w' }) { const auto k = juce::String("ln_") + c; lanes[laneIndex(c)] = st.hasProperty(k) ? parseLane(st.getProperty(k).toString(), c == 'c' ? 1.0 : 0.5) : parseLane({}, c == 'c' ? 1.0 : 0.5); }
        updateLiveLanes();
        sliceSet = stretchcat::SliceSet::fromString(st.getProperty("slices").toString().toStdString()); sliceSet.sanitise(selA(), selB());
        srcNorm = (bool) st.getProperty("srcNorm", false); srcRev = (bool) st.getProperty("srcRev", false);
        if (f.existsAsFile()) { const bool had = !sliceSet.marks.empty(); const auto keep = sliceSet; loadFile(f); if (had) sliceSet = keep; }
        dirty = true;
    }
}
