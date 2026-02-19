#pragma once

#include "PluginProcessor.h"
#include "BinaryData.h"


//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor,
    private juce::Timer
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
private:


    void timerCallback() override;

    // ===== Oscilloscope =====
    juce::Rectangle<int> scopeBounds;
    juce::Path scopePath;
    static constexpr int scopeNumSamples = 512;
    std::vector<float> scopeDisplayBuffer;
    std::vector<float> scopePullBuffer;

    juce::TextButton loadWavButton{ "Load WAV" };
    juce::ToggleButton playToggle{ "Play" };
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::Slider volumeSlider;
    juce::Label volumeLabel{ "Vol Label", "Volume" };
    AudioPluginAudioProcessor& processorRef;
    
    juce::AudioProcessorValueTreeState::SliderAttachment volumeSliderAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
