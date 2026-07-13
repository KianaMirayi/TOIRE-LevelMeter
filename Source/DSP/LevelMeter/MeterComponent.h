#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>

// ====== LevelMeterData ======
// Pure DSP data structure for audio level metering — no GUI, no JUCE Component.
// Everything runs on the audio thread, lock-free via std::atomic where needed.
//
// Computes:
//   - RMS over a configurable window (default 300 ms)
//   - Peak over the current block
//   - Peak hold with linear decay (default 2 s hold time)

struct LevelMeterData
{
    // --- RMS (per-channel ring buffer with running sum) ---
    void prepareRms(double sampleRate, int numChannels, float windowMs = 300.0f);
    void clearRms();
    void processRms(int channel, const float* samples, int numSamples, float* outRms = nullptr);

    // --- Peak hold (instant attack, configurable hold before decay) ---
    void preparePeakHold(double sampleRate, float holdMs = 2000.0f);
    void clearPeakHold();
    void updatePeakHold(float blockPeak, int numSamples);

    // Store raw block peak (non-smoothed) for the editor
    void setLatestPeak(float val) { latestPeak.store(val); }

    // --- Output (atomic reads safe from any thread) ---
    float getPeak()     const { return latestPeak.load(); }
    float getRms()      const { return latestRms.load(); }
    float getPeakHold() const { return peakHold.load(); }

private:
    // RMS ring buffer (one per channel, stores squared samples)
    struct RmsChannel
    {
        std::vector<float> squaresRing;
        int writePos = 0;
        float runningSum = 0.0f;
        int windowSize = 0;
        int samplesWritten = 0;  // tracks fill state for initial ramp-up
    };
    std::vector<RmsChannel> rmsChans;

    // Peak hold: sample-based counter
    std::atomic<float> peakHold { 0.0f };
    int holdCounter = 0;   // accumulated sample count since last peak
    int holdSamples = 0;   // sample count for hold time (2s @ sampleRate)

    // Latest computed values
    std::atomic<float> latestPeak { 0.0f };
    std::atomic<float> latestRms  { 0.0f };
};