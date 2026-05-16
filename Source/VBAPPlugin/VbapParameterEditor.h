#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <memory>

#include "../Common/PluginEditorControls.h"

class AudioPluginAudioProcessor;

class VbapParameterEditor final : public juce::AudioProcessorEditor,
                                  private juce::Timer,
                                  private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit VbapParameterEditor(AudioPluginAudioProcessor& p);
    ~VbapParameterEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp  (const juce::MouseEvent& e) override;

private:
    using APVTS = juce::AudioProcessorValueTreeState;

    AudioPluginAudioProcessor& processorRef;
    pluginUI::DarkLookAndFeel darkLnf;

    juce::Slider inputGainSlider;
    std::unique_ptr<APVTS::SliderAttachment> inputGainAttachment;

    std::unique_ptr<pluginUI::StepperControl>   sourceAzimuthStepper;
    std::unique_ptr<pluginUI::SteppedIntSlider> speakerCountSlider;
    std::array<std::unique_ptr<pluginUI::StepperControl>, 8> speakerSteppers;

    juce::Label inputGainLabel;
    juce::Label sourceAzimuthLabel;
    juce::Label sourceAzimuthHint;
    juce::Label speakerCountLabel;
    std::array<juce::Label, 8> speakerLabels;

    juce::TextButton defaultLayoutBtn { "Default layout" };
    juce::TextButton loadAudioBtn     { "Load audio file..." };
    juce::TextButton playStopBtn;
    juce::Label      audioFileLabel;

    bool wasPlayingLastTick = false;

    juce::Rectangle<float> circleArea;
    juce::Point<float>     circleCenter;
    float                  circleR = 0.0f;

    // -2 = none, -1 = source, 0..7 = speaker index
    int dragTargetIndex = -2;

    int  getCurrentSpeakerCount() const;
    void applyDefaultLayout();
    void styleAccentButton(juce::TextButton& btn);
    void stylePlayStopButton(bool playingNow);
    void setupSideLabel(juce::Label& l, const juce::String& text);
    void updateSpeakerEnabledStates();

    void onLoadAudio();
    void onPlayStopToggle();

    void paintCircle(juce::Graphics& g);
    void paintSectionHeader(juce::Graphics& g, int x, int y, int w, const juce::String& text);
    static juce::Point<float> angleToPt(float deg, float r, float cx, float cy);

    int  hitTestCircle (const juce::MouseEvent& e) const;
    void updateAngleFromMouse(const juce::MouseEvent& e);

    void parameterChanged(const juce::String& id, float newValue) override;
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VbapParameterEditor)
};
