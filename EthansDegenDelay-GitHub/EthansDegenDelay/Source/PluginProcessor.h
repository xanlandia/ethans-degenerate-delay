#pragma once
#include <JuceHeader.h>

// ============================================================
//  Ethan's Degenerate Delay — VST3/AU Plugin
//  Each echo tap is progressively bitcrushed and optionally
//  downsampled, producing a degrading digital echo effect.
// ============================================================

class EthansDegenDelayAudioProcessor : public juce::AudioProcessor,
                                    public juce::AudioProcessorValueTreeState::Listener
{
public:
    EthansDegenDelayAudioProcessor();
    ~EthansDegenDelayAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void parameterChanged (const juce::String& paramID, float newValue) override;

    juce::AudioProcessorValueTreeState apvts;

    // Max taps visible in UI
    static constexpr int MAX_TAPS = 8;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Per-tap delay line (stereo)
    static constexpr int MAX_DELAY_SAMPLES = 192000; // ~4s @ 48kHz
    std::array<std::vector<float>, 2> delayBuffer;
    int writePos = 0;
    int delayBufferSize = 0;

    // Per-tap downsampler state
    std::array<float, MAX_TAPS> downsampleHeld[2]; // [channel][tap]
    std::array<int,   MAX_TAPS> downsampleCounter;

    double currentSampleRate = 44100.0;

    // Parameter helpers (atomic snapshots updated in parameterChanged)
    std::atomic<float> p_delayMs    { 375.f };
    std::atomic<float> p_feedback   { 0.55f };
    std::atomic<float> p_startBits  { 16.f  };
    std::atomic<float> p_bitDecay   { 2.f   };
    std::atomic<float> p_mix        { 0.6f  };
    std::atomic<float> p_downsample { 1.f   };

    // Inline bitcrusher
    static float crushSample (float input, int bits)
    {
        if (bits >= 24) return input;
        const float steps = static_cast<float>(1 << bits);
        return std::round (input * steps) / steps;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EthansDegenDelayAudioProcessor)
};
