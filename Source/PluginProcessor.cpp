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
    parameters.addParameterListener("frequency", this);
    scopeFifoBuffer.resize(scopeFifoSize, 0.0f);
    parameters.addParameterListener("volume", this);
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
    parameters.removeParameterListener("frequency", this);
    parameters.removeParameterListener("volume", this);
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
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    sineWave.prepare (sampleRate, getTotalNumOutputChannels());
    if (auto* freq = parameters.getRawParameterValue("frequency"))
        sineWave.setFrequency(freq->load());

    if (auto* vol = parameters.getRawParameterValue("volume"))
        sineWave.setAmplitude(vol->load());
}

void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    sineWave.process(buffer);
    if (buffer.getNumChannels() > 0)
        pushScopeSamples(buffer.getReadPointer(0), buffer.getNumSamples());
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
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameterList;

    juce::NormalisableRange<float> frequencyRange { 20.0f, 2000.0f, 0.1f, 0.5f };

    parameterList.push_back(std::make_unique<juce::AudioParameterFloat>("frequency",
                                                                                    "Frequency",
                                                                                    frequencyRange,
                                                                                    500.0f));
    juce::NormalisableRange<float> volumeRange{ 0.0f, 1.0f, 0.001f, 1.0f };

    parameterList.push_back(std::make_unique<juce::AudioParameterFloat>("volume",
        "Volume",
        volumeRange,
        0.02f));
    return { parameterList.begin(), parameterList.end() };
}

void AudioPluginAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "frequency")
    {
        sineWave.setFrequency (newValue);
    }
    else if (parameterID == "volume")
    {
        sineWave.setAmplitude (newValue);
    }
}


void AudioPluginAudioProcessor::pushScopeSamples(const float* samples, int numSamples) noexcept
{
    if (samples == nullptr || numSamples <= 0)
        return;

    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    scopeFifo.prepareToWrite(numSamples, start1, size1, start2, size2);

    if (size1 > 0)
        std::memcpy(scopeFifoBuffer.data() + start1, samples, (size_t)size1 * sizeof(float));

    if (size2 > 0)
        std::memcpy(scopeFifoBuffer.data() + start2, samples + size1, (size_t)size2 * sizeof(float));

    scopeFifo.finishedWrite(size1 + size2);
}

int AudioPluginAudioProcessor::popScopeSamples(float* dest, int maxSamples) noexcept
{
    if (dest == nullptr || maxSamples <= 0)
        return 0;

    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    scopeFifo.prepareToRead(maxSamples, start1, size1, start2, size2);

    if (size1 > 0)
        std::memcpy(dest, scopeFifoBuffer.data() + start1, (size_t)size1 * sizeof(float));

    if (size2 > 0)
        std::memcpy(dest + size1, scopeFifoBuffer.data() + start2, (size_t)size2 * sizeof(float));

    scopeFifo.finishedRead(size1 + size2);
    return size1 + size2;
}