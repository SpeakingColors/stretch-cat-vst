// Stretch Cat: the plugin. Sampler with time stretch: load a sound, the stretched version is rendered on a background thread
// whenever a stretch control changes (the audio thread keeps playing the previous render until the new one is ready, then
// crossfades), and MIDI notes play it. Engine in src/core. The editor (StretchCatEditor.cpp) is the cat-ui window built
// from docs/mockup/stretchcat.html through src/editor/Layout.h.
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../core/RenderWorker.h"
#include <atomic>
#include <memory>

class StretchCatProcessor : public juce::AudioProcessor, private juce::Timer, private juce::AudioProcessorValueTreeState::Listener
{
public:
    static constexpr int kStateVersion = 2;             // 1 = v0.1 (no rack, no lanes, no slices)
    StretchCatProcessor();
    ~StretchCatProcessor() override;
    const juce::String getName() const override { return "Stretch Cat"; }
    void prepareToPlay(double sampleRate, int maxBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& l) const override { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // ---- for the editor (message thread only unless noted) ----
    bool loadFile(const juce::File& f);                  // reads the whole file, then schedules a render on the worker
    bool hasSource() const { return source != nullptr; }
    juce::String sourceName() const { return fileName; }
    double sourceSeconds() const { return source ? (double) source->L.size() / source->sr : 0.0; }
    const std::vector<float>& srcPeaks() const { return peaksSrc; }
    const std::vector<float>& resultPeaks() const { return ui.peaks; }
    const std::vector<double>& resultSliceEdges() const { return ui.sliceFrac; }   // fractions of the result, count + 1 values
    const std::vector<std::pair<double, double>>& resultMap() const { return ui.map; }   // (result sample, source sample) points
    long long resultLength() const { return ui.length; }
    long long sourceSamples() const { return source ? (long long) source->L.size() : 0; }
    double resultSeconds() const { return ui.seconds; }
    double renderMs() const { return ui.ms; }
    bool rendering() const { return pendingRender || !worker.idle(); }
    int dataRevision() const { return revision; }
    float playPosition() const { return playPos.load(); }                          // audio thread writes: 0..1 of the result, or -1
    float meterL() const { return mL.load(); } float meterR() const { return mR.load(); }   // block peaks, linear
    float compressorGrDb() const { return grDb.load(); }
    // selection (fractions of the file) and slices
    double selA() const { return get("sela"); } double selB() const { return get("selb"); }
    void setSelection(double a, double b);
    const stretchcat::SliceSet& slices() const { return sliceSet; }
    void setSlices(const stretchcat::SliceSet& s);
    void slicesEven();
    void slicesByTransient();
    // lanes: 'p' pitch, 's' speed, 'c' cutoff, 'f' formant, 'w' width
    stretchcat::Lane getLane(char id) const;
    void setLane(char id, const stretchcat::Lane& l);
    // source edits (they change the audio the stretch starts from)
    void normalizeSource(); void reverseSource(); void resetSource();
    // pads: any thread; the audio thread picks them up (lock free)
    void triggerSlice(int index, bool on);
    void triggerPlay(bool on);                           // the Play button: the root note
    // plugin settings (saved with the plugin, not with presets)
    bool bubbles() const { return (bool) apvts.state.getProperty("bubbles", true); }
    void setBubbles(bool b) { apvts.state.setProperty("bubbles", b, nullptr); }
    bool eco() const { return (bool) apvts.state.getProperty("eco", false); }
    void setEco(bool b) { apvts.state.setProperty("eco", b, nullptr); }
    juce::String presetName() const { return apvts.state.getProperty("preset", "Init").toString(); }
    void setPresetName(const juce::String& n) { apvts.state.setProperty("preset", n, nullptr); }
    // presets: parameters and lanes (not the file, selection, slices or settings)
    juce::ValueTree capturePreset();
    void applyPreset(const juce::ValueTree& p);
    float getReal(const char* id) const { return get(id); }
    void setReal(const char* id, float v) { if (auto* p = apvts.getParameter(id)) p->setValueNotifyingHost(p->convertTo0to1(v)); }
    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    float get(const char* id) const { return apvts.getRawParameterValue(id)->load(); }
    void parameterChanged(const juce::String& id, float) override;
    void timerCallback() override;
    void requestRender();                                // message thread: builds a Request and posts it to the worker
    void updateLiveLanes();
    void applySource(std::shared_ptr<const stretchcat::Source> s);
    void saveLanesToState();

    stretchcat::ResultBus bus; stretchcat::RenderWorker worker { bus };
    stretchcat::ResultBus::AudioHandle handle;
    stretchcat::Sampler sampler; stretchcat::LiveLanes live; stretchcat::Rack rack;
    double fs = 44100;
    // source (message thread only)
    std::shared_ptr<const stretchcat::Source> original, source; juce::String fileName; juce::File lastFile; bool srcNorm = false, srcRev = false;
    std::vector<float> peaksSrc; stretchcat::UiResult ui; int revision = 0;
    stretchcat::SliceSet sliceSet; stretchcat::Lane lanes[5];
    std::atomic<bool> dirty { false }; bool pendingRender = false;
    std::atomic<double> hostBpmSeen { 0.0 };
    // pad events from the editor to the audio thread: code = (index + 1) * 2 + on, index -1 means the root note
    juce::AbstractFifo padFifo { 64 }; int padBuf[64] {};
    std::atomic<float> playPos { -1.f }, mL { 0.f }, mR { 0.f }, grDb { 0.f };
    static int laneIndex(char id) { switch (id) { case 'p': return 0; case 's': return 1; case 'c': return 2; case 'f': return 3; default: return 4; } }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StretchCatProcessor)
};
