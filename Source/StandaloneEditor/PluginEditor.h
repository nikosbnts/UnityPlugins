#pragma once
#include "PluginProcessor.h"

class AudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        public juce::Button::Listener,
                                        public juce::Slider::Listener
{
public:
    AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    
    void buttonClicked (juce::Button* button) override;
    void sliderValueChanged (juce::Slider* slider) override;

private:
    // Βασικά Controls (Σύνδεση με Processor)
    juce::Slider azimuthSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> azimuthAttachment;

    // --- TEST GUI SECTION (Αντίστοιχο του HTML) ---
    juce::GroupComponent testGroup { "testGroup", "3D Audio Testing" };
    
    juce::Label numPointsLabel { {}, "Number of Test Points:" };
    juce::Slider numPointsSlider; // Επιλογή για 4, 8, 12, 16 σημεία
    
    juce::TextButton nextBtn { "Next Position >>" };
    juce::TextButton prevBtn { "<< Previous" };
    
    juce::Label currentInfoLabel; // Δείχνει: "Point 2 of 8 (Angle: 90°)"
    
    int currentTestIndex = 0;
    int totalTestPoints = 4;

    void updateTestPosition();

    AudioPluginAudioProcessor& audioProcessor;
    JU_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};