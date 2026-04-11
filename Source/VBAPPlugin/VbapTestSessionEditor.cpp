#include "VbapTestSessionEditor.h"
#include "PluginProcessor.h"

#include <cmath>

static constexpr int kEditorW = 620;
static constexpr int kEditorH = 860;

VbapTestSessionEditor::VbapTestSessionEditor(AudioPluginAudioProcessor& p)
    : AudioProcessorEditor(p), processorRef(p)
{
    setSize(kEditorW, kEditorH);
    setResizable(false, false);

    nameEditor.setText("Session 1");
    nameEditor.setJustification(juce::Justification::centredLeft);
    addChildComponent(nameEditor);

    speakerCountBox.addItem("2", 2);
    speakerCountBox.addItem("3", 3);
    speakerCountBox.addItem("4", 4);
    speakerCountBox.addItem("5", 5);
    speakerCountBox.addItem("6", 6);
    speakerCountBox.addItem("7", 7);
    speakerCountBox.addItem("8", 8);
    speakerCountBox.setSelectedId(speakerCount);
    speakerCountBox.onChange = [this]
    {
        speakerCount = speakerCountBox.getSelectedId();
        if (speakerCount < 2)
            speakerCount = 4;
        updateSpeakerEditorVisibility();
        resized();
        repaint();
    };
    addChildComponent(speakerCountBox);

    for (int i = 0; i < 8; ++i)
    {
        speakerAzLabels[(size_t) i].setText("Speaker " + juce::String(i + 1) + " azimuth", juce::dontSendNotification);
        speakerAzLabels[(size_t) i].setJustificationType(juce::Justification::centredLeft);
        speakerAzLabels[(size_t) i].setColour(juce::Label::textColourId, juce::Colour(colMuted));
        addChildComponent(speakerAzLabels[(size_t) i]);

        speakerAzEditors[(size_t) i].setText(juce::String(speakerAzimuths[(size_t) i], 1));
        speakerAzEditors[(size_t) i].setInputRestrictions(6, "0123456789.");
        addChildComponent(speakerAzEditors[(size_t) i]);
    }

    anglesEditor.setMultiLine(true, true);
    anglesEditor.setReturnKeyStartsNewLine(false);
    anglesEditor.setText("0, 45, 90, 135, 180, 225, 270, 315");
    addChildComponent(anglesEditor);

    numTrialsEditor.setText("8");
    numTrialsEditor.setInputRestrictions(2, "0123456789");
    addChildComponent(numTrialsEditor);

    randomBtn.onClick    = [this] { onRandomize(); };
    presetBtn.onClick    = [this] { onPreset(); };
    loadAudioBtn.onClick = [this] { onLoadAudio(); };
    startBtn.onClick     = [this] { onStart(); };

    addChildComponent(randomBtn);
    addChildComponent(presetBtn);
    addChildComponent(loadAudioBtn);
    addChildComponent(startBtn);

    audioFileLabel.setText("No audio file (use DAW input)", juce::dontSendNotification);
    audioFileLabel.setFont(juce::Font(juce::FontOptions(12.0f)));
    audioFileLabel.setColour(juce::Label::textColourId, juce::Colour(colHint));
    addChildComponent(audioFileLabel);

    playBtn.onClick   = [this] { onPlay(); };
    stopBtn.onClick   = [this] { onStop(); };
    submitBtn.onClick = [this] { onSubmit(); };

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

    nextBtn.onClick    = [this] { onNext(); };
    saveBtn.onClick    = [this] { onSave(); };
    newSessBtn.onClick = [this] { onNewSession(); };

    addChildComponent(nextBtn);
    addChildComponent(saveBtn);
    addChildComponent(newSessBtn);

    submitBtn.setEnabled(false);
    showSetupWidgets(true);
    updateSpeakerEditorVisibility();
    startTimerHz(10);
}

void VbapTestSessionEditor::timerCallback()
{
    if (processorRef.audioPlayer.isPlaying())
        playBtn.setButtonText("Playing...");
    else
        playBtn.setButtonText("Play");
}

void VbapTestSessionEditor::hideAllWidgets()
{
    showSetupWidgets(false);
    showTrialWidgets(false);
    showFeedbackWidgets(false);
    showSummaryWidgets(false);
}

void VbapTestSessionEditor::showSetupWidgets(bool v)
{
    nameEditor.setVisible(v);
    speakerCountBox.setVisible(v);
    anglesEditor.setVisible(v);
    numTrialsEditor.setVisible(v);
    randomBtn.setVisible(v);
    presetBtn.setVisible(v);
    loadAudioBtn.setVisible(v);
    audioFileLabel.setVisible(v);
    startBtn.setVisible(v);

    for (int i = 0; i < 8; ++i)
    {
        const bool showThisSpeaker = v && (i < speakerCount);
        speakerAzLabels[(size_t) i].setVisible(showThisSpeaker);
        speakerAzEditors[(size_t) i].setVisible(showThisSpeaker);
    }
}

void VbapTestSessionEditor::showTrialWidgets(bool v)
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

void VbapTestSessionEditor::showFeedbackWidgets(bool v)
{
    nextBtn.setVisible(v);
}

void VbapTestSessionEditor::showSummaryWidgets(bool v)
{
    saveBtn.setVisible(v);
    newSessBtn.setVisible(v);
}

void VbapTestSessionEditor::updateSpeakerEditorVisibility()
{
    for (int i = 0; i < 8; ++i)
    {
        const bool showThisSpeaker = nameEditor.isVisible() && (i < speakerCount);
        speakerAzLabels[(size_t) i].setVisible(showThisSpeaker);
        speakerAzEditors[(size_t) i].setVisible(showThisSpeaker);
    }
}

bool VbapTestSessionEditor::readSpeakerSetupFromUi(juce::String& errorMessage)
{
    speakerCount = speakerCountBox.getSelectedId();
    if (speakerCount < 2 || speakerCount > 8)
    {
        errorMessage = "Speaker count must be between 2 and 8.";
        return false;
    }

    for (int i = 0; i < speakerCount; ++i)
    {
        auto txt = speakerAzEditors[(size_t) i].getText().trim().replaceCharacter(',', '.');
        if (txt.isEmpty())
        {
            errorMessage = "Enter all visible speaker azimuths.";
            return false;
        }

        const float value = txt.getFloatValue();
        if (value < 0.0f || value > 360.0f)
        {
            errorMessage = "Speaker azimuths must be between 0 and 360 degrees.";
            return false;
        }

        speakerAzimuths[(size_t) i] = value;
    }

    return true;
}

juce::String VbapTestSessionEditor::buildSetupLabel() const
{
    juce::String label;
    label << juce::String(speakerCount) << " spk: ";

    for (int i = 0; i < speakerCount; ++i)
    {
        if (i > 0)
            label << ", ";
        label << juce::String(speakerAzimuths[(size_t) i], 1) << "°";
    }

    return label;
}

void VbapTestSessionEditor::onRandomize()
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

void VbapTestSessionEditor::onPreset()
{
    anglesEditor.setText("0, 45, 90, 135, 180, 225, 270, 315");
}

void VbapTestSessionEditor::onLoadAudio()
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
                        "Error",
                        "Could not load the audio file.");
                }
            }
        });
}

void VbapTestSessionEditor::onStart()
{
    session.sessionName = nameEditor.getText().trim();
    if (session.sessionName.isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Error",
            "Enter a session name.");
        return;
    }

    session.targetAngles = TestSession::parseAngles(anglesEditor.getText());
    if (session.targetAngles.empty())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Error",
            "Enter at least one angle (0-360).");
        return;
    }

    juce::String speakerError;
    if (!readSpeakerSetupFromUi(speakerError))
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Error",
            speakerError);
        return;
    }

    session.setupLabel = buildSetupLabel();
    session.results.clear();
    session.currentTrial = 0;
    session.userResponse = -1.0f;
    session.userConfidence = 0;
    session.screen = TestSession::Screen::Trial;

    syncParametersToProcessor();

    hideAllWidgets();
    showTrialWidgets(true);
    submitBtn.setEnabled(false);
    repaint();
}

void VbapTestSessionEditor::onPlay()
{
    if (session.useInternalAudio && processorRef.audioPlayer.isLoaded())
    {
        processorRef.audioPlayer.setLooping(true);
        processorRef.audioPlayer.play();
    }
}

void VbapTestSessionEditor::onStop()
{
    processorRef.audioPlayer.stop();
}

void VbapTestSessionEditor::onConfidence(int level)
{
    session.userConfidence = level;
    submitBtn.setEnabled(session.canSubmit());

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

void VbapTestSessionEditor::onSubmit()
{
    if (!session.canSubmit())
        return;

    processorRef.audioPlayer.stop();
    session.submitCurrentTrial();

    hideAllWidgets();
    showFeedbackWidgets(true);
    repaint();
}

void VbapTestSessionEditor::onNext()
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
        onConfidence(0);
    }

    repaint();
}

void VbapTestSessionEditor::onSave()
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
                    "Saved",
                    "Results saved to:\n" + file.getFullPathName());
            }
        });
}

void VbapTestSessionEditor::onNewSession()
{
    session.reset();
    hideAllWidgets();
    showSetupWidgets(true);
    updateSpeakerEditorVisibility();
    onConfidence(0);
    repaint();
}

void VbapTestSessionEditor::syncParametersToProcessor()
{
    if (session.currentTrial < static_cast<int>(session.targetAngles.size()))
    {
        const float angle = session.targetAngles[(size_t) session.currentTrial];
        if (auto* azParam = processorRef.parameters.getParameter("sourceAzimuth"))
            azParam->setValueNotifyingHost(azParam->convertTo0to1(angle));
    }

    if (auto* countParam = processorRef.parameters.getParameter("speakerCount"))
        countParam->setValueNotifyingHost(
            countParam->convertTo0to1(static_cast<float>(speakerCount)));

    for (int i = 0; i < 8; ++i)
    {
        if (auto* p = processorRef.parameters.getParameter("speakerAz" + juce::String(i + 1)))
            p->setValueNotifyingHost(
                p->convertTo0to1(speakerAzimuths[(size_t) i]));
    }
}

void VbapTestSessionEditor::resized()
{
    const int W = getWidth();
    const int pad = 24;
    const int fieldH = 34;
    const int gap = 12;

    int y = 60;

    nameEditor.setBounds(pad, y, W - 2 * pad, fieldH);
    y += fieldH + 28;

    speakerCountBox.setBounds(pad, y, 140, fieldH);
    y += fieldH + 28;

    const int colGap = 28;
    const int colW = (W - 2 * pad - colGap) / 2;
    const int labelW = 130;
    const int rowStep = fieldH + 12;

    for (int i = 0; i < 8; ++i)
    {
        const int row = i / 2;
        const int col = i % 2;
        const int x = pad + col * (colW + colGap);
        const int rowY = y + row * rowStep;

        speakerAzLabels[(size_t) i].setBounds(x, rowY, labelW, fieldH);
        speakerAzEditors[(size_t) i].setBounds(x + labelW + 10, rowY, colW - labelW - 10, fieldH);
    }

    y += 4 * rowStep + 26;

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

    circR = 108.0f;
    circCx = W * 0.5f;
    circCy = 320.0f;
    circBounds = juce::Rectangle<float>(circCx - circR - 30, circCy - circR - 30,
                                        (circR + 30) * 2, (circR + 30) * 2);

    int trialY = static_cast<int>(circCy + circR) + 60;

    playBtn.setBounds(pad, trialY, 80, fieldH);
    stopBtn.setBounds(pad + 88, trialY, 80, fieldH);
    trialY += fieldH + 20;

    const int confW = 44;
    const int confGap = 8;
    const int confTotalW = 5 * confW + 4 * confGap;
    const int confX = (W - confTotalW) / 2;

    confBtn1.setBounds(confX, trialY, confW, confW);
    confBtn2.setBounds(confX + (confW + confGap), trialY, confW, confW);
    confBtn3.setBounds(confX + 2 * (confW + confGap), trialY, confW, confW);
    confBtn4.setBounds(confX + 3 * (confW + confGap), trialY, confW, confW);
    confBtn5.setBounds(confX + 4 * (confW + confGap), trialY, confW, confW);
    trialY += confW + 24;

    submitBtn.setBounds(pad, trialY, W - 2 * pad, 40);

    nextBtn.setBounds(pad, kEditorH - 60, W - 2 * pad, 40);

    const int sumBtnW = (W - 2 * pad - gap) / 2;
    saveBtn.setBounds(pad, kEditorH - 60, sumBtnW, 40);
    newSessBtn.setBounds(pad + sumBtnW + gap, kEditorH - 60, sumBtnW, 40);
}

void VbapTestSessionEditor::paint(juce::Graphics& g)
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

void VbapTestSessionEditor::paintSetup(juce::Graphics& g)
{
    g.setColour(juce::Colour(colText));
    g.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
    g.drawText("VBAP session setup", 24, 16, getWidth() - 48, 30, juce::Justification::centredLeft);

    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.setColour(juce::Colour(colMuted));

    // Session name
    g.drawText("Session name",
               nameEditor.getX(),
               nameEditor.getY() - 18,
               220,
               16,
               juce::Justification::centredLeft);

    // Speaker count
    g.drawText("Speaker count",
               speakerCountBox.getX(),
               speakerCountBox.getY() - 18,
               220,
               16,
               juce::Justification::centredLeft);

    // Speaker azimuths section title
    const int speakerSectionY = speakerAzLabels[0].getY() - 18;
    g.drawText("Speaker azimuths (degrees)",
               speakerAzLabels[0].getX(),
               speakerSectionY,
               260,
               16,
               juce::Justification::centredLeft);

    // Source angles
    g.drawText("Source angles (degrees, 0-360, comma-separated)",
               anglesEditor.getX(),
               anglesEditor.getY() - 18,
               420,
               16,
               juce::Justification::centredLeft);

    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.setColour(juce::Colour(colHint));
    g.drawText("0 = front   90 = right   180 = back   270 = left",
               anglesEditor.getX(),
               anglesEditor.getBottom() + 2,
               420,
               16,
               juce::Justification::centredLeft);
}

void VbapTestSessionEditor::paintTrial(juce::Graphics& g)
{
    const int W = getWidth();
    const int n = session.currentTrial + 1;
    const int tot = static_cast<int>(session.targetAngles.size());
    const float pct = static_cast<float>(session.currentTrial) / static_cast<float>(tot);

    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.setColour(juce::Colour(colMuted));
    g.drawText("Trial " + juce::String(n) + " of " + juce::String(tot),
               24, 12, 200, 20, juce::Justification::centredLeft);

    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText(session.sessionName, W - 224, 12, 200, 20, juce::Justification::centredRight);

    juce::Rectangle<float> barBg(24.0f, 36.0f, static_cast<float>(W - 48), 3.0f);
    g.setColour(juce::Colour(0xFFE8E7E0));
    g.fillRoundedRectangle(barBg, 1.5f);
    g.setColour(juce::Colour(colBlue));
    g.fillRoundedRectangle(barBg.withWidth(barBg.getWidth() * pct), 1.5f);

    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.setColour(juce::Colour(colMuted));
    g.drawText("Listen to the sound, then click where you think it came from",
               24, 52, W - 48, 20, juce::Justification::centred);

    drawCircle(g, circCx, circCy, circR, session.userResponse, -1.0f, true);

    const float textY = circCy + circR + 24;
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.setColour(juce::Colour(colMuted));
    const juce::String selText = session.userResponse >= 0.0f
        ? "Selected: " + juce::String(static_cast<int>(session.userResponse))
        : "Selected: click circle to select";
    g.drawText(selText, 24, static_cast<int>(textY), W - 48, 20, juce::Justification::centred);

    const int confLabelY = static_cast<int>(circCy + circR) + 60 - 20;
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.setColour(juce::Colour(colMuted));
    g.drawText("Confidence level", 24, confLabelY, 200, 16, juce::Justification::centredLeft);

    const int confBtnBottom = confBtn1.getBottom() + 4;
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.setColour(juce::Colour(colHint));
    g.drawText("Not confident", confBtn1.getX(), confBtnBottom, 120, 14, juce::Justification::centredLeft);
    g.drawText("Very confident", confBtn5.getRight() - 120, confBtnBottom, 120, 14, juce::Justification::centredRight);
}

void VbapTestSessionEditor::paintFeedback(juce::Graphics& g)
{
    const int W = getWidth();
    const auto& trial = session.results.back();
    const int trialNum = static_cast<int>(session.results.size());
    const int tot = static_cast<int>(session.targetAngles.size());
    const float pct = static_cast<float>(trialNum) / static_cast<float>(tot);

    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.setColour(juce::Colour(colMuted));
    g.drawText("Trial " + juce::String(trialNum) + " of " + juce::String(tot) + " - result",
               24, 12, 300, 20, juce::Justification::centredLeft);

    juce::Rectangle<float> barBg(24.0f, 36.0f, static_cast<float>(W - 48), 3.0f);
    g.setColour(juce::Colour(0xFFE8E7E0));
    g.fillRoundedRectangle(barBg, 1.5f);
    g.setColour(juce::Colour(colBlue));
    g.fillRoundedRectangle(barBg.withWidth(barBg.getWidth() * pct), 1.5f);

    drawCircle(g, circCx, circCy, circR, trial.responseAngle, trial.targetAngle, false);

    const float legY = circCy + circR + 20;
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

    const float metY = legY + 28;
    const float metW = (W - 48.0f - 16.0f) / 3.0f;
    drawMetricBox(g, { 24.0f, metY, metW, 60.0f }, "Actual",
                  juce::String(static_cast<int>(trial.targetAngle)));
    drawMetricBox(g, { 24.0f + metW + 8.0f, metY, metW, 60.0f }, "Your answer",
                  juce::String(static_cast<int>(trial.responseAngle)));
    drawMetricBox(g, { 24.0f + 2.0f * (metW + 8.0f), metY, metW, 60.0f }, "Angular error",
                  juce::String(trial.angularError, 1),
                  errorColour(trial.angularError));

    const bool isLast = session.currentTrial >= static_cast<int>(session.targetAngles.size());
    nextBtn.setButtonText(isLast ? "View summary" : "Next trial");
}

void VbapTestSessionEditor::paintSummary(juce::Graphics& g)
{
    const int W = getWidth();

    g.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
    g.setColour(juce::Colour(colText));
    g.drawText("Session complete", 24, 16, W - 48, 24, juce::Justification::centredLeft);

    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.setColour(juce::Colour(colMuted));
    g.drawText(session.sessionName + "  |  " + session.setupLabel + "  |  "
               + juce::String(static_cast<int>(session.results.size())) + " trials",
               24, 44, W - 48, 18, juce::Justification::centredLeft);

    const float metY = 72;
    const float metW = (W - 48.0f - 16.0f) / 3.0f;
    drawMetricBox(g, { 24, metY, metW, 60 }, "Mean error",
                  juce::String(session.meanError(), 1));
    drawMetricBox(g, { 24 + metW + 8, metY, metW, 60 }, "Best trial",
                  juce::String(session.bestError(), 1),
                  juce::Colour(colGreen));
    drawMetricBox(g, { 24 + 2 * (metW + 8), metY, metW, 60 }, "Worst trial",
                  juce::String(session.worstError(), 1),
                  juce::Colour(colRed));

    const int tableY = static_cast<int>(metY) + 76;
    const int rowH = 24;
    g.setFont(juce::Font(juce::FontOptions(12.0f)));

    g.setColour(juce::Colour(colMuted));
    g.drawText("#",         24,  tableY, 30,  rowH, juce::Justification::centredLeft);
    g.drawText("Actual",    60,  tableY, 80,  rowH, juce::Justification::centredLeft);
    g.drawText("Response",  150, tableY, 80,  rowH, juce::Justification::centredLeft);
    g.drawText("Error",     240, tableY, 80,  rowH, juce::Justification::centredLeft);
    g.drawText("Confidence",340, tableY, 100, rowH, juce::Justification::centredLeft);

    g.setColour(juce::Colour(colBorder));
    g.drawLine(24.0f, static_cast<float>(tableY + rowH), static_cast<float>(W - 24),
               static_cast<float>(tableY + rowH), 0.5f);

    g.setFont(juce::Font(juce::FontOptions("Courier New", 13.0f, juce::Font::plain)));
    const int maxVisible = std::min(static_cast<int>(session.results.size()), 20);
    for (int i = 0; i < maxVisible; ++i)
    {
        const int ry = tableY + rowH + i * rowH;
        auto& r = session.results[(size_t) i];

        g.setColour(juce::Colour(colText));
        g.drawText(juce::String(r.trialNumber), 24, ry, 30, rowH, juce::Justification::centredLeft);
        g.drawText(juce::String(static_cast<int>(r.targetAngle)), 60, ry, 80, rowH, juce::Justification::centredLeft);
        g.drawText(juce::String(static_cast<int>(r.responseAngle)), 150, ry, 80, rowH, juce::Justification::centredLeft);

        g.setColour(errorColour(r.angularError));
        g.drawText(juce::String(r.angularError, 1), 240, ry, 80, rowH, juce::Justification::centredLeft);

        g.setColour(juce::Colour(colText));
        g.drawText(juce::String(r.confidence) + "/5", 340, ry, 100, rowH, juce::Justification::centredLeft);

        g.setColour(juce::Colour(colBorder));
        g.drawLine(24.0f, static_cast<float>(ry + rowH), static_cast<float>(W - 24),
                   static_cast<float>(ry + rowH), 0.5f);
    }
}

juce::Point<float> VbapTestSessionEditor::angleToPt(float deg, float r, float cx, float cy)
{
    const float rad = (deg - 90.0f) * juce::MathConstants<float>::pi / 180.0f;
    return { cx + r * std::cos(rad), cy + r * std::sin(rad) };
}

void VbapTestSessionEditor::drawCircle(juce::Graphics& g,
                                       float cx, float cy, float r,
                                       float userAngle, float actualAngle,
                                       bool /*interactive*/)
{
    g.setColour(juce::Colour(0x26000000));
    g.drawEllipse(cx - r, cy - r, r * 2, r * 2, 1.5f);

    g.setColour(juce::Colour(0x12000000));
    g.drawEllipse(cx - r * 0.5f, cy - r * 0.5f, r, r, 0.75f);

    g.setColour(juce::Colour(colHint));
    g.fillEllipse(cx - 3, cy - 3, 6, 6);

    for (int a = 0; a < 360; a += 10)
    {
        const bool major = (a % 90 == 0);
        const bool med   = (a % 30 == 0);
        const float innerR = r - (major ? 13.0f : med ? 8.0f : 4.0f);
        const auto p1 = angleToPt(static_cast<float>(a), r, cx, cy);
        const auto p2 = angleToPt(static_cast<float>(a), innerR, cx, cy);
        g.setColour(juce::Colour(0x33000000));
        g.drawLine(p1.x, p1.y, p2.x, p2.y, major ? 1.5f : 0.75f);
    }

    g.setFont(juce::Font(juce::FontOptions("Courier New", 11.0f, juce::Font::plain)));
    g.setColour(juce::Colour(colHint));
    const char* degLabels[] = { "0", "90", "180", "270" };
    const float degAngles[] = { 0, 90, 180, 270 };
    for (int i = 0; i < 4; ++i)
    {
        const auto pt = angleToPt(degAngles[i], r + 18.0f, cx, cy);
        g.drawText(juce::String(degLabels[i]),
                   static_cast<int>(pt.x - 20), static_cast<int>(pt.y - 8), 40, 16,
                   juce::Justification::centred);
    }

    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.setColour(juce::Colour(0xFFB4B2A9));
    const char* dirLabels[] = { "Front", "Right", "Back", "Left" };
    for (int i = 0; i < 4; ++i)
    {
        const auto pt = angleToPt(degAngles[i], r - 28.0f, cx, cy);
        g.drawText(dirLabels[i],
                   static_cast<int>(pt.x - 24), static_cast<int>(pt.y - 8), 48, 16,
                   juce::Justification::centred);
    }

    if (actualAngle >= 0.0f)
    {
        const auto apt = angleToPt(actualAngle, r, cx, cy);
        g.setColour(juce::Colour(colGreen).withAlpha(0.65f));
        g.drawLine(cx, cy, apt.x, apt.y, 1.2f);
        g.setColour(juce::Colour(colGreen).withAlpha(0.18f));
        g.fillEllipse(apt.x - 9, apt.y - 9, 18, 18);
        g.setColour(juce::Colour(colGreen));
        g.fillEllipse(apt.x - 5, apt.y - 5, 10, 10);
    }

    if (userAngle >= 0.0f)
    {
        const auto upt = angleToPt(userAngle, r, cx, cy);
        g.setColour(juce::Colour(colBlue).withAlpha(0.65f));
        g.drawLine(cx, cy, upt.x, upt.y, 1.2f);
        g.setColour(juce::Colour(colBlue).withAlpha(0.18f));
        g.fillEllipse(upt.x - 9, upt.y - 9, 18, 18);
        g.setColour(juce::Colour(colBlue));
        g.fillEllipse(upt.x - 5, upt.y - 5, 10, 10);
    }
}

void VbapTestSessionEditor::drawMetricBox(juce::Graphics& g,
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

juce::Colour VbapTestSessionEditor::errorColour(float err) const
{
    if (err <= 15.0f) return juce::Colour(colGreen);
    if (err <= 30.0f) return juce::Colour(colOrange);
    return juce::Colour(colRed);
}

void VbapTestSessionEditor::mouseDown(const juce::MouseEvent& e)
{
    if (session.screen != TestSession::Screen::Trial)
        return;

    if (!circBounds.contains(e.position))
        return;

    const float dx = e.position.x - circCx;
    const float dy = e.position.y - circCy;
    const float dist = std::sqrt(dx * dx + dy * dy);
    if (dist > circR + 16.0f)
        return;

    float deg = std::atan2(dy, dx) * 180.0f / juce::MathConstants<float>::pi + 90.0f;
    while (deg < 0.0f)   deg += 360.0f;
    while (deg >= 360.0f) deg -= 360.0f;

    session.userResponse = deg;
    submitBtn.setEnabled(session.canSubmit());
    repaint();
}