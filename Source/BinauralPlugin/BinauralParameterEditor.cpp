#include "BinauralParameterEditor.h"
#include "PluginProcessor.h"
#include "../Common/VbapEngine2D.h"

#include <cmath>

using namespace pluginUI;

//==============================================================================
//  Layout constants
//==============================================================================
static constexpr int kEditorW    = 620;
static constexpr int kEditorH    = 760;
static constexpr int kPad        = 22;
static constexpr int kCircleSize = 300;

static const char* kLayoutNames[] = {
    "Direct HRTF",
    "VBAP 5",
    "VBAP 7",
    "VBAP 9",
    "VBAP 12",
    "VBAP 18"
};

static const char* kTopologyNames[] = {
    "Standard",
    "Symmetrical"
};

//==============================================================================
//  Editor
//==============================================================================
BinauralParameterEditor::BinauralParameterEditor(AudioPluginAudioProcessor& p)
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

    setupSideLabel(inputGainLabel,     "Input gain");
    setupSideLabel(sourceAzimuthLabel, "Source azimuth");

    sourceAzimuthHint.setText("or drag the blue dot above", juce::dontSendNotification);
    sourceAzimuthHint.setJustificationType(juce::Justification::centredLeft);
    sourceAzimuthHint.setColour(juce::Label::textColourId, juce::Colour(colHint));
    sourceAzimuthHint.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::italic)));
    sourceAzimuthHint.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(sourceAzimuthHint);

    for (int i = 0; i < 6; ++i)
    {
        auto& chip = layoutChips[(size_t) i];
        chip.setButtonText(kLayoutNames[i]);
        chip.setClickingTogglesState(false);
        chip.onClick = [this, i] { onLayoutChipClicked(i); };
        addAndMakeVisible(chip);
    }

    for (int i = 0; i < 2; ++i)
    {
        auto& chip = topologyChips[(size_t) i];
        chip.setButtonText(kTopologyNames[i]);
        chip.setClickingTogglesState(false);
        chip.onClick = [this, i] { onTopologyChipClicked(i); };
        addAndMakeVisible(chip);
    }

    loadAudioBtn.onClick = [this] { onLoadAudio(); };
    playStopBtn.onClick  = [this] { onPlayStopToggle(); };
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

    processorRef.parameters.addParameterListener("sourceAzimuth", this);
    processorRef.parameters.addParameterListener("layoutMode",   this);
    processorRef.parameters.addParameterListener("topology",     this);

    refreshLayoutChips();
    refreshTopologyChips();

    setResizable(false, false);
    setSize(kEditorW, kEditorH);

    startTimerHz(15);
}

BinauralParameterEditor::~BinauralParameterEditor()
{
    processorRef.parameters.removeParameterListener("sourceAzimuth", this);
    processorRef.parameters.removeParameterListener("layoutMode",   this);
    processorRef.parameters.removeParameterListener("topology",     this);

    setLookAndFeel(nullptr);
}

void BinauralParameterEditor::setupSideLabel(juce::Label& l, const juce::String& text)
{
    l.setText(text, juce::dontSendNotification);
    l.setJustificationType(juce::Justification::centredLeft);
    l.setColour(juce::Label::textColourId, juce::Colour(colMuted));
    l.setFont(juce::Font(juce::FontOptions(13.0f)));
    l.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(l);
}

void BinauralParameterEditor::styleAccentButton(juce::TextButton& btn)
{
    btn.setColour(juce::TextButton::buttonColourId,  juce::Colour(colAccent));
    btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    btn.setColour(juce::TextButton::textColourOnId,  juce::Colours::white);
}

void BinauralParameterEditor::styleChipButton(juce::TextButton& btn, bool selected, bool enabled)
{
    if (selected && enabled)
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
    btn.setEnabled(enabled);
    btn.setAlpha(enabled ? 1.0f : 0.4f);
}

void BinauralParameterEditor::stylePlayStopButton(bool playingNow)
{
    if (playingNow)
    {
        playStopBtn.setButtonText(juce::String::fromUTF8("\xe2\x96\xa0  Stop"));
        playStopBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(colRed));
    }
    else
    {
        playStopBtn.setButtonText(juce::String::fromUTF8("\xe2\x96\xb6  Play"));
        playStopBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(colGreen));
    }
    playStopBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    playStopBtn.setColour(juce::TextButton::textColourOnId,  juce::Colours::white);
}

int BinauralParameterEditor::getLayoutMode() const
{
    if (auto* p = processorRef.parameters.getRawParameterValue("layoutMode"))
        return juce::jlimit(0, 5, (int) std::round(p->load()));
    return 0;
}

int BinauralParameterEditor::getTopology() const
{
    if (auto* p = processorRef.parameters.getRawParameterValue("topology"))
        return juce::jlimit(0, 1, (int) std::round(p->load()));
    return 0;
}

bool BinauralParameterEditor::topologyValidForLayout(int layoutMode, int topology) const
{
    // Direct HRTF: topology has no effect.
    if (layoutMode == 0) return false;
    // VBAP 5/7/9: both topologies valid.
    if (layoutMode >= 1 && layoutMode <= 3) return true;
    // VBAP 12/18: only Symmetrical (topology == 1) is valid.
    return topology == 1;
}

void BinauralParameterEditor::onLayoutChipClicked(int layoutMode)
{
    if (auto* param = processorRef.parameters.getParameter("layoutMode"))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost(param->convertTo0to1((float) layoutMode));
        param->endChangeGesture();
    }

    // Auto-correct topology if the new layout doesn't support the current one.
    const int curTopology = getTopology();
    if (!topologyValidForLayout(layoutMode, curTopology) && layoutMode >= 4)
    {
        // VBAP 12/18 → force Symmetrical
        if (auto* tp = processorRef.parameters.getParameter("topology"))
        {
            tp->beginChangeGesture();
            tp->setValueNotifyingHost(tp->convertTo0to1(1.0f));
            tp->endChangeGesture();
        }
    }
}

void BinauralParameterEditor::onTopologyChipClicked(int topology)
{
    if (auto* param = processorRef.parameters.getParameter("topology"))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost(param->convertTo0to1((float) topology));
        param->endChangeGesture();
    }
}

void BinauralParameterEditor::refreshLayoutChips()
{
    const int current = getLayoutMode();
    for (int i = 0; i < 6; ++i)
        styleChipButton(layoutChips[(size_t) i], i == current, true);
}

void BinauralParameterEditor::refreshTopologyChips()
{
    const int layoutMode = getLayoutMode();
    const int current = getTopology();

    for (int i = 0; i < 2; ++i)
    {
        const bool enabled = topologyValidForLayout(layoutMode, i);
        styleChipButton(topologyChips[(size_t) i], i == current && enabled, enabled);
    }
}

void BinauralParameterEditor::resized()
{
    const int W = getWidth();
    int y = kPad + 50;

    const int circleX = (W - kCircleSize) / 2;
    circleArea = juce::Rectangle<float>((float) circleX, (float) y,
                                        (float) kCircleSize, (float) kCircleSize);
    circleCenter = circleArea.getCentre();
    circleR = (float) kCircleSize * 0.5f - 26.0f;
    y += kCircleSize + 12;

    y += 22; // AUDIO SOURCE header reserve

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

    y += 22; // LAYOUT MODE header reserve

    // Layout chips: 2 rows × 3 columns
    const int chipCols = 3;
    const int chipGap = 8;
    const int chipW = (W - 2 * kPad - (chipCols - 1) * chipGap) / chipCols;
    const int chipH = 30;

    for (int i = 0; i < 6; ++i)
    {
        const int col = i % chipCols;
        const int row = i / chipCols;
        const int x = kPad + col * (chipW + chipGap);
        const int chipY = y + row * (chipH + 6);
        layoutChips[(size_t) i].setBounds(x, chipY, chipW, chipH);
    }
    y += 2 * chipH + 6 + 14;

    y += 22; // TOPOLOGY header reserve

    // Topology chips: 1 row × 2, smaller width
    const int topoChipW = 140;
    for (int i = 0; i < 2; ++i)
    {
        const int x = kPad + i * (topoChipW + chipGap);
        topologyChips[(size_t) i].setBounds(x, y, topoChipW, chipH);
    }
    y += chipH + 22;

    const int btnH = 32;
    loadAudioBtn.setBounds(kPad, y, 180, btnH);
    playStopBtn.setBounds(kPad + 188, y, 100, btnH);
    audioFileLabel.setBounds(kPad + 298, y, W - kPad - 298, btnH);
}

void BinauralParameterEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(colBg));

    g.setColour(juce::Colour(colText));
    g.setFont(juce::Font(juce::FontOptions(20.0f, juce::Font::bold)));
    g.drawText("Binaural", kPad, 12, getWidth() - 2 * kPad, 28, juce::Justification::centredLeft);

    g.setColour(juce::Colour(colHint));
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    const juce::String dot = juce::String::fromUTF8("\xc2\xb7");
    g.drawText("Drag the blue dot to position source.  " + currentPresetName(),
               kPad, 42, getWidth() - 2 * kPad, 16, juce::Justification::centredLeft);

    paintCircle(g);

    paintSectionHeader(g, kPad, inputGainLabel.getY() - 24,
                       getWidth() - 2 * kPad, "Audio source");
    paintSectionHeader(g, kPad, layoutChips[0].getY() - 24,
                       getWidth() - 2 * kPad, "Layout mode");
    paintSectionHeader(g, kPad, topologyChips[0].getY() - 24,
                       getWidth() - 2 * kPad, "Topology");
}

void BinauralParameterEditor::paintSectionHeader(juce::Graphics& g, int x, int y, int w,
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

juce::String BinauralParameterEditor::currentPresetName() const
{
    const int layoutMode = getLayoutMode();
    const int topology = getTopology();

    juce::String name = kLayoutNames[layoutMode];
    if (layoutMode >= 1 && layoutMode <= 3)
        name << "  " << juce::String::fromUTF8("\xc2\xb7") << "  " << kTopologyNames[topology];
    else if (layoutMode >= 4)
        name << "  " << juce::String::fromUTF8("\xc2\xb7") << "  " << kTopologyNames[1];
    return name;
}

void BinauralParameterEditor::paintCircle(juce::Graphics& g)
{
    const float cx = circleCenter.x;
    const float cy = circleCenter.y;
    const float r  = circleR;

    g.setColour(juce::Colour(colBorder));
    g.drawEllipse(cx - r, cy - r, r * 2, r * 2, 1.5f);

    g.setColour(juce::Colour(colSurface2));
    g.drawEllipse(cx - r * 0.5f, cy - r * 0.5f, r, r, 1.0f);

    g.setColour(juce::Colour(colHint));
    g.fillEllipse(cx - 3, cy - 3, 6, 6);

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

    // Speakers from current (layoutMode, topology) — read-only.
    const int layoutMode = getLayoutMode();
    const int topology   = getTopology();
    if (layoutMode > 0) // Direct HRTF has no speakers
    {
        std::array<float, vbap::kMaxSpeakers> spk{};
        int spkCount = 0;
        processorRef.fillSpeakerAnglesForLayout(layoutMode, topology, spk, spkCount);

        // Smaller dots when there are many speakers
        const float solidR = (spkCount > 9) ? 5.0f : 7.0f;
        const float glowR  = (spkCount > 9) ? 9.0f : 12.0f;

        for (int i = 0; i < spkCount; ++i)
        {
            const auto pt = angleToPt(spk[(size_t) i], r, cx, cy);
            g.setColour(juce::Colour(colGreen).withAlpha(0.18f));
            g.fillEllipse(pt.x - glowR, pt.y - glowR, glowR * 2, glowR * 2);
            g.setColour(juce::Colour(colGreen));
            g.fillEllipse(pt.x - solidR, pt.y - solidR, solidR * 2, solidR * 2);
        }
    }

    // Source dot — draggable.
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

juce::Point<float> BinauralParameterEditor::angleToPt(float deg, float r, float cx, float cy)
{
    const float rad = (deg - 90.0f) * juce::MathConstants<float>::pi / 180.0f;
    return { cx + r * std::cos(rad), cy + r * std::sin(rad) };
}

bool BinauralParameterEditor::hitTestSource(const juce::MouseEvent& e) const
{
    const float cx = circleCenter.x;
    const float cy = circleCenter.y;

    const float dxc = e.position.x - cx;
    const float dyc = e.position.y - cy;
    const float distC = std::sqrt(dxc * dxc + dyc * dyc);

    return distC <= circleR + 30.0f;
}

void BinauralParameterEditor::mouseDown(const juce::MouseEvent& e)
{
    if (!hitTestSource(e))
        return;

    draggingSource = true;
    if (auto* param = processorRef.parameters.getParameter("sourceAzimuth"))
        param->beginChangeGesture();
    updateSourceAngleFromMouse(e);
    repaint();
}

void BinauralParameterEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (draggingSource)
    {
        updateSourceAngleFromMouse(e);
        repaint();
    }
}

void BinauralParameterEditor::mouseUp(const juce::MouseEvent&)
{
    if (draggingSource)
    {
        if (auto* param = processorRef.parameters.getParameter("sourceAzimuth"))
            param->endChangeGesture();
    }
    draggingSource = false;
}

void BinauralParameterEditor::updateSourceAngleFromMouse(const juce::MouseEvent& e)
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

    if (auto* param = processorRef.parameters.getParameter("sourceAzimuth"))
        param->setValueNotifyingHost(param->convertTo0to1(deg));
}

void BinauralParameterEditor::parameterChanged(const juce::String& id, float)
{
    juce::MessageManager::callAsync(
        [safe = juce::Component::SafePointer<BinauralParameterEditor>(this), id]()
        {
            if (safe == nullptr) return;

            if (id == "layoutMode")
            {
                safe->refreshLayoutChips();
                safe->refreshTopologyChips();
            }
            else if (id == "topology")
            {
                safe->refreshTopologyChips();
            }
            safe->repaint();
        });
}

void BinauralParameterEditor::onLoadAudio()
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

void BinauralParameterEditor::onPlayStopToggle()
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

void BinauralParameterEditor::timerCallback()
{
    const bool nowPlaying = processorRef.audioPlayer.isPlaying();
    if (nowPlaying != wasPlayingLastTick)
    {
        stylePlayStopButton(nowPlaying);
        wasPlayingLastTick = nowPlaying;
    }
}
