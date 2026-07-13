#include "MeterComponent.h"

// ====== RMS Ring Buffer ======
// Uses a running-sum ring buffer: on each sample, subtract the oldest
// squared value and add the newest, keeping O(1) per-sample RMS computation
// without needing to sum the entire window every time.

void LevelMeterData::prepareRms(double sampleRate, int numChannels, float windowMs)
{
    rmsChans.resize(static_cast<size_t>(numChannels));

    const int windowSize = static_cast<int>(sampleRate * windowMs / 1000.0);

    for (auto& ch : rmsChans)
    {
        ch.windowSize = windowSize;
        ch.writePos = 0;
        ch.runningSum = 0.0f;
        ch.samplesWritten = 0;
        ch.squaresRing.assign(static_cast<size_t>(windowSize), 0.0f);
    }
}

void LevelMeterData::clearRms()
{
    for (auto& ch : rmsChans)
    {
        ch.writePos = 0;
        ch.runningSum = 0.0f;
        ch.samplesWritten = 0;
        std::fill(ch.squaresRing.begin(), ch.squaresRing.end(), 0.0f);
    }
}

void LevelMeterData::processRms(int channel, const float* samples, int numSamples, float* outRms)
{
    if (channel < 0 || channel >= static_cast<int>(rmsChans.size()))
        return;

    auto& ch = rmsChans[static_cast<size_t>(channel)];

    if (ch.windowSize <= 0)
        return;

    for (int i = 0; i < numSamples; ++i)
    {
        const float sampleVal = samples[i];
        const float sqr = sampleVal * sampleVal;

        // Subtract the oldest value falling out of the window
        const float oldest = ch.squaresRing[static_cast<size_t>(ch.writePos)];
        ch.runningSum = ch.runningSum - oldest + sqr;

        // Store new squared value
        ch.squaresRing[static_cast<size_t>(ch.writePos)] = sqr;

        // Advance ring pointer
        ch.writePos = (ch.writePos + 1) % ch.windowSize;

        // Track how many samples we've seen for correct early-window RMS
        if (ch.samplesWritten < ch.windowSize)
            ++ch.samplesWritten;
    }

    // Compute RMS = sqrt(mean of squared samples)
    const int effectiveCount = std::min(ch.samplesWritten, ch.windowSize);
    if (effectiveCount > 0)
    {
        const float meanSqr = ch.runningSum / static_cast<float>(effectiveCount);
        const float rmsVal = std::sqrt(std::max(0.0f, meanSqr));

        if (outRms)
            *outRms = rmsVal;
        else
            latestRms.store(rmsVal);
    }
}


// ====== Peak Hold ======
// Instant attack (new peak > current hold → overwrite immediately).
// Hold for holdMs milliseconds, then decay linearly toward zero.
// Uses sample-based counting so hold time is accurate regardless of block size.

void LevelMeterData::preparePeakHold(double sampleRate, float holdMs)
{
    holdSamples = static_cast<int>(sampleRate * holdMs / 1000.0);
    holdCounter = 0;
    peakHold.store(0.0f);
}

void LevelMeterData::clearPeakHold()
{
    holdCounter = 0;
    peakHold.store(0.0f);
}

void LevelMeterData::updatePeakHold(float blockPeak, int numSamples)
{
    const float currentHold = peakHold.load();

    if (blockPeak >= currentHold)
    {
        // Instant attack — new peak, reset hold counter
        peakHold.store(blockPeak);
        holdCounter = 0;
    }
    else
    {
        // Below hold level, accumulate sample count
        holdCounter += numSamples;

        if (holdCounter >= holdSamples && holdSamples > 0)
        {
            // Enter decay phase: linear ramp to zero over holdSamples
            const int decayElapsed = holdCounter - holdSamples;
            if (decayElapsed >= holdSamples)
            {
                // Fully decayed
                peakHold.store(0.0f);
            }
            else
            {
                const float frac = static_cast<float>(decayElapsed) / static_cast<float>(holdSamples);
                const float decayed = currentHold * (1.0f - frac);
                peakHold.store(std::max(0.0f, decayed));
            }
        }
    }
}