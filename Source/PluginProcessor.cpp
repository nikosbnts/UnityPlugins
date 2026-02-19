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


    parameters.addParameterListener("volumeL", this);
    parameters.addParameterListener("volumeR", this);

    
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{

    parameters.removeParameterListener("volumeL", this);
    parameters.removeParameterListener("volumeR", this);
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

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    const int numSamples = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    if (numCh == 0 || numSamples == 0)
        return;

    const float volL = currentVolumeL.load();
    const float volR = currentVolumeR.load();

    // Apply gain per channel (stereo)
    float* left = buffer.getWritePointer(0);
    for (int i = 0; i < numSamples; ++i)
        left[i] *= volL;

    if (numCh > 1)
    {
        float* right = buffer.getWritePointer(1);
        for (int i = 0; i < numSamples; ++i)
            right[i] *= volR;
    }



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

    return { params.begin(), params.end() };
}




