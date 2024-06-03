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
