#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// ============================================================
//  Custom look and feel — dark industrial aesthetic
// ============================================================
class BitDecayLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BitDecayLookAndFeel()
    {
        setColour (juce::Slider::thumbColourId,           juce::Colour (0xff5DCAA5));
        setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff5DCAA5));
        setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff333333));
        setColour (juce::Slider::backgroundColourId,      juce::Colour (0xff222222));
        setColour (juce::Label::textColourId,             juce::Colour (0xffCCCCCC));
        setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (0xff1a1a1a));
    }

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int w, int h,
                           float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override
    {
        const float radius  = (float)std::min (w, h) * 0.5f - 4.f;
        const float centreX = (float)x + (float)w * 0.5f;
        const float centreY = (float)y + (float)h * 0.5f;
        const float angle   = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Track ring
        g.setColour (juce::Colour (0xff333333));
        g.drawEllipse (centreX - radius, centreY - radius, radius * 2.f, radius * 2.f, 2.f);

        // Arc fill
        juce::Path arc;
        arc.addArc (centreX - radius, centreY - radius,
                    radius * 2.f, radius * 2.f,
                    rotaryStartAngle, angle, true);
        g.setColour (juce::Colour (0xff5DCAA5));
        g.strokePath (arc, juce::PathStrokeType (3.f));

        // Thumb dot
        const float tx = centreX + (radius - 5.f) * std::cos (angle - juce::MathConstants<float>::halfPi);
        const float ty = centreY + (radius - 5.f) * std::sin (angle - juce::MathConstants<float>::halfPi);
        g.setColour (juce::Colours::white);
        g.fillEllipse (tx - 3.f, ty - 3.f, 6.f, 6.f);
    }
};

// ============================================================
//  Simple knob + label helper
// ============================================================
struct KnobGroup
{
    juce::Slider slider;
    juce::Label  label;
    juce::Label  valueLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    void setup (juce::AudioProcessorValueTreeState& apvts,
                const juce::String& paramID,
                const juce::String& name,
                juce::Component& parent,
                juce::LookAndFeel& laf)
    {
        slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setLookAndFeel (&laf);
        parent.addAndMakeVisible (slider);

        label.setText (name, juce::dontSendNotification);
        label.setFont (juce::Font (10.f, juce::Font::plain));
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::textColourId, juce::Colour (0xff888888));
        parent.addAndMakeVisible (label);

        valueLabel.setFont (juce::Font (10.f, juce::Font::plain));
        valueLabel.setJustificationType (juce::Justification::centred);
        valueLabel.setColour (juce::Label::textColourId, juce::Colour (0xff5DCAA5));
        parent.addAndMakeVisible (valueLabel);

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
                     (apvts, paramID, slider);

        // Update value label on change
        slider.onValueChange = [this]
        {
            valueLabel.setText (juce::String (slider.getValue(), 1), juce::dontSendNotification);
        };
        slider.onValueChange(); // initial
    }

    void setBounds (int x, int y, int knobSize)
    {
        slider.setBounds (x, y, knobSize, knobSize);
        label.setBounds  (x, y + knobSize, knobSize, 14);
        valueLabel.setBounds (x, y + knobSize + 14, knobSize, 12);
    }
};

// ============================================================
//  Main editor
// ============================================================
class EthansDegenDelayAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit EthansDegenDelayAudioProcessorEditor (EthansDegenDelayAudioProcessor&);
    ~EthansDegenDelayAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    EthansDegenDelayAudioProcessor& processor;
    BitDecayLookAndFeel laf;

    KnobGroup knobDelay, knobFeedback, knobStartBits, knobBitDecay, knobMix, knobDownsample;

    // Echo cascade visualiser
    void paintEchoCascade (juce::Graphics&, juce::Rectangle<int>);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EthansDegenDelayAudioProcessorEditor)
};
