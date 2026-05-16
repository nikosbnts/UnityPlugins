#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <memory>

#include "../Common/PluginEditorControls.h"

class AudioPluginAudioProcessor;

class BinauralParameterEditor final : public juce::AudioProcessorEditor,
                                      private juce::Timer,
                                      private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit BinauralParameterEditor(AudioPluginAudioProcessor& p);
    ~BinauralParameterEditor() override;

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

    std::unique_ptr<pluginUI::StepperControl> sourceAzimuthStepper;

    juce::Label inputGainLabel;
    juce::Label sourceAzimuthLabel;
    juce::Label sourceAzimuthHint;

    std::array<juce::TextButton, 6> layoutChips;     // Direct HRTF / VBAP 5/7/9/12/18
    std::array<juce::TextButton, 2> topologyChips;   // Standard / Symmetrical

    juce::TextButton loadAudioBtn { "Load audio file..." };
    juce::TextButton playStopBtn;
    juce::Label      audioFileLabel;

    bool wasPlayingLastTick = false;

    juce::Rectangle<float> circleArea;
    juce::Point<float>     circleCenter;
    float                  circleR = 0.0f;

    // Only the source dot is draggable on the circle. Speakers are read-only.
    bool draggingSource = false;

    int  getLayoutMode() const;
    int  getTopology()   const;
    bool topologyValidForLayout(int layoutMode, int topology) const;

    void onLayoutChipClicked(int layoutMode);
    void onTopologyChipClicked(int topology);
    void refreshLayoutChips();
    void refreshTopologyChips();

    void styleAccentButton(juce::TextButton& btn);
    void styleChipButton(juce::TextButton& btn, bool selected, bool enabled);
    void stylePlayStopButton(bool playingNow);
    void setupSideLabel(juce::Label& l, const juce::String& text);

    void onLoadAudio();
    void onPlayStopToggle();

    void paintCircle(juce::Graphics& g);
    void paintSectionHeader(juce::Graphics& g, int x, int y, int w, const juce::String& text);
    juce::String currentPresetName() const;

    static juce::Point<float> angleToPt(float deg, float r, float cx, float cy);

    bool hitTestSource(const juce::MouseEvent& e) const;
    void updateSourceAngleFromMouse(const juce::MouseEvent& e);

    void parameterChanged(const juce::String& id, float newValue) override;
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BinauralParameterEditor)
};
