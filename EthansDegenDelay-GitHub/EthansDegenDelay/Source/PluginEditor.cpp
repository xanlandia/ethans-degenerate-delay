#include "PluginEditor.h"

static constexpr int KNOB_SIZE  = 70;
static constexpr int KNOB_COLS  = 6;
static constexpr int KNOB_ROW_H = KNOB_SIZE + 32; // knob + label + value
static constexpr int PADDING    = 20;
static constexpr int CASCADE_H  = 60;
static constexpr int HEADER_H   = 44;

EthansDegenDelayAudioProcessorEditor::EthansDegenDelayAudioProcessorEditor
    (EthansDegenDelayAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&laf);

    auto& apvts = p.apvts;
    knobDelay.setup     (apvts, "delayMs",    "DELAY",   *this, laf);
    knobFeedback.setup  (apvts, "feedback",   "FDBK",    *this, laf);
    knobStartBits.setup (apvts, "startBits",  "START",   *this, laf);
    knobBitDecay.setup  (apvts, "bitDecay",   "DECAY",   *this, laf);
    knobMix.setup       (apvts, "mix",        "MIX",     *this, laf);
    knobDownsample.setup(apvts, "downsample", "SRATE÷",  *this, laf);

    // Repaint cascade on any knob change
    auto repaintFn = [this]{ repaint(); };
    for (auto* kg : { &knobDelay, &knobFeedback, &knobStartBits,
                      &knobBitDecay, &knobMix, &knobDownsample })
    {
        kg->slider.onValueChange = [kg, repaintFn]
        {
            kg->valueLabel.setText (juce::String (kg->slider.getValue(), 1),
                                    juce::dontSendNotification);
            repaintFn();
        };
        kg->slider.onValueChange();
    }

    const int totalW = PADDING*2 + KNOB_COLS*KNOB_SIZE + (KNOB_COLS-1)*10;
    const int totalH = HEADER_H + KNOB_ROW_H + CASCADE_H + PADDING*2;
    setSize (totalW, totalH);
}

EthansDegenDelayAudioProcessorEditor::~EthansDegenDelayAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

// ============================================================
//  Layout
// ============================================================
void EthansDegenDelayAudioProcessorEditor::resized()
{
    const int gap  = (getWidth() - PADDING*2 - KNOB_COLS*KNOB_SIZE) / (KNOB_COLS - 1);
    const int y    = HEADER_H + PADDING;

    KnobGroup* knobs[] = { &knobDelay, &knobFeedback, &knobStartBits,
                            &knobBitDecay, &knobMix, &knobDownsample };
    for (int i = 0; i < 6; ++i)
        knobs[i]->setBounds (PADDING + i * (KNOB_SIZE + gap), y, KNOB_SIZE);
}

// ============================================================
//  Paint
// ============================================================
void EthansDegenDelayAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Background
    g.fillAll (juce::Colour (0xff1a1a1a));

    // Header bar
    g.setColour (juce::Colour (0xff111111));
    g.fillRect (0, 0, getWidth(), HEADER_H);

    // Title
    g.setColour (juce::Colour (0xff5DCAA5));
    g.setFont (juce::Font (18.f, juce::Font::bold));
    g.drawText ("ETHANS DEGENERATE DELAY", PADDING, 0, 220, HEADER_H, juce::Justification::centredLeft);

    // Subtitle
    g.setColour (juce::Colour (0xff555555));
    g.setFont (juce::Font (10.f, juce::Font::plain));
    g.drawText ("Dirtbag Audio | Ethan's Degenerate Delay", PADDING, HEADER_H/2 + 4, 260, 14,
                juce::Justification::centredLeft);

    // Separator
    g.setColour (juce::Colour (0xff333333));
    g.drawLine (0.f, (float)HEADER_H, (float)getWidth(), (float)HEADER_H, 0.5f);

    // Echo cascade
    const int cascadeY = HEADER_H + PADDING + KNOB_ROW_H + 4;
    paintEchoCascade (g, { PADDING, cascadeY, getWidth() - PADDING*2, CASCADE_H - 8 });
}

void EthansDegenDelayAudioProcessorEditor::paintEchoCascade (juce::Graphics& g,
                                                           juce::Rectangle<int> area)
{
    const int startBits = static_cast<int> (*processor.apvts.getRawParameterValue ("startBits"));
    const int bitDecay  = static_cast<int> (*processor.apvts.getRawParameterValue ("bitDecay"));
    const float feedback = *processor.apvts.getRawParameterValue ("feedback");

    g.setColour (juce::Colour (0xff222222));
    g.fillRoundedRectangle (area.toFloat(), 4.f);

    g.setFont (juce::Font (9.f, juce::Font::plain));
    g.setColour (juce::Colour (0xff444444));
    g.drawText ("ECHO CASCADE", area.getX() + 8, area.getY() + 4, 100, 12,
                juce::Justification::centredLeft);

    constexpr int maxTaps = 8;
    const int barAreaY  = area.getY() + 18;
    const int barAreaH  = area.getHeight() - 26;
    const int barW      = (area.getWidth() - 16) / maxTaps - 4;
    const juce::Colour barColour (0xff5DCAA5);

    for (int tap = 0; tap < maxTaps; ++tap)
    {
        const int bits = startBits - tap * bitDecay;
        if (bits < 1) break;

        const float amp    = std::pow (feedback, static_cast<float>(tap));
        const float fill   = ((float)bits / 16.f) * amp;
        const int   barH   = juce::roundToInt (fill * barAreaH);
        const int   bx     = area.getX() + 8 + tap * (barW + 4);
        const int   by     = barAreaY + (barAreaH - barH);

        g.setColour (barColour.withAlpha (0.25f + 0.75f * amp));
        g.fillRoundedRectangle ((float)bx, (float)by, (float)barW, (float)barH, 2.f);

        // Bit label
        g.setColour (juce::Colour (0xff444444));
        g.setFont (juce::Font (8.f));
        g.drawText (juce::String(bits) + "b",
                    bx, barAreaY + barAreaH + 2, barW, 10,
                    juce::Justification::centred);
    }
}
