#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <functional>
#include <vector>
#include "../Common/TestSession.h"

class AudioPluginAudioProcessor;

//==============================================================================
class BinauralTestSessionEditor final : public juce::AudioProcessorEditor,
                                        private juce::Timer,
                                        public juce::ListBoxModel
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
    static constexpr juce::uint32 colBg       = 0xFF14171C;
    static constexpr juce::uint32 colSurface  = 0xFF1E232B;
    static constexpr juce::uint32 colSurface2 = 0xFF252B35;
    static constexpr juce::uint32 colBorder   = 0xFF2E3540;
    static constexpr juce::uint32 colText     = 0xFFE6E8EC;
    static constexpr juce::uint32 colMuted    = 0xFF9098A4;
    static constexpr juce::uint32 colHint     = 0xFF5F6673;
    static constexpr juce::uint32 colAccent   = 0xFF4A75B5;
    static constexpr juce::uint32 colAccentSub= 0xFF243D60;
    static constexpr juce::uint32 colGreen    = 0xFF10B981;
    static constexpr juce::uint32 colOrange   = 0xFFFF7652;
    static constexpr juce::uint32 colRed      = 0xFFEF4444;

    //==========================================================================
    //  DarkLookAndFeel
    //==========================================================================
    class DarkLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        DarkLookAndFeel()
        {
            setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(colBg));

            setColour(juce::TextEditor::backgroundColourId,        juce::Colour(colSurface2));
            setColour(juce::TextEditor::textColourId,              juce::Colour(colText));
            setColour(juce::TextEditor::highlightColourId,         juce::Colour(colAccent).withAlpha(0.35f));
            setColour(juce::TextEditor::highlightedTextColourId,   juce::Colour(colText));
            setColour(juce::TextEditor::outlineColourId,           juce::Colour(colBorder));
            setColour(juce::TextEditor::focusedOutlineColourId,    juce::Colour(colAccent));
            setColour(juce::CaretComponent::caretColourId,         juce::Colour(colAccent));

            setColour(juce::Label::textColourId,       juce::Colour(colText));
            setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

            setColour(juce::TextButton::buttonColourId,    juce::Colour(colSurface2));
            setColour(juce::TextButton::buttonOnColourId,  juce::Colour(colAccentSub));
            setColour(juce::TextButton::textColourOffId,   juce::Colour(colText));
            setColour(juce::TextButton::textColourOnId,    juce::Colour(colAccent));

            setColour(juce::ListBox::backgroundColourId,   juce::Colour(colSurface2));
            setColour(juce::ListBox::outlineColourId,      juce::Colour(colBorder));
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
    //  Trial entry built by researcher
    //==========================================================================
    struct TrialEntry
    {
        float  angle       = 0.0f;
        int    layoutMode  = 0;
        int    topology    = 0;
        juce::String displayName;
    };

    //==========================================================================
    //  Chip configuration table
    //==========================================================================
    struct ChipConfig
    {
        int         layoutMode;
        int         topology;
        const char* label;
        bool        fullWidth;
    };
    static const std::array<ChipConfig, 10> kChipConfigs;

    //==========================================================================
    AudioPluginAudioProcessor& processorRef;
    TestSession session;
    DarkLookAndFeel darkLnf;

    bool isResearcherMode = true;

    // Researcher trial list (built on setup screen)
    std::vector<TrialEntry> researcherTrialList;
    // Ordered (shuffled) list used during the session
    std::vector<TrialEntry> orderedTrials;

    //-- Setup — shared
    juce::TextEditor nameEditor;
    juce::TextEditor participantEditor;
    juce::TextButton loadAudioBtn { "Load file" };
    juce::Label      audioFileLabel;
    juce::TextButton switchModeBtn { "Switch to Subject Mode" };
    juce::TextButton startBtn      { "Start session" };

    //-- Setup — researcher only
    juce::TextEditor angleEditor;
    std::array<juce::TextButton, 10> chipBtns;
    juce::TextButton addTrialBtn      { "+ Add to trial list" };
    juce::TextButton loadPresetBtn   { "Load R test"  };
    juce::TextButton loadLPresetBtn  { "Load L test"  };
    juce::TextButton loadLRPresetBtn { "Load L+R test" };
    juce::ListBox    trialListBox;

    //-- Trial / feedback / summary
    juce::TextButton playBtn   { "Play" };
    juce::TextButton submitBtn { "Submit answer" };
    juce::TextButton confBtn1  { "1" }, confBtn2 { "2" }, confBtn3 { "3" },
                     confBtn4  { "4" }, confBtn5 { "5" };
    juce::TextButton nextBtn    { "Next trial" };
    juce::TextButton saveBtn    { "Save CSV" };
    juce::TextButton newSessBtn { "New session" };
    juce::TextButton endSessBtn { "End session" };

    //-- Geometry (trial/feedback/summary)
    float circCx = 0, circCy = 0, circR = 0;
    juce::Rectangle<float> circBounds;

    //-- Progress bar flash after submit
    int barFlashTicks = 0;

    //-- Card rectangles (setup)
    juce::Rectangle<int> cardSession;
    juce::Rectangle<int> cardAudio;
    juce::Rectangle<int> cardTrialConfig;

    //-- Chip grid bounds (for layout in resized)
    std::array<juce::Rectangle<int>, 10> chipBounds;

    //==========================================================================
    //  Visibility helpers
    //==========================================================================
    void showSetupWidgets(bool);
    void showTrialWidgets(bool);
    void showFeedbackWidgets(bool);
    void showSummaryWidgets(bool);
    void hideAllWidgets();

    //==========================================================================
    //  Setup actions
    //==========================================================================
    void updateStartButton();
    void onAddTrial();
    void onLoadPreset();
    void onLoadLPreset();
    void onLoadLRPreset();
    void confirmAndLoadPreset(std::function<void()> loader, const juce::String& description);
    void loadRPreset();
    void loadLPreset();
    void loadLRPreset();
    void onLoadAudio();
    void onSwitchMode();
    void onStart();

    //==========================================================================
    //  Trial / feedback / summary actions
    //==========================================================================
    void onPlay();
    void onConfidence(int level);
    void onSubmit();
    void onNext();
    void onSave();
    void onNewSession();
    void onEndSession();

    //==========================================================================
    //  Styling helpers
    //==========================================================================
    void styleChipButton(juce::TextButton& btn, bool selected);
    void styleAccentButton(juce::TextButton& btn);
    void stylePlayButton(bool isStop);
    void styleConfidenceButton(juce::TextButton& btn, bool selected);

    //==========================================================================
    //  Sync
    //==========================================================================
    void syncParametersToProcessor();

    //==========================================================================
    //  Paint helpers
    //==========================================================================
    void paintSetupResearcher(juce::Graphics& g);
    void paintSetupSubject(juce::Graphics& g);
    void paintTrial(juce::Graphics& g);
    void paintFeedback(juce::Graphics& g);
    void paintSummary(juce::Graphics& g);

    void paintCard(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title);
    void paintMiniLabel(juce::Graphics& g, int x, int y, int w, const juce::String& text);
    void paintProgressBar(juce::Graphics& g, int y, float pct, bool flash = false);

    void drawCircle(juce::Graphics& g, float cx, float cy, float r,
        float userAngle, float actualAngle, bool glowRing);
    void drawMetricBox(juce::Graphics& g, juce::Rectangle<float> area,
        const juce::String& label, const juce::String& value,
        juce::Colour valueCol);

    juce::Colour errorColour(float err) const;
    static juce::Point<float> angleToPt(float deg, float r, float cx, float cy);

    //==========================================================================
    //  ListBoxModel
    //==========================================================================
    int  getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics& g, int width, int height,
                          bool rowIsSelected) override;
    juce::Component* refreshComponentForRow(int rowNumber, bool isRowSelected,
                                             juce::Component* existing) override;

    //==========================================================================
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BinauralTestSessionEditor)
};
