#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "TestSession.h"

// Forward-declare the processor so we can store a reference
class AudioPluginAudioProcessor;

//==============================================================================
class TestSessionEditor final : public juce::AudioProcessorEditor,
                                               private juce::Timer
{
public:
    explicit TestSessionEditor(AudioPluginAudioProcessor& p);
    ~TestSessionEditor() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

private:
    // ── references ───────────────────────────────────────────
    AudioPluginAudioProcessor& processorRef;
    TestSession session;

    // ── colour palette (matches the HTML app) ────────────────
    static constexpr juce::uint32 colBg = 0xFFF7F5EF; // main background
    static constexpr juce::uint32 colCard = 0xFFFFFFFF; // panels / inputs
    static constexpr juce::uint32 colText = 0xFF1F2937; // main text
    static constexpr juce::uint32 colMuted = 0xFF667085; // secondary text
    static constexpr juce::uint32 colHint = 0xFF98A2B3; // hints / subtle labels
    static constexpr juce::uint32 colBlue = 0xFF3B82F6; // primary accent
    static constexpr juce::uint32 colGreen = 0xFF10B981; // success
    static constexpr juce::uint32 colOrange = 0xFFF59E0B; // warning
    static constexpr juce::uint32 colRed = 0xFFEF4444; // error
    static constexpr juce::uint32 colMetric = 0xFFFCFBF8; // metric boxes
    static constexpr juce::uint32 colBorder = 0x1F1F2937; // subtle borders

    // ── setup-screen widgets ─────────────────────────────────
    juce::TextEditor    nameEditor;
    juce::TextEditor    anglesEditor;
    juce::TextEditor    numTrialsEditor;
    juce::ComboBox      layoutBox;
    juce::TextButton    randomBtn     { "Randomize N trials" };
    juce::TextButton    presetBtn     { "8-point preset" };
    juce::TextButton    loadAudioBtn  { "Load audio file..." };
    juce::Label         audioFileLabel;
    juce::TextButton    startBtn      { "Start session" };

    // ── trial-screen widgets ─────────────────────────────────
    juce::TextButton    playBtn       { "Play" };
    juce::TextButton    stopBtn       { "Stop" };
    juce::TextButton    submitBtn     { "Submit answer" };
    juce::TextButton    confBtn1 { "1" }, confBtn2 { "2" }, confBtn3 { "3" },
                        confBtn4 { "4" }, confBtn5 { "5" };

    // ── feedback / summary widgets ───────────────────────────
    juce::TextButton    nextBtn       { "Next trial" };
    juce::TextButton    saveBtn       { "Save CSV" };
    juce::TextButton    newSessBtn    { "New session" };

    // ── circle geometry (trial & feedback) ───────────────────
    float circCx = 0, circCy = 0, circR = 0;
    juce::Rectangle<float> circBounds;

    // ── helpers ──────────────────────────────────────────────
    void showSetupWidgets(bool);
    void showTrialWidgets(bool);
    void showFeedbackWidgets(bool);
    void showSummaryWidgets(bool);
    void hideAllWidgets();

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

    void syncParametersToProcessor();

    // painting helpers
    void paintSetup(juce::Graphics& g);
    void paintTrial(juce::Graphics& g);
    void paintFeedback(juce::Graphics& g);
    void paintSummary(juce::Graphics& g);

    void drawCircle(juce::Graphics& g, float cx, float cy, float r,
                    float userAngle, float actualAngle, bool interactive);
    void drawMetricBox(juce::Graphics& g, juce::Rectangle<float> area,
                       const juce::String& label, const juce::String& value,
                       juce::Colour valueCol = juce::Colour(0xFF1A1A18));

    juce::Colour errorColour(float err) const;

    static juce::Point<float> angleToPt(float deg, float r, float cx, float cy);

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestSessionEditor)
};
