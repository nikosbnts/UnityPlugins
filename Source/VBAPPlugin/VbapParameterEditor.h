#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <memory>

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

    //==========================================================================
    //  Palette (matches Binaural test)
    //==========================================================================
    static constexpr juce::uint32 colBg        = 0xFF14171C;
    static constexpr juce::uint32 colSurface   = 0xFF1E232B;
    static constexpr juce::uint32 colSurface2  = 0xFF252B35;
    static constexpr juce::uint32 colBorder    = 0xFF2E3540;
    static constexpr juce::uint32 colText      = 0xFFE6E8EC;
    static constexpr juce::uint32 colMuted     = 0xFF9098A4;
    static constexpr juce::uint32 colHint      = 0xFF5F6673;
    static constexpr juce::uint32 colAccent    = 0xFF4A75B5;
    static constexpr juce::uint32 colAccentSub = 0xFF243D60;
    static constexpr juce::uint32 colGreen     = 0xFF10B981;
    static constexpr juce::uint32 colOrange    = 0xFFFF7652;
    static constexpr juce::uint32 colRed       = 0xFFEF4444;

    //==========================================================================
    //  DarkLookAndFeel — rounded buttons & text editors, dark palette
    //==========================================================================
    class DarkLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        DarkLookAndFeel();
        juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
        void drawButtonBackground(juce::Graphics&, juce::Button&,
                                  const juce::Colour& backgroundColour,
                                  bool shouldDrawAsHighlighted,
                                  bool shouldDrawAsDown) override;
        void drawTextEditorOutline(juce::Graphics&, int width, int height,
                                   juce::TextEditor&) override;
        void fillTextEditorBackground(juce::Graphics&, int width, int height,
                                      juce::TextEditor&) override;
    };

    //==========================================================================
    //  StepperControl — [−] [editor] [+], bound to an APVTS parameter
    //==========================================================================
    class StepperControl : public juce::Component,
                           private juce::AudioProcessorValueTreeState::Listener
    {
    public:
        StepperControl(APVTS& apvts, const juce::String& paramId,
                       float minVal, float maxVal, float step, bool wraps);
        ~StepperControl() override;

        void resized() override;
        void setEnabledState(bool enabled);

    private:
        void parameterChanged(const juce::String& id, float newValue) override;
        void refreshFromParameter();
        void stepBy(float delta);
        void applyTypedValue();
        float wrapOrClamp(float v) const;

        APVTS& apvtsRef;
        juce::String paramId;
        float minValue, maxValue, stepSize;
        bool wraps;

        juce::TextButton decBtn;
        juce::TextButton incBtn { "+" };
        juce::TextEditor valueEditor;
    };

    //==========================================================================
    //  SteppedIntSlider — discrete horizontal slider with dot per integer value
    //==========================================================================
    class SteppedIntSlider : public juce::Component,
                             private juce::AudioProcessorValueTreeState::Listener
    {
    public:
        SteppedIntSlider(APVTS& apvts, const juce::String& paramId, int minVal, int maxVal);
        ~SteppedIntSlider() override;

        void paint(juce::Graphics&) override;
        void mouseDown(const juce::MouseEvent&) override;
        void mouseDrag(const juce::MouseEvent&) override;

        std::function<void()> onValueChange;

    private:
        void parameterChanged(const juce::String& id, float newValue) override;
        void refreshFromParameter();
        void setValueFromX(float xpos);
        float xForValue(int v) const;

        APVTS& apvtsRef;
        juce::String paramId;
        int minValue, maxValue;
        int currentValue = 2;
    };

    //==========================================================================
    AudioPluginAudioProcessor& processorRef;
    DarkLookAndFeel darkLnf;

    juce::Slider inputGainSlider;
    std::unique_ptr<APVTS::SliderAttachment> inputGainAttachment;

    std::unique_ptr<StepperControl>   sourceAzimuthStepper;
    std::unique_ptr<SteppedIntSlider> speakerCountSlider;
    std::array<std::unique_ptr<StepperControl>, 8> speakerSteppers;

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

    int  hitTestCircle(const juce::MouseEvent& e) const;
    void updateAngleFromMouse(const juce::MouseEvent& e);

    void parameterChanged(const juce::String& id, float newValue) override;
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VbapParameterEditor)
};
