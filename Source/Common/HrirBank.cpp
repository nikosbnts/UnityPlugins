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

    for (int az = 0; az < 360; az += 10)
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

    return !entries.empty();
}

bool HrirBank::isLoaded() const noexcept
{
    return !entries.empty();
}

const HrirBank::Entry* HrirBank::getNearest(float azimuthDeg) const noexcept
{
    if (entries.empty())
        return nullptr;

    const int wanted = wrapAzimuthToNearest10(azimuthDeg);

    for (const auto& entry : entries)
    {
        if (entry.azimuthDeg == wanted)
            return &entry;
    }

    return &entries.front();
}

int HrirBank::wrapAzimuthToNearest10(float azimuthDeg) noexcept
{
    float wrapped = std::fmod(azimuthDeg, 360.0f);
    if (wrapped < 0.0f)
        wrapped += 360.0f;

    int rounded = static_cast<int>(std::round(wrapped / 10.0f)) * 10;
    if (rounded == 360)
        rounded = 0;

    return rounded;
}

juce::String HrirBank::makeExpectedFileName(int azimuthDeg)
{
    return "azi_" + juce::String(azimuthDeg) + ",0_ele_0,0.wav";
}