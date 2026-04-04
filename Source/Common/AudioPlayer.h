#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

/**
 *  Simple audio-file player that lives inside the processor.
 *  Call loadFile() from the GUI thread.
 *  Call getNextSample() from processBlock to pull mono samples.
 *  Thread-safe via atomics for play/stop/loop control.
 */
class AudioFilePlayer
{
public:
    AudioFilePlayer()
    {
        formatManager.registerBasicFormats();  // wav, aiff, mp3, etc.
    }

    bool loadFile(const juce::File& file)
    {
        std::unique_ptr<juce::AudioFormatReader> reader(
            formatManager.createReaderFor(file));

        if (reader == nullptr)
            return false;

        const int numSamples = static_cast<int>(reader->lengthInSamples);
        const int numChannels = static_cast<int>(reader->numChannels);

        juce::AudioBuffer<float> temp(numChannels, numSamples);
        reader->read(&temp, 0, numSamples, 0, true, true);

        // Downmix to mono
        juce::AudioBuffer<float> mono(1, numSamples);
        mono.copyFrom(0, 0, temp, 0, 0, numSamples);
        if (numChannels > 1)
        {
            mono.addFrom(0, 0, temp, 1, 0, numSamples);
            mono.applyGain(0.5f);
        }

        // Swap in the new buffer
        {
            const juce::SpinLock::ScopedLockType lock(bufferLock);
            fileBuffer = std::move(mono);
            fileSampleRate = reader->sampleRate;
        }

        readPosition.store(0);
        loaded.store(true);
        playing.store(false);
        fileName = file.getFileName();
        return true;
    }

    void play()   { if (loaded.load()) { readPosition.store(0); playing.store(true);  } }
    void stop()   { playing.store(false); }
    void pause()  { playing.store(false); }
    bool isPlaying() const { return playing.load(); }
    bool isLoaded() const  { return loaded.load(); }

    void setLooping(bool l) { looping.store(l); }
    bool isLooping() const  { return looping.load(); }

    juce::String getFileName() const { return fileName; }

    /** Pull one mono sample.  Called from audio thread. */
    float getNextSample()
    {
        if (!playing.load() || !loaded.load())
            return 0.0f;

        const juce::SpinLock::ScopedTryLockType lock(bufferLock);
        if (!lock.isLocked())
            return 0.0f;

        const int len = fileBuffer.getNumSamples();
        if (len == 0) return 0.0f;

        int pos = readPosition.load();
        if (pos >= len)
        {
            if (looping.load())
                pos = 0;
            else
            {
                playing.store(false);
                return 0.0f;
            }
        }

        float sample = fileBuffer.getSample(0, pos);
        readPosition.store(pos + 1);
        return sample;
    }

    double getFileSampleRate() const { return fileSampleRate; }

private:
    juce::AudioFormatManager formatManager;
    juce::AudioBuffer<float> fileBuffer;
    juce::SpinLock bufferLock;
    double fileSampleRate = 44100.0;

    std::atomic<int> readPosition { 0 };
    std::atomic<bool> loaded { false };
    std::atomic<bool> playing { false };
    std::atomic<bool> looping { true };

    juce::String fileName;
};
