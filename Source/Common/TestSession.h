#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <cmath>

struct TrialResult
{
    int trialNumber = 0;
    float targetAngle = 0.0f;
    float responseAngle = 0.0f;
    float angularError = 0.0f;
    int confidence = 0;
    juce::String timestamp;
};

struct TestSession
{
    enum class Screen { Setup, Trial, Feedback, Summary };

    Screen screen = Screen::Setup;
    juce::String sessionName = "Session 1";
    int layoutMode = 3;                // 0=9spk, 1=12spk, 2=18spk, 3=36spk
    std::vector<float> targetAngles;
    std::vector<TrialResult> results;
    int currentTrial = 0;

    float userResponse = -1.0f;        // -1 = not set
    int userConfidence = 0;            // 0 = not set

    juce::String audioFilePath;
    bool useInternalAudio = false;

    // ── helpers ──────────────────────────────────────────────
    static float angularError(float a, float b)
    {
        float e = std::fmod(std::abs(a - b), 360.0f);
        return e > 180.0f ? 360.0f - e : e;
    }

    void reset()
    {
        screen = Screen::Setup;
        results.clear();
        currentTrial = 0;
        userResponse = -1.0f;
        userConfidence = 0;
    }

    bool canSubmit() const
    {
        return userResponse >= 0.0f && userConfidence > 0;
    }

    void submitCurrentTrial()
    {
        if (currentTrial >= static_cast<int>(targetAngles.size())) return;

        TrialResult r;
        r.trialNumber = currentTrial + 1;
        r.targetAngle = targetAngles[static_cast<size_t>(currentTrial)];
        r.responseAngle = userResponse;
        r.angularError = angularError(r.targetAngle, r.responseAngle);
        r.confidence = userConfidence;
        r.timestamp = juce::Time::getCurrentTime().toISO8601(true);
        results.push_back(r);

        ++currentTrial;
        screen = Screen::Feedback;
    }

    void advanceAfterFeedback()
    {
        userResponse = -1.0f;
        userConfidence = 0;
        screen = (currentTrial >= static_cast<int>(targetAngles.size()))
                     ? Screen::Summary
                     : Screen::Trial;
    }

    float meanError() const
    {
        if (results.empty()) return 0.0f;
        float sum = 0.0f;
        for (auto& r : results) sum += r.angularError;
        return sum / static_cast<float>(results.size());
    }

    float bestError() const
    {
        float b = 999.0f;
        for (auto& r : results) b = std::min(b, r.angularError);
        return b;
    }

    float worstError() const
    {
        float w = 0.0f;
        for (auto& r : results) w = std::max(w, r.angularError);
        return w;
    }

    static juce::String escapeForCSV(const juce::String& text, juce::juce_wchar sep = ';')
{
    auto escaped = text;
    escaped = escaped.replace("\"", "\"\"");

    const bool needsQuotes =
        escaped.containsChar(sep) ||
        escaped.containsChar('"') ||
        escaped.containsChar('\n') ||
        escaped.containsChar('\r');

    return needsQuotes ? "\"" + escaped + "\"" : escaped;
}

static juce::String numberForExcel(float value)
{
    // Για καλύτερη συμβατότητα με ελληνικό Excel
    return juce::String(value, 1).replaceCharacter('.', ',');
}

juce::String toCSV() const
{
    constexpr juce::juce_wchar sep = ';';

    juce::String layoutStr;
    switch (layoutMode)
    {
        case 0: layoutStr = "9spk";  break;
        case 1: layoutStr = "12spk"; break;
        case 2: layoutStr = "18spk"; break;
        default: layoutStr = "36spk"; break;
    }

    juce::String csv;

    // ── Session info ─────────────────────────────────────
    csv << "Session Name" << sep << escapeForCSV(sessionName, sep) << "\n";
    csv << "Layout"       << sep << layoutStr << "\n";
    csv << "Audio Source" << sep << (useInternalAudio ? "Internal file" : "DAW input") << "\n";
    csv << "Audio File"   << sep
        << escapeForCSV(audioFilePath.isNotEmpty() ? audioFilePath : "-", sep) << "\n";
    csv << "Total Trials" << sep << juce::String(results.size()) << "\n";
    csv << "\n";

    // ── Results table ───────────────────────────────────
    csv << "Trial" << sep
        << "TargetAngle" << sep
        << "ResponseAngle" << sep
        << "AngularError" << sep
        << "Confidence" << sep
        << "Timestamp" << "\n";

    for (const auto& r : results)
    {
        csv << juce::String(r.trialNumber) << sep
            << numberForExcel(r.targetAngle) << sep
            << numberForExcel(r.responseAngle) << sep
            << numberForExcel(r.angularError) << sep
            << juce::String(r.confidence) << sep
            << escapeForCSV(r.timestamp, sep) << "\n";
    }

    return csv;
}

    static std::vector<float> parseAngles(const juce::String& text)
    {
        std::vector<float> out;
        juce::StringArray tokens;
        tokens.addTokens(text, ",; \t\n", "");
        for (auto& t : tokens)
        {
            float v = t.trim().getFloatValue();
            if (t.trim().isNotEmpty() && v >= 0.0f && v <= 360.0f)
                out.push_back(v);
        }
        return out;
    }

    static std::vector<float> generateRandom(int n)
    {
        std::vector<float> out;
        juce::Random rng;
        for (int i = 0; i < n; ++i)
            out.push_back(static_cast<float>(rng.nextInt(360)));
        return out;
    }

    static std::vector<float> preset8()
    {
        return { 0, 45, 90, 135, 180, 225, 270, 315 };
    }
};


