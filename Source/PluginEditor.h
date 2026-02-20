#pragma once

#include "PluginProcessor.h"

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Two volume sliders only (as requested)
    juce::Slider volumeLSlider, volumeRSlider;
    juce::Label  volumeLLabel{ "v1", "Channel 1" };
    juce::Label  volumeRLabel{ "v2", "Channel 2" };

    // VBAP control (not a volume slider)
    juce::Slider azimuthSlider;
    juce::Label  azimuthLabel{ "az", "Azimuth (deg)" };

    AudioPluginAudioProcessor& processorRef;

    juce::AudioProcessorValueTreeState::SliderAttachment volumeLAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment volumeRAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment azimuthAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};