#include "BinauralTestSessionEditor.h"
#include "PluginProcessor.h"

//==============================================================================
//  Layout constants
//==============================================================================
static constexpr int kEditorW   = 660;
static constexpr int kEditorH   = 880;
static constexpr int kPad       = 22;
static constexpr int kFieldH    = 36;   // setup fields
static constexpr int kTrialFieldH = 40; // trial/feedback widgets — original value
static constexpr int kCardPad   = 18;
static constexpr int kCardGap   = 12;
static constexpr int kLblH      = 15;
static constexpr int kLblGap    = 4;
static constexpr int kChipH       = 26;
static constexpr int kChipGap     = 4;
static constexpr int kCardTitleH  = 20;  // 14px title + 6px gap before content

//==============================================================================
//  Chip configuration table
//==============================================================================
const std::array<BinauralTestSessionEditor::ChipConfig, 9>
    BinauralTestSessionEditor::kChipConfigs = {{
        { 0, 0, "Direct HRTF",          true  },   // No topology
        { 1, 0, "VBAP 5 Standard",      false },   // T1 = Standard
        { 1, 1, "VBAP 5 Symmetrical",   false },   // T2 = Symmetrical
        { 2, 0, "VBAP 7 Standard",      false },   // T1 = Standard
        { 2, 1, "VBAP 7 Symmetrical",   false },   // T2 = Symmetrical
        { 3, 0, "VBAP 9 Standard",      false },   // T1 = Standard
        { 3, 1, "VBAP 9 Symmetrical",   false },   // T2 = Symmetrical
        { 4, 1, "VBAP 12 Symmetrical",  true  },   // Symmetrical only
        { 5, 1, "VBAP 18 Symmetrical",  true  },   // Symmetrical only
    }};

//==============================================================================
//  Row component for the trial ListBox
//==============================================================================
class TrialRowComponent : public juce::Component
{
public:
    std::function<void()> onRemove;

    void setup(const juce::String& angleStr, const juce::String& config)
    {
        angleText  = angleStr;
        configText = config;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        const int w = getWidth();
        const int h = getHeight();

        g.setColour(juce::Colour(0xFF252B35));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 6.0f);
        g.setColour(juce::Colour(0xFF2E3540));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 6.0f, 1.0f);

        // angle — orange
        g.setColour(juce::Colour(0xFFFF7652));
        g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
        g.drawText(angleText, 10, 0, 46, h, juce::Justification::centredLeft);

        // config name
        g.setColour(juce::Colour(0xFFE6E8EC));
        g.setFont(juce::Font(juce::FontOptions(13.0f)));
        g.drawText(configText, 58, 0, w - 58 - 30, h, juce::Justification::centredLeft);
    }

    void resized() override
    {
        removeBtn.setBounds(getWidth() - 26, (getHeight() - 20) / 2, 20, 20);
    }

    TrialRowComponent()
    {
        removeBtn.setButtonText("x");
        removeBtn.setColour(juce::TextButton::buttonColourId,  juce::Colours::transparentBlack);
        removeBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF5F6673));
        removeBtn.onClick = [this] { if (onRemove) onRemove(); };
        addAndMakeVisible(removeBtn);
    }

private:
    juce::String angleText, configText;
    juce::TextButton removeBtn;
};

//==============================================================================
//  Constructor
//==============================================================================
BinauralTestSessionEditor::BinauralTestSessionEditor(AudioPluginAudioProcessor& p)
    : AudioProcessorEditor(p), processorRef(p)
{
    setLookAndFeel(&darkLnf);
    setSize(kEditorW, kEditorH);
    setResizable(false, false);

    // ── Session info ────────────────────────────────────────────────
    nameEditor.setText("Session 1");
    nameEditor.setJustification(juce::Justification::centredLeft);
    addChildComponent(nameEditor);

    participantEditor.setText("");
    participantEditor.setJustification(juce::Justification::centredLeft);
    participantEditor.setTextToShowWhenEmpty("e.g. P01", juce::Colour(colHint));
    addChildComponent(participantEditor);

    // ── Audio ────────────────────────────────────────────────────────
    loadAudioBtn.onClick = [this] { onLoadAudio(); };
    addChildComponent(loadAudioBtn);

    audioFileLabel.setText("No audio file loaded", juce::dontSendNotification);
    audioFileLabel.setFont(juce::Font(juce::FontOptions(13.0f)));
    audioFileLabel.setColour(juce::Label::textColourId, juce::Colour(colHint));
    addChildComponent(audioFileLabel);

    // ── Angle input ──────────────────────────────────────────────────
    angleEditor.setInputRestrictions(6, "0123456789.");
    angleEditor.setJustification(juce::Justification::centredLeft);
    angleEditor.setTextToShowWhenEmpty("0-360", juce::Colour(colHint));
    angleEditor.onTextChange = [this] { updateStartButton(); };
    addChildComponent(angleEditor);

    // ── Chip buttons ─────────────────────────────────────────────────
    for (int i = 0; i < static_cast<int>(kChipConfigs.size()); ++i)
    {
        chipBtns[i].setButtonText(kChipConfigs[i].label);
        chipBtns[i].setClickingTogglesState(true);
        styleChipButton(chipBtns[i], false);
        chipBtns[i].onClick = [this] { updateStartButton(); };
        addChildComponent(chipBtns[i]);
    }

    // ── Add / mode switch / start ────────────────────────────────────
    addTrialBtn.onClick = [this] { onAddTrial(); };
    styleAccentButton(addTrialBtn);
    addChildComponent(addTrialBtn);

    loadPresetBtn.onClick = [this] { onLoadPreset(); };
    loadPresetBtn.setColour(juce::TextButton::buttonColourId,  juce::Colour(colSurface2));
    loadPresetBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(colAccent));
    addChildComponent(loadPresetBtn);

    loadLPresetBtn.onClick = [this] { onLoadLPreset(); };
    loadLPresetBtn.setColour(juce::TextButton::buttonColourId,  juce::Colour(colSurface2));
    loadLPresetBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(colAccent));
    addChildComponent(loadLPresetBtn);

    loadLRPresetBtn.onClick = [this] { onLoadLRPreset(); };
    loadLRPresetBtn.setColour(juce::TextButton::buttonColourId,  juce::Colour(colSurface2));
    loadLRPresetBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(colAccent));
    addChildComponent(loadLRPresetBtn);

    switchModeBtn.onClick = [this] { onSwitchMode(); };
    addChildComponent(switchModeBtn);

    startBtn.onClick = [this] { onStart(); };
    styleAccentButton(startBtn);
    startBtn.setEnabled(false);
    addChildComponent(startBtn);

    // ── Trial list box ────────────────────────────────────────────────
    trialListBox.setModel(this);
    trialListBox.setRowHeight(32);
    trialListBox.setColour(juce::ListBox::backgroundColourId,  juce::Colour(colSurface2));
    trialListBox.setColour(juce::ListBox::outlineColourId,     juce::Colour(colBorder));
    trialListBox.setOutlineThickness(1);
    trialListBox.setMultipleSelectionEnabled(false);
    addChildComponent(trialListBox);

    // ── Trial screen widgets ─────────────────────────────────────────
    playBtn.onClick   = [this] { onPlay(); };
    submitBtn.onClick = [this] { onSubmit(); };
    addChildComponent(playBtn);
    addChildComponent(submitBtn);
    stylePlayButton(false);
    styleAccentButton(submitBtn);

    confBtn1.onClick = [this] { onConfidence(1); };
    confBtn2.onClick = [this] { onConfidence(2); };
    confBtn3.onClick = [this] { onConfidence(3); };
    confBtn4.onClick = [this] { onConfidence(4); };
    confBtn5.onClick = [this] { onConfidence(5); };
    for (auto* b : { &confBtn1, &confBtn2, &confBtn3, &confBtn4, &confBtn5 })
    {
        addChildComponent(*b);
        styleConfidenceButton(*b, false);
    }

    // ── Summary / feedback widgets ───────────────────────────────────
    nextBtn.onClick    = [this] { onNext(); };
    saveBtn.onClick    = [this] { onSave(); };
    newSessBtn.onClick = [this] { onNewSession(); };
    endSessBtn.onClick = [this] { onEndSession(); };

    addChildComponent(nextBtn);
    addChildComponent(saveBtn);
    addChildComponent(newSessBtn);
    addChildComponent(endSessBtn);
    styleAccentButton(nextBtn);
    styleAccentButton(saveBtn);

    endSessBtn.setColour(juce::TextButton::buttonColourId,  juce::Colours::transparentBlack);
    endSessBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(colMuted));

    showSetupWidgets(true);
    startTimerHz(10);
}

BinauralTestSessionEditor::~BinauralTestSessionEditor()
{
    setLookAndFeel(nullptr);
}

//==============================================================================
void BinauralTestSessionEditor::timerCallback()
{
    if (processorRef.audioPlayer.isPlaying())
    {
        playBtn.setButtonText("Stop");
        stylePlayButton(true);
    }
    else
    {
        playBtn.setButtonText("Play");
        stylePlayButton(false);
    }

    if (barFlashTicks > 0)
    {
        --barFlashTicks;
        repaint();
    }
}

//==============================================================================
//  Button styling
//==============================================================================
void BinauralTestSessionEditor::styleChipButton(juce::TextButton& btn, bool selected)
{
    if (selected)
    {
        btn.setColour(juce::TextButton::buttonColourId,  juce::Colour(colAccent));
        btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        btn.setColour(juce::TextButton::textColourOnId,  juce::Colours::white);
    }
    else
    {
        btn.setColour(juce::TextButton::buttonColourId,  juce::Colour(colSurface2));
        btn.setColour(juce::TextButton::textColourOffId, juce::Colour(colMuted));
        btn.setColour(juce::TextButton::textColourOnId,  juce::Colour(colMuted));
    }
}

void BinauralTestSessionEditor::styleAccentButton(juce::TextButton& btn)
{
    btn.setColour(juce::TextButton::buttonColourId,  juce::Colour(colAccent));
    btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    btn.setColour(juce::TextButton::textColourOnId,  juce::Colours::white);
}

void BinauralTestSessionEditor::stylePlayButton(bool isStop)
{
    playBtn.setColour(juce::TextButton::buttonColourId,  juce::Colour(isStop ? colRed : colGreen));
    playBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    playBtn.setColour(juce::TextButton::textColourOnId,  juce::Colours::white);
}

void BinauralTestSessionEditor::styleConfidenceButton(juce::TextButton& btn, bool selected)
{
    if (selected)
    {
        btn.setColour(juce::TextButton::buttonColourId,  juce::Colour(colAccent));
        btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    }
    else
    {
        btn.setColour(juce::TextButton::buttonColourId,  juce::Colour(colSurface2));
        btn.setColour(juce::TextButton::textColourOffId, juce::Colour(colMuted));
    }
}

//==============================================================================
//  Visibility
//==============================================================================
void BinauralTestSessionEditor::hideAllWidgets()
{
    showSetupWidgets(false);
    showTrialWidgets(false);
    showFeedbackWidgets(false);
    showSummaryWidgets(false);
}

void BinauralTestSessionEditor::showSetupWidgets(bool v)
{
    const bool researcherVisible = v && isResearcherMode;

    // Editors and audio controls are researcher-only; subject mode paints them as text
    nameEditor.setVisible(researcherVisible);
    participantEditor.setVisible(researcherVisible);
    loadAudioBtn.setVisible(researcherVisible);
    audioFileLabel.setVisible(researcherVisible);

    switchModeBtn.setVisible(v);
    startBtn.setVisible(v);

    angleEditor.setVisible(researcherVisible);
    addTrialBtn.setVisible(researcherVisible);
    loadPresetBtn.setVisible(researcherVisible);
    loadLPresetBtn.setVisible(researcherVisible);
    loadLRPresetBtn.setVisible(researcherVisible);
    trialListBox.setVisible(researcherVisible);
    for (auto& chip : chipBtns)
        chip.setVisible(researcherVisible);
}

void BinauralTestSessionEditor::showTrialWidgets(bool v)
{
    playBtn.setVisible(v);
    submitBtn.setVisible(v);
    confBtn1.setVisible(v);
    confBtn2.setVisible(v);
    confBtn3.setVisible(v);
    confBtn4.setVisible(v);
    confBtn5.setVisible(v);
    endSessBtn.setVisible(v);
}

void BinauralTestSessionEditor::showFeedbackWidgets(bool v)
{
    nextBtn.setVisible(v);
    endSessBtn.setVisible(v);
}

void BinauralTestSessionEditor::showSummaryWidgets(bool v)
{
    saveBtn.setVisible(v);
    newSessBtn.setVisible(v);
}

//==============================================================================
//  Setup
//==============================================================================
void BinauralTestSessionEditor::updateStartButton()
{
    const bool hasTrials = !researcherTrialList.empty();
    const bool hasAudio  = processorRef.audioPlayer.isLoaded();
    startBtn.setEnabled(hasTrials && hasAudio);
}

void BinauralTestSessionEditor::onAddTrial()
{
    const float angle = angleEditor.getText().trim().getFloatValue();
    if (angle < 0.0f || angle > 360.0f || angleEditor.getText().trim().isEmpty())
    {
        angleEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(colRed));
        angleEditor.repaint();
        return;
    }

    angleEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(colBorder));

    bool anySelected = false;
    for (int i = 0; i < static_cast<int>(kChipConfigs.size()); ++i)
    {
        if (chipBtns[i].getToggleState())
        {
            TrialEntry e;
            e.angle       = angle;
            e.layoutMode  = kChipConfigs[i].layoutMode;
            e.topology    = kChipConfigs[i].topology;
            e.displayName = kChipConfigs[i].label;
            researcherTrialList.push_back(e);
            anySelected = true;
        }
    }

    if (!anySelected) return;

    for (auto& chip : chipBtns)
    {
        chip.setToggleState(false, juce::dontSendNotification);
        styleChipButton(chip, false);
    }

    angleEditor.clear();
    trialListBox.updateContent();
    trialListBox.repaint();
    updateStartButton();
    repaint();
}

void BinauralTestSessionEditor::confirmAndLoadPreset(std::function<void()> loader,
                                                      const juce::String& description)
{
    if (researcherTrialList.empty())
    {
        loader();
        return;
    }
    juce::AlertWindow::showOkCancelBox(
        juce::MessageBoxIconType::QuestionIcon,
        "Load preset?",
        "This will replace the current trial list with the " + description + ". Continue?",
        "Load", "Cancel", nullptr,
        juce::ModalCallbackFunction::create([this, loader](int result)
        {
            if (result == 1) loader();
        }));
}

void BinauralTestSessionEditor::onLoadPreset()   { confirmAndLoadPreset([this] { loadRPreset();  }, "R test (31 trials)");   }
void BinauralTestSessionEditor::onLoadLPreset()  { confirmAndLoadPreset([this] { loadLPreset();  }, "L test (31 trials)");   }
void BinauralTestSessionEditor::onLoadLRPreset() { confirmAndLoadPreset([this] { loadLRPreset(); }, "L+R test (31 trials)"); }

void BinauralTestSessionEditor::loadRPreset()
{
    struct P { float a; int lm, tp; const char* n; };
    static const P k[] = {
        {  15.0f, 1, 0, "VBAP 5 Standard"  }, {  15.0f, 1, 1, "VBAP 5 Symmetrical"},
        {  15.0f, 0, 0, "Direct HRTF"      },
        {  30.0f, 1, 1, "VBAP 5 Symmetrical"}, {  30.0f, 0, 0, "Direct HRTF"      },
        {  45.0f, 1, 0, "VBAP 5 Standard"  }, {  45.0f, 1, 1, "VBAP 5 Symmetrical"},
        {  45.0f, 4, 1, "VBAP 12 Symmetrical"}, {  45.0f, 0, 0, "Direct HRTF"      },
        {  75.0f, 1, 0, "VBAP 5 Standard"  }, {  75.0f, 2, 0, "VBAP 7 Standard"  },
        {  75.0f, 1, 1, "VBAP 5 Symmetrical"}, {  75.0f, 4, 1, "VBAP 12 Symmetrical"},
        {  75.0f, 0, 0, "Direct HRTF"      },
        { 110.0f, 1, 0, "VBAP 5 Standard"  }, { 110.0f, 4, 1, "VBAP 12 Symmetrical"},
        { 110.0f, 1, 1, "VBAP 5 Symmetrical"}, { 110.0f, 2, 0, "VBAP 7 Standard"  },
        { 110.0f, 0, 0, "Direct HRTF"      },
        { 120.0f, 2, 0, "VBAP 7 Standard"  }, { 120.0f, 0, 0, "Direct HRTF"      },
        { 135.0f, 1, 0, "VBAP 5 Standard"  }, { 135.0f, 2, 0, "VBAP 7 Standard"  },
        { 135.0f, 4, 1, "VBAP 12 Symmetrical"}, { 135.0f, 0, 0, "Direct HRTF"      },
        { 150.0f, 1, 0, "VBAP 5 Standard"  }, { 150.0f, 0, 0, "Direct HRTF"      },
        { 165.0f, 1, 0, "VBAP 5 Standard"  }, { 165.0f, 2, 0, "VBAP 7 Standard"  },
        { 165.0f, 4, 1, "VBAP 12 Symmetrical"}, { 165.0f, 0, 0, "Direct HRTF"      },
    };
    researcherTrialList.clear();
    for (const auto& p : k) { TrialEntry e; e.angle=p.a; e.layoutMode=p.lm; e.topology=p.tp; e.displayName=p.n; researcherTrialList.push_back(e); }
    trialListBox.updateContent(); trialListBox.repaint(); updateStartButton(); repaint();
}

void BinauralTestSessionEditor::loadLPreset()
{
    struct P { float a; int lm, tp; const char* n; };
    static const P k[] = {
        { 195.0f, 1, 0, "VBAP 5 Standard"  }, { 195.0f, 2, 0, "VBAP 7 Standard"  },
        { 195.0f, 4, 1, "VBAP 12 Symmetrical"}, { 195.0f, 0, 0, "Direct HRTF"      },
        { 210.0f, 1, 0, "VBAP 5 Standard"  }, { 210.0f, 0, 0, "Direct HRTF"      },
        { 225.0f, 1, 0, "VBAP 5 Standard"  }, { 225.0f, 2, 0, "VBAP 7 Standard"  },
        { 225.0f, 4, 1, "VBAP 12 Symmetrical"}, { 225.0f, 0, 0, "Direct HRTF"      },
        { 240.0f, 2, 0, "VBAP 7 Standard"  }, { 240.0f, 0, 0, "Direct HRTF"      },
        { 250.0f, 1, 0, "VBAP 5 Standard"  }, { 250.0f, 4, 1, "VBAP 12 Symmetrical"},
        { 250.0f, 1, 1, "VBAP 5 Symmetrical"}, { 250.0f, 2, 0, "VBAP 7 Standard"  },
        { 250.0f, 0, 0, "Direct HRTF"      },
        { 285.0f, 1, 0, "VBAP 5 Standard"  }, { 285.0f, 2, 0, "VBAP 7 Standard"  },
        { 285.0f, 1, 1, "VBAP 5 Symmetrical"}, { 285.0f, 4, 1, "VBAP 12 Symmetrical"},
        { 285.0f, 0, 0, "Direct HRTF"      },
        { 315.0f, 1, 0, "VBAP 5 Standard"  }, { 315.0f, 1, 1, "VBAP 5 Symmetrical"},
        { 315.0f, 4, 1, "VBAP 12 Symmetrical"}, { 315.0f, 0, 0, "Direct HRTF"      },
        { 330.0f, 1, 1, "VBAP 5 Symmetrical"}, { 330.0f, 0, 0, "Direct HRTF"      },
        { 345.0f, 1, 0, "VBAP 5 Standard"  }, { 345.0f, 1, 1, "VBAP 5 Symmetrical"},
        { 345.0f, 0, 0, "Direct HRTF"      },
    };
    researcherTrialList.clear();
    for (const auto& p : k) { TrialEntry e; e.angle=p.a; e.layoutMode=p.lm; e.topology=p.tp; e.displayName=p.n; researcherTrialList.push_back(e); }
    trialListBox.updateContent(); trialListBox.repaint(); updateStartButton(); repaint();
}

void BinauralTestSessionEditor::loadLRPreset()
{
    struct P { float a; int lm, tp; const char* n; };
    static const P k[] = {
        // Right side
        {  15.0f, 1, 0, "VBAP 5 Standard"  }, {  15.0f, 0, 0, "Direct HRTF"      },
        {  30.0f, 1, 1, "VBAP 5 Symmetrical"},
        {  45.0f, 1, 0, "VBAP 5 Standard"  }, {  45.0f, 0, 0, "Direct HRTF"      },
        {  75.0f, 2, 0, "VBAP 7 Standard"  }, {  75.0f, 1, 1, "VBAP 5 Symmetrical"},
        {  75.0f, 0, 0, "Direct HRTF"      },
        { 110.0f, 1, 1, "VBAP 5 Symmetrical"}, { 110.0f, 4, 1, "VBAP 12 Symmetrical"},
        { 120.0f, 2, 0, "VBAP 7 Standard"  },
        { 135.0f, 1, 0, "VBAP 5 Standard"  }, { 135.0f, 4, 1, "VBAP 12 Symmetrical"},
        { 150.0f, 0, 0, "Direct HRTF"      },
        { 165.0f, 1, 0, "VBAP 5 Standard"  }, { 165.0f, 0, 0, "Direct HRTF"      },
        // Left side (mirrored, 360 - angle)
        { 195.0f, 2, 0, "VBAP 7 Standard"  }, { 195.0f, 4, 1, "VBAP 12 Symmetrical"},
        { 210.0f, 1, 0, "VBAP 5 Standard"  },
        { 225.0f, 2, 0, "VBAP 7 Standard"  }, { 225.0f, 0, 0, "Direct HRTF"      },
        { 240.0f, 0, 0, "Direct HRTF"      },
        { 250.0f, 1, 0, "VBAP 5 Standard"  }, { 250.0f, 2, 0, "VBAP 7 Standard"  },
        { 250.0f, 0, 0, "Direct HRTF"      },
        { 285.0f, 1, 0, "VBAP 5 Standard"  }, { 285.0f, 4, 1, "VBAP 12 Symmetrical"},
        { 315.0f, 1, 1, "VBAP 5 Symmetrical"}, { 315.0f, 4, 1, "VBAP 12 Symmetrical"},
        { 330.0f, 0, 0, "Direct HRTF"      },
        { 345.0f, 1, 1, "VBAP 5 Symmetrical"},
    };
    researcherTrialList.clear();
    for (const auto& p : k) { TrialEntry e; e.angle=p.a; e.layoutMode=p.lm; e.topology=p.tp; e.displayName=p.n; researcherTrialList.push_back(e); }
    trialListBox.updateContent(); trialListBox.repaint(); updateStartButton(); repaint();
}

void BinauralTestSessionEditor::onLoadAudio()
{
    auto chooser = std::make_shared<juce::FileChooser>(
        "Select audio file", juce::File(), "*.wav;*.mp3;*.aiff;*.flac");

    chooser->launchAsync(juce::FileBrowserComponent::openMode |
        juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.existsAsFile())
            {
                if (processorRef.audioPlayer.loadFile(file))
                {
                    session.audioFilePath  = file.getFullPathName();
                    session.useInternalAudio = true;
                    audioFileLabel.setText(file.getFileName(), juce::dontSendNotification);
                    audioFileLabel.setColour(juce::Label::textColourId, juce::Colour(colGreen));
                    updateStartButton();
                }
                else
                {
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::MessageBoxIconType::WarningIcon,
                        "Error", "Could not load the audio file.");
                }
            }
        });
}

void BinauralTestSessionEditor::onSwitchMode()
{
    isResearcherMode = !isResearcherMode;
    switchModeBtn.setButtonText(isResearcherMode ? "Switch to Subject Mode"
                                                 : "Researcher Mode");
    showSetupWidgets(true);
    resized();
    repaint();
}

void BinauralTestSessionEditor::onStart()
{
    session.sessionName = nameEditor.getText().trim();
    if (session.sessionName.isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
            "Error", "Enter a session name.");
        return;
    }

    if (researcherTrialList.empty())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
            "Error", "Add at least one trial.");
        return;
    }

    // ── Build ordered trial list ──────────────────────────────────────
    orderedTrials = researcherTrialList;

    // Fisher-Yates shuffle
    juce::Random rng;
    for (int i = static_cast<int>(orderedTrials.size()) - 1; i > 0; --i)
    {
        int j = rng.nextInt(i + 1);
        std::swap(orderedTrials[static_cast<size_t>(i)],
                  orderedTrials[static_cast<size_t>(j)]);
    }

    // Fix constraint: no same angle twice in a row
    for (int i = 0; i + 1 < static_cast<int>(orderedTrials.size()); ++i)
    {
        if (juce::exactlyEqual(orderedTrials[static_cast<size_t>(i)].angle,
                               orderedTrials[static_cast<size_t>(i + 1)].angle))
        {
            // Find the next element further ahead with a different angle
            for (int j = i + 2; j < static_cast<int>(orderedTrials.size()); ++j)
            {
                if (!juce::exactlyEqual(orderedTrials[static_cast<size_t>(j)].angle,
                                        orderedTrials[static_cast<size_t>(i)].angle))
                {
                    std::swap(orderedTrials[static_cast<size_t>(i + 1)],
                              orderedTrials[static_cast<size_t>(j)]);
                    break;
                }
            }
        }
    }

    // ── Populate TestSession ──────────────────────────────────────────
    session.targetAngles.clear();
    session.trialConfigs.clear();
    for (const auto& e : orderedTrials)
    {
        session.targetAngles.push_back(e.angle);
        session.trialConfigs.push_back(e.displayName);
    }

    const juce::String participant = participantEditor.getText().trim();
    session.setupLabel = participant;

    session.results.clear();
    session.currentTrial = 0;
    session.userResponse  = -1.0f;
    session.userConfidence = 0;
    session.screen = TestSession::Screen::Trial;

    syncParametersToProcessor();
    processorRef.logCurrentTrialSelection(session.targetAngles[0]);

    hideAllWidgets();
    showTrialWidgets(true);
    repaint();
}

//==============================================================================
//  Trial actions (unchanged logic)
//==============================================================================
void BinauralTestSessionEditor::onPlay()
{
    if (processorRef.audioPlayer.isPlaying())
        processorRef.audioPlayer.stop();
    else if (session.useInternalAudio && processorRef.audioPlayer.isLoaded())
    {
        processorRef.audioPlayer.setLooping(true);
        processorRef.audioPlayer.play();
    }
}

void BinauralTestSessionEditor::onConfidence(int level)
{
    session.userConfidence = level;
    submitBtn.setEnabled(session.canSubmit());
    styleConfidenceButton(confBtn1, level == 1);
    styleConfidenceButton(confBtn2, level == 2);
    styleConfidenceButton(confBtn3, level == 3);
    styleConfidenceButton(confBtn4, level == 4);
    styleConfidenceButton(confBtn5, level == 5);
    repaint();
}

void BinauralTestSessionEditor::onSubmit()
{
    if (!session.canSubmit()) return;
    processorRef.audioPlayer.stop();
    session.submitCurrentTrial();
    session.advanceAfterFeedback();

    if (session.screen == TestSession::Screen::Summary)
    {
        hideAllWidgets();
        showSummaryWidgets(true);
    }
    else
    {
        syncParametersToProcessor();
        processorRef.logCurrentTrialSelection(
            session.targetAngles[static_cast<size_t>(session.currentTrial)]);
        submitBtn.setEnabled(false);
        onConfidence(0);
        barFlashTicks = 5;  // 500ms bar flash
    }
    repaint();
}

void BinauralTestSessionEditor::onNext()
{
    session.advanceAfterFeedback();
    hideAllWidgets();

    if (session.screen == TestSession::Screen::Summary)
    {
        showSummaryWidgets(true);
    }
    else
    {
        syncParametersToProcessor();
        processorRef.logCurrentTrialSelection(
            session.targetAngles[static_cast<size_t>(session.currentTrial)]);
        showTrialWidgets(true);
        submitBtn.setEnabled(false);
        onConfidence(0);
    }
    repaint();
}

void BinauralTestSessionEditor::onSave()
{
    auto chooser = std::make_shared<juce::FileChooser>(
        "Save results",
        juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
            .getChildFile(session.sessionName + "_results.csv"),
        "*.csv");

    chooser->launchAsync(juce::FileBrowserComponent::saveMode |
        juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file != juce::File())
            {
                file.replaceWithText(session.toCSV());
                juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
                    "Saved", "Results saved to:\n" + file.getFullPathName());
            }
        });
}

void BinauralTestSessionEditor::onNewSession()
{
    session.reset();
    researcherTrialList.clear();
    orderedTrials.clear();
    isResearcherMode = true;
    switchModeBtn.setButtonText("Switch to Subject Mode");
    audioFileLabel.setText("No audio file loaded", juce::dontSendNotification);
    audioFileLabel.setColour(juce::Label::textColourId, juce::Colour(colHint));
    trialListBox.updateContent();
    for (auto& chip : chipBtns)
    {
        chip.setToggleState(false, juce::dontSendNotification);
        styleChipButton(chip, false);
    }
    updateStartButton();
    hideAllWidgets();
    showSetupWidgets(true);
    resized();
    onConfidence(0);
    repaint();
}

void BinauralTestSessionEditor::onEndSession()
{
    juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::QuestionIcon,
        "End session?",
        session.results.empty()
            ? "End the session now? No trials have been completed yet."
            : "End the session now? You will see the summary for the "
              + juce::String(static_cast<int>(session.results.size()))
              + " completed trials.",
        "End session", "Cancel", nullptr,
        juce::ModalCallbackFunction::create([this](int result)
            {
                if (result == 1)
                {
                    processorRef.audioPlayer.stop();
                    session.userResponse   = -1.0f;
                    session.userConfidence = 0;
                    session.screen = TestSession::Screen::Summary;
                    hideAllWidgets();
                    showSummaryWidgets(true);
                    repaint();
                }
            }));
}

void BinauralTestSessionEditor::syncParametersToProcessor()
{
    const int idx = session.currentTrial;
    if (idx < 0 || idx >= static_cast<int>(orderedTrials.size()))
        return;

    const auto& entry = orderedTrials[static_cast<size_t>(idx)];

    if (auto* azParam = processorRef.parameters.getParameter("sourceAzimuth"))
        azParam->setValueNotifyingHost(azParam->convertTo0to1(entry.angle));

    if (auto* layoutParam = processorRef.parameters.getParameter("layoutMode"))
        layoutParam->setValueNotifyingHost(
            layoutParam->convertTo0to1(static_cast<float>(entry.layoutMode)));

    if (auto* topologyParam = processorRef.parameters.getParameter("topology"))
        topologyParam->setValueNotifyingHost(
            topologyParam->convertTo0to1(static_cast<float>(entry.topology)));
}

//==============================================================================
//  ListBoxModel
//==============================================================================
int BinauralTestSessionEditor::getNumRows()
{
    return static_cast<int>(researcherTrialList.size());
}

void BinauralTestSessionEditor::paintListBoxItem(int, juce::Graphics&, int, int, bool)
{
    // Rows are fully custom via refreshComponentForRow — nothing to paint here
}

juce::Component* BinauralTestSessionEditor::refreshComponentForRow(
    int rowNumber, bool, juce::Component* existing)
{
    auto* row = existing ? dynamic_cast<TrialRowComponent*>(existing)
                         : new TrialRowComponent();

    if (row == nullptr)
        row = new TrialRowComponent();

    if (rowNumber < static_cast<int>(researcherTrialList.size()))
    {
        const auto& e = researcherTrialList[static_cast<size_t>(rowNumber)];
        row->setup(juce::String(static_cast<int>(e.angle))
                       + juce::String::fromUTF8("\xc2\xb0"),
                   e.displayName);

        const int capturedRow = rowNumber;
        row->onRemove = [this, capturedRow]()
        {
            if (capturedRow < static_cast<int>(researcherTrialList.size()))
            {
                researcherTrialList.erase(
                    researcherTrialList.begin() + capturedRow);
                trialListBox.updateContent();
                trialListBox.repaint();
                updateStartButton();
                repaint();
            }
        };
    }

    return row;
}

//==============================================================================
//  resized
//==============================================================================
void BinauralTestSessionEditor::resized()
{
    const int W = getWidth();
    const int H = getHeight();

    //------------------------------------------------------------------
    // Setup screen — layout branches by mode
    //------------------------------------------------------------------
    if (isResearcherMode)
    {
        int y = kPad + 32 + 12;  // title row height

        // Card 1: Session (name + participant side-by-side)
        {
            const int cardH = kCardPad + kCardTitleH + kLblH + kLblGap + kFieldH + kCardPad;
            cardSession = { kPad, y, W - 2 * kPad, cardH };

            const int innerX = cardSession.getX() + kCardPad;
            const int innerW = cardSession.getWidth() - 2 * kCardPad;
            const int colGap  = 10;
            const int colW    = (innerW - colGap) / 2;
            const int fieldY  = cardSession.getY() + kCardPad + kCardTitleH + kLblH + kLblGap;

            nameEditor.setBounds(innerX, fieldY, colW, kFieldH);
            participantEditor.setBounds(innerX + colW + colGap, fieldY, colW, kFieldH);
            y = cardSession.getBottom() + kCardGap;
        }

        // Card 2: Audio
        {
            const int cardH = kCardPad + kCardTitleH + kFieldH + kCardPad;
            cardAudio = { kPad, y, W - 2 * kPad, cardH };

            const int innerX = cardAudio.getX() + kCardPad;
            const int innerW = cardAudio.getWidth() - 2 * kCardPad;
            const int fieldY = cardAudio.getY() + kCardPad + kCardTitleH;
            const int btnW   = 120;

            loadAudioBtn.setBounds(innerX, fieldY, btnW, kFieldH);
            audioFileLabel.setBounds(innerX + btnW + 10, fieldY, innerW - btnW - 10, kFieldH);
            y = cardAudio.getBottom() + kCardGap;
        }

        // Card 3: Trial configuration
        {
            const int chipRows  = 6;
            const int chipGridH = chipRows * kChipH + (chipRows - 1) * kChipGap;
            const int listH     = 96;
            const int cardH = kCardPad + kCardTitleH
                + (kLblH + kLblGap + kFieldH) + 10
                + (kLblH + kLblGap) + chipGridH + 10
                + kFieldH + 14
                + kLblH + 6 + listH + 10
                + kCardPad;
            cardTrialConfig = { kPad, y, W - 2 * kPad, cardH };

            const int innerX = cardTrialConfig.getX() + kCardPad;
            const int innerW = cardTrialConfig.getWidth() - 2 * kCardPad;
            int cy = cardTrialConfig.getY() + kCardPad + kCardTitleH;

            cy += kLblH + kLblGap;
            angleEditor.setBounds(innerX, cy, 100, kFieldH);
            cy += kFieldH + 10;
            cy += kLblH + kLblGap;

            const int halfW = (innerW - kChipGap) / 2;
            int chipIdx = 0;
            const int numChips = static_cast<int>(kChipConfigs.size());
            for (int row = 0; row < chipRows && chipIdx < numChips; ++row)
            {
                const int rowY = cy + row * (kChipH + kChipGap);
                if (kChipConfigs[chipIdx].fullWidth)
                {
                    chipBounds[chipIdx] = { innerX, rowY, innerW, kChipH };
                    chipBtns[chipIdx].setBounds(chipBounds[chipIdx]);
                    ++chipIdx;
                }
                else
                {
                    chipBounds[chipIdx] = { innerX, rowY, halfW, kChipH };
                    chipBtns[chipIdx].setBounds(chipBounds[chipIdx]);
                    ++chipIdx;
                    if (chipIdx < numChips && !kChipConfigs[chipIdx].fullWidth)
                    {
                        chipBounds[chipIdx] = { innerX + halfW + kChipGap, rowY, halfW, kChipH };
                        chipBtns[chipIdx].setBounds(chipBounds[chipIdx]);
                        ++chipIdx;
                    }
                }
            }

            cy += chipGridH + 10;
            {
                const int addW    = 160;
                const int gap     = 8;
                const int presetW = (innerW - addW - 3 * gap) / 3;
                loadLRPresetBtn.setBounds(innerX,                          cy, presetW, kFieldH);
                loadPresetBtn.setBounds  (innerX + presetW + gap,          cy, presetW, kFieldH);
                loadLPresetBtn.setBounds (innerX + 2 * (presetW + gap),    cy, presetW, kFieldH);
                addTrialBtn.setBounds    (innerX + innerW - addW,          cy, addW,    kFieldH);
            }
            cy += kFieldH + 14;
            cy += kLblH + 6;
            trialListBox.setBounds(innerX, cy, innerW, listH);
            y = cardTrialConfig.getBottom() + kCardGap;
        }

        // Bottom: Switch mode (half) | Start session (half)
        const int btnHalfW = (W - 2 * kPad - 10) / 2;
        switchModeBtn.setBounds(kPad, y, btnHalfW, 44);
        startBtn.setBounds(kPad + btnHalfW + 10, y, btnHalfW, 44);
    }
    else
    {
        // ── Subject mode layout ───────────────────────────────────────
        int y = kPad + 32 + 12;  // title row

        // Big info card: session name, participant, trial count — no editors
        const int infoCardH = 120;
        cardSession = { kPad, y, W - 2 * kPad, infoCardH };
        y = cardSession.getBottom() + kCardGap;

        // Instructions card
        const int instrCardH = 80;
        cardAudio = { kPad, y, W - 2 * kPad, instrCardH };
        y = cardAudio.getBottom() + kCardGap;

        // Start button: full-width at bottom
        startBtn.setBounds(kPad, y, W - 2 * kPad, 44);

        // Switch button: small ghost, top-right
        switchModeBtn.setBounds(W - kPad - 148, kPad + 3, 148, 26);
    }

    //------------------------------------------------------------------
    // Trial / feedback geometry — original values preserved
    //------------------------------------------------------------------
    circR  = 155.0f;
    circCx = W * 0.5f;
    circCy = 280.0f;
    circBounds = juce::Rectangle<float>(circCx - circR - 30, circCy - circR - 30,
        (circR + 30) * 2.0f, (circR + 30) * 2.0f);

    const int confW      = 60;
    const int confGap    = 10;
    const int confTotalW = 5 * confW + 4 * confGap;
    const int confX      = (W - confTotalW) / 2;

    int trialY = static_cast<int>(circCy + circR) + 66;
    playBtn.setBounds(confX, trialY, confTotalW, kTrialFieldH);

    trialY += kTrialFieldH + 42;
    confBtn1.setBounds(confX,                         trialY, confW, confW);
    confBtn2.setBounds(confX + 1 * (confW + confGap), trialY, confW, confW);
    confBtn3.setBounds(confX + 2 * (confW + confGap), trialY, confW, confW);
    confBtn4.setBounds(confX + 3 * (confW + confGap), trialY, confW, confW);
    confBtn5.setBounds(confX + 4 * (confW + confGap), trialY, confW, confW);

    trialY += confW + 82;
    submitBtn.setBounds(kPad, trialY, W - 2 * kPad, 48);

    endSessBtn.setBounds(W - kPad - 110, 12, 110, 26);

    nextBtn.setBounds(kPad, H - 64, W - 2 * kPad, 44);

    const int sumBtnW = (W - 2 * kPad - 12) / 2;
    saveBtn.setBounds(kPad,                  H - 64, sumBtnW, 44);
    newSessBtn.setBounds(kPad + sumBtnW + 12, H - 64, sumBtnW, 44);
}

//==============================================================================
//  paint
//==============================================================================
void BinauralTestSessionEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(colBg));
    switch (session.screen)
    {
    case TestSession::Screen::Setup:
        if (isResearcherMode) paintSetupResearcher(g);
        else                  paintSetupSubject(g);
        break;
    case TestSession::Screen::Trial:    paintTrial(g);    break;
    case TestSession::Screen::Feedback: paintFeedback(g); break;
    case TestSession::Screen::Summary:  paintSummary(g);  break;
    }
}

//==============================================================================
//  Card + label helpers
//==============================================================================
void BinauralTestSessionEditor::paintCard(juce::Graphics& g, juce::Rectangle<int> r,
    const juce::String& title)
{
    g.setColour(juce::Colour(colSurface));
    g.fillRoundedRectangle(r.toFloat(), 10.0f);
    g.setColour(juce::Colour(colBorder));
    g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 10.0f, 1.0f);

    g.setColour(juce::Colour(colHint));
    g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    g.drawText(title.toUpperCase(),
        r.getX() + kCardPad, r.getY() + kCardPad,
        r.getWidth() - 2 * kCardPad, 14,
        juce::Justification::centredLeft);
}

void BinauralTestSessionEditor::paintMiniLabel(juce::Graphics& g, int x, int y, int w,
    const juce::String& text)
{
    g.setColour(juce::Colour(colMuted));
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.drawText(text, x, y, w, kLblH, juce::Justification::centredLeft);
}

void BinauralTestSessionEditor::paintProgressBar(juce::Graphics& g, int y, float pct, bool flash)
{
    const int W = getWidth();
    const float h = flash ? 8.0f : 6.0f;
    const float yf = flash ? (float)y - 1.0f : (float)y;
    juce::Rectangle<float> bg((float)kPad, yf, (float)(W - 2 * kPad), h);
    g.setColour(juce::Colour(colSurface));
    g.fillRoundedRectangle(bg, h * 0.5f);
    g.setColour(flash ? juce::Colour(colGreen) : juce::Colour(colAccent));
    g.fillRoundedRectangle(bg.withWidth(bg.getWidth() * juce::jlimit(0.0f, 1.0f, pct)), h * 0.5f);
}

//==============================================================================
//  Setup — researcher
//==============================================================================
void BinauralTestSessionEditor::paintSetupResearcher(juce::Graphics& g)
{
    const int W = getWidth();

    // Title + mode switch hint
    g.setColour(juce::Colour(colText));
    g.setFont(juce::Font(juce::FontOptions(20.0f, juce::Font::bold)));
    g.drawText("Session setup", kPad, kPad, W / 2, 32, juce::Justification::centredLeft);

    g.setColour(juce::Colour(colHint));
    g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    g.drawText("RESEARCHER MODE", kPad, kPad + 4, W - 2 * kPad, 14,
        juce::Justification::centredRight);

    // Card 1: Session
    paintCard(g, cardSession, "Session");
    const int nameY = nameEditor.getY() - kLblGap - kLblH;
    paintMiniLabel(g, nameEditor.getX(), nameY, nameEditor.getWidth(), "Session name");
    paintMiniLabel(g, participantEditor.getX(), nameY,
                   participantEditor.getWidth(), "Participant ID");

    // Card 2: Audio
    paintCard(g, cardAudio, "Audio stimulus");

    // Card 3: Trial configuration
    paintCard(g, cardTrialConfig, "Trial configuration");

    const int innerX = cardTrialConfig.getX() + kCardPad;
    const int innerW = cardTrialConfig.getWidth() - 2 * kCardPad;

    paintMiniLabel(g, innerX, angleEditor.getY() - kLblGap - kLblH,
                   innerW, "Virtual source angle (deg)");

    g.setColour(juce::Colour(colHint));
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.drawText("Then select which configurations to test at this angle",
        angleEditor.getRight() + 10, angleEditor.getY(),
        cardTrialConfig.getRight() - kCardPad - angleEditor.getRight() - 10,
        kFieldH, juce::Justification::centredLeft);

    // Chip section label
    paintMiniLabel(g, innerX, chipBtns[0].getY() - kLblGap - kLblH,
                   innerW, "Rendering configurations");

    // Divider between Add button and trial list
    const int divY = addTrialBtn.getBottom() + 7;
    g.setColour(juce::Colour(colBorder));
    g.drawHorizontalLine(divY, (float)innerX, (float)(innerX + innerW));

    // Trial list label
    paintMiniLabel(g, innerX, divY + 5,
                   200, "Trial list");
    // Randomisation hint
    g.setColour(juce::Colour(colAccent));
    g.setFont(juce::Font(juce::FontOptions(12.0f)));

    // Trial count
    if (!researcherTrialList.empty())
    {
        const int cnt = static_cast<int>(researcherTrialList.size());
        g.setColour(juce::Colour(colHint));
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        g.drawText(juce::String(cnt) + " trial" + (cnt != 1 ? "s" : ""),
            innerX, trialListBox.getBottom() + 4, innerW, 14,
            juce::Justification::centredRight);
    }
}

//==============================================================================
//  Setup — subject
//==============================================================================
void BinauralTestSessionEditor::paintSetupSubject(juce::Graphics& g)
{
    const int W = getWidth();

    // Title
    g.setColour(juce::Colour(colText));
    g.setFont(juce::Font(juce::FontOptions(20.0f, juce::Font::bold)));
    g.drawText("Ready", kPad, kPad, W / 2, 32, juce::Justification::centredLeft);

    // "SUBJECT MODE" badge — to the left of the switch button
    g.setColour(juce::Colour(colGreen));
    g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    g.drawText("SUBJECT MODE",
               kPad, kPad + 8,
               switchModeBtn.getX() - kPad - 12, 16,
               juce::Justification::centredRight);

    // ── Big info card (no card title — matches HTML .subject-info-card) ──
    g.setColour(juce::Colour(colSurface));
    g.fillRoundedRectangle(cardSession.toFloat(), 10.0f);
    g.setColour(juce::Colour(colBorder));
    g.drawRoundedRectangle(cardSession.toFloat().reduced(0.5f), 10.0f, 1.0f);

    const int top = cardSession.getY() + 24;

    // Session name — 22px bold, centred
    const juce::String sName = nameEditor.getText().trim();
    g.setColour(juce::Colour(colText));
    g.setFont(juce::Font(juce::FontOptions(22.0f, juce::Font::bold)));
    g.drawText(sName.isNotEmpty() ? sName : juce::String("Session 1"),
               cardSession.getX(), top, cardSession.getWidth(), 28,
               juce::Justification::centred);

    // Participant — 14px muted, centred
    const juce::String participant = participantEditor.getText().trim();
    g.setColour(juce::Colour(colMuted));
    g.setFont(juce::Font(juce::FontOptions(14.0f)));
    g.drawText("Participant: " + (participant.isNotEmpty() ? participant : juce::String("-")),
               cardSession.getX(), top + 34, cardSession.getWidth(), 20,
               juce::Justification::centred);

    // Trial count — 13px hint, centred
    const int cnt = static_cast<int>(researcherTrialList.size());
    g.setColour(juce::Colour(colHint));
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.drawText(juce::String(cnt) + (cnt != 1 ? " trials" : " trial") + " configured",
               cardSession.getX(), top + 60, cardSession.getWidth(), 18,
               juce::Justification::centred);

    // ── Instructions card ─────────────────────────────────────────────
    paintCard(g, cardAudio, "Instructions");
    g.setColour(juce::Colour(colMuted));
    g.setFont(juce::Font(juce::FontOptions(14.0f)));
    g.drawText("Put on your headphones and press Start session below.",
               cardAudio.getX() + kCardPad,
               cardAudio.getY() + kCardPad + kCardTitleH,
               cardAudio.getWidth() - 2 * kCardPad,
               cardAudio.getHeight() - 2 * kCardPad - kCardTitleH,
               juce::Justification::centred, true);
}

//==============================================================================
//  Trial / Feedback / Summary
//==============================================================================
void BinauralTestSessionEditor::paintTrial(juce::Graphics& g)
{
    const int W = getWidth();
    const int n = session.currentTrial + 1;
    const int tot = static_cast<int>(session.targetAngles.size());
    const float pct = static_cast<float>(session.currentTrial) / static_cast<float>(tot);

    g.setFont(juce::Font(juce::FontOptions(15.0f, juce::Font::bold)));
    g.setColour(juce::Colour(colText));
    g.drawText("Trial " + juce::String(n) + " of " + juce::String(tot),
        kPad, 16, 240, 22, juce::Justification::centredLeft);

    g.setFont(juce::Font(juce::FontOptions(16.0f)));
    g.setColour(juce::Colour(colMuted));
    g.drawText(session.sessionName,
        kPad, 16, W - 2 * kPad, 22, juce::Justification::centred);

    paintProgressBar(g, 44, pct, barFlashTicks > 0);

    g.setFont(juce::Font(juce::FontOptions(14.0f)));
    g.setColour(juce::Colour(colMuted));
    g.drawText("Listen to the sound, then click where you think it came from",
        kPad, 60, W - 2 * kPad, 22, juce::Justification::centred);

    drawCircle(g, circCx, circCy, circR, session.userResponse, -1.0f, false);

    const float textY = circCy + circR + 24;
    g.setFont(juce::Font(juce::FontOptions(15.0f, juce::Font::bold)));
    g.setColour(session.userResponse >= 0 ? juce::Colour(colText) : juce::Colour(colMuted));
    juce::String selText = session.userResponse >= 0
        ? "Selected: " + juce::String(static_cast<int>(session.userResponse))
          + juce::String::fromUTF8("\xc2\xb0")
        : "Click circle to select angle";
    g.drawText(selText, kPad, static_cast<int>(textY), W - 2 * kPad, 22,
        juce::Justification::centred);

    int confLabelY = confBtn1.getY() - 22;
    g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    g.setColour(juce::Colour(colMuted));
    g.drawText("CONFIDENCE", kPad, confLabelY, W - 2 * kPad, 18,
        juce::Justification::centred);

    int confBtnBottom = confBtn1.getBottom() + 6;
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.setColour(juce::Colour(colHint));
    g.drawText("Not sure",  confBtn1.getX(), confBtnBottom, 120, 16,
        juce::Justification::centredLeft);
    g.drawText("Very sure", confBtn5.getRight() - 120, confBtnBottom, 120, 16,
        juce::Justification::centredRight);
}

void BinauralTestSessionEditor::paintFeedback(juce::Graphics& g)
{
    const int W = getWidth();
    const auto& trial  = session.results.back();
    const int trialNum = static_cast<int>(session.results.size());
    const int tot      = static_cast<int>(session.targetAngles.size());
    const float pct    = static_cast<float>(trialNum) / static_cast<float>(tot);

    g.setFont(juce::Font(juce::FontOptions(15.0f, juce::Font::bold)));
    g.setColour(juce::Colour(colText));
    g.drawText("Trial " + juce::String(trialNum) + " of " + juce::String(tot) + " result",
        kPad, 16, 340, 22, juce::Justification::centredLeft);

    paintProgressBar(g, 44, pct);

    drawCircle(g, circCx, circCy, circR, trial.responseAngle, trial.targetAngle, false);

    const float legY = circCy + circR + 20;
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.setColour(juce::Colour(colGreen));
    g.fillEllipse(W * 0.5f - 110.0f, legY, 9.0f, 9.0f);
    g.setColour(juce::Colour(colMuted));
    g.drawText("Actual source", static_cast<int>(W * 0.5f - 96.0f),
        static_cast<int>(legY - 2), 110, 14, juce::Justification::centredLeft);

    g.setColour(juce::Colour(colAccent));
    g.fillEllipse(W * 0.5f + 18.0f, legY, 9.0f, 9.0f);
    g.setColour(juce::Colour(colMuted));
    g.drawText("Your response", static_cast<int>(W * 0.5f + 32.0f),
        static_cast<int>(legY - 2), 110, 14, juce::Justification::centredLeft);

    const float metY = legY + 30;
    const float metW = (W - 2.0f * kPad - 16.0f) / 3.0f;
    drawMetricBox(g, { (float)kPad, metY, metW, 70.0f }, "Actual",
        juce::String(static_cast<int>(trial.targetAngle)) + juce::String::fromUTF8("\xc2\xb0"),
        juce::Colour(colText));
    drawMetricBox(g, { kPad + metW + 8.0f, metY, metW, 70.0f }, "Your answer",
        juce::String(static_cast<int>(trial.responseAngle)) + juce::String::fromUTF8("\xc2\xb0"),
        juce::Colour(colText));
    drawMetricBox(g, { kPad + 2.0f * (metW + 8.0f), metY, metW, 70.0f }, "Error",
        juce::String(trial.angularError, 1) + juce::String::fromUTF8("\xc2\xb0"),
        errorColour(trial.angularError));

    const bool isLast = session.currentTrial >= static_cast<int>(session.targetAngles.size());
    nextBtn.setButtonText(isLast ? "View summary" : "Next trial");
}

void BinauralTestSessionEditor::paintSummary(juce::Graphics& g)
{
    const int W = getWidth();

    g.setFont(juce::Font(juce::FontOptions(20.0f, juce::Font::bold)));
    g.setColour(juce::Colour(colText));
    g.drawText("Session complete", kPad, 20, W - 2 * kPad, 30,
        juce::Justification::centredLeft);

    g.setFont(juce::Font(juce::FontOptions(15.0f)));
    g.setColour(juce::Colour(colMuted));
    {
        const juce::String dot = "  " + juce::String::fromUTF8("\xc2\xb7") + "  ";
        juce::String subtitle = session.sessionName;
        if (session.setupLabel.isNotEmpty())
            subtitle += dot + session.setupLabel;
        subtitle += dot + juce::String(static_cast<int>(session.results.size())) + " trials";
        g.drawText(subtitle, kPad, 54, W - 2 * kPad, 22, juce::Justification::centredLeft);
    }

    // Results table
    const int H      = getHeight();
    const int tableX = kPad;
    const int tableW = W - 2 * kPad;
    const int tableY = 84;
    const int nRows  = static_cast<int>(session.results.size());
    // Fit all rows in the space above the Save/New buttons (H-64-8)
    const int tableAvailH = (H - 64 - 8) - tableY;
    const int rowH  = nRows > 0 ? juce::jlimit(18, 28, tableAvailH / (nRows + 2)) : 28;
    const int tableH = rowH + (nRows * rowH) + kCardPad;

    juce::Rectangle<int> tableCard(tableX, tableY - 8, tableW, tableH);
    g.setColour(juce::Colour(colSurface));
    g.fillRoundedRectangle(tableCard.toFloat(), 10.0f);
    g.setColour(juce::Colour(colBorder));
    g.drawRoundedRectangle(tableCard.toFloat().reduced(0.5f), 10.0f, 1.0f);

    const int innerLeft  = tableX + kCardPad;
    const int innerRight = tableX + tableW - kCardPad;

    // Column x positions (added Config column)
    const int colHashX   = innerLeft;
    const int colCfgX    = innerLeft + 30;
    const int colActualX = innerLeft + 200;
    const int colRespX   = innerLeft + 270;
    const int colErrX    = innerLeft + 350;
    const int colConfX   = innerRight - 60;

    g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    g.setColour(juce::Colour(colMuted));
    const int headerY = tableY;
    g.drawText("#",        colHashX,   headerY, 26,  rowH, juce::Justification::centredLeft);
    g.drawText("CONFIG",   colCfgX,    headerY, 165, rowH, juce::Justification::centredLeft);
    g.drawText("ACTUAL",   colActualX, headerY, 64,  rowH, juce::Justification::centredLeft);
    g.drawText("RESP.",    colRespX,   headerY, 64,  rowH, juce::Justification::centredLeft);
    g.drawText("ERROR",    colErrX,    headerY, 64,  rowH, juce::Justification::centredLeft);
    g.drawText("CONF.",    colConfX,   headerY, 60,  rowH, juce::Justification::centredLeft);

    g.setColour(juce::Colour(colBorder));
    g.drawLine((float)innerLeft, (float)(headerY + rowH - 1),
               (float)innerRight, (float)(headerY + rowH - 1), 1.0f);

    const float rowFontSize = rowH >= 24 ? 14.0f : rowH >= 20 ? 13.0f : 12.0f;
    g.setFont(juce::Font(juce::FontOptions(rowFontSize)));
    for (int i = 0; i < nRows; ++i)
    {
        const int ry    = tableY + rowH + i * rowH;
        const auto& r   = session.results[static_cast<size_t>(i)];

        if (i % 2 == 0)
        {
            g.setColour(juce::Colour(colSurface2).withAlpha(0.4f));
            g.fillRect(innerLeft, ry, innerRight - innerLeft, rowH);
        }

        g.setColour(juce::Colour(colText));
        g.drawText(juce::String(r.trialNumber), colHashX,   ry, 26,  rowH, juce::Justification::centredLeft);
        g.drawText(r.configName,                colCfgX,    ry, 165, rowH, juce::Justification::centredLeft);
        g.drawText(juce::String(static_cast<int>(r.targetAngle))
                   + juce::String::fromUTF8("\xc2\xb0"), colActualX, ry, 64, rowH, juce::Justification::centredLeft);
        g.drawText(juce::String(static_cast<int>(r.responseAngle))
                   + juce::String::fromUTF8("\xc2\xb0"), colRespX,   ry, 64, rowH, juce::Justification::centredLeft);

        g.setColour(errorColour(r.angularError));
        g.drawText(juce::String(r.angularError, 1) + juce::String::fromUTF8("\xc2\xb0"),
            colErrX, ry, 64, rowH, juce::Justification::centredLeft);

        g.setColour(juce::Colour(colText));
        g.drawText(juce::String(r.confidence) + "/5",
            colConfX, ry, 60, rowH, juce::Justification::centredLeft);
    }
}

//==============================================================================
//  Circle drawing (unchanged)
//==============================================================================
juce::Point<float> BinauralTestSessionEditor::angleToPt(
    float deg, float r, float cx, float cy)
{
    const float rad = (deg - 90.0f) * juce::MathConstants<float>::pi / 180.0f;
    return { cx + r * std::cos(rad), cy + r * std::sin(rad) };
}

void BinauralTestSessionEditor::drawCircle(juce::Graphics& g,
    float cx, float cy, float r,
    float userAngle, float actualAngle, bool glowRing)
{
    g.setColour(juce::Colour(colBorder));
    g.drawEllipse(cx - r, cy - r, r * 2, r * 2, 1.5f);

    g.setColour(juce::Colour(colSurface2));
    g.drawEllipse(cx - r * 0.5f, cy - r * 0.5f, r, r, 1.0f);

    g.setColour(juce::Colour(colHint));
    g.fillEllipse(cx - 3, cy - 3, 6, 6);

    for (int a = 0; a < 360; a += 10)
    {
        bool major = (a % 90 == 0);
        bool med = (a % 30 == 0);
        float innerR = r - (major ? 14.0f : med ? 9.0f : 5.0f);
        auto p1 = angleToPt(static_cast<float>(a), r, cx, cy);
        auto p2 = angleToPt(static_cast<float>(a), innerR, cx, cy);
        g.setColour(juce::Colour(colBorder));
        g.drawLine(p1.x, p1.y, p2.x, p2.y, major ? 1.5f : 0.75f);
    }

    g.setFont(juce::Font(juce::FontOptions("Courier New", 13.0f, juce::Font::plain)));
    g.setColour(juce::Colour(colMuted));
    const char* degLabels[] = { "0", "90", "180", "270" };
    float degAngles[] = { 0, 90, 180, 270 };
    for (int i = 0; i < 4; ++i)
    {
        auto pt = angleToPt(degAngles[i], r + 18.0f, cx, cy);
        g.drawText(juce::String(degLabels[i]),
            static_cast<int>(pt.x - 20), static_cast<int>(pt.y - 8), 40, 16,
            juce::Justification::centred);
    }

    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.setColour(juce::Colour(colHint));
    const char* dirLabels[] = { "Front", "Right", "Back", "Left" };
    for (int i = 0; i < 4; ++i)
    {
        auto pt = angleToPt(degAngles[i], r - 30.0f, cx, cy);
        g.drawText(dirLabels[i],
            static_cast<int>(pt.x - 24), static_cast<int>(pt.y - 8), 48, 16,
            juce::Justification::centred);
    }

    if (actualAngle >= 0.0f)
    {
        auto apt = angleToPt(actualAngle, r, cx, cy);
        g.setColour(juce::Colour(colGreen).withAlpha(0.65f));
        g.drawLine(cx, cy, apt.x, apt.y, 1.5f);
        g.setColour(juce::Colour(colGreen).withAlpha(0.18f));
        g.fillEllipse(apt.x - 11, apt.y - 11, 22, 22);
        g.setColour(juce::Colour(colGreen));
        g.fillEllipse(apt.x - 6, apt.y - 6, 12, 12);
    }

    if (userAngle >= 0.0f)
    {
        auto upt = angleToPt(userAngle, r, cx, cy);
        g.setColour(juce::Colour(colAccent).withAlpha(0.65f));
        g.drawLine(cx, cy, upt.x, upt.y, 1.5f);
        g.setColour(juce::Colour(colAccent).withAlpha(0.18f));
        g.fillEllipse(upt.x - 11, upt.y - 11, 22, 22);
        g.setColour(juce::Colour(colAccent));
        g.fillEllipse(upt.x - 6, upt.y - 6, 12, 12);
    }
}

//==============================================================================
//  mouseDown — circle interaction
//==============================================================================
void BinauralTestSessionEditor::mouseDown(const juce::MouseEvent& e)
{
    if (session.screen != TestSession::Screen::Trial) return;

    float dx = static_cast<float>(e.x) - circCx;
    float dy = static_cast<float>(e.y) - circCy;
    float dist = std::sqrt(dx * dx + dy * dy);
    if (dist > circR + 20.0f) return;

    float angle = std::atan2(dx, -dy) * 180.0f / juce::MathConstants<float>::pi;
    if (angle < 0.0f) angle += 360.0f;

    session.userResponse = std::round(angle);
    submitBtn.setEnabled(session.canSubmit());
    repaint();
}

//==============================================================================
//  Metric box
//==============================================================================
void BinauralTestSessionEditor::drawMetricBox(juce::Graphics& g,
    juce::Rectangle<float> area,
    const juce::String& label,
    const juce::String& value,
    juce::Colour valueCol)
{
    g.setColour(juce::Colour(colSurface));
    g.fillRoundedRectangle(area, 10.0f);
    g.setColour(juce::Colour(colBorder));
    g.drawRoundedRectangle(area.reduced(0.5f), 10.0f, 1.0f);

    g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    g.setColour(juce::Colour(colMuted));
    g.drawText(label.toUpperCase(),
        area.reduced(14, 0).withHeight(20).translated(0, 10),
        juce::Justification::centredLeft);

    g.setFont(juce::Font(juce::FontOptions("Courier New", 28.0f, juce::Font::bold)));
    g.setColour(valueCol);
    g.drawText(value, area.reduced(14, 0).withTrimmedTop(28),
        juce::Justification::centredLeft);
}

juce::Colour BinauralTestSessionEditor::errorColour(float err) const
{
    if (err <= 15.0f) return juce::Colour(colGreen);
    if (err <= 45.0f) return juce::Colour(colOrange);
    return juce::Colour(colRed);
}
