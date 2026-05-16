#include "VbapParameterEditor.h"
#include "PluginProcessor.h"
#include "../Common/VbapEngine2D.h"

#include <cmath>

//==============================================================================
//  Layout constants
//==============================================================================
static constexpr int kEditorW    = 660;
static constexpr int kEditorH    = 840;
static constexpr int kPad        = 22;
static constexpr int kCircleSize = 320;

//==============================================================================
//  DarkLookAndFeel
//==============================================================================
VbapParameterEditor::DarkLookAndFeel::DarkLookAndFeel()
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

    setColour(juce::Slider::backgroundColourId,         juce::Colour(colBorder));
    setColour(juce::Slider::trackColourId,              juce::Colour(colAccent));
    setColour(juce::Slider::thumbColourId,              juce::Colour(colText));
    setColour(juce::Slider::textBoxBackgroundColourId,  juce::Colour(colSurface2));
    setColour(juce::Slider::textBoxTextColourId,        juce::Colour(colText));
    setColour(juce::Slider::textBoxOutlineColourId,     juce::Colour(colBorder));

    setColour(juce::PopupMenu::backgroundColourId,            juce::Colour(colSurface));
    setColour(juce::PopupMenu::textColourId,                  juce::Colour(colText));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(colAccent));
    setColour(juce::PopupMenu::highlightedTextColourId,       juce::Colours::white);

    setColour(juce::AlertWindow::backgroundColourId, juce::Colour(colSurface));
    setColour(juce::AlertWindow::textColourId,       juce::Colour(colText));
    setColour(juce::AlertWindow::outlineColourId,    juce::Colour(colBorder));
}

juce::Font VbapParameterEditor::DarkLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    return juce::Font(juce::FontOptions(juce::jmin(15.0f, buttonHeight * 0.55f), juce::Font::plain));
}

void VbapParameterEditor::DarkLookAndFeel::drawButtonBackground(juce::Graphics& g,
    juce::Button& b, const juce::Colour& backgroundColour,
    bool shouldDrawAsHighlighted, bool shouldDrawAsDown)
{
    const auto bounds = b.getLocalBounds().toFloat().reduced(0.5f, 0.5f);
    const float cornerSize = 8.0f;

    juce::Colour fill = backgroundColour;
    if (shouldDrawAsDown)
        fill = fill.brighter(0.10f);
    else if (shouldDrawAsHighlighted)
        fill = fill.brighter(0.05f);

    g.setColour(fill);
    g.fillRoundedRectangle(bounds, cornerSize);
    g.setColour(juce::Colour(colBorder));
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
}

void VbapParameterEditor::DarkLookAndFeel::drawTextEditorOutline(juce::Graphics& g,
    int width, int height, juce::TextEditor& te)
{
    const auto bounds = juce::Rectangle<float>(0, 0, (float) width, (float) height).reduced(0.5f);
    g.setColour(te.hasKeyboardFocus(true)
        ? te.findColour(juce::TextEditor::focusedOutlineColourId)
        : te.findColour(juce::TextEditor::outlineColourId));
    g.drawRoundedRectangle(bounds, 8.0f, 1.0f);
}

void VbapParameterEditor::DarkLookAndFeel::fillTextEditorBackground(juce::Graphics& g,
    int width, int height, juce::TextEditor& te)
{
    g.setColour(te.findColour(juce::TextEditor::backgroundColourId));
    g.fillRoundedRectangle(juce::Rectangle<float>(0, 0, (float) width, (float) height), 8.0f);
}

//==============================================================================
//  StepperControl
//==============================================================================
VbapParameterEditor::StepperControl::StepperControl(APVTS& apvts, const juce::String& id,
    float minVal, float maxVal, float step, bool wrapValues)
    : apvtsRef(apvts), paramId(id),
      minValue(minVal), maxValue(maxVal), stepSize(step), wraps(wrapValues),
      decBtn(juce::String::fromUTF8("\xe2\x88\x92"))
{
    decBtn.onClick = [this] { stepBy(-stepSize); };
    incBtn.onClick = [this] { stepBy(stepSize); };
    addAndMakeVisible(decBtn);
    addAndMakeVisible(incBtn);

    valueEditor.setInputRestrictions(8, "0123456789.");
    valueEditor.setJustification(juce::Justification::centred);
    valueEditor.setIndents(2, 2);
    valueEditor.setSelectAllWhenFocused(true);
    valueEditor.onReturnKey = [this] { applyTypedValue(); valueEditor.giveAwayKeyboardFocus(); };
    valueEditor.onEscapeKey = [this] { refreshFromParameter(); valueEditor.giveAwayKeyboardFocus(); };
    valueEditor.onFocusLost = [this] { applyTypedValue(); };
    addAndMakeVisible(valueEditor);

    apvtsRef.addParameterListener(paramId, this);
    refreshFromParameter();
}

VbapParameterEditor::StepperControl::~StepperControl()
{
    apvtsRef.removeParameterListener(paramId, this);
}

void VbapParameterEditor::StepperControl::resized()
{
    const int btnW = 26;
    const auto r = getLocalBounds();
    decBtn.setBounds(r.getX(), r.getY(), btnW, r.getHeight());
    incBtn.setBounds(r.getRight() - btnW, r.getY(), btnW, r.getHeight());
    valueEditor.setBounds(r.getX() + btnW + 2, r.getY(),
                          r.getWidth() - 2 * (btnW + 2), r.getHeight());
}

void VbapParameterEditor::StepperControl::setEnabledState(bool enabled)
{
    decBtn.setEnabled(enabled);
    incBtn.setEnabled(enabled);
    valueEditor.setEnabled(enabled);
    setAlpha(enabled ? 1.0f : 0.35f);
}

void VbapParameterEditor::StepperControl::parameterChanged(const juce::String&, float)
{
    juce::MessageManager::callAsync(
        [safe = juce::Component::SafePointer<StepperControl>(this)]()
        {
            if (safe != nullptr)
                safe->refreshFromParameter();
        });
}

void VbapParameterEditor::StepperControl::refreshFromParameter()
{
    if (auto* p = apvtsRef.getRawParameterValue(paramId))
        valueEditor.setText(juce::String(juce::roundToInt(p->load())) + juce::String::fromUTF8("\xc2\xb0"),
                            juce::dontSendNotification);
}

float VbapParameterEditor::StepperControl::wrapOrClamp(float v) const
{
    if (wraps)
    {
        const float range = maxValue - minValue;
        if (range <= 0.0f) return minValue;
        while (v >= maxValue) v -= range;
        while (v < minValue)  v += range;
        return v;
    }
    return juce::jlimit(minValue, maxValue, v);
}

void VbapParameterEditor::StepperControl::stepBy(float delta)
{
    if (auto* param = apvtsRef.getParameter(paramId))
    {
        const float current = param->convertFrom0to1(param->getValue());
        const float next    = wrapOrClamp(current + delta);
        param->beginChangeGesture();
        param->setValueNotifyingHost(param->convertTo0to1(next));
        param->endChangeGesture();
    }
}

void VbapParameterEditor::StepperControl::applyTypedValue()
{
    const float val = valueEditor.getText().getFloatValue();
    if (auto* param = apvtsRef.getParameter(paramId))
    {
        const float wrapped = wrapOrClamp(val);
        param->beginChangeGesture();
        param->setValueNotifyingHost(param->convertTo0to1(wrapped));
        param->endChangeGesture();
    }
    refreshFromParameter();
}

//==============================================================================
//  SteppedIntSlider
//==============================================================================
VbapParameterEditor::SteppedIntSlider::SteppedIntSlider(APVTS& apvts, const juce::String& id,
    int minVal, int maxVal)
    : apvtsRef(apvts), paramId(id), minValue(minVal), maxValue(maxVal)
{
    apvtsRef.addParameterListener(paramId, this);
    refreshFromParameter();
}

VbapParameterEditor::SteppedIntSlider::~SteppedIntSlider()
{
    apvtsRef.removeParameterListener(paramId, this);
}

float VbapParameterEditor::SteppedIntSlider::xForValue(int v) const
{
    const int n = maxValue - minValue;
    if (n <= 0) return 16.0f;
    const float t = (float) (v - minValue) / (float) n;
    return 16.0f + t * ((float) getWidth() - 32.0f);
}

void VbapParameterEditor::SteppedIntSlider::paint(juce::Graphics& g)
{
    const float trackY = 14.0f;

    // Track background
    g.setColour(juce::Colour(colBorder));
    g.fillRoundedRectangle(16.0f, trackY, (float) (getWidth() - 32), 3.0f, 1.5f);

    // Filled portion: from first dot to current value
    const float xMin = xForValue(minValue);
    const float xCur = xForValue(currentValue);
    g.setColour(juce::Colour(colAccent));
    g.fillRoundedRectangle(xMin, trackY, xCur - xMin, 3.0f, 1.5f);

    // Dots + numbers
    for (int i = minValue; i <= maxValue; ++i)
    {
        const float x = xForValue(i);
        const float y = trackY + 1.5f;
        const bool isActive = (i == currentValue);
        const bool isPassed = (i < currentValue);

        if (isActive)
        {
            g.setColour(juce::Colour(colAccent).withAlpha(0.20f));
            g.fillEllipse(x - 13, y - 13, 26, 26);
            g.setColour(juce::Colour(colAccent));
            g.fillEllipse(x - 9, y - 9, 18, 18);
            g.setColour(juce::Colour(colText));
            g.drawEllipse(x - 9, y - 9, 18, 18, 1.5f);
        }
        else if (isPassed)
        {
            g.setColour(juce::Colour(colAccent));
            g.fillEllipse(x - 7, y - 7, 14, 14);
        }
        else
        {
            g.setColour(juce::Colour(colSurface2));
            g.fillEllipse(x - 7, y - 7, 14, 14);
            g.setColour(juce::Colour(colBorder));
            g.drawEllipse(x - 7, y - 7, 14, 14, 1.5f);
        }

        g.setFont(juce::Font(juce::FontOptions("Courier New", 12.0f, juce::Font::bold)));
        g.setColour((isActive || isPassed) ? juce::Colour(colText) : juce::Colour(colMuted));
        g.drawText(juce::String(i),
                   (int) (x - 14), (int) (y + 16), 28, 14,
                   juce::Justification::centred);
    }
}

void VbapParameterEditor::SteppedIntSlider::mouseDown(const juce::MouseEvent& e)
{
    setValueFromX(e.position.x);
}

void VbapParameterEditor::SteppedIntSlider::mouseDrag(const juce::MouseEvent& e)
{
    setValueFromX(e.position.x);
}

void VbapParameterEditor::SteppedIntSlider::parameterChanged(const juce::String&, float)
{
    juce::MessageManager::callAsync(
        [safe = juce::Component::SafePointer<SteppedIntSlider>(this)]()
        {
            if (safe != nullptr)
                safe->refreshFromParameter();
        });
}

void VbapParameterEditor::SteppedIntSlider::refreshFromParameter()
{
    if (auto* p = apvtsRef.getRawParameterValue(paramId))
    {
        const int v = juce::jlimit(minValue, maxValue, (int) std::round(p->load()));
        if (v != currentValue)
        {
            currentValue = v;
            if (onValueChange) onValueChange();
        }
        repaint();
    }
}

void VbapParameterEditor::SteppedIntSlider::setValueFromX(float xpos)
{
    const int n = maxValue - minValue;
    if (n <= 0) return;
    const float t = juce::jlimit(0.0f, 1.0f, (xpos - 16.0f) / ((float) getWidth() - 32.0f));
    const int value = minValue + (int) std::round(t * n);
    const int clamped = juce::jlimit(minValue, maxValue, value);

    if (clamped != currentValue)
    {
        if (auto* param = apvtsRef.getParameter(paramId))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost(param->convertTo0to1((float) clamped));
            param->endChangeGesture();
        }
    }
}

//==============================================================================
//  Editor
//==============================================================================
VbapParameterEditor::VbapParameterEditor(AudioPluginAudioProcessor& p)
    : AudioProcessorEditor(p), processorRef(p)
{
    setLookAndFeel(&darkLnf);

    inputGainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    inputGainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 24);
    addAndMakeVisible(inputGainSlider);
    inputGainAttachment = std::make_unique<APVTS::SliderAttachment>(
        processorRef.parameters, "inputGain", inputGainSlider);
    inputGainSlider.setNumDecimalPlacesToDisplay(2);
    inputGainSlider.textFromValueFunction = [](double v) { return juce::String(v, 2); };
    inputGainSlider.updateText();

    sourceAzimuthStepper = std::make_unique<StepperControl>(
        processorRef.parameters, "sourceAzimuth", 0.0f, 360.0f, 1.0f, true);
    addAndMakeVisible(*sourceAzimuthStepper);

    speakerCountSlider = std::make_unique<SteppedIntSlider>(
        processorRef.parameters, "speakerCount", 2, 8);
    speakerCountSlider->onValueChange = [this]
    {
        updateSpeakerEnabledStates();
        repaint();
    };
    addAndMakeVisible(*speakerCountSlider);

    for (int i = 0; i < 8; ++i)
    {
        speakerSteppers[(size_t) i] = std::make_unique<StepperControl>(
            processorRef.parameters,
            "speakerAz" + juce::String(i + 1),
            0.0f, 360.0f, 1.0f, true);
        addAndMakeVisible(*speakerSteppers[(size_t) i]);

        auto& l = speakerLabels[(size_t) i];
        l.setText("Spk " + juce::String(i + 1), juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setColour(juce::Label::textColourId, juce::Colour(colMuted));
        l.setFont(juce::Font(juce::FontOptions(12.0f)));
        l.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(l);
    }

    setupSideLabel(inputGainLabel,     "Input gain");
    setupSideLabel(sourceAzimuthLabel, "Source azimuth");
    setupSideLabel(speakerCountLabel,  "Speaker count");

    sourceAzimuthHint.setText("or drag the blue dot above", juce::dontSendNotification);
    sourceAzimuthHint.setJustificationType(juce::Justification::centredLeft);
    sourceAzimuthHint.setColour(juce::Label::textColourId, juce::Colour(colHint));
    sourceAzimuthHint.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::italic)));
    sourceAzimuthHint.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(sourceAzimuthHint);

    defaultLayoutBtn.onClick = [this] { applyDefaultLayout(); };
    styleAccentButton(defaultLayoutBtn);
    addAndMakeVisible(defaultLayoutBtn);

    loadAudioBtn.onClick   = [this] { onLoadAudio(); };
    playStopBtn.onClick    = [this] { onPlayStopToggle(); };
    addAndMakeVisible(loadAudioBtn);
    addAndMakeVisible(playStopBtn);
    stylePlayStopButton(false);

    audioFileLabel.setText("No audio file (use DAW input)", juce::dontSendNotification);
    audioFileLabel.setFont(juce::Font(juce::FontOptions(12.0f)));
    audioFileLabel.setJustificationType(juce::Justification::centredLeft);
    audioFileLabel.setColour(juce::Label::textColourId, juce::Colour(colHint));
    audioFileLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(audioFileLabel);

    if (processorRef.audioPlayer.isLoaded())
        audioFileLabel.setText(processorRef.audioPlayer.getFileName(),
                               juce::dontSendNotification);

    // Editor-level parameter listeners: repaint circle when relevant params change
    processorRef.parameters.addParameterListener("sourceAzimuth", this);
    processorRef.parameters.addParameterListener("speakerCount",  this);
    for (int i = 0; i < 8; ++i)
        processorRef.parameters.addParameterListener("speakerAz" + juce::String(i + 1), this);

    // Set size LAST — triggers resized() which references all the child widgets above.
    setResizable(false, false);
    setSize(kEditorW, kEditorH);

    startTimerHz(15);
}

VbapParameterEditor::~VbapParameterEditor()
{
    processorRef.parameters.removeParameterListener("sourceAzimuth", this);
    processorRef.parameters.removeParameterListener("speakerCount",  this);
    for (int i = 0; i < 8; ++i)
        processorRef.parameters.removeParameterListener("speakerAz" + juce::String(i + 1), this);

    setLookAndFeel(nullptr);
}

void VbapParameterEditor::setupSideLabel(juce::Label& l, const juce::String& text)
{
    l.setText(text, juce::dontSendNotification);
    l.setJustificationType(juce::Justification::centredLeft);
    l.setColour(juce::Label::textColourId, juce::Colour(colMuted));
    l.setFont(juce::Font(juce::FontOptions(13.0f)));
    l.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(l);
}

void VbapParameterEditor::styleAccentButton(juce::TextButton& btn)
{
    btn.setColour(juce::TextButton::buttonColourId,  juce::Colour(colAccent));
    btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    btn.setColour(juce::TextButton::textColourOnId,  juce::Colours::white);
}

void VbapParameterEditor::stylePlayStopButton(bool playingNow)
{
    if (playingNow)
    {
        playStopBtn.setButtonText(juce::String::fromUTF8("\xe2\x96\xa0  Stop"));
        playStopBtn.setColour(juce::TextButton::buttonColourId,  juce::Colour(colRed));
    }
    else
    {
        playStopBtn.setButtonText(juce::String::fromUTF8("\xe2\x96\xb6  Play"));
        playStopBtn.setColour(juce::TextButton::buttonColourId,  juce::Colour(colGreen));
    }
    playStopBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    playStopBtn.setColour(juce::TextButton::textColourOnId,  juce::Colours::white);
}

int VbapParameterEditor::getCurrentSpeakerCount() const
{
    if (auto* p = processorRef.parameters.getRawParameterValue("speakerCount"))
        return juce::jlimit(2, 8, (int) std::round(p->load()));
    return 4;
}

void VbapParameterEditor::updateSpeakerEnabledStates()
{
    const int active = getCurrentSpeakerCount();
    for (int i = 0; i < 8; ++i)
    {
        speakerSteppers[(size_t) i]->setEnabledState(i < active);
        speakerLabels  [(size_t) i].setAlpha(i < active ? 1.0f : 0.45f);
    }
}

void VbapParameterEditor::applyDefaultLayout()
{
    const int n = getCurrentSpeakerCount();
    const auto angles = vbap::DefaultVBAPLayoutAngles(n);

    for (int i = 0; i < (int) angles.size() && i < 8; ++i)
    {
        const juce::String pid = "speakerAz" + juce::String(i + 1);
        if (auto* param = processorRef.parameters.getParameter(pid))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost(param->convertTo0to1(vbap::wrap360(angles[(size_t) i])));
            param->endChangeGesture();
        }
    }
}

void VbapParameterEditor::resized()
{
    const int W = getWidth();

    // Reserve top: title (drawn in paint) + hint line
    int y = kPad + 50;

    // Circle
    const int circleX = (W - kCircleSize) / 2;
    circleArea = juce::Rectangle<float>((float) circleX, (float) y,
                                        (float) kCircleSize, (float) kCircleSize);
    circleCenter = circleArea.getCentre();
    circleR = (float) kCircleSize * 0.5f - 26.0f;
    y += kCircleSize + 12;

    // "AUDIO SOURCE" section header reserve
    y += 22;

    const int labelW = 130;
    const int rowH = 28;

    inputGainLabel.setBounds(kPad, y, labelW, rowH);
    inputGainSlider.setBounds(kPad + labelW + 10, y,
                              W - 2 * kPad - labelW - 10, rowH);
    y += rowH + 8;

    sourceAzimuthLabel.setBounds(kPad, y, labelW, rowH);
    sourceAzimuthStepper->setBounds(kPad + labelW + 10, y, 130, rowH);
    sourceAzimuthHint.setBounds(kPad + labelW + 10 + 130 + 12, y,
                                W - kPad - (kPad + labelW + 10 + 130 + 12), rowH);
    y += rowH + 18;

    // "SPEAKERS" section header reserve
    y += 22;

    const int sliderRowH = 50;
    speakerCountLabel.setBounds(kPad, y, labelW, 28);
    speakerCountSlider->setBounds(kPad + labelW + 10, y - 4,
                                   W - 2 * kPad - labelW - 10, sliderRowH);
    y += sliderRowH;

    const int btnH = 32;
    defaultLayoutBtn.setBounds(kPad, y, 150, btnH);
    y += btnH + 18;

    const int cols = 4;
    const int cellGap = 18;
    const int cellW = (W - 2 * kPad - (cols - 1) * cellGap) / cols;
    const int stepperH = 30;
    const int cellH = stepperH + 20;
    const int rowGap = 18;

    for (int i = 0; i < 8; ++i)
    {
        const int col = i % cols;
        const int row = i / cols;
        const int x = kPad + col * (cellW + cellGap);
        const int cellY = y + row * (cellH + rowGap);

        speakerLabels[(size_t) i].setBounds(x, cellY, cellW, 16);
        speakerSteppers[(size_t) i]->setBounds(x, cellY + 20, cellW, stepperH);
    }
    updateSpeakerEnabledStates();

    y += 2 * cellH + rowGap + 32;

    loadAudioBtn.setBounds(kPad, y, 180, btnH);
    playStopBtn.setBounds(kPad + 188, y, 100, btnH);
    audioFileLabel.setBounds(kPad + 298, y, W - kPad - 298, btnH);
}

void VbapParameterEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(colBg));

    // Title
    g.setColour(juce::Colour(colText));
    g.setFont(juce::Font(juce::FontOptions(20.0f, juce::Font::bold)));
    g.drawText("VBAP", kPad, 12, getWidth() - 2 * kPad, 28, juce::Justification::centredLeft);

    // Subtitle / cardinal hint
    g.setColour(juce::Colour(colHint));
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    const juce::String dot = juce::String::fromUTF8("\xc2\xb7");
    g.drawText("Drag dots on the circle to position speakers and source.  0 = front "
                 + dot + " 90 = right " + dot + " 180 = back " + dot + " 270 = left",
               kPad, 42, getWidth() - 2 * kPad, 16, juce::Justification::centredLeft);

    paintCircle(g);

    // Section headers
    const int audioSourceY = inputGainLabel.getY() - 24;
    paintSectionHeader(g, kPad, audioSourceY, getWidth() - 2 * kPad, "Audio source");

    const int speakersY = speakerCountLabel.getY() - 24;
    paintSectionHeader(g, kPad, speakersY, getWidth() - 2 * kPad, "Speakers");
}

void VbapParameterEditor::paintSectionHeader(juce::Graphics& g, int x, int y, int w,
                                             const juce::String& text)
{
    g.setColour(juce::Colour(colHint));
    g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    g.drawText(text.toUpperCase(), x, y, w, 14, juce::Justification::centredLeft);

    g.setColour(juce::Colour(colBorder));
    const auto metrics = juce::Font(juce::FontOptions(11.0f, juce::Font::bold))
        .getStringWidthFloat(text.toUpperCase()) + 12.0f;
    g.drawHorizontalLine(y + 7, (float) (x + (int) metrics), (float) (x + w));
}

void VbapParameterEditor::paintCircle(juce::Graphics& g)
{
    const float cx = circleCenter.x;
    const float cy = circleCenter.y;
    const float r  = circleR;

    // Outer ring
    g.setColour(juce::Colour(colBorder));
    g.drawEllipse(cx - r, cy - r, r * 2, r * 2, 1.5f);

    // Inner ring at 0.5r (matches Binaural drawCircle)
    g.setColour(juce::Colour(colSurface2));
    g.drawEllipse(cx - r * 0.5f, cy - r * 0.5f, r, r, 1.0f);

    // Centre dot
    g.setColour(juce::Colour(colHint));
    g.fillEllipse(cx - 3, cy - 3, 6, 6);

    // Tick marks every 10°, major at 90°, medium at 30°
    for (int a = 0; a < 360; a += 10)
    {
        const bool major = (a % 90 == 0);
        const bool med   = (a % 30 == 0);
        const float innerR = r - (major ? 14.0f : med ? 9.0f : 5.0f);
        const auto p1 = angleToPt(static_cast<float>(a), r, cx, cy);
        const auto p2 = angleToPt(static_cast<float>(a), innerR, cx, cy);
        g.setColour(juce::Colour(colBorder));
        g.drawLine(p1.x, p1.y, p2.x, p2.y, major ? 1.5f : 0.75f);
    }

    // Degree labels
    g.setFont(juce::Font(juce::FontOptions("Courier New", 13.0f, juce::Font::plain)));
    g.setColour(juce::Colour(colMuted));
    const char* degLabels[] = { "0", "90", "180", "270" };
    const float degAngles[] = { 0.0f, 90.0f, 180.0f, 270.0f };
    for (int i = 0; i < 4; ++i)
    {
        const auto pt = angleToPt(degAngles[i], r + 18.0f, cx, cy);
        g.drawText(juce::String(degLabels[i]),
                   static_cast<int>(pt.x - 20), static_cast<int>(pt.y - 8), 40, 16,
                   juce::Justification::centred);
    }

    // Direction labels (inside circle)
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.setColour(juce::Colour(colHint));
    const char* dirLabels[] = { "Front", "Right", "Back", "Left" };
    for (int i = 0; i < 4; ++i)
    {
        const auto pt = angleToPt(degAngles[i], r - 30.0f, cx, cy);
        g.drawText(dirLabels[i],
                   static_cast<int>(pt.x - 24), static_cast<int>(pt.y - 8), 48, 16,
                   juce::Justification::centred);
    }

    // Speakers — green dots numbered
    const int speakerCount = getCurrentSpeakerCount();
    for (int i = 0; i < speakerCount; ++i)
    {
        float angle = 0.0f;
        if (auto* p = processorRef.parameters.getRawParameterValue("speakerAz" + juce::String(i + 1)))
            angle = p->load();
        const auto pt = angleToPt(angle, r, cx, cy);

        g.setColour(juce::Colour(colGreen).withAlpha(0.18f));
        g.fillEllipse(pt.x - 14, pt.y - 14, 28, 28);
        g.setColour(juce::Colour(colGreen));
        g.fillEllipse(pt.x - 9, pt.y - 9, 18, 18);

        g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        g.setColour(juce::Colour(colBg));
        g.drawText(juce::String(i + 1),
                   static_cast<int>(pt.x - 9), static_cast<int>(pt.y - 9), 18, 18,
                   juce::Justification::centred);
    }

    // Source — accent blue
    float srcAngle = 0.0f;
    if (auto* p = processorRef.parameters.getRawParameterValue("sourceAzimuth"))
        srcAngle = p->load();
    const auto spt = angleToPt(srcAngle, r, cx, cy);

    g.setColour(juce::Colour(colAccent).withAlpha(0.65f));
    g.drawLine(cx, cy, spt.x, spt.y, 1.5f);
    g.setColour(juce::Colour(colAccent).withAlpha(0.20f));
    g.fillEllipse(spt.x - 13, spt.y - 13, 26, 26);
    g.setColour(juce::Colour(colAccent));
    g.fillEllipse(spt.x - 7, spt.y - 7, 14, 14);
}

juce::Point<float> VbapParameterEditor::angleToPt(float deg, float r, float cx, float cy)
{
    const float rad = (deg - 90.0f) * juce::MathConstants<float>::pi / 180.0f;
    return { cx + r * std::cos(rad), cy + r * std::sin(rad) };
}

int VbapParameterEditor::hitTestCircle(const juce::MouseEvent& e) const
{
    const float cx = circleCenter.x;
    const float cy = circleCenter.y;
    const float r  = circleR;

    const float dxc = e.position.x - cx;
    const float dyc = e.position.y - cy;
    const float distC = std::sqrt(dxc * dxc + dyc * dyc);

    if (distC > r + 34.0f)
        return -2;

    float minDist = 20.0f;
    int   target  = -2;

    float srcAngle = 0.0f;
    if (auto* p = processorRef.parameters.getRawParameterValue("sourceAzimuth"))
        srcAngle = p->load();
    const auto spt = angleToPt(srcAngle, r, cx, cy);
    const float dSrc = std::sqrt(std::pow(e.position.x - spt.x, 2.0f)
                               + std::pow(e.position.y - spt.y, 2.0f));
    if (dSrc < minDist) { minDist = dSrc; target = -1; }

    const int speakerCount = getCurrentSpeakerCount();
    for (int i = 0; i < speakerCount; ++i)
    {
        float a = 0.0f;
        if (auto* p = processorRef.parameters.getRawParameterValue("speakerAz" + juce::String(i + 1)))
            a = p->load();
        const auto pp = angleToPt(a, r, cx, cy);
        const float d = std::sqrt(std::pow(e.position.x - pp.x, 2.0f)
                                + std::pow(e.position.y - pp.y, 2.0f));
        if (d < minDist) { minDist = d; target = i; }
    }

    if (target == -2 && distC <= r + 30.0f)
        target = -1;

    return target;
}

static juce::String paramIdForDragTarget(int target)
{
    if (target == -1) return "sourceAzimuth";
    if (target >= 0 && target < 8) return "speakerAz" + juce::String(target + 1);
    return {};
}

void VbapParameterEditor::mouseDown(const juce::MouseEvent& e)
{
    dragTargetIndex = hitTestCircle(e);
    if (dragTargetIndex == -2)
        return;

    if (auto* param = processorRef.parameters.getParameter(paramIdForDragTarget(dragTargetIndex)))
        param->beginChangeGesture();

    updateAngleFromMouse(e);
    repaint();
}

void VbapParameterEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (dragTargetIndex != -2)
    {
        updateAngleFromMouse(e);
        repaint();
    }
}

void VbapParameterEditor::mouseUp(const juce::MouseEvent&)
{
    if (dragTargetIndex != -2)
    {
        if (auto* param = processorRef.parameters.getParameter(paramIdForDragTarget(dragTargetIndex)))
            param->endChangeGesture();
    }
    dragTargetIndex = -2;
}

void VbapParameterEditor::updateAngleFromMouse(const juce::MouseEvent& e)
{
    const float cx = circleCenter.x;
    const float cy = circleCenter.y;
    const float dx = e.position.x - cx;
    const float dy = e.position.y - cy;
    if (std::abs(dx) < 0.0001f && std::abs(dy) < 0.0001f)
        return;

    float deg = std::atan2(dy, dx) * 180.0f / juce::MathConstants<float>::pi + 90.0f;
    while (deg < 0.0f)    deg += 360.0f;
    while (deg >= 360.0f) deg -= 360.0f;

    const juce::String pid = paramIdForDragTarget(dragTargetIndex);
    if (pid.isEmpty()) return;

    if (auto* param = processorRef.parameters.getParameter(pid))
        param->setValueNotifyingHost(param->convertTo0to1(deg));
}

void VbapParameterEditor::parameterChanged(const juce::String&, float)
{
    juce::MessageManager::callAsync(
        [safe = juce::Component::SafePointer<VbapParameterEditor>(this)]()
        {
            if (safe != nullptr)
                safe->repaint();
        });
}

void VbapParameterEditor::onLoadAudio()
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
                    audioFileLabel.setText(file.getFileName(), juce::dontSendNotification);
                    audioFileLabel.setColour(juce::Label::textColourId, juce::Colour(colText));
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

void VbapParameterEditor::onPlayStopToggle()
{
    if (processorRef.audioPlayer.isPlaying())
    {
        processorRef.audioPlayer.stop();
    }
    else if (processorRef.audioPlayer.isLoaded())
    {
        processorRef.audioPlayer.setLooping(true);
        processorRef.audioPlayer.play();
    }
}

void VbapParameterEditor::timerCallback()
{
    const bool nowPlaying = processorRef.audioPlayer.isPlaying();
    if (nowPlaying != wasPlayingLastTick)
    {
        stylePlayStopButton(nowPlaying);
        wasPlayingLastTick = nowPlaying;
    }
}
