#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true) // default stereo
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true) // always stereo (2 speakers)
#endif
    ),
    parameters(*this, nullptr, "ParameterTree", createParameterLayout())
{
    parameters.addParameterListener("volumeL", this);
    parameters.addParameterListener("volumeR", this);
    parameters.addParameterListener("azimuth", this);
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
    parameters.removeParameterListener("volumeL", this);
    parameters.removeParameterListener("volumeR", this);
    parameters.removeParameterListener("azimuth", this);
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const { return JucePlugin_Name; }

bool AudioPluginAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int AudioPluginAudioProcessor::getNumPrograms() { return 1; }
int AudioPluginAudioProcessor::getCurrentProgram() { return 0; }
void AudioPluginAudioProcessor::setCurrentProgram(int index) { juce::ignoreUnused(index); }
const juce::String AudioPluginAudioProcessor::getProgramName(int index) { juce::ignoreUnused(index); return {}; }
void AudioPluginAudioProcessor::changeProgramName(int index, const juce::String& newName) { juce::ignoreUnused(index, newName); }

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay(double, int)
{
    if (auto* v1 = parameters.getRawParameterValue("volumeL")) currentVolumeL.store(v1->load());
    if (auto* v2 = parameters.getRawParameterValue("volumeR")) currentVolumeR.store(v2->load());
    if (auto* az = parameters.getRawParameterValue("azimuth")) currentAzimuthDeg.store(az->load());
}

void AudioPluginAudioProcessor::releaseResources() {}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Always stereo output (2 speakers)
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

#if ! JucePlugin_IsSynth
    // Allow mono OR stereo input
    const auto in = layouts.getMainInputChannelSet();
    if (in != juce::AudioChannelSet::mono() && in != juce::AudioChannelSet::stereo())
        return false;
#endif

    return true;
}

// ---- VBAP helpers (match your MATLAB vbap2d convention) ----
static float degToRad(float deg) noexcept
{
    return deg * juce::MathConstants<float>::pi / 180.0f;
}

float AudioPluginAudioProcessor::wrap360(float deg) noexcept
{
    float x = std::fmod(deg, 360.0f);
    if (x < 0.0f) x += 360.0f;
    return x;
}

void AudioPluginAudioProcessor::vbap2Speakers(float srcAzDeg360, float ls1AzDeg360, float ls2AzDeg360,
    float& g1, float& g2) noexcept
{
    // l = [sin(az); cos(az)] , p = [sin(src); cos(src)]
    const float s1 = std::sin(degToRad(ls1AzDeg360));
    const float c1 = std::cos(degToRad(ls1AzDeg360));
    const float s2 = std::sin(degToRad(ls2AzDeg360));
    const float c2 = std::cos(degToRad(ls2AzDeg360));

    const float sp = std::sin(degToRad(srcAzDeg360));
    const float cp = std::cos(degToRad(srcAzDeg360));

    const float det = (s1 * c2 - s2 * c1);
    if (std::abs(det) < 1.0e-8f)
    {
        g1 = 0.7071f;
        g2 = 0.7071f;
        return;
    }

    g1 = (c2 * sp - s2 * cp) / det;
    g2 = (-c1 * sp + s1 * cp) / det;

    if (g1 < 0.0f) g1 = 0.0f;
    if (g2 < 0.0f) g2 = 0.0f;

    const float norm = std::sqrt(g1 * g1 + g2 * g2);
    const float safe = (norm > 1.0e-12f) ? norm : 1.0f;
    g1 /= safe;
    g2 /= safe;
}

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return;

    const int inCh = getTotalNumInputChannels();
    const int outCh = getTotalNumOutputChannels();

    // We need stereo output for 2 speakers
    if (outCh < 2 || buffer.getNumChannels() < 2)
        return;

    float* outL = buffer.getWritePointer(0);
    float* outR = buffer.getWritePointer(1);

    // These are CHANNEL volumes (not ear trims)
    const float vol1 = currentVolumeL.load(); // channel 1 (index 0)
    const float vol2 = currentVolumeR.load(); // channel 2 (index 1) if stereo

    const float centerAz = juce::jlimit(-90.0f, 90.0f, currentAzimuthDeg.load());

    // Decide mono case:
    // - true mono bus (inCh < 2)
    // - OR stereo bus but L and R are essentially identical (mono file duplicated to stereo)
    bool treatAsMono = (inCh < 2);

    if (!treatAsMono)
    {
        const int N = juce::jmin(numSamples, 256);
        double diff = 0.0, sum = 0.0;

        for (int i = 0; i < N; ++i)
        {
            const float a = outL[i];
            const float b = outR[i];
            diff += std::abs(a - b);
            sum += std::abs(a) + std::abs(b);
        }

        if (sum <= 1.0e-12)
            treatAsMono = true; // silence
        else
            treatAsMono = ((diff / sum) < 1.0e-6); // “almost identical”
    }

    if (treatAsMono)
    {
        // Mono: Volume 1 controls, Volume 2 does nothing.
        float gML = 0.0f, gMR = 0.0f;
        vbap2Speakers(wrap360(centerAz), speakerAzL, speakerAzR, gML, gMR);

        for (int i = 0; i < numSamples; ++i)
        {
            const float monoIn = (inCh >= 2) ? 0.5f * (outL[i] + outR[i]) : outL[i];
            const float x = monoIn * vol1;

            outL[i] = x * gML;
            outR[i] = x * gMR;
        }
    }
    else
    {
        // Stereo preserve: left source at center-30, right source at center+30
        const float srcAzL = centerAz - stereoHalfWidthDeg;
        const float srcAzR = centerAz + stereoHalfWidthDeg;

        float gLL = 0.0f, gRL = 0.0f; // gains for Left input source -> (Lspk,Rspk)
        float gLR = 0.0f, gRR = 0.0f; // gains for Right input source -> (Lspk,Rspk)

        vbap2Speakers(wrap360(srcAzL), speakerAzL, speakerAzR, gLL, gRL);
        vbap2Speakers(wrap360(srcAzR), speakerAzL, speakerAzR, gLR, gRR);

        for (int i = 0; i < numSamples; ++i)
        {
            // Apply volumes PER INPUT CHANNEL
            const float inL = outL[i] * vol1;
            const float inR = outR[i] * vol2;

            outL[i] = (gLL * inL + gLR * inR);
            outR[i] = (gRL * inL + gRR * inR);
        }
    }

    // Clear any extra channels beyond stereo
    for (int ch = 2; ch < buffer.getNumChannels(); ++ch)
        buffer.clear(ch, 0, numSamples);
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor(*this);
}

void AudioPluginAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "volumeL")
        currentVolumeL.store(newValue);
    else if (parameterID == "volumeR")
        currentVolumeR.store(newValue);
    else if (parameterID == "azimuth")
        currentAzimuthDeg.store(newValue);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AudioPluginAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // These are channel volumes (0..1), default 0.5
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "volumeL", "Volume 1",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "volumeR", "Volume 2",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.5f));

    // VBAP control (front stage only)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "azimuth", "Azimuth",
        juce::NormalisableRange<float>(-90.0f, 90.0f, 0.01f), 0.0f));

    return { params.begin(), params.end() };
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}