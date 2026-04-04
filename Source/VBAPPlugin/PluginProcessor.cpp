#include "PluginProcessor.h"
#include "../Common/PluginEditor.h"

AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::create7point1(), true)
#endif
    ),
    parameters(*this, nullptr, "ParameterTree", createParameterLayout())
{
    inputGainParam = parameters.getRawParameterValue("inputGain");
    sourceAzimuthParam = parameters.getRawParameterValue("sourceAzimuth");
    speakerCountParam = parameters.getRawParameterValue("speakerCount");

    for (int i = 0; i < kMaxSpeakers; ++i)
        speakerAzParams[i] = parameters.getRawParameterValue("speakerAz" + juce::String(i + 1));
}

void AudioPluginAudioProcessor::prepareToPlay(double, int samplesPerBlock)
{
    tempMonoInput.setSize(1, juce::jmax(1, samplesPerBlock), false, false, true);
}

void AudioPluginAudioProcessor::releaseResources()
{
    tempMonoInput.setSize(0, 0);
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if ! JucePlugin_IsSynth
    const auto in = layouts.getMainInputChannelSet();
    if (in != juce::AudioChannelSet::mono() && in != juce::AudioChannelSet::stereo())
        return false;
#endif

    const auto out = layouts.getMainOutputChannelSet();
    const int outCh = out.size();

    // Current practical Unity target: 2..8 physical outputs.
    if (outCh < 2 || outCh > 8)
        return false;

    return true;
}

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int inCh = getTotalNumInputChannels();
    const int outCh = buffer.getNumChannels();

    if (numSamples <= 0 || outCh <= 0)
        return;

    if (tempMonoInput.getNumSamples() < numSamples)
        tempMonoInput.setSize(1, numSamples, false, false, true);

    tempMonoInput.clear(0, 0, numSamples);

    // Build mono input from first one or two input channels
    if (inCh > 0)
        tempMonoInput.copyFrom(0, 0, buffer, 0, 0, numSamples);

    if (inCh > 1)
    {
        tempMonoInput.addFrom(0, 0, buffer, 1, 0, numSamples);
        tempMonoInput.applyGain(0, 0, numSamples, 0.5f);
    }

    // Clear all outputs first
    for (int ch = 0; ch < outCh; ++ch)
        buffer.clear(ch, 0, numSamples);

    const float inputGain = inputGainParam ? inputGainParam->load() : 1.0f;
    const float srcAz = sourceAzimuthParam ? sourceAzimuthParam->load() : 0.0f;

    int requestedSpeakers = speakerCountParam ? juce::roundToInt(speakerCountParam->load()) : 2;
    requestedSpeakers = juce::jlimit(2, kMaxSpeakers, requestedSpeakers);

    const int activeSpeakerCount = juce::jmin(requestedSpeakers, outCh, kMaxSpeakers);
    if (activeSpeakerCount < 2)
        return;

    std::array<float, kMaxSpeakers> speakerAz{};
    std::array<float, kMaxSpeakers> gainsMono{};
    std::array<float, kMaxSpeakers> defaults{};

    vbap::DefaultVBAPLayoutAngles(activeSpeakerCount);

    for (int i = 0; i < activeSpeakerCount; ++i)
    {
        const float raw = (speakerAzParams[i] != nullptr) ? speakerAzParams[i]->load()
            : defaults[i];
        speakerAz[i] = vbap::wrap360(raw);
    }

    vbap::computeVBAP_N(vbap::wrap360(srcAz),
        speakerAz.data(),
        activeSpeakerCount,
        gainsMono.data());

    const float* monoIn = tempMonoInput.getReadPointer(0);

    for (int ch = 0; ch < activeSpeakerCount; ++ch)
    {
        float* out = buffer.getWritePointer(ch);
        const float totalGain = inputGain * gainsMono[ch];

        for (int i = 0; i < numSamples; ++i)
            out[i] = monoIn[i] * totalGain;
    }

    for (int ch = activeSpeakerCount; ch < outCh; ++ch)
        buffer.clear(ch, 0, numSamples);
}

bool AudioPluginAudioProcessor::hasEditor() const { return false; }
juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor() { return nullptr; }

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
    return 1;
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String AudioPluginAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

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

juce::AudioProcessorValueTreeState::ParameterLayout
AudioPluginAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.reserve(3 + kMaxSpeakers);

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "inputGain", "Input Gain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "sourceAzimuth", "Source Azimuth",
        juce::NormalisableRange<float>(0.0f, 360.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        "speakerCount", "Speaker Count",
        2, kMaxSpeakers, 2));

    std::array<float, kMaxSpeakers> defaults{};
    vbap::DefaultVBAPLayoutAngles(8);

    for (int i = 0; i < kMaxSpeakers; ++i)
    {
        float def = 0.0f;
        if (i < 8)
            def = defaults[i];
        else
            def = vbap::wrap360((360.0f / static_cast<float>(kMaxSpeakers)) * static_cast<float>(i));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "speakerAz" + juce::String(i + 1),
            "Speaker " + juce::String(i + 1) + " Azimuth",
            juce::NormalisableRange<float>(0.0f, 360.0f, 0.01f), def));
    }

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}