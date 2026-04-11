#include "PluginEditor.h"

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    modeLabel.setText("Editor Mode", juce::dontSendNotification);
    modeLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(modeLabel);

    modeBox.addItem("Default Parameters", 1);
    modeBox.addItem("Test Session", 2);
    modeBox.onChange = [this] { rebuildInnerEditor(); };
    addAndMakeVisible(modeBox);

    modeBox.setSelectedId(1);

    setSize(760, 760);
    rebuildInnerEditor();
}

void AudioPluginAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);

    auto top = area.removeFromTop(32);
    modeLabel.setBounds(top.removeFromLeft(110));
    modeBox.setBounds(top.removeFromLeft(240));

    area.removeFromTop(10);

    if (innerEditor != nullptr)
        innerEditor->setBounds(area);
}

void AudioPluginAudioProcessorEditor::rebuildInnerEditor()
{
    if (innerEditor != nullptr)
        removeChildComponent(innerEditor.get());

    innerEditor.reset();

    if (modeBox.getSelectedId() == 1)
    {
        innerEditor = std::make_unique<juce::GenericAudioProcessorEditor>(processorRef);
    }
    else
    {
        innerEditor = std::make_unique<TestSessionEditor>(processorRef);
    }

    addAndMakeVisible(innerEditor.get());

    resized();
}