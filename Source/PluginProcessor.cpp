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
    formatManager.registerBasicFormats();          // <--- ADD

    scopeFifoBuffer.resize(scopeFifoSize, 0.0f);

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

    samplePosition = 0.0f;

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

    juce::ScopedNoDenormals noDenormals;

    // We want to generate output, not pass input through:
    buffer.clear();

    // Get snapshot of the loaded buffer safely
    std::shared_ptr<juce::AudioBuffer<float>> localSample;
    double localSampleRate = 0.0;

    {
        const juce::SpinLock::ScopedLockType lock(sampleLock);
        localSample = sampleBuffer;
        localSampleRate = sampleBufferRate;
    }

    if (!playing.load() || localSample == nullptr || localSample->getNumSamples() <= 0 || localSampleRate <= 0.0)
        return;

    const int outNumSamples = buffer.getNumSamples();
    const int outNumCh = buffer.getNumChannels();

    const int srcNumSamples = localSample->getNumSamples();
    const int srcNumCh = localSample->getNumChannels();

    const double hostRate = getSampleRate();
    const double step = localSampleRate / hostRate;  // resample ratio (simple linear)

    const float volL = currentVolumeL.load();
    const float volR = currentVolumeR.load();

    for (int i = 0; i < outNumSamples; ++i)
    {
        int idx0 = (int)std::floor(samplePosition);
        float frac = (float)(samplePosition - (double)idx0);

        // wrap
        if (idx0 >= srcNumSamples)
            idx0 %= srcNumSamples;

        int idx1 = idx0 + 1;
        if (idx1 >= srcNumSamples)
            idx1 = 0;

        for (int ch = 0; ch < outNumCh; ++ch)
        {
            const int srcCh = juce::jmin(ch, srcNumCh - 1); // mono -> duplicate
            const float* src = localSample->getReadPointer(srcCh);

            const float s0 = src[idx0];
            const float s1 = src[idx1];
            const float s = s0 + (s1 - s0) * frac;

            const float v = (ch == 0 ? volL : volR); // L/R volume
            buffer.setSample(ch, i, s * v);
        }

        samplePosition += step;
        while (samplePosition >= (double)srcNumSamples)
            samplePosition -= (double)srcNumSamples;
    }

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

  
    juce::NormalisableRange<float> volumeRange{ 0.0f, 1.0f, 0.001f, 1.0f };

    parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
        "volumeL", "Volume L",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
        0.5f));

    parameterList.push_back(std::make_unique<juce::AudioParameterFloat>(
        "volumeR", "Volume R",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
        0.5f));

    return { parameterList.begin(), parameterList.end() };
}

void AudioPluginAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "volumeL")
        currentVolumeL.store(newValue);
    else if (parameterID == "volumeR")
        currentVolumeR.store(newValue);
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

bool AudioPluginAudioProcessor::loadWavFile(const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

    if (reader == nullptr)
        return false;

    const int numCh = (int)reader->numChannels;
    const int numSamples = (int)reader->lengthInSamples;

    if (numCh <= 0 || numSamples <= 0)
        return false;

    auto newBuffer = std::make_shared<juce::AudioBuffer<float>>(numCh, numSamples);
    newBuffer->clear();

    reader->read(newBuffer.get(),
        0,
        numSamples,
        0,
        true,
        true);

    {
        const juce::SpinLock::ScopedLockType lock(sampleLock);
        sampleBuffer = std::move(newBuffer);
        sampleBufferRate = reader->sampleRate;
        samplePosition = 0.0;
    }

    return true;
}
