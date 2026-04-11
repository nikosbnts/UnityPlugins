#include "PluginProcessor.h"
#include "BinauralTestSessionEditor.h"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
    parameters(*this, nullptr, "ParameterTree", createParameterLayout())
{
    inputGainParam = parameters.getRawParameterValue("inputGain");
    sourceAzimuthParam = parameters.getRawParameterValue("sourceAzimuth");
    layoutModeParam = parameters.getRawParameterValue("layoutMode");
}

void AudioPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    monoInputBuffer.setSize(1, samplesPerBlock, false, false, true);

    if (!hrirLoaded)
        hrirLoaded = hrirBank.loadFromFolder(getDefaultHrirFolder());

    prepareHistoryBuffer();

    smoothedSourceAzimuth.reset(sampleRate, kSourceAzimuthSmoothingTimeSeconds);

    const float initialAzimuth = sourceAzimuthParam ? sourceAzimuthParam->load() : 0.0f;
    smoothedSourceAzimuth.setCurrentAndTargetValue(vbap::wrap360(initialAzimuth));

    selectionCrossfadeLengthSamples = juce::jmax(1, juce::roundToInt(sampleRate * kSelectionCrossfadeTimeSeconds));
    selectionCrossfadeSamplesRemaining = 0;

    currentSelection = {};
    previousSelection = {};
}

void AudioPluginAudioProcessor::releaseResources()
{
    monoInputBuffer.setSize(0, 0);
    inputHistory.clear();
    historyWritePos = 0;
    selectionCrossfadeSamplesRemaining = 0;
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if ! JucePlugin_IsSynth
    const auto input = layouts.getMainInputChannelSet();
    if (input != juce::AudioChannelSet::mono() &&
        input != juce::AudioChannelSet::stereo())
        return false;
#endif

    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

juce::File AudioPluginAudioProcessor::getDefaultHrirFolder() const
{
    return juce::File("C:/Users/nikos/Desktop/Diplomatiki/JucePlugins/Assets/HRIR");
}

float AudioPluginAudioProcessor::unwrapTargetAzimuthNearReference(float referenceDegrees,
    float targetDegrees) noexcept
{
    float wrappedTarget = vbap::wrap360(targetDegrees);

    while ((wrappedTarget - referenceDegrees) > 180.0f)
        wrappedTarget -= 360.0f;

    while ((wrappedTarget - referenceDegrees) < -180.0f)
        wrappedTarget += 360.0f;

    return wrappedTarget;
}

AudioPluginAudioProcessor::LayoutDefinition AudioPluginAudioProcessor::getLayoutDefinition(int layoutMode) const noexcept
{
    switch (layoutMode)
    {
    case 0: return { 9, 40 };
    case 1: return { 12, 30 };
    case 2: return { 18, 20 };
    case 3: return { 36, 10 };
    default: return { 36, 10 };
    }
}

void AudioPluginAudioProcessor::fillSpeakerAnglesForLayout(int layoutMode,
    std::array<float, kMaxSpeakers>& speakerAzimuths,
    int& speakerCount) const
{
    const auto layout = getLayoutDefinition(layoutMode);
    speakerCount = layout.speakerCount;

    for (int i = 0; i < speakerCount; ++i)
        speakerAzimuths[i] = vbap::wrap360(static_cast<float>(i * layout.stepDegrees));

    for (int i = speakerCount; i < kMaxSpeakers; ++i)
        speakerAzimuths[i] = 0.0f;
}

AudioPluginAudioProcessor::ActivePair AudioPluginAudioProcessor::findActivePair(
    float sourceAzimuthDeg,
    const std::array<float, kMaxSpeakers>& speakerAzimuths,
    int speakerCount) const noexcept
{
    std::array<float, kMaxSpeakers> gains{};
    vbap::computeVBAP_N(vbap::wrap360(sourceAzimuthDeg), speakerAzimuths.data(), speakerCount, gains.data());

    ActivePair pair{};

    int found = 0;
    for (int i = 0; i < speakerCount; ++i)
    {
        if (gains[i] > 1.0e-5f)
        {
            if (found == 0)
            {
                pair.indexA = i;
                pair.gainA = gains[i];
                pair.azimuthA = speakerAzimuths[i];
                ++found;
            }
            else
            {
                pair.indexB = i;
                pair.gainB = gains[i];
                pair.azimuthB = speakerAzimuths[i];
                ++found;
                break;
            }
        }
    }

    if (found == 0)
    {
        pair.indexA = 0;
        pair.indexB = 0;
        pair.gainA = 1.0f;
        pair.gainB = 0.0f;
        pair.azimuthA = speakerAzimuths[0];
        pair.azimuthB = speakerAzimuths[0];
    }
    else if (found == 1)
    {
        pair.indexB = pair.indexA;
        pair.gainB = 0.0f;
        pair.azimuthB = pair.azimuthA;
    }

    return pair;
}

AudioPluginAudioProcessor::RenderSelection AudioPluginAudioProcessor::buildRenderSelection(float sourceAzimuthDeg,
    int layoutMode) const noexcept
{
    RenderSelection selection{};

    if (!hrirLoaded || !hrirBank.isLoaded())
        return selection;

    std::array<float, kMaxSpeakers> speakerAzimuths{};
    int speakerCount = 36;
    fillSpeakerAnglesForLayout(layoutMode, speakerAzimuths, speakerCount);

    const auto pair = findActivePair(sourceAzimuthDeg, speakerAzimuths, speakerCount);

    const float mirroredAzimuthA = vbap::wrap360(360.0f - pair.azimuthA);
    const float mirroredAzimuthB = vbap::wrap360(360.0f - pair.azimuthB);

    selection.hrirA = hrirBank.getNearest(mirroredAzimuthA);
    selection.hrirB = hrirBank.getNearest(mirroredAzimuthB);

    selection.hrirAzimuthA = selection.hrirA != nullptr ? selection.hrirA->azimuthDeg : -1;
    selection.hrirAzimuthB = selection.hrirB != nullptr ? selection.hrirB->azimuthDeg : -1;

    selection.gainA = pair.gainA;
    selection.gainB = pair.gainB;
    selection.valid = (selection.hrirA != nullptr);

    return selection;
}

bool AudioPluginAudioProcessor::hasDifferentHrirPair(const RenderSelection& a,
    const RenderSelection& b) const noexcept
{
    return a.hrirAzimuthA != b.hrirAzimuthA || a.hrirAzimuthB != b.hrirAzimuthB;
}

void AudioPluginAudioProcessor::buildMonoInput(juce::AudioBuffer<float>& buffer, int numSamples)
{
    monoInputBuffer.setSize(1, numSamples, false, false, true);
    monoInputBuffer.clear();

    // ── Check if internal audio player is active ─────────────────
    if (audioPlayer.isPlaying())
    {
        float* mono = monoInputBuffer.getWritePointer(0);
        for (int i = 0; i < numSamples; ++i)
            mono[i] = audioPlayer.getNextSample();
    }
    else
    {
        // Use DAW input (original behaviour)
        if (buffer.getNumChannels() > 0)
            monoInputBuffer.copyFrom(0, 0, buffer, 0, 0, numSamples);

        if (getTotalNumInputChannels() > 1 && buffer.getNumChannels() > 1)
        {
            monoInputBuffer.addFrom(0, 0, buffer, 1, 0, numSamples);
            monoInputBuffer.applyGain(0, 0, numSamples, 0.5f);
        }
    }
}

void AudioPluginAudioProcessor::prepareHistoryBuffer()
{
    const int historySize = juce::jmax(1, hrirBank.getMaxLength());
    inputHistory.assign(static_cast<size_t>(historySize), 0.0f);
    historyWritePos = 0;
}

void AudioPluginAudioProcessor::pushInputSample(float x)
{
    if (inputHistory.empty())
        return;

    inputHistory[static_cast<size_t>(historyWritePos)] = x;
    ++historyWritePos;

    if (historyWritePos >= static_cast<int>(inputHistory.size()))
        historyWritePos = 0;
}

float AudioPluginAudioProcessor::convolveHistoryWithIr(const HrirBank::Entry& entry,
    int irChannel,
    float gain) const noexcept
{
    if (gain == 0.0f || inputHistory.empty())
        return 0.0f;

    const float* ir = entry.ir.getReadPointer(irChannel);
    const int tapCount = juce::jmin(entry.ir.getNumSamples(), static_cast<int>(inputHistory.size()));

    float acc = 0.0f;
    int readPos = historyWritePos - 1;
    if (readPos < 0)
        readPos = static_cast<int>(inputHistory.size()) - 1;

    for (int i = 0; i < tapCount; ++i)
    {
        acc += inputHistory[static_cast<size_t>(readPos)] * ir[i];

        --readPos;
        if (readPos < 0)
            readPos = static_cast<int>(inputHistory.size()) - 1;
    }

    return acc * gain;
}

void AudioPluginAudioProcessor::renderSelectionSample(const RenderSelection& selection,
    float& yL,
    float& yR) const noexcept
{
    yL = 0.0f;
    yR = 0.0f;

    if (!selection.valid || selection.hrirA == nullptr)
        return;

    yL += convolveHistoryWithIr(*selection.hrirA, 0, selection.gainA);
    if (selection.gainB > 0.0f && selection.hrirB != nullptr)
        yL += convolveHistoryWithIr(*selection.hrirB, 0, selection.gainB);

    yR += convolveHistoryWithIr(*selection.hrirA, 1, selection.gainA);
    if (selection.gainB > 0.0f && selection.hrirB != nullptr)
        yR += convolveHistoryWithIr(*selection.hrirB, 1, selection.gainB);
}

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numOutCh = getTotalNumOutputChannels();

    if (numSamples <= 0 || numOutCh < 2)
        return;

    buildMonoInput(buffer, numSamples);
    buffer.clear();

    if (!hrirLoaded || !hrirBank.isLoaded())
        return;

    const float inputGain = inputGainParam ? inputGainParam->load() : 1.0f;
    const int layoutMode = layoutModeParam ? juce::roundToInt(layoutModeParam->load()) : 3;
    const float targetSourceAzimuth = sourceAzimuthParam ? sourceAzimuthParam->load() : 0.0f;

    const float currentSmoothedAzimuth = smoothedSourceAzimuth.getCurrentValue();
    const float unwrappedTargetAzimuth =
        unwrapTargetAzimuthNearReference(currentSmoothedAzimuth, targetSourceAzimuth);

    smoothedSourceAzimuth.setTargetValue(unwrappedTargetAzimuth);

    float* outL = buffer.getWritePointer(0);
    float* outR = buffer.getWritePointer(1);
    const float* monoIn = monoInputBuffer.getReadPointer(0);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float smoothedAzimuth = smoothedSourceAzimuth.getNextValue();
        const RenderSelection sampleSelection = buildRenderSelection(smoothedAzimuth, layoutMode);

        if (!currentSelection.valid)
        {
            currentSelection = sampleSelection;
            previousSelection = sampleSelection;
            selectionCrossfadeSamplesRemaining = 0;
        }
        else if (sampleSelection.valid && hasAudibleSelectionChange(sampleSelection, currentSelection))
        {
            previousSelection = currentSelection;
            currentSelection = sampleSelection;
            selectionCrossfadeSamplesRemaining = selectionCrossfadeLengthSamples;
        }
        else
        {
            currentSelection = sampleSelection;
        }

        const float s = monoIn[sample] * inputGain;
        pushInputSample(s);

        float currentL = 0.0f, currentR = 0.0f;
        renderSelectionSample(currentSelection, currentL, currentR);

        if (selectionCrossfadeSamplesRemaining > 0)
        {
            float previousL = 0.0f, previousR = 0.0f;
            renderSelectionSample(previousSelection, previousL, previousR);

            const float alpha =
                1.0f - (static_cast<float>(selectionCrossfadeSamplesRemaining) /
                    static_cast<float>(selectionCrossfadeLengthSamples));

            const float fadeIn = std::sin(alpha * juce::MathConstants<float>::halfPi);
            const float fadeOut = std::cos(alpha * juce::MathConstants<float>::halfPi);

            outL[sample] = previousL * fadeOut + currentL * fadeIn;
            outR[sample] = previousR * fadeOut + currentR * fadeIn;

            --selectionCrossfadeSamplesRemaining;
        }
        else
        {
            outL[sample] = currentL;
            outR[sample] = currentR;
        }
    }
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const { 
    return true; 
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new BinauralTestSessionEditor(*this);
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
    params.reserve(3);

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "inputGain",
        "Input Gain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
        1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "sourceAzimuth",
        "Source Azimuth",
        juce::NormalisableRange<float>(0.0f, 360.0f, 0.01f),
        0.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "layoutMode",
        "Layout Mode",
        juce::StringArray{
            "9 speakers (40 deg)",
            "12 speakers (30 deg)",
            "18 speakers (20 deg)",
            "36 speakers (10 deg)"
        },
        3));

    return { params.begin(), params.end() };
}

bool AudioPluginAudioProcessor::hasAudibleSelectionChange(const RenderSelection& a,
    const RenderSelection& b) const noexcept
{
    if (a.hrirAzimuthA != b.hrirAzimuthA || a.hrirAzimuthB != b.hrirAzimuthB)
        return true;

    if (std::abs(a.gainA - b.gainA) > 0.02f)
        return true;

    if (std::abs(a.gainB - b.gainB) > 0.02f)
        return true;

    return false;
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
