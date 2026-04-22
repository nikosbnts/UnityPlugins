#include "HrirBank.h"
#include <cmath>

HrirBank::HrirBank()
{
    formatManager.registerBasicFormats();
}

bool HrirBank::loadFromFolder(const juce::File& folder)
{
    entries.clear();
    maxIrLength = 0;
    bankSampleRate = 0.0;

    if (!folder.exists() || !folder.isDirectory())
        return false;

    entries.reserve(static_cast<size_t>(kNumAzimuths));

    for (int az = 0; az < kNumAzimuths; az += kAzimuthStepDeg)
    {
        const auto file = folder.getChildFile(makeExpectedFileName(az));
        if (!file.existsAsFile())
            return false;

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
        if (reader == nullptr)
            return false;

        if (reader->numChannels < 2)
            return false;

        Entry e;
        e.azimuthDeg = az;
        e.sampleRate = reader->sampleRate;
        e.ir.setSize(2, static_cast<int>(reader->lengthInSamples));
        reader->read(&e.ir, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);

        if (bankSampleRate == 0.0)
            bankSampleRate = e.sampleRate;
        else if (std::abs(bankSampleRate - e.sampleRate) > 1.0)
            return false;

        maxIrLength = juce::jmax(maxIrLength, e.ir.getNumSamples());
        entries.push_back(std::move(e));
    }

    return static_cast<int>(entries.size()) == kNumAzimuths;
}

bool HrirBank::isLoaded() const noexcept
{
    return !entries.empty();
}

const HrirBank::Entry* HrirBank::getNearest(float azimuthDeg) const noexcept
{
    if (entries.empty())
        return nullptr;

    const int wanted = wrapAzimuthToNearest1(azimuthDeg);

    // Entries are loaded in order 0..359, so direct indexing works.
    if (wanted >= 0 && wanted < static_cast<int>(entries.size()))
        return &entries[static_cast<size_t>(wanted)];

    return &entries.front();
}

int HrirBank::wrapAzimuthToNearest1(float azimuthDeg) noexcept
{
    float wrapped = std::fmod(azimuthDeg, 360.0f);
    if (wrapped < 0.0f)
        wrapped += 360.0f;

    int rounded = static_cast<int>(std::round(wrapped));
    if (rounded >= 360)
        rounded = 0;
    if (rounded < 0)
        rounded = 0;

    return rounded;
}

juce::String HrirBank::makeExpectedFileName(int azimuthDeg)
{
    return "azi_" + juce::String(azimuthDeg) + ",0_ele_0,0.wav";
}