#include "PluginEditor.h"

// ===================================================================
//  COLOUR SCHEME (KEEP — shared between native and future React UI)
// ===================================================================
static const juce::Colour kBgDark   { 26, 26, 46  };
static const juce::Colour kPanelBg  { 22, 33, 62  };
static const juce::Colour kTrackBg  { 15, 15, 35  };
static const juce::Colour kPeakGreen{ 0,  200, 83  };
static const juce::Colour kPeakYel  { 255, 214, 0  };
static const juce::Colour kPeakRed  { 255, 23,  68 };
static const juce::Colour kRmsStart { 41,  121, 255};
static const juce::Colour kRmsEnd   { 0,  229, 255};
static const juce::Colour kTextDim  { 192, 192, 192};
static const juce::Colour kTextSub  { 102, 102, 102};

// ── HELPERS (KEEP — reusable) ─────────────────────────────────
static float linToDb(float lin)
{
    if (lin <= 0.0f) return -96.0f;
    return 20.0f * std::log10(lin);
}

static float dbToNorm(float db, float floorDb = -96.0f)
{
    return juce::jlimit(0.0f, 1.0f, (db - floorDb) / -floorDb);
}

static juce::Colour gradient(float t, juce::Colour a, juce::Colour b, juce::Colour c)
{
    if (t < 0.5f) return a.interpolatedWith(b, t * 2.0f);
    else          return b.interpolatedWith(c, (t - 0.5f) * 2.0f);
}

// ===================================================================
//  CONSTRUCTOR / DESTRUCTOR (KEEP)
// ===================================================================
TOIRELevelMeterAudioProcessorEditor::TOIRELevelMeterAudioProcessorEditor(
    TOIRELevelMeterAudioProcessor& processor)
    : AudioProcessorEditor(&processor)
    , audioProcessor(processor)
{
    setSize(440, 340);
    startTimerHz(30);
}

TOIRELevelMeterAudioProcessorEditor::~TOIRELevelMeterAudioProcessorEditor()
{
    stopTimer();
}

// ===================================================================
//  LAYOUT (Gemini replaceable — React UI will handle its own layout)
// ===================================================================
void TOIRELevelMeterAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    const int barHeight = 18;
    const int clipH     = 24;

    area.removeFromTop(40);  // header space
    area.removeFromTop(16);

    peakBarArea = area.removeFromTop(barHeight);
    area.removeFromTop(8);
    rmsBarArea = area.removeFromTop(barHeight);
    area.removeFromTop(12);
    clipArea = area.removeFromTop(clipH);
}

// ===================================================================
//  TIMER CALLBACK (KEEP — data flow from DSP to UI)
//
//  Gemini migration: when WebView + React are ready,
//  replace repaint() with evaluateJavascript("updateLevels(...)")
// ===================================================================
void TOIRELevelMeterAudioProcessorEditor::timerCallback()
{
    auto& meter = audioProcessor.getMeterData();

    const float peak = meter.getPeakHold();
    const float rms  = meter.getRms();

    displayPeakDb = linToDb(peak);
    displayRmsDb  = linToDb(rms);
    displayClip   = (displayPeakDb >= -0.1f);

    repaint();   // ← replace with evaluateJavascript when React UI is ready
}

// ===================================================================
//  PAINT — native JUCE Graphics rendering
//
//  ╔══════════════════════════════════════════════════════════════╗
//  ║  GEMINI: the entire paint() body below is replaceable.      ║
//  ║  When React UI is ready, this function can be empty or      ║
//  ║  just draw a very basic background. Gemini owns the look.  ║
//  ╚══════════════════════════════════════════════════════════════╝
// ===================================================================
void TOIRELevelMeterAudioProcessorEditor::paint(juce::Graphics& g)
{
    // ── Background (keep or simplify) ────────────────────────
    g.fillAll(kBgDark);

    auto area = getLocalBounds().reduced(20);

    // ── Header (Gemini replaceable) ──────────────────────────
    {
        g.setColour(kTextDim);
        g.setFont(juce::Font("Segoe UI", 16.0f, juce::Font::bold));
        g.drawText("TOIRE Level Meter", area.removeFromTop(24),
                   juce::Justification::centredTop, false);

        g.setColour(kTextSub);
        g.setFont(10.0f);
        g.drawText("Cline v3  |  native JUCE Graphics", area.removeFromTop(16),
                   juce::Justification::centredTop, false);
    }

    // ── Draw one bar (Gemini replaceable) ────────────────────
    auto drawBar = [&](const juce::Rectangle<int>& rect, float db,
                       const juce::Colour& labelColour, const juce::String& label)
    {
        const float norm = dbToNorm(db);
        const int barW = juce::jmax(4, juce::roundToInt(norm * rect.getWidth()));

        // Track background
        g.setColour(kTrackBg);
        g.fillRoundedRectangle(rect.toFloat(), 4.0f);

        // Filled bar
        if (barW > 0)
        {
            juce::Rectangle<int> fillRect = rect.withWidth(barW);
            if (fillRect.getWidth() > 2)
            {
                g.setColour(gradient(norm, kPeakGreen, kPeakYel, kPeakRed));
                g.fillRoundedRectangle(fillRect.toFloat(), 4.0f);
            }
        }

        // dB label
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font("Segoe UI", 13.0f, juce::Font::bold));
        g.drawText(juce::String(db, 1) + " dB", rect.reduced(6, 0),
                   juce::Justification::centredLeft, false);

        // Right label
        g.setColour(labelColour);
        g.setFont(juce::Font("Segoe UI", 11.0f, juce::Font::bold));
        g.drawText(label, rect.reduced(6, 0),
                   juce::Justification::centredRight, false);
    };

    // ── Bars (Gemini replaceable) ────────────────────────────
    drawBar(peakBarArea, displayPeakDb, juce::Colour(255, 214, 0), "PEAK");
    drawBar(rmsBarArea,  displayRmsDb,  juce::Colour(0, 229, 255),     "RMS");

    // ── Clip indicator (Gemini replaceable) ──────────────────
    {
        g.setColour(displayClip
            ? juce::Colour(255, 23, 68).withAlpha(0.6f)
            : juce::Colour(255, 23, 68).withAlpha(0.15f));
        g.fillRoundedRectangle(clipArea.toFloat(), 4.0f);

        g.setColour(displayClip ? juce::Colours::white : juce::Colours::white.withAlpha(0.4f));
        g.setFont(juce::Font("Segoe UI", 12.0f, juce::Font::bold));
        g.drawText("CLIP", clipArea, juce::Justification::centred, false);
    }

    // ── Footer (Gemini replaceable) ──────────────────────────
    {
        auto footer = getLocalBounds().removeFromBottom(20);
        g.setColour(kTextSub);
        g.setFont(8.0f);
        g.drawText("native JUCE render  |  Gemini React UI later", footer,
                   juce::Justification::centredBottom, false);
    }
}
