#include "PluginProcessor.h"
#include "PluginEditor.h"

TOIRELevelMeterAudioProcessor::TOIRELevelMeterAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

TOIRELevelMeterAudioProcessor::~TOIRELevelMeterAudioProcessor() = default;

const juce::String TOIRELevelMeterAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool TOIRELevelMeterAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void TOIRELevelMeterAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);

    meterData.prepareRms(sampleRate, getTotalNumInputChannels(), 300.0f);
    meterData.preparePeakHold(sampleRate, 2000.0f);
    meterData.prepareRmsHold(sampleRate, 2000.0f);
}

void TOIRELevelMeterAudioProcessor::releaseResources()
{
    meterData.clearRms();
    meterData.clearPeakHold();
    meterData.clearRmsHold();
}

void TOIRELevelMeterAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                  juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    float blockPeak = 0.0f;

    for (int channel = 0; channel < numChannels; ++channel)
    {
        const auto* channelData = buffer.getReadPointer(channel);

        // Per-channel RMS
        meterData.processRms(channel, channelData, numSamples);

        // Peak (fastest of all channels)
        auto range = juce::FloatVectorOperations::findMinAndMax(channelData, numSamples);
        const float chPeak = juce::jmax(std::abs(range.getStart()),
                                         std::abs(range.getEnd()));
        blockPeak = juce::jmax(blockPeak, chPeak);
    }

    // Store raw block peak for the editor (atomic write, lock-free)
    meterData.setLatestPeak(blockPeak);

    // Peak hold decay (pass block sample count for accurate hold timing)
    meterData.updatePeakHold(blockPeak, numSamples);

    // RMS hold — track highest RMS value (similar to peak hold)
    meterData.updateRmsHold(meterData.getRms(), numSamples);

    // Levels read directly by PluginEditor timerCallback() via meterData.getPeakHold()/getRms()
    // No push needed — Timer polls std::atomic, no audio-thread work wasted
}

void TOIRELevelMeterAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::ignoreUnused(destData);
}

void TOIRELevelMeterAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::ignoreUnused(data, sizeInBytes);
}

juce::AudioProcessorEditor* TOIRELevelMeterAudioProcessor::createEditor()
{
    return new TOIRELevelMeterAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TOIRELevelMeterAudioProcessor();
}