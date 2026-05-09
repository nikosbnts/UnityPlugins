#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <vector>

class HrirBank
{
public:
    struct Entry
    {
        int azimuthDeg = 0;              // 0, 1, 2, ... 359.
        juce::AudioBuffer<float> ir;     // ch0 = left ear, ch1 = right ear
        double sampleRate = 0.0;
    };

    HrirBank();

    bool loadFromFolder(const juce::File& folder);
    bool isLoaded() const noexcept;

    /** Nearest in 1-deg resolution. */
    const Entry* getNearest(float azimuthDeg) const noexcept;

    int getNumEntries() const noexcept { return static_cast<int>(entries.size()); }
    int getMaxLength() const noexcept { return maxIrLength; }
    double getSampleRate() const noexcept { return bankSampleRate; }

    static constexpr int kAzimuthStepDeg = 1;
    static constexpr int kNumAzimuths = 360;

private:
    static int wrapAzimuthToNearest1(float azimuthDeg) noexcept;
    static juce::String makeExpectedFileName(int azimuthDeg);

    juce::AudioFormatManager formatManager;
    std::vector<Entry> entries;
    int maxIrLength = 0;
    double bankSampleRate = 0.0;
};