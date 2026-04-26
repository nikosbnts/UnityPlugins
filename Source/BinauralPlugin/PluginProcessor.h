#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>
#include "../Common/VbapEngine2D.h"
#include "../Common/AudioPlayer.h"
#include "HrirBank.h"

class AudioPluginAudioProcessor final : public juce::AudioProcessor
{
public:
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    using ParameterState = juce::AudioProcessorValueTreeState;
    ParameterState parameters;

 
    /** Internal audio player – the editor uses this to load/play stimulus files. */
    AudioFilePlayer audioPlayer;

    /** Logs the active VBAP pair (or HRTF) for a given source azimuth, using
        the current layoutMode + topology parameters. Call from the editor when
        a new trial begins. */
    void logCurrentTrialSelection(float targetAzimuthDeg) const;

    /** True if the given layout mode supports an asymmetric topology. */
    static bool layoutSupportsAsymmetric(int layoutMode) noexcept;


private:
    static constexpr int kMaxSpeakers = vbap::kMaxSpeakers;
    static constexpr double kSourceAzimuthSmoothingTimeSeconds = 0.02;
    static constexpr double kSelectionCrossfadeTimeSeconds = 0.04;

    struct ActivePair
    {
        int indexA = 0;
        int indexB = 0;
        float gainA = 1.0f;
        float gainB = 0.0f;
        float azimuthA = 0.0f;
        float azimuthB = 0.0f;
    };

    struct RenderSelection
    {
        const HrirBank::Entry* hrirA = nullptr;
        const HrirBank::Entry* hrirB = nullptr;
        int hrirAzimuthA = -1;
        int hrirAzimuthB = -1;
        float gainA = 1.0f;
        float gainB = 0.0f;
        bool valid = false;
    };

    ParameterState::ParameterLayout createParameterLayout();

    juce::File getDefaultHrirFolder() const;

    /** Builds the speaker layout for the given (layoutMode, topology) pair. */
    void fillSpeakerAnglesForLayout(int layoutMode,
        int topology,
        std::array<float, kMaxSpeakers>& speakerAzimuths,
        int& speakerCount) const;

    ActivePair findActivePair(float sourceAzimuthDeg,
        const std::array<float, kMaxSpeakers>& speakerAzimuths,
        int speakerCount) const noexcept;

    RenderSelection buildRenderSelection(float sourceAzimuthDeg,
        int layoutMode,
        int topology) const noexcept;

    bool hasDifferentHrirPair(const RenderSelection& a, const RenderSelection& b) const noexcept;
    bool hasAudibleSelectionChange(const RenderSelection& a, const RenderSelection& b) const noexcept;

    void buildMonoInput(juce::AudioBuffer<float>& buffer, int numSamples);
    void prepareHistoryBuffer();
    void pushInputSample(float x);
    float convolveHistoryWithIr(const HrirBank::Entry& entry, int irChannel, float gain) const noexcept;
    void renderSelectionSample(const RenderSelection& selection, float& yL, float& yR) const noexcept;


    std::atomic<float>* inputGainParam = nullptr;
    std::atomic<float>* sourceAzimuthParam = nullptr;
    std::atomic<float>* layoutModeParam = nullptr;
    std::atomic<float>* topologyParam = nullptr;

    juce::AudioBuffer<float> monoInputBuffer;

    HrirBank hrirBank;
    bool hrirLoaded = false;

    std::vector<float> inputHistory;
    int historyWritePos = 0;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedSourceAzimuth;
    static float unwrapTargetAzimuthNearReference(float referenceDegrees, float targetDegrees) noexcept;

    RenderSelection currentSelection;
    RenderSelection previousSelection;
    int selectionCrossfadeLengthSamples = 0;
    int selectionCrossfadeSamplesRemaining = 0;
    // DEBUG state
    int lastLoggedLayoutMode = -1;
    int lastLoggedTopology = -1;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};