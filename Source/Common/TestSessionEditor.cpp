#include "TestSessionEditor.h"
#include "../BinauralPlugin/PluginProcessor.h"

static constexpr int kEditorW = 560;
static constexpr int kEditorH = 780;

//==============================================================================
TestSessionEditor::TestSessionEditor(AudioPluginAudioProcessor& p)
    : AudioProcessorEditor(p), processorRef(p)
{
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
    addChildComponent(numTrialsEditor);

    layoutBox.addItem("9 speakers (40 deg)",  1);
    layoutBox.addItem("12 speakers (30 deg)", 2);
    layoutBox.addItem("18 speakers (20 deg)", 3);
    layoutBox.addItem("36 speakers (10 deg)", 4);
    layoutBox.setSelectedId(4);
    addChildComponent(layoutBox);

    randomBtn.onClick  = [this] { onRandomize(); };
    presetBtn.onClick  = [this] { onPreset(); };
    loadAudioBtn.onClick = [this] { onLoadAudio(); };
    startBtn.onClick   = [this] { onStart(); };
    addChildComponent(randomBtn);
    addChildComponent(presetBtn);
    addChildComponent(loadAudioBtn);
    addChildComponent(startBtn);

    audioFileLabel.setText("No audio file (use DAW input)", juce::dontSendNotification);
    audioFileLabel.setFont(juce::Font(juce::FontOptions(12.0f)));
    audioFileLabel.setColour(juce::Label::textColourId, juce::Colour(colHint));
    addChildComponent(audioFileLabel);

    // ── Trial widgets ───────────────────────────────────────
    playBtn.onClick    = [this] { onPlay(); };
    stopBtn.onClick    = [this] { onStop(); };
    submitBtn.onClick  = [this] { onSubmit(); };
    addChildComponent(playBtn);
    addChildComponent(stopBtn);
    addChildComponent(submitBtn);

    confBtn1.onClick = [this] { onConfidence(1); };
    confBtn2.onClick = [this] { onConfidence(2); };
    confBtn3.onClick = [this] { onConfidence(3); };
    confBtn4.onClick = [this] { onConfidence(4); };
    confBtn5.onClick = [this] { onConfidence(5); };
    addChildComponent(confBtn1);
    addChildComponent(confBtn2);
    addChildComponent(confBtn3);
    addChildComponent(confBtn4);
    addChildComponent(confBtn5);

    // ── Feedback / summary widgets ──────────────────────────
    nextBtn.onClick    = [this] { onNext(); };
    saveBtn.onClick    = [this] { onSave(); };
    newSessBtn.onClick = [this] { onNewSession(); };
    addChildComponent(nextBtn);
    addChildComponent(saveBtn);
    addChildComponent(newSessBtn);

    // Start on setup screen
    showSetupWidgets(true);

    startTimerHz(10);
}

//==============================================================================
void TestSessionEditor::timerCallback()
{
    // Update play button text if internal audio is playing
    if (processorRef.audioPlayer.isPlaying())
        playBtn.setButtonText("Playing...");
    else
        playBtn.setButtonText("Play");
}

//==============================================================================
//  Widget visibility helpers
//==============================================================================
void TestSessionEditor::hideAllWidgets()
{
    showSetupWidgets(false);
    showTrialWidgets(false);
    showFeedbackWidgets(false);
    showSummaryWidgets(false);
}

void TestSessionEditor::showSetupWidgets(bool v)
{
    nameEditor.setVisible(v);
    anglesEditor.setVisible(v);
    numTrialsEditor.setVisible(v);
    layoutBox.setVisible(v);
    randomBtn.setVisible(v);
    presetBtn.setVisible(v);
    loadAudioBtn.setVisible(v);
    audioFileLabel.setVisible(v);
    startBtn.setVisible(v);
}

void TestSessionEditor::showTrialWidgets(bool v)
{
    playBtn.setVisible(v);
    stopBtn.setVisible(v);
    submitBtn.setVisible(v);
    confBtn1.setVisible(v);
    confBtn2.setVisible(v);
    confBtn3.setVisible(v);
    confBtn4.setVisible(v);
    confBtn5.setVisible(v);
}

void TestSessionEditor::showFeedbackWidgets(bool v)
{
    nextBtn.setVisible(v);
}

void TestSessionEditor::showSummaryWidgets(bool v)
{
    saveBtn.setVisible(v);
    newSessBtn.setVisible(v);
}

//==============================================================================
//  Actions
//==============================================================================
void TestSessionEditor::onRandomize()
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

void TestSessionEditor::onPreset()
{
    anglesEditor.setText("0, 45, 90, 135, 180, 225, 270, 315");
}

void TestSessionEditor::onLoadAudio()
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

void TestSessionEditor::onStart()
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

    session.layoutMode = layoutBox.getSelectedId() - 1;  // 0-3
    session.results.clear();
    session.currentTrial = 0;
    session.userResponse = -1.0f;
    session.userConfidence = 0;
    session.screen = TestSession::Screen::Trial;

    syncParametersToProcessor();

    hideAllWidgets();
    showTrialWidgets(true);
    repaint();
}

void TestSessionEditor::onPlay()
{
    if (session.useInternalAudio && processorRef.audioPlayer.isLoaded())
    {
        processorRef.audioPlayer.setLooping(true);
        processorRef.audioPlayer.play();
    }
}

void TestSessionEditor::onStop()
{
    processorRef.audioPlayer.stop();
}

void TestSessionEditor::onConfidence(int level)
{
    session.userConfidence = level;
    submitBtn.setEnabled(session.canSubmit());

    // Highlight the selected confidence button
    auto updateBtn = [&](juce::TextButton& btn, int idx)
    {
        if (idx == level)
        {
            btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFFE6F1FB));
            btn.setColour(juce::TextButton::textColourOffId, juce::Colour(colBlue));
        }
        else
        {
            btn.setColour(juce::TextButton::buttonColourId, juce::Colour(colCard));
            btn.setColour(juce::TextButton::textColourOffId, juce::Colour(colMuted));
        }
    };
    updateBtn(confBtn1, 1);
    updateBtn(confBtn2, 2);
    updateBtn(confBtn3, 3);
    updateBtn(confBtn4, 4);
    updateBtn(confBtn5, 5);
    repaint();
}

void TestSessionEditor::onSubmit()
{
    if (!session.canSubmit()) return;

    processorRef.audioPlayer.stop();
    session.submitCurrentTrial();

    hideAllWidgets();
    showFeedbackWidgets(true);
    repaint();
}

void TestSessionEditor::onNext()
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
        showTrialWidgets(true);
        submitBtn.setEnabled(false);
        onConfidence(0);  // reset
    }
    repaint();
}

void TestSessionEditor::onSave()
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

void TestSessionEditor::onNewSession()
{
    session.reset();
    hideAllWidgets();
    showSetupWidgets(true);
    onConfidence(0);
    repaint();
}

void TestSessionEditor::syncParametersToProcessor()
{
    if (session.currentTrial < static_cast<int>(session.targetAngles.size()))
    {
        float angle = session.targetAngles[static_cast<size_t>(session.currentTrial)];
        auto* azParam = processorRef.parameters.getParameter("sourceAzimuth");
        if (azParam)
            azParam->setValueNotifyingHost(
                azParam->convertTo0to1(angle));
    }

    auto* layoutParam = processorRef.parameters.getParameter("layoutMode");
    if (layoutParam)
        layoutParam->setValueNotifyingHost(
            layoutParam->convertTo0to1(static_cast<float>(session.layoutMode)));
}

//==============================================================================
//  resized
//==============================================================================
void TestSessionEditor::resized()
{
    const int W = getWidth();
    const int pad = 24;
    const int fieldH = 34;
    int y = 60;

    // ── Setup layout ────────────────────────────────────
    nameEditor.setBounds(pad, y, W - 2 * pad, fieldH);
    y += fieldH + 28;
    layoutBox.setBounds(pad, y, W - 2 * pad, fieldH);
    y += fieldH + 28;
    anglesEditor.setBounds(pad, y, W - 2 * pad, 60);
    y += 60 + 26;

    numTrialsEditor.setBounds(pad, y, 60, fieldH);
    randomBtn.setBounds(pad + 68, y, 160, fieldH);
    presetBtn.setBounds(pad + 236, y, 130, fieldH);
    y += fieldH + 16;

    loadAudioBtn.setBounds(pad, y, 160, fieldH);
    audioFileLabel.setBounds(pad + 168, y, W - 2 * pad - 168, fieldH);
    y += fieldH + 16;

    startBtn.setBounds(pad, y, W - 2 * pad, 40);

    // ── Trial layout ────────────────────────────────────
    // Circle is drawn at paint time; store geometry
    circR = 108.0f;
    circCx = W * 0.5f;
    circCy = 300.0f;
    circBounds = juce::Rectangle<float>(circCx - circR - 30, circCy - circR - 30,
                                         (circR + 30) * 2, (circR + 30) * 2);

    int trialY = static_cast<int>(circCy + circR) + 60;

    playBtn.setBounds(pad, trialY, 80, fieldH);
    stopBtn.setBounds(pad + 88, trialY, 80, fieldH);
    trialY += fieldH + 20;

    int confW = 44, confGap = 8;
    int confTotalW = 5 * confW + 4 * confGap;
    int confX = (W - confTotalW) / 2;
    confBtn1.setBounds(confX, trialY, confW, confW);
    confBtn2.setBounds(confX + (confW + confGap),     trialY, confW, confW);
    confBtn3.setBounds(confX + 2 * (confW + confGap), trialY, confW, confW);
    confBtn4.setBounds(confX + 3 * (confW + confGap), trialY, confW, confW);
    confBtn5.setBounds(confX + 4 * (confW + confGap), trialY, confW, confW);
    trialY += confW + 24;

    submitBtn.setBounds(pad, trialY, W - 2 * pad, 40);

    // ── Feedback layout ─────────────────────────────────
    nextBtn.setBounds(pad, kEditorH - 60, W - 2 * pad, 40);

    // ── Summary layout ──────────────────────────────────
    int sumBtnW = (W - 2 * pad - 8) / 2;
    saveBtn.setBounds(pad, kEditorH - 60, sumBtnW, 40);
    newSessBtn.setBounds(pad + sumBtnW + 8, kEditorH - 60, sumBtnW, 40);
}

//==============================================================================
//  paint
//==============================================================================
void TestSessionEditor::paint(juce::Graphics& g)
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
void TestSessionEditor::paintSetup(juce::Graphics& g)
{
    g.setColour(juce::Colour(colText));
    g.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
    g.drawText("Session setup", 24, 16, getWidth() - 48, 30, juce::Justification::centredLeft);

    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.setColour(juce::Colour(colMuted));
    g.drawText("Session name", 24, 44, 200, 16, juce::Justification::centredLeft);
    g.drawText("Speaker layout", 24, 44 + 34 + 12, 200, 16, juce::Justification::centredLeft);
    g.drawText("Source angles (degrees, 0-360, comma-separated)", 24, 44 + 34 + 12 + 34 + 12, 500, 16, juce::Justification::centredLeft);

    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.setColour(juce::Colour(colHint));
    g.drawText("0 = front   90 = right   180 = back   270 = left",
               24, 44 + 34 + 12 + 34 + 12 + 60 + 2, 500, 16, juce::Justification::centredLeft);
}

//==============================================================================
void TestSessionEditor::paintTrial(juce::Graphics& g)
{
    const int W = getWidth();
    const int n = session.currentTrial + 1;
    const int tot = static_cast<int>(session.targetAngles.size());
    const float pct = static_cast<float>(session.currentTrial) / static_cast<float>(tot);

    // ── Header ─────────────────────────────────────────
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.setColour(juce::Colour(colMuted));
    g.drawText("Trial " + juce::String(n) + " of " + juce::String(tot),
               24, 12, 200, 20, juce::Justification::centredLeft);

    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText(session.sessionName, W - 224, 12, 200, 20, juce::Justification::centredRight);

    // ── Progress bar ──────────────────────────────────
    juce::Rectangle<float> barBg(24.0f, 36.0f, static_cast<float>(W - 48), 3.0f);
    g.setColour(juce::Colour(0xFFE8E7E0));
    g.fillRoundedRectangle(barBg, 1.5f);
    g.setColour(juce::Colour(colBlue));
    g.fillRoundedRectangle(barBg.withWidth(barBg.getWidth() * pct), 1.5f);

    // ── Instruction ───────────────────────────────────
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.setColour(juce::Colour(colMuted));
    g.drawText("Listen to the sound, then click where you think it came from",
               24, 52, W - 48, 20, juce::Justification::centred);

    // ── Circle ────────────────────────────────────────
    drawCircle(g, circCx, circCy, circR, session.userResponse, -1.0f, true);

    // ── Selected angle text ───────────────────────────
    float textY = circCy + circR + 24;
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.setColour(juce::Colour(colMuted));
    juce::String selText = session.userResponse >= 0
        ? "Selected: " + juce::String(static_cast<int>(session.userResponse))
        : "Selected: click circle to select";
    g.drawText(selText, 24, static_cast<int>(textY), W - 48, 20, juce::Justification::centred);

    // ── Confidence label ──────────────────────────────
    int confLabelY = static_cast<int>(circCy + circR) + 60 - 20;
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.setColour(juce::Colour(colMuted));
    g.drawText("Confidence level", 24, confLabelY, 200, 16, juce::Justification::centredLeft);

    // labels below confidence buttons
    int confBtnBottom = confBtn1.getBottom() + 4;
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.setColour(juce::Colour(colHint));
    g.drawText("Not confident", confBtn1.getX(), confBtnBottom, 120, 14, juce::Justification::centredLeft);
    g.drawText("Very confident", confBtn5.getRight() - 120, confBtnBottom, 120, 14, juce::Justification::centredRight);
}

//==============================================================================
void TestSessionEditor::paintFeedback(juce::Graphics& g)
{
    const int W = getWidth();
    const auto& trial = session.results.back();
    const int trialNum = static_cast<int>(session.results.size());
    const int tot = static_cast<int>(session.targetAngles.size());
    const float pct = static_cast<float>(trialNum) / static_cast<float>(tot);

    // ── Header ─────────────────────────────────────────
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.setColour(juce::Colour(colMuted));
    g.drawText("Trial " + juce::String(trialNum) + " of " + juce::String(tot) + " - result",
               24, 12, 300, 20, juce::Justification::centredLeft);

    // ── Progress ──────────────────────────────────────
    juce::Rectangle<float> barBg(24.0f, 36.0f, static_cast<float>(W - 48), 3.0f);
    g.setColour(juce::Colour(0xFFE8E7E0));
    g.fillRoundedRectangle(barBg, 1.5f);
    g.setColour(juce::Colour(colBlue));
    g.fillRoundedRectangle(barBg.withWidth(barBg.getWidth() * pct), 1.5f);

    // ── Circle showing both markers ───────────────────
    drawCircle(g, circCx, circCy, circR, trial.responseAngle, trial.targetAngle, false);

    // ── Legend ─────────────────────────────────────────
    float legY = circCy + circR + 20;
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.setColour(juce::Colour(colGreen));
    g.fillEllipse(W * 0.5f - 100.0f, legY, 9.0f, 9.0f);
    g.setColour(juce::Colour(colMuted));
    g.drawText("Actual source", static_cast<int>(W * 0.5f - 88.0f), static_cast<int>(legY - 2), 100, 14,
               juce::Justification::centredLeft);

    g.setColour(juce::Colour(colBlue));
    g.fillEllipse(W * 0.5f + 20.0f, legY, 9.0f, 9.0f);
    g.setColour(juce::Colour(colMuted));
    g.drawText("Your response", static_cast<int>(W * 0.5f + 32.0f), static_cast<int>(legY - 2), 100, 14,
               juce::Justification::centredLeft);

    // ── Metrics ───────────────────────────────────────
    float metY = legY + 28;
    float metW = (W - 48.0f - 16.0f) / 3.0f;
    drawMetricBox(g, { 24.0f, metY, metW, 60.0f }, "Actual",
                  juce::String(static_cast<int>(trial.targetAngle)) );
    drawMetricBox(g, { 24.0f + metW + 8.0f, metY, metW, 60.0f }, "Your answer",
                  juce::String(static_cast<int>(trial.responseAngle)));
    drawMetricBox(g, { 24.0f + 2.0f * (metW + 8.0f), metY, metW, 60.0f }, "Angular error",
                  juce::String(trial.angularError, 1),
                  errorColour(trial.angularError));

    // Next button label
    bool isLast = session.currentTrial >= static_cast<int>(session.targetAngles.size());
    nextBtn.setButtonText(isLast ? "View summary" : juce::String("Next trial "));
}

//==============================================================================
void TestSessionEditor::paintSummary(juce::Graphics& g)
{
    const int W = getWidth();

    g.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
    g.setColour(juce::Colour(colText));
    g.drawText("Session complete", 24, 16, W - 48, 24, juce::Justification::centredLeft);

    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.setColour(juce::Colour(colMuted));

    juce::String layoutStr;
    switch (session.layoutMode) {
        case 0: layoutStr = "9 spk"; break;
        case 1: layoutStr = "12 spk"; break;
        case 2: layoutStr = "18 spk"; break;
        default: layoutStr = "36 spk"; break;
    }
    g.drawText(session.sessionName + "  |  " + layoutStr + "  |  "
               + juce::String(static_cast<int>(session.results.size())) + " trials",
               24, 44, W - 48, 18, juce::Justification::centredLeft);

    // ── Metrics ───────────────────────────────────────
    float metY = 72;
    float metW = (W - 48.0f - 16.0f) / 3.0f;
    drawMetricBox(g, { 24, metY, metW, 60 }, "Mean error",
                  juce::String(session.meanError(), 1));
    drawMetricBox(g, { 24 + metW + 8, metY, metW, 60 }, "Best trial",
                  juce::String(session.bestError(), 1),
                  juce::Colour(colGreen));
    drawMetricBox(g, { 24 + 2 * (metW + 8), metY, metW, 60 }, "Worst trial",
                  juce::String(session.worstError(), 1),
                  juce::Colour(colRed));

    // ── Results table ─────────────────────────────────
    int tableY = static_cast<int>(metY) + 76;
    int rowH = 24;
    g.setFont(juce::Font(juce::FontOptions(12.0f)));

    // Header
    g.setColour(juce::Colour(colMuted));
    g.drawText("#",         24,  tableY, 30,  rowH, juce::Justification::centredLeft);
    g.drawText("Actual",    60,  tableY, 80,  rowH, juce::Justification::centredLeft);
    g.drawText("Response",  150, tableY, 80,  rowH, juce::Justification::centredLeft);
    g.drawText("Error",     240, tableY, 80,  rowH, juce::Justification::centredLeft);
    g.drawText("Confidence",340, tableY, 100, rowH, juce::Justification::centredLeft);

    g.setColour(juce::Colour(colBorder));
    g.drawLine(24.0f, static_cast<float>(tableY + rowH), static_cast<float>(W - 24), static_cast<float>(tableY + rowH), 0.5f);

    // Rows
    g.setFont(juce::Font(juce::FontOptions("Courier New", 13.0f, juce::Font::plain)));
    int maxVisible = std::min(static_cast<int>(session.results.size()), 20);
    for (int i = 0; i < maxVisible; ++i)
    {
        int ry = tableY + rowH + i * rowH;
        auto& r = session.results[static_cast<size_t>(i)];

        g.setColour(juce::Colour(colText));
        g.drawText(juce::String(r.trialNumber),     24,  ry, 30, rowH, juce::Justification::centredLeft);
        g.drawText(juce::String(static_cast<int>(r.targetAngle)),
                   60, ry, 80, rowH, juce::Justification::centredLeft);
        g.drawText(juce::String(static_cast<int>(r.responseAngle)),
                   150, ry, 80, rowH, juce::Justification::centredLeft);

        g.setColour(errorColour(r.angularError));
        g.drawText(juce::String(r.angularError, 1),
                   240, ry, 80, rowH, juce::Justification::centredLeft);

        g.setColour(juce::Colour(colText));
        g.drawText(juce::String(r.confidence) + "/5",
                   340, ry, 100, rowH, juce::Justification::centredLeft);

        g.setColour(juce::Colour(colBorder));
        g.drawLine(24.0f, static_cast<float>(ry + rowH), static_cast<float>(W - 24), static_cast<float>(ry + rowH), 0.5f);
    }
}

//==============================================================================
//  Circle drawing  (same logic as the HTML SVG)
//==============================================================================
juce::Point<float> TestSessionEditor::angleToPt(float deg, float r, float cx, float cy)
{
    float rad = (deg - 90.0f) * juce::MathConstants<float>::pi / 180.0f;
    return { cx + r * std::cos(rad), cy + r * std::sin(rad) };
}

void TestSessionEditor::drawCircle(juce::Graphics& g,
                                                  float cx, float cy, float r,
                                                  float userAngle, float actualAngle,
                                                  bool /*interactive*/)
{
    // Outer ring
    g.setColour(juce::Colour(0x26000000));
    g.drawEllipse(cx - r, cy - r, r * 2, r * 2, 1.5f);

    // Inner dashed ring
    g.setColour(juce::Colour(0x12000000));
    g.drawEllipse(cx - r * 0.5f, cy - r * 0.5f, r, r, 0.75f);

    // Center dot
    g.setColour(juce::Colour(colHint));
    g.fillEllipse(cx - 3, cy - 3, 6, 6);

    // Tick marks
    for (int a = 0; a < 360; a += 10)
    {
        bool major = (a % 90 == 0);
        bool med   = (a % 30 == 0);
        float innerR = r - (major ? 13.0f : med ? 8.0f : 4.0f);
        auto p1 = angleToPt(static_cast<float>(a), r, cx, cy);
        auto p2 = angleToPt(static_cast<float>(a), innerR, cx, cy);
        g.setColour(juce::Colour(0x33000000));
        g.drawLine(p1.x, p1.y, p2.x, p2.y, major ? 1.5f : 0.75f);
    }

    // Degree labels
    g.setFont(juce::Font(juce::FontOptions("Courier New", 11.0f, juce::Font::plain)));
    g.setColour(juce::Colour(colHint));
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
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.setColour(juce::Colour(0xFFB4B2A9));
    const char* dirLabels[] = { "Front", "Right", "Back", "Left" };
    for (int i = 0; i < 4; ++i)
    {
        auto pt = angleToPt(degAngles[i], r - 28.0f, cx, cy);
        g.drawText(dirLabels[i],
                   static_cast<int>(pt.x - 24), static_cast<int>(pt.y - 8), 48, 16,
                   juce::Justification::centred);
    }

    // Actual source marker (green)
    if (actualAngle >= 0.0f)
    {
        auto apt = angleToPt(actualAngle, r, cx, cy);
        g.setColour(juce::Colour(colGreen).withAlpha(0.65f));
        g.drawLine(cx, cy, apt.x, apt.y, 1.2f);
        g.setColour(juce::Colour(colGreen).withAlpha(0.18f));
        g.fillEllipse(apt.x - 9, apt.y - 9, 18, 18);
        g.setColour(juce::Colour(colGreen));
        g.fillEllipse(apt.x - 5, apt.y - 5, 10, 10);
    }

    // User response marker (blue)
    if (userAngle >= 0.0f)
    {
        auto upt = angleToPt(userAngle, r, cx, cy);
        g.setColour(juce::Colour(colBlue).withAlpha(0.65f));
        g.drawLine(cx, cy, upt.x, upt.y, 1.2f);
        g.setColour(juce::Colour(colBlue).withAlpha(0.18f));
        g.fillEllipse(upt.x - 9, upt.y - 9, 18, 18);
        g.setColour(juce::Colour(colBlue));
        g.fillEllipse(upt.x - 5, upt.y - 5, 10, 10);
    }
}

//==============================================================================
void TestSessionEditor::drawMetricBox(juce::Graphics& g,
                                                    juce::Rectangle<float> area,
                                                    const juce::String& label,
                                                    const juce::String& value,
                                                    juce::Colour valueCol)
{
    g.setColour(juce::Colour(colMetric));
    g.fillRoundedRectangle(area, 8.0f);

    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.setColour(juce::Colour(colMuted));
    g.drawText(label, area.reduced(10, 0).withHeight(20).translated(0, 8),
               juce::Justification::centredLeft);

    g.setFont(juce::Font(juce::FontOptions("Courier New", 22.0f, juce::Font::bold)));
    g.setColour(valueCol);
    g.drawText(value, area.reduced(10, 0).withTrimmedTop(28),
               juce::Justification::centredLeft);
}

//==============================================================================
juce::Colour TestSessionEditor::errorColour(float err) const
{
    if (err <= 15.0f) return juce::Colour(colGreen);
    if (err <= 45.0f) return juce::Colour(colOrange);
    return juce::Colour(colRed);
}

//==============================================================================
//  Mouse click on the circle
//==============================================================================
void TestSessionEditor::mouseDown(const juce::MouseEvent& e)
{
    if (session.screen != TestSession::Screen::Trial) return;

    float mx = static_cast<float>(e.x);
    float my = static_cast<float>(e.y);
    float dx = mx - circCx;
    float dy = my - circCy;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist > circR + 20.0f) return;  // outside circle

    float angle = std::atan2(dx, -dy) * 180.0f / juce::MathConstants<float>::pi;
    if (angle < 0.0f) angle += 360.0f;

    session.userResponse = std::round(angle);
    submitBtn.setEnabled(session.canSubmit());
    repaint();
}
