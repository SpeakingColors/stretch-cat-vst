// Stretch Cat Track Effect: the effect plugin. It listens to the incoming track in a ring buffer (src/core/TrackFx.h); HOLD (a host
// parameter, so it can be automated) freezes that moment and repeats it as grains at any speed and pitch. Latency 0; while HOLD is off the
// audio passes bit exactly. The window is the Track Effect page of docs/mockup/stretchcat.html (src/trackfx/TrackFxEditor.cpp).
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../core/TrackFx.h"
#include <array>
#include <atomic>

class TrackFxProcessor : public juce::AudioProcessor
{
public:
    static constexpr int kStateVersion = 1;
    static constexpr int kCols = 400;                  // columns of the 8 second history shown in the window (20 ms each)
    static constexpr double kHistorySeconds = 8.0;
    TrackFxProcessor();
    const juce::String getName() const override { return "Stretch Cat Track Effect"; }
    void prepareToPlay(double sampleRate, int maxBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& l) const override { return l.getMainInputChannelSet() == juce::AudioChannelSet::stereo() && l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // ---- for the editor: all of these are safe to read from the message thread ----
    // history[i], oldest first when read from (writeIndex() % kCols); linear peak of the input in a 20 ms column
    float historyAt(int i) const { return cols[(size_t) i].load(std::memory_order_relaxed); }
    int writeIndex() const { return wcol.load(std::memory_order_acquire); }
    bool holding() const { return heldNow.load(); }
    float playheadMs() const { return playMs.load(); }
    float heldLengthMs() const { return heldMs.load(); }
    float meterL() const { return mL.load(); } float meterR() const { return mR.load(); }   // block peaks of the output, linear
    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    float get(const char* id) const { return apvts.getRawParameterValue(id)->load(); }
    stretchcat::TrackFx fx; double fs = 48000; bool lastHold = false;
    std::array<std::atomic<float>, kCols> cols {}; std::atomic<int> wcol { 0 };
    double colSamples = 960, colPos = 0; float colPeak = 0.f;
    std::atomic<bool> heldNow { false }; std::atomic<float> playMs { 0.f }, heldMs { 0.f }, mL { 0.f }, mR { 0.f };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackFxProcessor)
};
