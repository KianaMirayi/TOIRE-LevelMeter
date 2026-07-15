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
//   - RMS hold with linear decay (default 2 s hold time)

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

    // --- RMS hold (same logic as peak hold) ---
    void prepareRmsHold(double sampleRate, float holdMs = 2000.0f);
    void clearRmsHold();
    void updateRmsHold(float rmsVal, int numSamples);

    // Store raw block peak (non-smoothed) for the editor
    void setLatestPeak(float val) { latestPeak.store(val); }

    // --- Output (atomic reads safe from any thread) ---
    float getPeak()     const { return latestPeak.load(); }
    float getRms()      const { return latestRms.load(); }
    float getPeakHold() const { return peakHold.load(); }
    float getRmsHold()  const { return rmsHold.load(); }

private:
    // RMS ring buffer (one per channel, stores squared samples)
    struct RmsChannel
    {
        std::vector<float> squaresRing;
        int writePos = 0;
        float runningSum = 0.0f;
        int windowSize = 0;
        int samplesWritten = 0;
    };
    std::vector<RmsChannel> rmsChans;

    // Peak hold
    std::atomic<float> peakHold { 0.0f };
    int holdCounter = 0;
    int holdSamples = 0;

    // RMS hold (mirrors peak hold logic)
    std::atomic<float> rmsHold { 0.0f };
    int rmsHoldCounter = 0;
    int rmsHoldSamples = 0;

    // Latest computed values
    std::atomic<float> latestPeak { 0.0f };
    std::atomic<float> latestRms  { 0.0f };
};
