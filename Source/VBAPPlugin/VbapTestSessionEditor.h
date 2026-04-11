#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include "../Common/TestSession.h"

class AudioPluginAudioProcessor;

class VbapTestSessionEditor final : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit VbapTestSessionEditor(AudioPluginAudioProcessor& p);
    ~VbapTestSessionEditor() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

private:
    AudioPluginAudioProcessor& processorRef;
    TestSession session;

    static constexpr juce::uint32 colBg     = 0xFFF7F5EF;
    static constexpr juce::uint32 colCard   = 0xFFFFFFFF;
    static constexpr juce::uint32 colText   = 0xFF1F2937;
    static constexpr juce::uint32 colMuted  = 0xFF667085;
    static constexpr juce::uint32 colHint   = 0xFF98A2B3;
    static constexpr juce::uint32 colBlue   = 0xFF3B82F6;
    static constexpr juce::uint32 colGreen  = 0xFF10B981;
    static constexpr juce::uint32 colOrange = 0xFFF59E0B;
    static constexpr juce::uint32 colRed    = 0xFFEF4444;
    static constexpr juce::uint32 colMetric = 0xFFFCFBF8;
    static constexpr juce::uint32 colBorder = 0x1F1F2937;

    // Setup screen
    juce::TextEditor nameEditor;
    juce::ComboBox   speakerCountBox;
    std::array<juce::Label, 8>      speakerAzLabels;
    std::array<juce::TextEditor, 8> speakerAzEditors;
    juce::TextEditor anglesEditor;
    juce::TextEditor numTrialsEditor;
    juce::TextButton randomBtn    { "Randomize N trials" };
    juce::TextButton presetBtn    { "8-point preset" };
    juce::TextButton loadAudioBtn { "Load audio file..." };
    juce::Label      audioFileLabel;
    juce::TextButton startBtn     { "Start session" };

    // Trial screen
    juce::TextButton playBtn   { "Play" };
    juce::TextButton stopBtn   { "Stop" };
    juce::TextButton submitBtn { "Submit answer" };
    juce::TextButton confBtn1  { "1" };
    juce::TextButton confBtn2  { "2" };
    juce::TextButton confBtn3  { "3" };
    juce::TextButton confBtn4  { "4" };
    juce::TextButton confBtn5  { "5" };

    // Feedback / summary
    juce::TextButton nextBtn    { "Next trial" };
    juce::TextButton saveBtn    { "Save CSV" };
    juce::TextButton newSessBtn { "New session" };

    // Geometry
    float circCx = 0.0f, circCy = 0.0f, circR = 0.0f;
    juce::Rectangle<float> circBounds;

    int speakerCount = 4;
    std::array<float, 8> speakerAzimuths { 330.0f, 30.0f, 250.0f, 110.0f, 0.0f, 180.0f, 90.0f, 270.0f };

    void showSetupWidgets(bool v);
    void showTrialWidgets(bool v);
    void showFeedbackWidgets(bool v);
    void showSummaryWidgets(bool v);
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
    void updateSpeakerEditorVisibility();
    bool readSpeakerSetupFromUi(juce::String& errorMessage);
    juce::String buildSetupLabel() const;

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VbapTestSessionEditor)
};