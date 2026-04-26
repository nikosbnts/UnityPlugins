#include "BinauralTestSessionEditor.h"
#include "PluginProcessor.h"

static constexpr int kEditorW = 640;
static constexpr int kEditorH = 760;
static constexpr int kPad = 24;
static constexpr int kFieldH = 36;
static constexpr int kCardPad = 16;

//==============================================================================
BinauralTestSessionEditor::BinauralTestSessionEditor(AudioPluginAudioProcessor& p)
    : AudioProcessorEditor(p), processorRef(p)
{
    setLookAndFeel(&darkLnf);
    setSize(kEditorW, kEditorH);
    setResizable(false, false);

    // ── Setup widgets ───────────────────────────────────────
    nameEditor.setText("Session 1");
    nameEditor.setJustification(juce::Justification::centredLeft);
    addChildComponent(nameEditor);

    anglesEditor.setMultiLine(true, true);
    anglesEditor.setReturnKeyStartsNewLine(false);
    anglesEditor.setText("0, 45, 90, 135, 180, 225, 270, 315");
    addChildComponent(anglesEditor);

    numTrialsEditor.setText("8");
    numTrialsEditor.setInputRestrictions(2, "0123456789");
    numTrialsEditor.setJustification(juce::Justification::centred);
    addChildComponent(numTrialsEditor);

    layoutBox.addItem("Direct HRTF", 1);
    layoutBox.addItem("VBAP 9 speakers", 2);
    layoutBox.addItem("VBAP 12 speakers", 3);
    layoutBox.addItem("VBAP 18 speakers", 4);
    layoutBox.addItem("VBAP 36 speakers", 5);
    layoutBox.setSelectedId(5);
    layoutBox.onChange = [this] { onLayoutChanged(); };
    addChildComponent(layoutBox);

    topologyBox.addItem("Symmetric", 1);
    topologyBox.addItem("Asymmetric", 2);
    topologyBox.setSelectedId(1);
    addChildComponent(topologyBox);

    randomBtn.onClick = [this] { onRandomize(); };
    presetBtn.onClick = [this] { onPreset(); };
    loadAudioBtn.onClick = [this] { onLoadAudio(); };
    startBtn.onClick = [this] { onStart(); };
    addChildComponent(randomBtn);
    addChildComponent(presetBtn);
    addChildComponent(loadAudioBtn);
    addChildComponent(startBtn);
    styleAccentButton(startBtn);

    audioFileLabel.setText("No audio file (use DAW input)", juce::dontSendNotification);
    audioFileLabel.setFont(juce::Font(juce::FontOptions(14.0f)));
    audioFileLabel.setColour(juce::Label::textColourId, juce::Colour(colHint));
    addChildComponent(audioFileLabel);

    // ── Trial widgets ───────────────────────────────────────
    playBtn.onClick = [this] { onPlay(); };
    stopBtn.onClick = [this] { onStop(); };
    submitBtn.onClick = [this] { onSubmit(); };
    addChildComponent(playBtn);
    addChildComponent(stopBtn);
    addChildComponent(submitBtn);
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

    // ── Feedback / summary widgets ──────────────────────────
    nextBtn.onClick = [this] { onNext(); };
    saveBtn.onClick = [this] { onSave(); };
    newSessBtn.onClick = [this] { onNewSession(); };

    addChildComponent(nextBtn);
    addChildComponent(saveBtn);
    addChildComponent(newSessBtn);
    addChildComponent(endSessBtn);
    styleAccentButton(nextBtn);
    styleAccentButton(saveBtn);

    endSessBtn.onClick = [this] { onEndSession(); };
    endSessBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(colSurface));
    endSessBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(colRed));

    showSetupWidgets(true);
    onLayoutChanged();

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
        playBtn.setButtonText("Playing...");
    else
        playBtn.setButtonText("Play");
}

//==============================================================================
//  Button styling
//==============================================================================
void BinauralTestSessionEditor::styleAccentButton(juce::TextButton& btn)
{
    btn.setColour(juce::TextButton::buttonColourId, juce::Colour(colAccent));
    btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    btn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
}

void BinauralTestSessionEditor::styleConfidenceButton(juce::TextButton& btn, bool selected)
{
    if (selected)
    {
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour(colAccent));
        btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    }
    else
    {
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour(colSurface));
        btn.setColour(juce::TextButton::textColourOffId, juce::Colour(colMuted));
    }
}

//==============================================================================
//  Widget visibility helpers
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
    nameEditor.setVisible(v);
    anglesEditor.setVisible(v);
    numTrialsEditor.setVisible(v);
    layoutBox.setVisible(v);
    topologyBox.setVisible(v);
    randomBtn.setVisible(v);
    presetBtn.setVisible(v);
    loadAudioBtn.setVisible(v);
    audioFileLabel.setVisible(v);
    startBtn.setVisible(v);
}

void BinauralTestSessionEditor::showTrialWidgets(bool v)
{
    playBtn.setVisible(v);
    stopBtn.setVisible(v);
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
//  Topology dropdown sync
//==============================================================================
void BinauralTestSessionEditor::onLayoutChanged()
{
    const int mode = layoutBox.getSelectedId() - 1;
    const bool hasTopologyChoice = AudioPluginAudioProcessor::layoutSupportsAsymmetric(mode);

    topologyBox.setEnabled(hasTopologyChoice);

    if (!hasTopologyChoice)
        topologyBox.setSelectedId(1, juce::dontSendNotification);

    repaint();
}

//==============================================================================
//  Actions
//==============================================================================
void BinauralTestSessionEditor::onRandomize()
{
    int n = numTrialsEditor.getText().getIntValue();
    if (n < 1) n = 8;
    if (n > 40) n = 40;
    auto angles = TestSession::generateRandom(n);
    juce::String txt;
    for (size_t i = 0; i < angles.size(); ++i)
    {
        if (i > 0) txt << ", ";
        txt << juce::String(static_cast<int>(angles[i]));
    }
    anglesEditor.setText(txt);
}

void BinauralTestSessionEditor::onPreset()
{
    anglesEditor.setText("0, 45, 90, 135, 180, 225, 270, 315");
}

void BinauralTestSessionEditor::onLoadAudio()
{
    auto chooser = std::make_shared<juce::FileChooser>(
        "Select audio file",
        juce::File(),
        "*.wav;*.mp3;*.aiff;*.flac");

    chooser->launchAsync(juce::FileBrowserComponent::openMode |
        juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.existsAsFile())
            {
                if (processorRef.audioPlayer.loadFile(file))
                {
                    session.audioFilePath = file.getFullPathName();
                    session.useInternalAudio = true;
                    audioFileLabel.setText(file.getFileName(), juce::dontSendNotification);
                    audioFileLabel.setColour(juce::Label::textColourId, juce::Colour(colText));
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

juce::String BinauralTestSessionEditor::buildLayoutLabel() const
{
    juce::String base;
    bool topologyApplies = false;

    switch (selectedLayoutMode)
    {
    case 0: return "Direct HRTF";
    case 1: base = "VBAP 9 speakers";  topologyApplies = true; break;
    case 2: base = "VBAP 12 speakers"; topologyApplies = true; break;
    case 3: base = "VBAP 18 speakers"; topologyApplies = true; break;
    case 4: return "VBAP 36 speakers";
    default: return "VBAP 36 speakers";
    }

    if (topologyApplies)
        base << " - " << (selectedTopology == 1 ? "Asymmetric" : "Symmetric");

    return base;
}

void BinauralTestSessionEditor::onStart()
{
    session.sessionName = nameEditor.getText().trim();
    if (session.sessionName.isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Error", "Enter a session name.");
        return;
    }

    session.targetAngles = TestSession::parseAngles(anglesEditor.getText());
    if (session.targetAngles.empty())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Error", "Enter at least one angle (0-360).");
        return;
    }

    selectedLayoutMode = layoutBox.getSelectedId() - 1;
    selectedTopology = topologyBox.getSelectedId() - 1;

    if (!AudioPluginAudioProcessor::layoutSupportsAsymmetric(selectedLayoutMode))
        selectedTopology = 0;

    session.setupLabel = buildLayoutLabel();
    session.results.clear();
    session.currentTrial = 0;
    session.userResponse = -1.0f;
    session.userConfidence = 0;
    session.screen = TestSession::Screen::Trial;

    syncParametersToProcessor();

    if (!session.targetAngles.empty())
        processorRef.logCurrentTrialSelection(session.targetAngles[0]);

    hideAllWidgets();
    showTrialWidgets(true);
    repaint();
}

void BinauralTestSessionEditor::onPlay()
{
    if (session.useInternalAudio && processorRef.audioPlayer.isLoaded())
    {
        processorRef.audioPlayer.setLooping(true);
        processorRef.audioPlayer.play();
    }
}

void BinauralTestSessionEditor::onStop()
{
    processorRef.audioPlayer.stop();
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

    hideAllWidgets();
    showFeedbackWidgets(true);
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

        if (session.currentTrial < static_cast<int>(session.targetAngles.size()))
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
                juce::AlertWindow::showMessageBoxAsync(
                    juce::MessageBoxIconType::InfoIcon,
                    "Saved", "Results saved to:\n" + file.getFullPathName());
            }
        });
}

void BinauralTestSessionEditor::onNewSession()
{
    session.reset();
    selectedLayoutMode = layoutBox.getSelectedId() - 1;
    selectedTopology = topologyBox.getSelectedId() - 1;
    hideAllWidgets();
    showSetupWidgets(true);
    onLayoutChanged();
    onConfidence(0);
    repaint();
}

void BinauralTestSessionEditor::onEndSession()
{
    juce::AlertWindow::showOkCancelBox(
        juce::MessageBoxIconType::QuestionIcon,
        "End session?",
        session.results.empty()
        ? "End the session now? No trials have been completed yet."
        : "End the session now? You will see the summary for completed trials so far ("
        + juce::String(static_cast<int>(session.results.size())) + " trials).",
        "End session",
        "Cancel",
        nullptr,
        juce::ModalCallbackFunction::create([this](int result)
            {
                if (result == 1)  // OK = End session
                {
                    processorRef.audioPlayer.stop();

                    // Force jump to summary screen with whatever results we have
                    session.userResponse = -1.0f;
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
    if (session.currentTrial < static_cast<int>(session.targetAngles.size()))
    {
        float angle = session.targetAngles[static_cast<size_t>(session.currentTrial)];
        auto* azParam = processorRef.parameters.getParameter("sourceAzimuth");
        if (azParam)
            azParam->setValueNotifyingHost(azParam->convertTo0to1(angle));
    }

    auto* layoutParam = processorRef.parameters.getParameter("layoutMode");
    if (layoutParam)
        layoutParam->setValueNotifyingHost(
            layoutParam->convertTo0to1(static_cast<float>(selectedLayoutMode)));

    auto* topologyParam = processorRef.parameters.getParameter("topology");
    if (topologyParam)
        topologyParam->setValueNotifyingHost(
            topologyParam->convertTo0to1(static_cast<float>(selectedTopology)));
}

//==============================================================================
//  resized — card-based setup, big circle for trial
//==============================================================================
void BinauralTestSessionEditor::resized()
{
    const int W = getWidth();
    const int H = getHeight();

    //----------------------------------------------------------------
    // Setup screen — three cards
    //----------------------------------------------------------------
    int y = 60;  // leave room for title

    // Card 1: SESSION
    {
        const int cardH = kCardPad + 20 + 8 + kFieldH + kCardPad;
        cardSession = { kPad, y, W - 2 * kPad, cardH };

        const int innerX = cardSession.getX() + kCardPad;
        const int innerW = cardSession.getWidth() - 2 * kCardPad;
        const int fieldY = cardSession.getY() + kCardPad + 12 + 8;

        nameEditor.setBounds(innerX, fieldY, innerW, kFieldH);

        y = cardSession.getBottom() + 14;
    }

    // Card 2: RENDERING
    {
        const int cardH = kCardPad + 15 + 8 + kFieldH + 14 + 20 + 8 + kFieldH + kCardPad;
        cardRendering = { kPad, y, W - 2 * kPad, cardH };

        const int innerX = cardRendering.getX() + kCardPad;
        const int innerW = cardRendering.getWidth() - 2 * kCardPad;

        const int row1 = cardRendering.getY() + kCardPad + 10 + 8;
        layoutBox.setBounds(innerX, row1, innerW, kFieldH);

        const int row2 = row1 + kFieldH + 14 + 20 + 8;
        topologyBox.setBounds(innerX, row2, innerW, kFieldH);

        y = cardRendering.getBottom() + 14;
    }

    // Card 3: TRIALS
    {
        const int cardH = kCardPad + 20 + 8 + 70 + 12 + kFieldH + 12 + kFieldH + kCardPad;
        cardTrials = { kPad, y, W - 2 * kPad, cardH };

        const int innerX = cardTrials.getX() + kCardPad;
        const int innerW = cardTrials.getWidth() - 2 * kCardPad;

        const int anglesY = cardTrials.getY() + kCardPad + 20 + 8;
        anglesEditor.setBounds(innerX, anglesY, innerW, 70);

        const int trialsRowY = anglesY + 70 + 12;
        numTrialsEditor.setBounds(innerX, trialsRowY, 60, kFieldH);
        randomBtn.setBounds(innerX + 70, trialsRowY, 170, kFieldH);
        presetBtn.setBounds(innerX + 250, trialsRowY, 130, kFieldH);

        const int audioRowY = trialsRowY + kFieldH + 12;
        loadAudioBtn.setBounds(innerX, audioRowY, 160, kFieldH);
        audioFileLabel.setBounds(innerX + 170, audioRowY, innerW - 170, kFieldH);

        y = cardTrials.getBottom() + 18;
    }

    // Start session button
    startBtn.setBounds(kPad, y, W - 2 * kPad, 44);

    //----------------------------------------------------------------
    // Trial / Feedback geometry — big centered circle
    //----------------------------------------------------------------
    circR = 130.0f;
    circCx = W * 0.5f;
    circCy = 290.0f;
    circBounds = juce::Rectangle<float>(circCx - circR - 30, circCy - circR - 30,
        (circR + 30) * 2, (circR + 30) * 2);

    int trialY = static_cast<int>(circCy + circR) + 60;

    playBtn.setBounds(kPad, trialY, 90, kFieldH);
    stopBtn.setBounds(kPad + 100, trialY, 90, kFieldH);

    // Confidence buttons centered
    const int confW = 48;
    const int confGap = 10;
    const int confTotalW = 5 * confW + 4 * confGap;
    const int confX = (W - confTotalW) / 2;

    trialY += kFieldH + 28;
    confBtn1.setBounds(confX, trialY, confW, confW);
    confBtn2.setBounds(confX + 1 * (confW + confGap), trialY, confW, confW);
    confBtn3.setBounds(confX + 2 * (confW + confGap), trialY, confW, confW);
    confBtn4.setBounds(confX + 3 * (confW + confGap), trialY, confW, confW);
    confBtn5.setBounds(confX + 4 * (confW + confGap), trialY, confW, confW);

    trialY += confW + 28;
    submitBtn.setBounds(kPad, trialY, W - 2 * kPad, 44);

    // End session button — small, top-right corner of trial/feedback screens
    endSessBtn.setBounds(W - kPad - 110, 10, 110, 26);

    // Feedback / summary bottom buttons
    nextBtn.setBounds(kPad, H - 64, W - 2 * kPad, 44);

    const int sumBtnW = (W - 2 * kPad - 12) / 2;
    saveBtn.setBounds(kPad, H - 64, sumBtnW, 44);
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
    case TestSession::Screen::Setup:    paintSetup(g);    break;
    case TestSession::Screen::Trial:    paintTrial(g);    break;
    case TestSession::Screen::Feedback: paintFeedback(g); break;
    case TestSession::Screen::Summary:  paintSummary(g);  break;
    }
}

//==============================================================================
//  Card helper
//==============================================================================
void BinauralTestSessionEditor::paintCard(juce::Graphics& g, juce::Rectangle<int> r,
    const juce::String& title)
{
    g.setColour(juce::Colour(colSurface));
    g.fillRoundedRectangle(r.toFloat(), 10.0f);

    g.setColour(juce::Colour(colBorder));
    g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 10.0f, 1.0f);

    // Title — small, uppercase, muted
    g.setColour(juce::Colour(colMuted));
    g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    g.drawText(title,
        r.getX() + kCardPad,
        r.getY() + kCardPad - 9,
        r.getWidth() - 2 * kCardPad,
        25,
        juce::Justification::centredLeft);
}

void BinauralTestSessionEditor::paintProgressBar(juce::Graphics& g, int y, float pct)
{
    const int W = getWidth();
    juce::Rectangle<float> bg(static_cast<float>(kPad), static_cast<float>(y),
        static_cast<float>(W - 2 * kPad), 4.0f);

    g.setColour(juce::Colour(colSurface));
    g.fillRoundedRectangle(bg, 2.0f);

    g.setColour(juce::Colour(colAccent));
    g.fillRoundedRectangle(bg.withWidth(bg.getWidth() * juce::jlimit(0.0f, 1.0f, pct)), 2.0f);
}

//==============================================================================
//  Setup screen — title + cards + small labels above each input
//==============================================================================
void BinauralTestSessionEditor::paintSetup(juce::Graphics& g)
{
    // Title
    g.setColour(juce::Colour(colText));
    g.setFont(juce::Font(juce::FontOptions(22.0f, juce::Font::bold)));
    g.drawText("Session setup", kPad, 20, getWidth() - 2 * kPad, 28,
        juce::Justification::centredLeft);

    // Card 1
    paintCard(g, cardSession, "Session Name");
    g.setColour(juce::Colour(colMuted));
    g.setFont(juce::Font(juce::FontOptions(14.0f)));

    // Card 2
    paintCard(g, cardRendering, "Rendering Mode");
    g.drawText("Topology",
        topologyBox.getX(), topologyBox.getY() - 25,
        220, 16, juce::Justification::centredLeft);

    // Card 3
    paintCard(g, cardTrials, "Source angles (degrees, 0-360)");
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.setColour(juce::Colour(colHint));
    g.drawText("0 = front   90 = right   180 = back   270 = left",
        anglesEditor.getX(),
        anglesEditor.getY() + anglesEditor.getHeight() - 16,
        420, 14, juce::Justification::centredRight);
}

//==============================================================================
//  Trial screen
//==============================================================================
void BinauralTestSessionEditor::paintTrial(juce::Graphics& g)
{
    const int W = getWidth();
    const int n = session.currentTrial + 1;
    const int tot = static_cast<int>(session.targetAngles.size());
    const float pct = static_cast<float>(session.currentTrial) / static_cast<float>(tot);

    // Header
    g.setFont(juce::Font(juce::FontOptions(15.0f, juce::Font::bold)));
    g.setColour(juce::Colour(colText));
    g.drawText("Trial " + juce::String(n) + " of " + juce::String(tot),
        kPad, 16, 240, 22, juce::Justification::centredLeft);

    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.setColour(juce::Colour(colMuted));
    g.drawText(session.sessionName,
        kPad + 250, 16, W - kPad - 250 - 120, 22,
        juce::Justification::centredLeft);

    paintProgressBar(g, 44, pct);

    // Instruction
    g.setFont(juce::Font(juce::FontOptions(14.0f)));
    g.setColour(juce::Colour(colMuted));
    g.drawText("Listen to the sound, then click where you think it came from",
        kPad, 60, W - 2 * kPad, 22, juce::Justification::centred);

    // Circle
    drawCircle(g, circCx, circCy, circR, session.userResponse, -1.0f, true);

    // Selected text
    const float textY = circCy + circR + 24;
    g.setFont(juce::Font(juce::FontOptions(15.0f, juce::Font::bold)));
    g.setColour(session.userResponse >= 0 ? juce::Colour(colText) : juce::Colour(colMuted));
    juce::String selText = session.userResponse >= 0
        ? "Selected: " + juce::String(static_cast<int>(session.userResponse)) + juce::String::fromUTF8("\xc2\xb0")
        : "Click circle to select angle";
    g.drawText(selText, kPad, static_cast<int>(textY), W - 2 * kPad, 22,
        juce::Justification::centred);

    // Confidence label
    int confLabelY = confBtn1.getY() - 22;
    g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    g.setColour(juce::Colour(colMuted));
    g.drawText("CONFIDENCE", kPad, confLabelY, 200, 18,
        juce::Justification::centredLeft);

    // Hints below confidence buttons
    int confBtnBottom = confBtn1.getBottom() + 6;
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.setColour(juce::Colour(colHint));
    g.drawText("Not sure", confBtn1.getX(), confBtnBottom, 120, 16,
        juce::Justification::centredLeft);
    g.drawText("Very sure", confBtn5.getRight() - 120, confBtnBottom, 120, 16,
        juce::Justification::centredRight);
}

//==============================================================================
//  Feedback screen
//==============================================================================
void BinauralTestSessionEditor::paintFeedback(juce::Graphics& g)
{
    const int W = getWidth();
    const auto& trial = session.results.back();
    const int trialNum = static_cast<int>(session.results.size());
    const int tot = static_cast<int>(session.targetAngles.size());
    const float pct = static_cast<float>(trialNum) / static_cast<float>(tot);

    // Header
    g.setFont(juce::Font(juce::FontOptions(15.0f, juce::Font::bold)));
    g.setColour(juce::Colour(colText));
    g.drawText("Trial " + juce::String(trialNum) + " of " + juce::String(tot) + " - result",
        kPad, 16, 340, 22, juce::Justification::centredLeft);

    paintProgressBar(g, 44, pct);

    // Circle showing both
    drawCircle(g, circCx, circCy, circR, trial.responseAngle, trial.targetAngle, false);

    // Legend
    const float legY = circCy + circR + 24;
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

    // Metrics
    const float metY = legY + 32;
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

    bool isLast = session.currentTrial >= static_cast<int>(session.targetAngles.size());
    nextBtn.setButtonText(isLast ? "View summary" : "Next trial");
}

//==============================================================================
//  Summary screen — metric cards + functional table
//==============================================================================
void BinauralTestSessionEditor::paintSummary(juce::Graphics& g)
{
    const int W = getWidth();

    // Title
    g.setFont(juce::Font(juce::FontOptions(22.0f, juce::Font::bold)));
    g.setColour(juce::Colour(colText));
    g.drawText("Session complete", kPad, 20, W - 2 * kPad, 30,
        juce::Justification::centredLeft);

    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.setColour(juce::Colour(colMuted));
    g.drawText(session.sessionName + "  ·  " + session.setupLabel + "  ·  "
        + juce::String(static_cast<int>(session.results.size())) + " trials",
        kPad, 54, W - 2 * kPad, 20, juce::Justification::centredLeft);

    // Metrics
    const float metY = 88;
    const float metW = (W - 2.0f * kPad - 16.0f) / 3.0f;
    drawMetricBox(g, { (float)kPad, metY, metW, 70.0f }, "Mean error",
        juce::String(session.meanError(), 1) + juce::String::fromUTF8("\xc2\xb0"),
        juce::Colour(colText));
    drawMetricBox(g, { kPad + metW + 8.0f, metY, metW, 70.0f }, "Best",
        juce::String(session.bestError(), 1) + juce::String::fromUTF8("\xc2\xb0"),
        juce::Colour(colGreen));
    drawMetricBox(g, { kPad + 2.0f * (metW + 8.0f), metY, metW, 70.0f }, "Worst",
        juce::String(session.worstError(), 1) + juce::String::fromUTF8("\xc2\xb0"),
        juce::Colour(colRed));

    //----------------------------------------------------------------
    // Functional results table
    //----------------------------------------------------------------
    const int tableX = kPad;
    const int tableW = W - 2 * kPad;
    const int tableY = static_cast<int>(metY) + 90;
    const int rowH = 26;

    // Table card background
    const int maxRows = std::min(static_cast<int>(session.results.size()), 16);
    const int tableH = rowH + (maxRows * rowH) + kCardPad;

    juce::Rectangle<int> tableCard(tableX, tableY - 8, tableW, tableH);
    g.setColour(juce::Colour(colSurface));
    g.fillRoundedRectangle(tableCard.toFloat(), 10.0f);
    g.setColour(juce::Colour(colBorder));
    g.drawRoundedRectangle(tableCard.toFloat().reduced(0.5f), 10.0f, 1.0f);

    // Column positions (relative to card)
    const int innerLeft = tableX + kCardPad;
    const int innerRight = tableX + tableW - kCardPad;
    const int colHashX = innerLeft;
    const int colActualX = innerLeft + 40;
    const int colRespX = innerLeft + 130;
    const int colErrX = innerLeft + 230;
    const int colConfX = innerRight - 80;

    // Header row
    g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    g.setColour(juce::Colour(colMuted));
    const int headerY = tableY;
    g.drawText("#", colHashX, headerY, 30, rowH, juce::Justification::centredLeft);
    g.drawText("ACTUAL", colActualX, headerY, 90, rowH, juce::Justification::centredLeft);
    g.drawText("RESPONSE", colRespX, headerY, 90, rowH, juce::Justification::centredLeft);
    g.drawText("ERROR", colErrX, headerY, 90, rowH, juce::Justification::centredLeft);
    g.drawText("CONF.", colConfX, headerY, 80, rowH, juce::Justification::centredLeft);

    // Header underline
    g.setColour(juce::Colour(colBorder));
    g.drawLine((float)innerLeft, (float)(headerY + rowH - 1),
        (float)innerRight, (float)(headerY + rowH - 1), 1.0f);

    // Rows
    g.setFont(juce::Font(juce::FontOptions("Courier New", 14.0f, juce::Font::plain)));
    for (int i = 0; i < maxRows; ++i)
    {
        const int ry = tableY + rowH + i * rowH;
        const auto& r = session.results[static_cast<size_t>(i)];

        // Zebra rows
        if (i % 2 == 0)
        {
            g.setColour(juce::Colour(colSurface2).withAlpha(0.4f));
            g.fillRect(innerLeft, ry, innerRight - innerLeft, rowH);
        }

        g.setColour(juce::Colour(colText));
        g.drawText(juce::String(r.trialNumber), colHashX, ry, 30, rowH,
            juce::Justification::centredLeft);
        g.drawText(juce::String(static_cast<int>(r.targetAngle)) + juce::String::fromUTF8("\xc2\xb0"),
            colActualX, ry, 90, rowH, juce::Justification::centredLeft);
        g.drawText(juce::String(static_cast<int>(r.responseAngle)) + juce::String::fromUTF8("\xc2\xb0"),
            colRespX, ry, 90, rowH, juce::Justification::centredLeft);

        g.setColour(errorColour(r.angularError));
        g.drawText(juce::String(r.angularError, 1)+juce::String::fromUTF8("\xc2\xb0"),
            colErrX, ry, 90, rowH, juce::Justification::centredLeft);

        g.setColour(juce::Colour(colText));
        g.drawText(juce::String(r.confidence) + "/5",
            colConfX, ry, 80, rowH, juce::Justification::centredLeft);
    }
}

//==============================================================================
//  Circle drawing (dark theme)
//==============================================================================
juce::Point<float> BinauralTestSessionEditor::angleToPt(float deg, float r, float cx, float cy)
{
    float rad = (deg - 90.0f) * juce::MathConstants<float>::pi / 180.0f;
    return { cx + r * std::cos(rad), cy + r * std::sin(rad) };
}

void BinauralTestSessionEditor::drawCircle(juce::Graphics& g,
    float cx, float cy, float r,
    float userAngle, float actualAngle,
    bool /*interactive*/)
{
    // Outer ring
    g.setColour(juce::Colour(colBorder));
    g.drawEllipse(cx - r, cy - r, r * 2, r * 2, 1.5f);

    // Inner ring
    g.setColour(juce::Colour(colSurface2));
    g.drawEllipse(cx - r * 0.5f, cy - r * 0.5f, r, r, 1.0f);

    // Center dot
    g.setColour(juce::Colour(colHint));
    g.fillEllipse(cx - 3, cy - 3, 6, 6);

    // Tick marks
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

    // Degree labels
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

    // Direction labels
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

    // Actual source marker (green)
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

    // User response marker (accent blue)
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

//==============================================================================
juce::Colour BinauralTestSessionEditor::errorColour(float err) const
{
    if (err <= 15.0f) return juce::Colour(colGreen);
    if (err <= 45.0f) return juce::Colour(colOrange);
    return juce::Colour(colRed);
}

//==============================================================================
//  Mouse click on circle
//==============================================================================
void BinauralTestSessionEditor::mouseDown(const juce::MouseEvent& e)
{
    if (session.screen != TestSession::Screen::Trial) return;

    float mx = static_cast<float>(e.x);
    float my = static_cast<float>(e.y);
    float dx = mx - circCx;
    float dy = my - circCy;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist > circR + 20.0f) return;

    float angle = std::atan2(dx, -dy) * 180.0f / juce::MathConstants<float>::pi;
    if (angle < 0.0f) angle += 360.0f;

    session.userResponse = std::round(angle);
    submitBtn.setEnabled(session.canSubmit());
    repaint();
}