#include "PluginEditor.h"

namespace
{
    juce::String speakerAzParamId(int index1Based)
    {
        return "speakerAz" + juce::String(index1Based);
    }
}

void AudioPluginAudioProcessorEditor::setupPercentSlider(juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 20);
    slider.setRange(0.0, 1.0, 0.0001);
    slider.textFromValueFunction = [](double v)
    {
        return juce::String(juce::roundToInt(v * 100.0)) + "%";
    };
    slider.valueFromTextFunction = [](const juce::String& text)
    {
        auto t = text.upToFirstOccurrenceOf("%", false, false).trim();
        return t.getDoubleValue() / 100.0;
    };
}

void AudioPluginAudioProcessorEditor::setupAngleSlider(juce::Slider& slider, double min, double max)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 20);
    slider.setRange(min, max, 0.01);

    slider.textFromValueFunction = [](double v) -> juce::String
        {
            return juce::String(v, 1) + " deg";
        };

    slider.valueFromTextFunction = [](const juce::String& text) -> double
        {
            auto t = text.upToFirstOccurrenceOf("deg", false, false).trim();
            return t.getDoubleValue();
        };
}

void AudioPluginAudioProcessorEditor::setupCountSlider(juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 20);
    slider.setRange(2.0, static_cast<double>(vbap::kmaxSpeakers), 1.0);
    slider.setNumDecimalPlacesToDisplay(0);
}

void AudioPluginAudioProcessorEditor::setupLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centredLeft);
    label.setColour(juce::Label::textColourId, juce::Colours::white);
}

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p)
{
    titleLabel.setText("VBAP 2D Unity Plugin", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);

    setupLabel(sourceAzimuthLabel, "Source Azimuth");
    setupLabel(stereoWidthLabel, "Stereo Width");
    setupLabel(speakerCountLabel, "Speaker Count");
    setupLabel(volumeLLabel, "Volume 1 (input L / mono)");
    setupLabel(volumeRLabel, "Volume 2 (input R)");

    addAndMakeVisible(sourceAzimuthLabel);
    addAndMakeVisible(stereoWidthLabel);
    addAndMakeVisible(speakerCountLabel);
    addAndMakeVisible(volumeLLabel);
    addAndMakeVisible(volumeRLabel);

    setupAngleSlider(sourceAzimuthSlider, 0.0, 360.0);
    setupAngleSlider(stereoWidthSlider, 0.0, 90.0);
    setupCountSlider(speakerCountSlider);
    setupPercentSlider(volumeLSlider);
    setupPercentSlider(volumeRSlider);

    addAndMakeVisible(sourceAzimuthSlider);
    addAndMakeVisible(stereoWidthSlider);
    addAndMakeVisible(speakerCountSlider);
    addAndMakeVisible(volumeLSlider);
    addAndMakeVisible(volumeRSlider);

    sourceAzimuthAttachment = std::make_unique<SliderAttachment>(processorRef.parameters, "sourceAzimuth", sourceAzimuthSlider);
    stereoWidthAttachment   = std::make_unique<SliderAttachment>(processorRef.parameters, "stereoWidth", stereoWidthSlider);
    speakerCountAttachment  = std::make_unique<SliderAttachment>(processorRef.parameters, "speakerCount", speakerCountSlider);
    volumeLAttachment       = std::make_unique<SliderAttachment>(processorRef.parameters, "volumeL", volumeLSlider);
    volumeRAttachment       = std::make_unique<SliderAttachment>(processorRef.parameters, "volumeR", volumeRSlider);

    for (int i = 0; i < vbap::maxSpeakers; ++i)
    {
        setupLabel(speakerAzLabels[i], "Speaker " + juce::String(i + 1) + " Azimuth");
        setupAngleSlider(speakerAzSliders[i], 0.0, 360.0);

        addAndMakeVisible(speakerAzLabels[i]);
        addAndMakeVisible(speakerAzSliders[i]);

        speakerAzAttachments[i] = std::make_unique<SliderAttachment>(processorRef.parameters,
                                                                     speakerAzParamId(i + 1),
                                                                     speakerAzSliders[i]);
    }

    setSize(820, 620);
}

void AudioPluginAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff111111));
    g.setColour(juce::Colours::darkgrey);
    g.drawRect(getLocalBounds(), 1);
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(12);

    titleLabel.setBounds(area.removeFromTop(28));
    area.removeFromTop(8);

    auto placeRow = [](juce::Rectangle<int> row, juce::Label& label, juce::Slider& slider)
    {
        auto left = row.removeFromLeft(220);
        label.setBounds(left);
        slider.setBounds(row);
    };

    placeRow(area.removeFromTop(32), sourceAzimuthLabel, sourceAzimuthSlider);
    area.removeFromTop(4);
    placeRow(area.removeFromTop(32), stereoWidthLabel, stereoWidthSlider);
    area.removeFromTop(4);
    placeRow(area.removeFromTop(32), speakerCountLabel, speakerCountSlider);
    area.removeFromTop(10);
    placeRow(area.removeFromTop(32), volumeLLabel, volumeLSlider);
    area.removeFromTop(4);
    placeRow(area.removeFromTop(32), volumeRLabel, volumeRSlider);
    area.removeFromTop(12);

    const int rowHeight = 30;
    const int gap = 4;
    const int columnGap = 14;
    auto leftColumn = area.removeFromLeft((area.getWidth() - columnGap) / 2);
    area.removeFromLeft(columnGap);
    auto rightColumn = area;

    for (int i = 0; i < 4; ++i)
    {
        placeRow(leftColumn.removeFromTop(rowHeight), speakerAzLabels[i], speakerAzSliders[i]);
        leftColumn.removeFromTop(gap);
    }

    for (int i = 4; i < 8; ++i)
    {
        placeRow(rightColumn.removeFromTop(rowHeight), speakerAzLabels[i], speakerAzSliders[i]);
        rightColumn.removeFromTop(gap);
    }
}
