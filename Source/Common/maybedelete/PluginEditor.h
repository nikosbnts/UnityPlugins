#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../BinauralPlugin/PluginProcessor.h"
#include "TestSessionEditor.h"

class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void rebuildInnerEditor();

    AudioPluginAudioProcessor& processorRef;

    juce::Label modeLabel;
    juce::ComboBox modeBox;

    std::unique_ptr<juce::Component> innerEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};