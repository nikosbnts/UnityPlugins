#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/**
 *  Shared dark-themed UI controls used by VBAP / Binaural parameter editors.
 *  Palette + components are based on the Binaural test editor styling.
 */
namespace pluginUI
{
    constexpr juce::uint32 colBg        = 0xFF14171C;
    constexpr juce::uint32 colSurface   = 0xFF1E232B;
    constexpr juce::uint32 colSurface2  = 0xFF252B35;
    constexpr juce::uint32 colBorder    = 0xFF2E3540;
    constexpr juce::uint32 colText      = 0xFFE6E8EC;
    constexpr juce::uint32 colMuted     = 0xFF9098A4;
    constexpr juce::uint32 colHint      = 0xFF5F6673;
    constexpr juce::uint32 colAccent    = 0xFF4A75B5;
    constexpr juce::uint32 colAccentSub = 0xFF243D60;
    constexpr juce::uint32 colGreen     = 0xFF10B981;
    constexpr juce::uint32 colOrange    = 0xFFFF7652;
    constexpr juce::uint32 colRed       = 0xFFEF4444;

    /** Rounded buttons + text editors, dark palette. */
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

    /** [−] [editor] [+] widget bound to an APVTS float parameter.
        Step ±stepSize, click-to-type, optional wrap on [min, max). */
    class StepperControl : public juce::Component,
                           private juce::AudioProcessorValueTreeState::Listener
    {
    public:
        using APVTS = juce::AudioProcessorValueTreeState;

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

    /** Discrete horizontal slider with a dot per integer in [min, max].
        Bound to an APVTS int parameter; emits onValueChange when value flips. */
    class SteppedIntSlider : public juce::Component,
                             private juce::AudioProcessorValueTreeState::Listener
    {
    public:
        using APVTS = juce::AudioProcessorValueTreeState;

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
}
