#pragma once

#include <array>
#include <algorithm>
#include <cmath>

namespace vbap
{
    static constexpr int kMaxSpeakers = 36;

    inline float wrap360(float deg) noexcept
    {
        float x = std::fmod(deg, 360.0f);
        if (x < 0.0f)
            x += 360.0f;
        return x;
    }

    inline float wrap180(float deg) noexcept
    {
        float x = wrap360(deg);
        if (x > 180.0f)
            x -= 360.0f;
        return x;
    }

    struct Vec2
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    inline Vec2 azToDir(float azDeg360) noexcept
    {
        constexpr float degToRad = 3.14159265358979323846f / 180.0f;
        const float r = azDeg360 * degToRad;
        return { std::sin(r), std::cos(r) };
    }

    inline bool solvePair(float srcAz, float spkA, float spkB, float& gA, float& gB) noexcept
    {
        const Vec2 s = azToDir(srcAz);
        const Vec2 a = azToDir(spkA);
        const Vec2 b = azToDir(spkB);

        const float det = a.x * b.y - a.y * b.x;
        if (std::abs(det) < 1.0e-8f)
        {
            gA = 0.0f;
            gB = 0.0f;
            return false;
        }

        gA = (s.x * b.y - s.y * b.x) / det;
        gB = (-s.x * a.y + s.y * a.x) / det;
        return true;
    }

    inline void normalize2(float& a, float& b) noexcept
    {
        const float n = std::sqrt(a * a + b * b);
        const float safe = (n > 1.0e-12f) ? n : 1.0f;
        a /= safe;
        b /= safe;
    }

    inline void defaultLayoutAngles(int n, float* outAngles) noexcept
    {
        if (outAngles == nullptr || n < 2)
            return;

        switch (n)
        {
        case 2:
            outAngles[0] = 330.0f; outAngles[1] = 30.0f;
            return;
        case 3:
            outAngles[0] = 330.0f; outAngles[1] = 0.0f; outAngles[2] = 30.0f;
            return;
        case 4:
            outAngles[0] = 330.0f; outAngles[1] = 30.0f; outAngles[2] = 250.0f; outAngles[3] = 110.0f;
            return;
        case 5:
            outAngles[0] = 30.0f; outAngles[1] = 330.0f; outAngles[2] = 110.0f; outAngles[3] = 250.0f; outAngles[4] = 0.0f;
            return;
        case 6:
            outAngles[0] = 30.0f; outAngles[1] = 330.0f; outAngles[2] = 0.0f; outAngles[3] = 110.0f; outAngles[4] = 250.0f; outAngles[5] = 180.0f;
            return;
        case 7:
            outAngles[0] = 30.0f; outAngles[1] = 330.0f; outAngles[2] = 0.0f; outAngles[3] = 110.0f; outAngles[4] = 250.0f; outAngles[5] = 210.0f; outAngles[6] = 150.0f;
            return;
        case 8:
            outAngles[0] = 30.0f; outAngles[1] = 330.0f; outAngles[2] = 0.0f; outAngles[3] = 110.0f;
            outAngles[4] = 250.0f; outAngles[5] = 210.0f; outAngles[6] = 150.0f; outAngles[7] = 180.0f;
            return;
        default:
            break;
        }

        const float step = 360.0f / static_cast<float>(n);
        for (int i = 0; i < n; ++i)
            outAngles[i] = wrap360(step * static_cast<float>(i));
    }

    inline void computeVBAP_N(float sourceAzDeg360,
        const float* speakerAzDeg360,
        int n,
        float* outGains) noexcept
    {
        if (speakerAzDeg360 == nullptr || outGains == nullptr || n < 2)
            return;

        for (int i = 0; i < n; ++i)
            outGains[i] = 0.0f;

        std::array<int, kMaxSpeakers> idx{};
        std::array<float, kMaxSpeakers> sortedAz{};

        for (int i = 0; i < n; ++i)
        {
            idx[i] = i;
            sortedAz[i] = wrap360(speakerAzDeg360[i]);
        }

        std::sort(idx.begin(), idx.begin() + n,
            [&](int a, int b) { return sortedAz[a] < sortedAz[b]; });

        std::array<float, kMaxSpeakers> orderedAz{};
        for (int i = 0; i < n; ++i)
            orderedAz[i] = sortedAz[idx[i]];

        const float src = wrap360(sourceAzDeg360);

        for (int i = 0; i < n; ++i)
        {
            const int j = (i + 1) % n;

            float g1 = 0.0f, g2 = 0.0f;
            if (!solvePair(src, orderedAz[i], orderedAz[j], g1, g2))
                continue;

            constexpr float eps = -1.0e-5f;
            if (g1 >= eps && g2 >= eps)
            {
                if (g1 < 0.0f) g1 = 0.0f;
                if (g2 < 0.0f) g2 = 0.0f;

                normalize2(g1, g2);

                outGains[idx[i]] = g1;
                outGains[idx[j]] = g2;
                return;
            }
        }

        int nearest = 0;
        float best = 999999.0f;

        for (int i = 0; i < n; ++i)
        {
            const float d = std::abs(wrap180(src - wrap360(speakerAzDeg360[i])));
            if (d < best)
            {
                best = d;
                nearest = i;
            }
        }

        outGains[nearest] = 1.0f;
    }
}