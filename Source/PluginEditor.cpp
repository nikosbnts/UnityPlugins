#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cstring> 

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor& p)
    : AudioProcessorEditor(&p),
    processorRef(p),
    volumeLAttachment(processorRef.parameters, "volumeL", volumeLSlider),
    volumeRAttachment(processorRef.parameters, "volumeR", volumeRSlider),
    azimuthAttachment(processorRef.parameters, "azimuth", azimuthSlider)
{
    auto setupVolSlider = [](juce::Slider& s)
        {
            s.setSliderStyle(juce::Slider::LinearVertical);
            s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
            s.setRange(0.0, 1.0, 0.0001);

            s.textFromValueFunction = [](double v)
                {
                    return juce::String(juce::roundToInt(v * 100.0)) + "%";
                };
            s.valueFromTextFunction = [](const juce::String& text)
                {
                    auto t = text.upToFirstOccurrenceOf("%", false, false).trim();
                    return t.getDoubleValue() / 100.0;
                };
        };

    azimuthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    azimuthSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 20);
    azimuthSlider.setRange(-90.0, 90.0, 0.01);

    azimuthLabel.setJustificationType(juce::Justification::centred);
    azimuthLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    addAndMakeVisible(azimuthLabel);
    addAndMakeVisible(azimuthSlider);

    setupVolSlider(volumeLSlider);
    setupVolSlider(volumeRSlider);

    addAndMakeVisible(volumeLSlider);
    addAndMakeVisible(volumeRSlider);

    // Labels: give them text + make them visible on black background
    volumeLLabel.setText("Left", juce::dontSendNotification);
    volumeRLabel.setText("Right", juce::dontSendNotification);

    volumeLLabel.setJustificationType(juce::Justification::centred);
    volumeRLabel.setJustificationType(juce::Justification::centred);

    volumeLLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    volumeRLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    addAndMakeVisible(volumeLLabel);
    addAndMakeVisible(volumeRLabel);

    // THIS is what fixes the “tiny window”
    setSize(500, 300);
}


AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::darkgrey);
    g.drawRect(getLocalBounds(), 1);
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);

    auto top = area.removeFromTop(80);
    azimuthLabel.setBounds(top.removeFromTop(20));
    azimuthSlider.setBounds(top.reduced(10, 0));

    area.removeFromTop(10);

    auto leftArea = area.removeFromLeft(area.getWidth() / 2);
    auto rightArea = area;

    volumeLLabel.setBounds(leftArea.removeFromTop(20));
    volumeLSlider.setBounds(leftArea.reduced(20, 0));

    volumeRLabel.setBounds(rightArea.removeFromTop(20));
    volumeRSlider.setBounds(rightArea.reduced(20, 0));
}