#pragma once
#include <JuceHeader.h>
#include "LevelMeter/MeterComponent.h"

class TOIRELevelMeterAudioProcessor : public juce::AudioProcessor
{
public:
    TOIRELevelMeterAudioProcessor();
    ~TOIRELevelMeterAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override;
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    // Public access for the editor
    LevelMeterData& getMeterData() { return meterData; }

    // Frame counter: incremented every processBlock call.
    // Editor reads this to detect pause/bypass/deactivate (counter stops -> decay).
    std::atomic<uint64_t> frameCounter{0};
    uint64_t getFrameCount() const { return frameCounter.load(); }

private:
    LevelMeterData meterData;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TOIRELevelMeterAudioProcessor)
};
