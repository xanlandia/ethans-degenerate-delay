#include "PluginProcessor.h"
#include "PluginEditor.h"

// ============================================================
//  Parameter layout
// ============================================================
juce::AudioProcessorValueTreeState::ParameterLayout
EthansDegenDelayAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "delayMs", "Delay Time",
        juce::NormalisableRange<float> (50.f, 900.f, 1.f), 375.f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "feedback", "Feedback",
        juce::NormalisableRange<float> (0.f, 0.90f, 0.01f), 0.55f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "startBits", "Starting Bit Depth",
        juce::NormalisableRange<float> (4.f, 16.f, 1.f), 16.f,
        juce::AudioParameterFloatAttributes().withLabel ("bit")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "bitDecay", "Bit Decay Per Echo",
        juce::NormalisableRange<float> (1.f, 6.f, 1.f), 2.f,
        juce::AudioParameterFloatAttributes().withLabel ("bits")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "mix", "Wet/Dry Mix",
        juce::NormalisableRange<float> (0.f, 1.f, 0.01f), 0.6f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    // Downsampling divisor: 1 = off, 2–8 = progressive lo-fi
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "downsample", "Downsample Factor",
        juce::NormalisableRange<float> (1.f, 8.f, 1.f), 1.f,
        juce::AudioParameterFloatAttributes().withLabel ("÷")));

    return { params.begin(), params.end() };
}

// ============================================================
//  Constructor / Destructor
// ============================================================
EthansDegenDelayAudioProcessor::EthansDegenDelayAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    apvts.addParameterListener ("delayMs",    this);
    apvts.addParameterListener ("feedback",   this);
    apvts.addParameterListener ("startBits",  this);
    apvts.addParameterListener ("bitDecay",   this);
    apvts.addParameterListener ("mix",        this);
    apvts.addParameterListener ("downsample", this);

    // Initialise per-tap downsampler state
    for (int t = 0; t < MAX_TAPS; ++t)
    {
        downsampleHeld[0][t] = 0.f;
        downsampleHeld[1][t] = 0.f;
        downsampleCounter[t] = 0;
    }
}

EthansDegenDelayAudioProcessor::~EthansDegenDelayAudioProcessor()
{
    apvts.removeParameterListener ("delayMs",    this);
    apvts.removeParameterListener ("feedback",   this);
    apvts.removeParameterListener ("startBits",  this);
    apvts.removeParameterListener ("bitDecay",   this);
    apvts.removeParameterListener ("mix",        this);
    apvts.removeParameterListener ("downsample", this);
}

// ============================================================
//  Prepare / Release
// ============================================================
void EthansDegenDelayAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    delayBufferSize   = static_cast<int> (sampleRate * 4.0); // 4 seconds max
    jassert (delayBufferSize <= MAX_DELAY_SAMPLES);

    for (int ch = 0; ch < 2; ++ch)
    {
        delayBuffer[ch].assign (static_cast<size_t>(delayBufferSize), 0.f);
    }
    writePos = 0;
}

void EthansDegenDelayAudioProcessor::releaseResources() {}

// ============================================================
//  Layout check
// ============================================================
#ifndef JucePlugin_PreferredChannelConfigurations
bool EthansDegenDelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet()  != juce::AudioChannelSet::stereo())
        return false;
    return true;
}
#endif

// ============================================================
//  Process block — the core algorithm
//
//  Architecture:
//    Each tap reads from a fixed position in the delay buffer
//    (tapIndex * delaySamples behind write head), applies
//    progressive bitcrushing + optional downsampling, scales
//    by feedback^tap, and accumulates into the wet mix.
//
//    No feedback write-back is needed because each tap reads
//    the original dry signal from the buffer; this gives a
//    multi-tap "ping-pong" style rather than a recursive loop.
//    This is intentional — it avoids runaway feedback and lets
//    the plugin be used at high feedback values safely.
// ============================================================
void EthansDegenDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                juce::MidiBuffer& /*midi*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    // Snapshot parameters (thread-safe atomics)
    const float delayMs    = p_delayMs.load();
    const float feedback   = p_feedback.load();
    const int   startBits  = static_cast<int> (p_startBits.load());
    const int   bitDecay   = static_cast<int> (p_bitDecay.load());
    const float mix        = p_mix.load();
    const int   downsample = static_cast<int> (p_downsample.load());

    const int delaySamples = static_cast<int> (delayMs * currentSampleRate / 1000.0);

    for (int s = 0; s < numSamples; ++s)
    {
        // --- Write dry signal into delay buffer ---
        for (int ch = 0; ch < std::min (numChannels, 2); ++ch)
        {
            delayBuffer[ch][writePos] = buffer.getSample (ch, s);
        }

        // --- Accumulate taps ---
        float wetL = 0.f, wetR = 0.f;

        for (int tap = 0; tap < MAX_TAPS; ++tap)
        {
            const int bits = startBits - tap * bitDecay;
            if (bits < 1) break;

            const float tapGain = std::pow (feedback, static_cast<float> (tap + 1));
            if (tapGain < 0.001f) break;

            const int readOffset = delaySamples * (tap + 1);
            if (readOffset >= delayBufferSize) break;

            int readPos = writePos - readOffset;
            if (readPos < 0) readPos += delayBufferSize;

            // Read from buffer
            float l = delayBuffer[0][readPos];
            float r = (numChannels > 1) ? delayBuffer[1][readPos] : l;

            // Downsample — hold-and-sample per tap, per channel
            if (downsample > 1)
            {
                if (downsampleCounter[tap] == 0)
                {
                    downsampleHeld[0][tap] = l;
                    downsampleHeld[1][tap] = r;
                }
                l = downsampleHeld[0][tap];
                r = downsampleHeld[1][tap];
            }

            // Bitcrush
            l = crushSample (l, bits);
            r = crushSample (r, bits);

            // Accumulate
            wetL += l * tapGain;
            wetR += r * tapGain;
        }

        // Advance downsample counters
        for (int tap = 0; tap < MAX_TAPS; ++tap)
        {
            downsampleCounter[tap]++;
            if (downsampleCounter[tap] >= downsample)
                downsampleCounter[tap] = 0;
        }

        // --- Mix dry + wet ---
        const float dryL = buffer.getSample (0, s);
        const float dryR = (numChannels > 1) ? buffer.getSample (1, s) : dryL;

        buffer.setSample (0, s, dryL * (1.f - mix) + wetL * mix);
        if (numChannels > 1)
            buffer.setSample (1, s, dryR * (1.f - mix) + wetR * mix);

        // Advance write head
        if (++writePos >= delayBufferSize)
            writePos = 0;
    }
}

// ============================================================
//  State save/restore
// ============================================================
void EthansDegenDelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void EthansDegenDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// ============================================================
//  Parameter listener
// ============================================================
void EthansDegenDelayAudioProcessor::parameterChanged (const juce::String& id, float val)
{
    if      (id == "delayMs")    p_delayMs.store (val);
    else if (id == "feedback")   p_feedback.store (val);
    else if (id == "startBits")  p_startBits.store (val);
    else if (id == "bitDecay")   p_bitDecay.store (val);
    else if (id == "mix")        p_mix.store (val);
    else if (id == "downsample") p_downsample.store (val);
}

// ============================================================
//  Editor factory
// ============================================================
juce::AudioProcessorEditor* EthansDegenDelayAudioProcessor::createEditor()
{
    return new EthansDegenDelayAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EthansDegenDelayAudioProcessor();
}
