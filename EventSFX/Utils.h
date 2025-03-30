#pragma once

#include <string>
#include <windows.h>

enum
{
    RlAngle180 = 32767,
    RlAngle90  = 16384, // Rounded down from _.5
};

namespace Utils
{
    namespace RlValues
    {
        constexpr float RL_ANGLE180      = 32767.0f;
        constexpr float RL_ANGLE90       = RL_ANGLE180 / 2.0f;
        constexpr float MAP_WIDTH        = 8192.0f;
        constexpr float MAP_HEIGHT       = 10240.0f;
        constexpr float MAP_ASPECT_RATIO = MAP_WIDTH / MAP_HEIGHT;
    }

    inline std::wstring StringToWString(const std::string& str)
    {
        if (str.empty()) return {};

        const int    count = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        std::wstring wstr(count, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], count);

        wstr.pop_back(); // Remove the null terminator added by MultiByteToWideChar

        return wstr;
    }

    inline std::string WStringToString(const std::wstring& wstr)
    {
        if (wstr.empty()) return {};

        const int sizeNeeded = WideCharToMultiByte(
            CP_UTF8, 0,
            wstr.data(),
            static_cast<int>(wstr.size()),
            nullptr,
            0,
            nullptr,
            nullptr);

        std::string strTo(sizeNeeded, 0);
        WideCharToMultiByte(
            CP_UTF8, 0,
            wstr.data(),
            static_cast<int>(wstr.size()),
            strTo.data(),
            sizeNeeded,
            nullptr,
            nullptr);

        return strTo;
    }

    inline float ComputeVolumeMultiplier(float speed)
    {
        // === Tweakable Parameters ===
        const float minSpeed    = 100.0f;
        const float midStart    = 800.0f;
        const float midEnd      = 2500.0f;
        const float maxSpeed    = 6000.0f;

        const float minVolume   = 0.0f;
        const float midVolume   = 0.25f;
        const float highVolume  = 0.85f;
        const float maxVolume   = 1.2f;

        float volume = 0.0f;

        if (speed < minSpeed) return 0.0f;

        if (speed <= midStart) {
            // Ease-in: from minVolume to midVolume
            float t = (speed - minSpeed) / (midStart - minSpeed);
            float eased = std::pow(t, 2.0f); // ease-in quad
            volume = minVolume + eased * (midVolume - minVolume);
        }
        else if (speed <= midEnd) {
            // Linear region: from midVolume to highVolume
            float t = (speed - midStart) / (midEnd - midStart);
            volume = midVolume + t * (highVolume - midVolume);
        }
        else {
            // Ease-out: from highVolume to maxVolume
            float t = (speed - midEnd) / (maxSpeed - midEnd);
            float eased = 1.0f - std::pow(1.0f - t, 2.0f); // ease-out quad
            volume = highVolume + eased * (maxVolume - highVolume);
        }

        if (volume > maxVolume)
            volume = maxVolume;

        if (volume < minVolume)
            volume = minVolume;

        return std::clamp(volume, minVolume, maxVolume);
    }

    inline float GetRandomFloat(const float min, const float max)
    {
        std::mt19937                          gen(std::random_device{}());
        std::uniform_real_distribution<float> dist(min, max);

        return dist(gen);
    }

    inline Vector GetRandomVector(const float xMax, const float yMax)
    {
        std::mt19937                          gen(std::random_device{}());
        std::uniform_real_distribution<float> distX(-xMax, xMax);
        std::uniform_real_distribution<float> distY(-yMax, yMax);

        Vector vector;
        vector.X = distX(gen);
        vector.Y = distY(gen);
        vector.Z = 20.0f;

        return vector;
    }

    inline Vector GetRandomVector(const float distance)
    {
        std::mt19937                          gen(std::random_device{}());
        std::uniform_real_distribution<float> distYaw(-RlAngle180 + 1, RlAngle180);

        const int yaw = static_cast<int>(distYaw(gen) + 0.5f);

        const auto rotation = Rotator(0, yaw, 0);
        Vector     vector   = RotatorToVector(rotation);

        vector *= distance / vector.magnitude();

        return vector;
    }
}
