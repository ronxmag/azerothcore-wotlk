/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license
 * Copyright (C) 2025+ DarkChaos-255 Custom Scripts
 *
 * dc_constants.h - Shared constants for all DC systems
 * Centralizes magic numbers and provides type-safe constants
 */

#ifndef DC_CONSTANTS_H
#define DC_CONSTANTS_H

#include <cstdint>

namespace DCConstants
{
     // =========================================================================
    // AoE Loot Constants
    // =========================================================================
    constexpr float DEFAULT_AOELOOT_RANGE = 30.0f;
    constexpr float MIN_AOELOOT_RANGE = 5.0f;
    constexpr float MAX_AOELOOT_RANGE = 100.0f;
    constexpr uint32 DEFAULT_MAX_CORPSES = 10;
    constexpr uint32 MAX_CORPSES_LIMIT = 50;
    constexpr uint8 DEFAULT_MAX_MERGE_SLOTS = 15;
    constexpr uint8 MAX_MERGE_SLOTS_LIMIT = 16;

}  // namespace DCConstants

#endif // DC_CONSTANTS_H
