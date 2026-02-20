#pragma once

#include "PluginProcessor.h"
#include "BinaryData.h"


//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor
  
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
private:
    juce::Slider volumeLSlider, volumeRSlider;
    juce::Label  volumeLLabel{ "VolL Label", "Volume L" };
    juce::Label  volumeRLabel{ "VolR Label", "Volume R" };

    juce::Slider azimuthSlider;
    juce::Label  azimuthLabel{ "AzLabel", "Azimuth (deg)" };

    AudioPluginAudioProcessor& processorRef;

    juce::AudioProcessorValueTreeState::SliderAttachment volumeLAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment volumeRAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment azimuthAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
