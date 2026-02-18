#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cstring> 

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor& p)
    : AudioProcessorEditor(&p),
    processorRef(p),
    frequencySliderAttachment(processorRef.parameters, "frequency", frequencySlider),
    volumeSliderAttachment(processorRef.parameters, "volume", volumeSlider)
{

    juce::ignoreUnused (processorRef);

    juce::MemoryInputStream imageStream(BinaryData::tap_logo_png, BinaryData::tap_logo_pngSize, false);
   

    frequencySlider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    frequencySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 24);
    addAndMakeVisible (frequencySlider);

    frequencyLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible (frequencyLabel);
    volumeSlider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    volumeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 24);
    volumeSlider.textFromValueFunction = [](double v)
        {
            return juce::String(juce::roundToInt(v * 100.0)) + "%";
        };

    volumeSlider.valueFromTextFunction = [](const juce::String& text)
        {
            auto t = text.upToFirstOccurrenceOf("%", false, false).trim();
            return t.getDoubleValue() / 100.0;
        };
    volumeSlider.updateText();
    addAndMakeVisible(volumeSlider);

    volumeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(volumeLabel);
    scopeDisplayBuffer.resize((size_t)scopeNumSamples, 0.0f);
    scopePullBuffer.resize((size_t)scopeNumSamples, 0.0f);
    startTimerHz(20);
    setSize (500, 500);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (juce::Colours::black);
    if (!scopeBounds.isEmpty())
    {
        g.setColour(juce::Colours::darkgrey);
        g.drawRect(scopeBounds);

        // center line
        g.drawLine((float)scopeBounds.getX(),
            (float)scopeBounds.getCentreY(),
            (float)scopeBounds.getRight(),
            (float)scopeBounds.getCentreY(),
            1.0f);

        g.setColour(juce::Colours::white);
        g.strokePath(scopePath, juce::PathStrokeType(3.0f));
    }

}

void AudioPluginAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    auto main = bounds;
    int y = 180;
    // Frequency (left)
    frequencyLabel.setBounds(main.getCentreX() - 150, main.getCentreY() - y, 100, 20);
    frequencySlider.setBounds(main.getCentreX() - 200, main.getCentreY() - y + 20, 200, 200);

    // Volume (right)
    volumeLabel.setBounds (main.getCentreX() + 50, main.getCentreY() - y, 100, 20);
    volumeSlider.setBounds(main.getCentreX() + 0, main.getCentreY() - y + 20, 200, 200);
    

    // Bottom area reserved for the oscilloscope
    auto scopeArea = bounds.removeFromBottom(160);
    scopeBounds = scopeArea.reduced(3);

    // Everything else (slider + label)


}

void AudioPluginAudioProcessorEditor::timerCallback()
{
    const int pulled = processorRef.popScopeSamples(scopePullBuffer.data(), scopeNumSamples);
    if (pulled <= 0)
        return;

    // Keep a rolling window of the last scopeNumSamples samples.
    if (pulled >= scopeNumSamples)
    {
        const int offset = pulled - scopeNumSamples;
        std::memcpy(scopeDisplayBuffer.data(), scopePullBuffer.data() + offset,
            (size_t)scopeNumSamples * sizeof(float));
    }
    else
    {
        const int keep = scopeNumSamples - pulled;
        std::memmove(scopeDisplayBuffer.data(), scopeDisplayBuffer.data() + pulled,
            (size_t)keep * sizeof(float));
        std::memcpy(scopeDisplayBuffer.data() + keep, scopePullBuffer.data(),
            (size_t)pulled * sizeof(float));
    }

    if (scopeBounds.isEmpty())
        return;

    auto area = scopeBounds.toFloat();
    const float left = area.getX();
    const float w = area.getWidth();
    const float midY = area.getCentreY();
    const float halfH = area.getHeight() * 0.4f;

    // IMPORTANT: your sine amplitude defaults to 0.02, which is tiny.
    // This gain is only for the visual so you can actually see it.
    const float visualGain = 1.0f;

    scopePath.clear();
    for (int i = 0; i < scopeNumSamples; ++i)
    {
        const float x = left + w * (float)i / (float)(scopeNumSamples - 1);
        const float s = juce::jlimit(-1.0f, 1.0f, scopeDisplayBuffer[(size_t)i] * visualGain);
        const float y = midY - s * halfH;

        if (i == 0) scopePath.startNewSubPath(x, y);
        else        scopePath.lineTo(x, y);
    }

    repaint(scopeBounds);
}