#include "PluginProcessor.h"

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
    tempMonoInput.setSize(1, samplesPerBlock, false, false, true);
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

    tempMonoInput.setSize(1, numSamples, false, false, true);
    tempMonoInput.clear();

    // Build mono input from mono or stereo source
    if (buffer.getNumChannels() > 0)
        tempMonoInput.copyFrom(0, 0, buffer, 0, 0, numSamples);

    if (inCh > 1 && buffer.getNumChannels() > 1)
    {
        tempMonoInput.addFrom(0, 0, buffer, 1, 0, numSamples);
        tempMonoInput.applyGain(0, 0, numSamples, 0.5f);
    }

    for (int ch = 0; ch < outCh; ++ch)
        buffer.clear(ch, 0, numSamples);

    const float inputGain = inputGainParam ? inputGainParam->load() : 1.0f;
    const float srcAz = sourceAzimuthParam ? sourceAzimuthParam->load() : 0.0f;

    int requestedSpeakers = speakerCountParam ? juce::roundToInt(speakerCountParam->load()) : 2;
    requestedSpeakers = juce::jlimit(2, kMaxSpeakers, requestedSpeakers);

    std::array<float, kMaxSpeakers> speakerAz{};
    std::array<float, kMaxSpeakers> gainsMono{};
    std::array<float, kMaxSpeakers> defaults{};

    vbap::defaultLayoutAngles(requestedSpeakers, defaults.data());

    for (int i = 0; i < requestedSpeakers; ++i)
    {
        const float raw = (speakerAzParams[i] != nullptr) ? speakerAzParams[i]->load() : defaults[i];
        speakerAz[i] = vbap::wrap360(raw);
    }

    vbap::computeVBAP_N(vbap::wrap360(srcAz), speakerAz.data(), requestedSpeakers, gainsMono.data());

    const float* monoIn = tempMonoInput.getReadPointer(0);

    // Multichannel render if host really gives >2 outputs
    if (outCh > 2)
    {
        const int activeOutputs = juce::jmin(requestedSpeakers, outCh, kMaxSpeakers);

        for (int ch = 0; ch < activeOutputs; ++ch)
        {
            float* out = buffer.getWritePointer(ch);
            const float g = gainsMono[ch];

            for (int i = 0; i < numSamples; ++i)
                out[i] = monoIn[i] * inputGain * g;
        }

        for (int ch = activeOutputs; ch < outCh; ++ch)
            buffer.clear(ch, 0, numSamples);

        return;
    }

    // Stereo preview mode for Unity Editor / headphones
    float* outL = buffer.getWritePointer(0);
    float* outR = buffer.getWritePointer(1);

    for (int spk = 0; spk < requestedSpeakers; ++spk)
    {
        const float g = gainsMono[spk];
        if (g <= 0.0f)
            continue;

        const float azRad = speakerAz[spk] * juce::MathConstants<float>::pi / 180.0f;
        const float x = std::sin(azRad); // -1 left, +1 right

        const float leftW = std::sqrt(0.5f * (1.0f - x));
        const float rightW = std::sqrt(0.5f * (1.0f + x));

        const float totalL = inputGain * g * leftW;
        const float totalR = inputGain * g * rightW;

        for (int i = 0; i < numSamples; ++i)
        {
            outL[i] += monoIn[i] * totalL;
            outR[i] += monoIn[i] * totalR;
        }
    }
}

bool AudioPluginAudioProcessor::hasEditor() const
{
    return false;
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return nullptr;
}

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
    vbap::defaultLayoutAngles(8, defaults.data());

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