#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Common/TestSession.h"

class AudioPluginAudioProcessor;

//==============================================================================
class BinauralTestSessionEditor final : public juce::AudioProcessorEditor,
    private juce::Timer
{
public:
    explicit BinauralTestSessionEditor(AudioPluginAudioProcessor& p);
    ~BinauralTestSessionEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

private:
    //==========================================================================
    //  Palette
    //==========================================================================
    static constexpr juce::uint32 colBg = 0xFF14171C;
    static constexpr juce::uint32 colSurface = 0xFF1E232B;
    static constexpr juce::uint32 colSurface2 = 0xFF252B35;
    static constexpr juce::uint32 colBorder = 0xFF2E3540;
    static constexpr juce::uint32 colText = 0xFFE6E8EC;
    static constexpr juce::uint32 colMuted = 0xFF9098A4;   // ← FIXED was same as colText
    static constexpr juce::uint32 colHint = 0xFF5F6673;
    static constexpr juce::uint32 colAccent = 0xFF4A75B5;   // was 0xFF32A2FF
    static constexpr juce::uint32 colAccentSub = 0xFF243D60;   // was 0xFF1E3A5F
    static constexpr juce::uint32 colGreen = 0xFF10B981;
    static constexpr juce::uint32 colOrange = 0xFFFF7652;
    static constexpr juce::uint32 colRed = 0xFFEF4444;

    //==========================================================================
    //  DarkLookAndFeel
    //==========================================================================
    class DarkLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        DarkLookAndFeel()
        {
            setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(colBg));

            setColour(juce::TextEditor::backgroundColourId, juce::Colour(colSurface2));
            setColour(juce::TextEditor::textColourId, juce::Colour(colText));
            setColour(juce::TextEditor::highlightColourId, juce::Colour(colAccent).withAlpha(0.35f));
            setColour(juce::TextEditor::highlightedTextColourId, juce::Colour(colText));
            setColour(juce::TextEditor::outlineColourId, juce::Colour(colBorder));
            setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(colAccent));
            setColour(juce::CaretComponent::caretColourId, juce::Colour(colAccent));

            setColour(juce::ComboBox::backgroundColourId, juce::Colour(colSurface2));
            setColour(juce::ComboBox::textColourId, juce::Colour(colText));
            setColour(juce::ComboBox::outlineColourId, juce::Colour(colBorder));
            setColour(juce::ComboBox::arrowColourId, juce::Colour(colMuted));
            setColour(juce::ComboBox::buttonColourId, juce::Colour(colSurface2));
            setColour(juce::ComboBox::focusedOutlineColourId, juce::Colour(colAccent));

            setColour(juce::PopupMenu::backgroundColourId, juce::Colour(colSurface));
            setColour(juce::PopupMenu::textColourId, juce::Colour(colText));
            setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(colAccentSub));
            setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(colText));

            setColour(juce::Label::textColourId, juce::Colour(colText));
            setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            setColour(juce::TextButton::buttonColourId, juce::Colour(colSurface2));
            setColour(juce::TextButton::buttonOnColourId, juce::Colour(colAccentSub));
            setColour(juce::TextButton::textColourOffId, juce::Colour(colText));
            setColour(juce::TextButton::textColourOnId, juce::Colour(colAccent));
        }

        juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override
        {
            return juce::Font(juce::FontOptions(juce::jmin(15.0f, buttonHeight * 0.55f),
                juce::Font::plain));
        }

        void drawButtonBackground(juce::Graphics& g,
            juce::Button& b,
            const juce::Colour& backgroundColour,
            bool shouldDrawButtonAsHighlighted,
            bool shouldDrawButtonAsDown) override
        {
            const auto bounds = b.getLocalBounds().toFloat().reduced(0.5f, 0.5f);
            const float cornerSize = 8.0f;

            juce::Colour fill = backgroundColour;
            if (shouldDrawButtonAsDown)
                fill = fill.brighter(0.10f);
            else if (shouldDrawButtonAsHighlighted)
                fill = fill.brighter(0.05f);

            g.setColour(fill);
            g.fillRoundedRectangle(bounds, cornerSize);

            g.setColour(juce::Colour(colBorder));
            g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
        }

        void drawComboBox(juce::Graphics& g, int width, int height,
            bool, int, int, int, int, juce::ComboBox& box) override
        {
            const auto bounds = juce::Rectangle<float>(0, 0, (float)width, (float)height).reduced(0.5f);
            const float cornerSize = 8.0f;

            g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
            g.fillRoundedRectangle(bounds, cornerSize);

            g.setColour(box.findColour(juce::ComboBox::outlineColourId));
            g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

            const float arrowX = (float)width - 22.0f;
            const float arrowY = height * 0.5f;
            juce::Path p;
            p.addTriangle(arrowX, arrowY - 3.0f,
                arrowX + 10.0f, arrowY - 3.0f,
                arrowX + 5.0f, arrowY + 3.5f);
            g.setColour(box.findColour(juce::ComboBox::arrowColourId)
                .withAlpha(box.isEnabled() ? 1.0f : 0.4f));
            g.fillPath(p);
        }

        void drawTextEditorOutline(juce::Graphics& g, int width, int height,
            juce::TextEditor& te) override
        {
            const auto bounds = juce::Rectangle<float>(0, 0, (float)width, (float)height).reduced(0.5f);
            g.setColour(te.hasKeyboardFocus(true)
                ? te.findColour(juce::TextEditor::focusedOutlineColourId)
                : te.findColour(juce::TextEditor::outlineColourId));
            g.drawRoundedRectangle(bounds, 8.0f, 1.0f);
        }

        void fillTextEditorBackground(juce::Graphics& g, int width, int height,
            juce::TextEditor& te) override
        {
            g.setColour(te.findColour(juce::TextEditor::backgroundColourId));
            g.fillRoundedRectangle(juce::Rectangle<float>(0, 0, (float)width, (float)height), 8.0f);
        }
    };

    //==========================================================================
    AudioPluginAudioProcessor& processorRef;
    TestSession session;
    DarkLookAndFeel darkLnf;

    int selectedLayoutMode = 4;
    int selectedTopology = 0;

    juce::String buildLayoutLabel() const;

    // Setup widgets
    juce::TextEditor nameEditor;
    juce::TextEditor anglesEditor;
    juce::TextEditor numTrialsEditor;
    juce::ComboBox   layoutBox;
    juce::ComboBox   topologyBox;
    juce::TextButton randomBtn{ "Randomize N trials" };
    juce::TextButton presetBtn{ "8-point preset" };
    juce::TextButton loadAudioBtn{ "Load audio file..." };
    juce::Label      audioFileLabel;
    juce::TextButton startBtn{ "Start session" };

    // Trial widgets
    juce::TextButton playBtn{ "Play" };
    juce::TextButton stopBtn{ "Stop" };
    juce::TextButton submitBtn{ "Submit answer" };
    juce::TextButton confBtn1{ "1" }, confBtn2{ "2" }, confBtn3{ "3" },
        confBtn4{ "4" }, confBtn5{ "5" };

    // Feedback / summary
    juce::TextButton nextBtn{ "Next trial" };
    juce::TextButton saveBtn{ "Save CSV" };
    juce::TextButton newSessBtn{ "New session" };
    juce::TextButton endSessBtn{ "End session" };

    // Geometry
    float circCx = 0, circCy = 0, circR = 0;
    juce::Rectangle<float> circBounds;

    // Card rectangles for setup screen
    juce::Rectangle<int> cardSession;
    juce::Rectangle<int> cardRendering;
    juce::Rectangle<int> cardTrials;
    juce::Rectangle<int> cardAudio;          // ← NEW

    void showSetupWidgets(bool);
    void showTrialWidgets(bool);
    void showFeedbackWidgets(bool);
    void showSummaryWidgets(bool);
    void hideAllWidgets();

    void onLayoutChanged();
    void onRandomize();
    void onPreset();
    void onLoadAudio();
    void onStart();
    void onPlay();
    void onStop();
    void onConfidence(int level);
    void onSubmit();
    void onNext();
    void onSave();
    void onNewSession();
    void onEndSession();

    void styleConfidenceButton(juce::TextButton& btn, bool selected);
    void styleAccentButton(juce::TextButton& btn);

    void syncParametersToProcessor();

    void paintSetup(juce::Graphics& g);
    void paintTrial(juce::Graphics& g);
    void paintFeedback(juce::Graphics& g);
    void paintSummary(juce::Graphics& g);

    void paintCard(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title);
    void paintMiniLabel(juce::Graphics& g, int x, int y, int w, const juce::String& text);  // ← NEW
    void paintProgressBar(juce::Graphics& g, int y, float pct);

    void drawCircle(juce::Graphics& g, float cx, float cy, float r,
        float userAngle, float actualAngle, bool interactive);
    void drawMetricBox(juce::Graphics& g, juce::Rectangle<float> area,
        const juce::String& label, const juce::String& value,
        juce::Colour valueCol);

    juce::Colour errorColour(float err) const;
    static juce::Point<float> angleToPt(float deg, float r, float cx, float cy);

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BinauralTestSessionEditor)
};