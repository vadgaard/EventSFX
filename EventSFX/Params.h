#pragma once

#include "pch.h"

struct StatTickerParams
{
    uintptr_t Receiver;
    uintptr_t Victim;
    uintptr_t StatEvent;
};

struct BumpParams
{
    uintptr_t OtherCar;
    Vector    HitLocation;
    Vector    HitNormal;
};
