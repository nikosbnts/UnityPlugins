#pragma once
#include <cmath>
#include <algorithm>

namespace vbap2
{
    inline float wrap360(float deg) noexcept
    {
        float x = std::fmod(deg, 360.0f);
        if (x < 0.0f) x += 360.0f;
        return x;
    }

    inline float degToRad(float deg) noexcept
    {
        return deg * 3.14159265358979323846f / 180.0f;
    }

    // MATLAB vbap2d convention: vec = [sin(az); cos(az)]
    inline void vbap2Speakers(float srcAzDeg360, float ls1AzDeg360, float ls2AzDeg360,
        float& g1, float& g2) noexcept
    {
        const float s1 = std::sin(degToRad(ls1AzDeg360));
        const float c1 = std::cos(degToRad(ls1AzDeg360));
        const float s2 = std::sin(degToRad(ls2AzDeg360));
        const float c2 = std::cos(degToRad(ls2AzDeg360));

        const float sp = std::sin(degToRad(srcAzDeg360));
        const float cp = std::cos(degToRad(srcAzDeg360));

        const float det = (s1 * c2 - s2 * c1);
        if (std::abs(det) < 1.0e-8f)
        {
            g1 = 0.70710678f;
            g2 = 0.70710678f;
            return;
        }

        g1 = (c2 * sp - s2 * cp) / det;
        g2 = (-c1 * sp + s1 * cp) / det;

        // clamp negatives like typical VBAP
        if (g1 < 0.0f) g1 = 0.0f;
        if (g2 < 0.0f) g2 = 0.0f;

        // constant-power normalize
        const float norm = std::sqrt(g1 * g1 + g2 * g2);
        const float safe = (norm > 1.0e-12f) ? norm : 1.0f;
        g1 /= safe;
        g2 /= safe;
    }

    inline bool isEffectivelyMono(const float* L, const float* R, int n) noexcept
    {
        if (!L || !R || n <= 0) return true;

        const int N = std::min(n, 256);
        double diff = 0.0, sum = 0.0;

        for (int i = 0; i < N; ++i)
        {
            const float a = L[i];
            const float b = R[i];
            diff += std::abs(a - b);
            sum += std::abs(a) + std::abs(b);
        }

        if (sum <= 1.0e-12) return true;      // silence
        return ((diff / sum) < 1.0e-6);       // almost identical
    }

    // Your exact behavior:
    // - 2 speakers at (spkAzL, spkAzR)
    // - front stage only: clamp azimuth to [-90, +90]
    // - preserve stereo: L source at center-halfWidth, R source at center+halfWidth
    // - channel volumes: vol1 scales input ch0, vol2 scales input ch1
    // - mono rule: if treatAsMono, vol2 does nothing, single VBAP source at center
    inline void process2SpeakerPreserve(float* outL, float* outR, int n, int inCh,
        float vol1, float vol2, float azimuthDeg,
        float spkAzL, float spkAzR, float halfWidthDeg) noexcept
    {
        if (!outL || !outR || n <= 0) return;

        const float centerAz = std::clamp(azimuthDeg, -90.0f, 90.0f);

        bool treatAsMono = (inCh < 2);
        if (!treatAsMono)
            treatAsMono = isEffectivelyMono(outL, outR, n);

        if (treatAsMono)
        {
            float gML = 0.0f, gMR = 0.0f;
            vbap2Speakers(wrap360(centerAz), spkAzL, spkAzR, gML, gMR);

            for (int i = 0; i < n; ++i)
            {
                const float monoIn = (inCh >= 2) ? 0.5f * (outL[i] + outR[i]) : outL[i];
                const float x = monoIn * vol1; // vol2 does nothing
                outL[i] = x * gML;
                outR[i] = x * gMR;
            }
            return;
        }

        const float srcAzL = centerAz - halfWidthDeg;
        const float srcAzR = centerAz + halfWidthDeg;

        float gLL = 0.0f, gRL = 0.0f;
        float gLR = 0.0f, gRR = 0.0f;

        vbap2Speakers(wrap360(srcAzL), spkAzL, spkAzR, gLL, gRL);
        vbap2Speakers(wrap360(srcAzR), spkAzL, spkAzR, gLR, gRR);

        for (int i = 0; i < n; ++i)
        {
            const float inL = outL[i] * vol1;
            const float inR = outR[i] * vol2;

            outL[i] = (gLL * inL + gLR * inR);
            outR[i] = (gRL * inL + gRR * inR);
        }
    }
} // namespace vbap2