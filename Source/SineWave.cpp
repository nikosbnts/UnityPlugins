//
//

#include "SineWave.h"

void SineWave::prepare (const double rate, const int numChannels)
{
    sampleRate = static_cast<float>(rate);
    timeIncrement = 1.0f / sampleRate;
    currentTime.resize(numChannels, 0.0f);
}

void SineWave::process (juce::AudioBuffer<float>& buffer)
{
    // You didn't call prepare!
    assert (sampleRate > 0.0f && timeIncrement > 0.0f);
    assert(amplitude >= 0.0f && amplitude <= 1.0f);
    assert(buffer.getNumChannels() == currentTime.size());

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            channelData[sample] = amplitude * std::sin(doublePi * frequency * currentTime[channel]);

            currentTime[channel] += timeIncrement;

            if (currentTime[channel] >= 1.0f)
            {
                currentTime[channel] -= 1.0f;
            }
        }
    }
}