#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <vector>

class HrirBank
{
public:
    struct Entry
    {
        int azimuthDeg = 0;              // 0, 10, 20, ... 350
        juce::AudioBuffer<float> ir;     // ch0 = left ear, ch1 = right ear
        double sampleRate = 0.0;
    };

    HrirBank();

    bool loadFromFolder(const juce::File& folder);
    bool isLoaded() const noexcept;

    const Entry* getNearest(float azimuthDeg) const noexcept;

    int getNumEntries() const noexcept { return static_cast<int>(entries.size()); }
    int getMaxLength() const noexcept { return maxIrLength; }
    double getSampleRate() const noexcept { return bankSampleRate; }

private:
    static int wrapAzimuthToNearest10(float azimuthDeg) noexcept;
    static juce::String makeExpectedFileName(int azimuthDeg);

    juce::AudioFormatManager formatManager;
    std::vector<Entry> entries;
    int maxIrLength = 0;
    double bankSampleRate = 0.0;
};