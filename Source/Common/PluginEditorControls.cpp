#include "PluginEditorControls.h"

#include <cmath>

namespace pluginUI
{

//==============================================================================
//  DarkLookAndFeel
//==============================================================================
DarkLookAndFeel::DarkLookAndFeel()
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

juce::Font DarkLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    return juce::Font(juce::FontOptions(juce::jmin(15.0f, buttonHeight * 0.55f), juce::Font::plain));
}

void DarkLookAndFeel::drawButtonBackground(juce::Graphics& g,
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

void DarkLookAndFeel::drawTextEditorOutline(juce::Graphics& g,
    int width, int height, juce::TextEditor& te)
{
    const auto bounds = juce::Rectangle<float>(0, 0, (float) width, (float) height).reduced(0.5f);
    g.setColour(te.hasKeyboardFocus(true)
        ? te.findColour(juce::TextEditor::focusedOutlineColourId)
        : te.findColour(juce::TextEditor::outlineColourId));
    g.drawRoundedRectangle(bounds, 8.0f, 1.0f);
}

void DarkLookAndFeel::fillTextEditorBackground(juce::Graphics& g,
    int width, int height, juce::TextEditor& te)
{
    g.setColour(te.findColour(juce::TextEditor::backgroundColourId));
    g.fillRoundedRectangle(juce::Rectangle<float>(0, 0, (float) width, (float) height), 8.0f);
}

//==============================================================================
//  StepperControl
//==============================================================================
StepperControl::StepperControl(APVTS& apvts, const juce::String& id,
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

StepperControl::~StepperControl()
{
    apvtsRef.removeParameterListener(paramId, this);
}

void StepperControl::resized()
{
    const int btnW = 26;
    const auto r = getLocalBounds();
    decBtn.setBounds(r.getX(), r.getY(), btnW, r.getHeight());
    incBtn.setBounds(r.getRight() - btnW, r.getY(), btnW, r.getHeight());
    valueEditor.setBounds(r.getX() + btnW + 2, r.getY(),
                          r.getWidth() - 2 * (btnW + 2), r.getHeight());
}

void StepperControl::setEnabledState(bool enabled)
{
    decBtn.setEnabled(enabled);
    incBtn.setEnabled(enabled);
    valueEditor.setEnabled(enabled);
    setAlpha(enabled ? 1.0f : 0.35f);
}

void StepperControl::parameterChanged(const juce::String&, float)
{
    juce::MessageManager::callAsync(
        [safe = juce::Component::SafePointer<StepperControl>(this)]()
        {
            if (safe != nullptr)
                safe->refreshFromParameter();
        });
}

void StepperControl::refreshFromParameter()
{
    if (auto* p = apvtsRef.getRawParameterValue(paramId))
        valueEditor.setText(juce::String(juce::roundToInt(p->load())) + juce::String::fromUTF8("\xc2\xb0"),
                            juce::dontSendNotification);
}

float StepperControl::wrapOrClamp(float v) const
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

void StepperControl::stepBy(float delta)
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

void StepperControl::applyTypedValue()
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
SteppedIntSlider::SteppedIntSlider(APVTS& apvts, const juce::String& id,
    int minVal, int maxVal)
    : apvtsRef(apvts), paramId(id), minValue(minVal), maxValue(maxVal)
{
    apvtsRef.addParameterListener(paramId, this);
    refreshFromParameter();
}

SteppedIntSlider::~SteppedIntSlider()
{
    apvtsRef.removeParameterListener(paramId, this);
}

float SteppedIntSlider::xForValue(int v) const
{
    const int n = maxValue - minValue;
    if (n <= 0) return 16.0f;
    const float t = (float) (v - minValue) / (float) n;
    return 16.0f + t * ((float) getWidth() - 32.0f);
}

void SteppedIntSlider::paint(juce::Graphics& g)
{
    const float trackY = 14.0f;

    g.setColour(juce::Colour(colBorder));
    g.fillRoundedRectangle(16.0f, trackY, (float) (getWidth() - 32), 3.0f, 1.5f);

    const float xMin = xForValue(minValue);
    const float xCur = xForValue(currentValue);
    g.setColour(juce::Colour(colAccent));
    g.fillRoundedRectangle(xMin, trackY, xCur - xMin, 3.0f, 1.5f);

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

void SteppedIntSlider::mouseDown(const juce::MouseEvent& e)
{
    setValueFromX(e.position.x);
}

void SteppedIntSlider::mouseDrag(const juce::MouseEvent& e)
{
    setValueFromX(e.position.x);
}

void SteppedIntSlider::parameterChanged(const juce::String&, float)
{
    juce::MessageManager::callAsync(
        [safe = juce::Component::SafePointer<SteppedIntSlider>(this)]()
        {
            if (safe != nullptr)
                safe->refreshFromParameter();
        });
}

void SteppedIntSlider::refreshFromParameter()
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

void SteppedIntSlider::setValueFromX(float xpos)
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

} // namespace pluginUI
