#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cstring> // memcpy

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), parameters(*this, nullptr, "ParameterTree", createParameterLayout())
{

    parameters.addParameterListener("azimuth", this);
    parameters.addParameterListener("volumeL", this);
    parameters.addParameterListener("volumeR", this);

    
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{

    parameters.removeParameterListener("volumeL", this);
    parameters.removeParameterListener("volumeR", this);
    parameters.removeParameterListener("azimuth", this);
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

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

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(sampleRate, samplesPerBlock);

    if (auto* az = parameters.getRawParameterValue("azimuth"))
        currentAzimuthDeg.store(az->load());
    if (auto* vL = parameters.getRawParameterValue("volumeL"))
        currentVolumeL.store(vL->load());

    if (auto* vR = parameters.getRawParameterValue("volumeR"))
        currentVolumeR.store(vR->load());
}


void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Must be stereo output
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

#if ! JucePlugin_IsSynth
    // If it's an effect, require stereo input too (keeps host happy)
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
#endif

    return true;
}
float AudioPluginAudioProcessor::wrap360(float deg) noexcept
{
    float x = std::fmod(deg, 360.0f);
    if (x < 0.0f) x += 360.0f;
    return x;
}

static float degToRad(float deg) noexcept
{
    return deg * juce::MathConstants<float>::pi / 180.0f;
}

void AudioPluginAudioProcessor::vbap2Speakers(float srcAzDeg360, float ls1AzDeg360, float ls2AzDeg360,
    float& g1, float& g2) noexcept
{
    // MATLAB vbap2d.m convention:
    // l = [sind(az); cosd(az)], p = [sind(src); cosd(src)]
    const float s1 = std::sin(degToRad(ls1AzDeg360));
    const float c1 = std::cos(degToRad(ls1AzDeg360));
    const float s2 = std::sin(degToRad(ls2AzDeg360));
    const float c2 = std::cos(degToRad(ls2AzDeg360));

    const float sp = std::sin(degToRad(srcAzDeg360));
    const float cp = std::cos(degToRad(srcAzDeg360));

    const float det = (s1 * c2 - s2 * c1);

    if (std::abs(det) < 1.0e-8f)
    {
        g1 = 0.7071f; g2 = 0.7071f;
        return;
    }

    g1 = (c2 * sp - s2 * cp) / det;
    g2 = (-c1 * sp + s1 * cp) / det;

    // clamp negatives like MATLAB
    if (g1 < 0.0f) g1 = 0.0f;
    if (g2 < 0.0f) g2 = 0.0f;

    // normalize constant power
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
    const int numCh = buffer.getNumChannels();
    if (numCh < 2 || numSamples == 0)
        return;

    // User trims (per ear)
    const float trimL = currentVolumeL.load();
    const float trimR = currentVolumeR.load();

    // Front-stage only
    const float centerAz = juce::jlimit(-90.0f, 90.0f, currentAzimuthDeg.load());

    // Preserve stereo: treat input L and input R as two sources around the center
    const float srcAzL = centerAz - stereoHalfWidthDeg;
    const float srcAzR = centerAz + stereoHalfWidthDeg;

    float gLL = 0.0f, gRL = 0.0f; // gains to (LeftSpeaker, RightSpeaker) for LeftInput source
    float gLR = 0.0f, gRR = 0.0f; // gains to (LeftSpeaker, RightSpeaker) for RightInput source

    vbap2Speakers(wrap360(srcAzL), speakerAzL, speakerAzR, gLL, gRL);
    vbap2Speakers(wrap360(srcAzR), speakerAzL, speakerAzR, gLR, gRR);

    float* inOutL = buffer.getWritePointer(0);
    float* inOutR = buffer.getWritePointer(1);

    for (int i = 0; i < numSamples; ++i)
    {
        const float inL = inOutL[i];
        const float inR = inOutR[i];

        const float outL = (gLL * inL + gLR * inR) * trimL;
        const float outR = (gRL * inL + gRR * inR) * trimR;

        inOutL[i] = outL;
        inOutR[i] = outR;
    }

    // If host gives more than 2 channels, clear the rest
    for (int ch = 2; ch < numCh; ++ch)
        buffer.clear(ch, 0, numSamples);
}




//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
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
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}

juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "volumeL", "Volume L",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "volumeR", "Volume R",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "azimuth", "Azimuth",
        juce::NormalisableRange<float>(-90.0f, 90.0f, 0.01f),
        0.0f));

    return { params.begin(), params.end() };
}




