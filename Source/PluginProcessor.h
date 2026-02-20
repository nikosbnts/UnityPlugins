#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <atomic>
#include <memory>
#include <cmath>

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor, public juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;


    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    using ParameterState = juce::AudioProcessorValueTreeState;

    ParameterState parameters;
private:
    ParameterState::ParameterLayout createParameterLayout();
    
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    std::atomic<float> currentVolumeL{ 0.5f };
    std::atomic<float> currentVolumeR{ 0.5f };
    std::atomic<float> currentAzimuthDeg{ 0.0f };   // UI: -90..+90

    static constexpr float speakerAzL = 330.0f;      // Left speaker azimuth
    static constexpr float speakerAzR = 30.0f;      // Right speaker azimuth
    static constexpr float stereoHalfWidthDeg = 30.0f; // preserve stereo width (L=-30, R=+30 around center)

    static float wrap360(float deg) noexcept;
    static void vbap2Speakers(float srcAzDeg360, float ls1AzDeg360, float ls2AzDeg360,
        float& g1, float& g2) noexcept;
 

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
