#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>
#include <memory>

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor, public juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;
    bool loadWavFile(const juce::File& file);
    void setPlaying(bool shouldPlay) noexcept { playing.store(shouldPlay); }
    bool isPlaying() const noexcept { return playing.load(); }
    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    int popScopeSamples(float* dest, int maxSamples) noexcept;
    static constexpr const char* kVolumeParamID = "volume";
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

    juce::AudioFormatManager formatManager;

    juce::SpinLock sampleLock;
    std::shared_ptr<juce::AudioBuffer<float>> sampleBuffer;
    double sampleBufferRate = 0.0;

    double samplePosition = 0.0;          // in "sampleBuffer samples" (can be fractional)
    std::atomic<bool> playing{ true };
    std::atomic<float> currentVolume{ 0.02f };
    static constexpr int scopeFifoSize = 8192;
    juce::AbstractFifo scopeFifo{ scopeFifoSize };
    std::vector<float> scopeFifoBuffer;
    void pushScopeSamples(const float* samples, int numSamples) noexcept;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
