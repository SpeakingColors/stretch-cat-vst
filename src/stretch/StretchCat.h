// Stretch Cat: the plugin. Sampler with time stretch: load a sound, the stretched version is rendered on a
// background thread whenever a stretch control changes, and MIDI notes play it. Engine in src/core.
// The editor (StretchCatEditor.cpp) is the cat-ui window built from docs/mockup/stretchcat.html.
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../core/StretchSampler.h"
#include <atomic>
#include <memory>

class StretchCatProcessor : public juce::AudioProcessor, private juce::Timer, private juce::AudioProcessorValueTreeState::Listener
{
public:
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

    bool loadFile(const juce::File& f);          // message thread; reads the whole file, then schedules a render
    juce::String status() const { return statusText; }
    // for the editor (message thread only): waveform peaks of the loaded sound and of the stretched result
    const std::vector<float>& srcPeaks() const { return peaksSrc; }
    const std::vector<float>& resultPeaks() const { return peaksRes; }
    int dataRevision() const { return revision; }
    double resultSeconds() const { return resSeconds; }
    double sourceSeconds() const { return srcL.empty() ? 0.0 : (double) srcL.size() / srcSr; }
    juce::String sourceName() const { return fileName; }
    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    float get(const char* id) const { return apvts.getRawParameterValue(id)->load(); }
    void parameterChanged(const juce::String& id, float) override;
    void timerCallback() override;
    void renderNow();

    stretchcat::Sampler sampler;
    double fs = 44100;
    // source (message thread only)
    std::vector<float> srcL, srcR; double srcSr = 44100; juce::String fileName, statusText = "No sound loaded. Press Load.";
    std::atomic<bool> dirty { false };
    std::atomic<double> hostBpmSeen { 0.0 };
    // hand-off of a finished render to the audio thread
    juce::SpinLock handoff; std::shared_ptr<const stretchcat::Stereo> pending; double pendingSr = 44100; bool hasPending = false;
    std::vector<std::shared_ptr<const stretchcat::Stereo>> retired;     // old buffers are freed on the message thread, not the audio thread
    juce::File lastFile;
    std::vector<float> peaksSrc, peaksRes; int revision = 0; double resSeconds = 0.0;
    static std::vector<float> makePeaks(const std::vector<float>& l, const std::vector<float>& r, int bins);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StretchCatProcessor)
};
