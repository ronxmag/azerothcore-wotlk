/*
 * This file is part of the AzerothCore Project. See AUTHORS file for
 * Copyright information.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Common.h"
#include "Log.h"
#include "ScriptMgr.h"

#include <exception>
#include <utility>

// ============================================================================
// Forward Declarations - DC Script Entry Points
// ============================================================================
// --- AoE loot system ---
void AddSC_dc_aoeloot_unified();              // QOL/dc_aoeloot_unified.cpp

// ============================================================================
// Script Loader Helpers
// ============================================================================

template <typename Func>
inline bool TryLoadScript(char const* name, Func&& loader)
{
    try
    {
        loader();
        LOG_INFO("scripts.dc", ">>   [OK] {}", name);
        return true;
    }
    catch (std::exception const& e)
    {
        LOG_ERROR("scripts.dc", ">>   [EXCEPTION] {}: {}", name, e.what());
    }
    catch (...)
    {
        LOG_ERROR("scripts.dc", ">>   [CRASH] {}: unknown exception", name);
    }

    return false;
}

inline void LogSection(char const* title)
{
    LOG_INFO("scripts.dc", ">> ===========================================================");
    LOG_INFO("scripts.dc", ">> {}", title);
    LOG_INFO("scripts.dc", ">> ===========================================================");
}

template <typename Func>
inline void LoadAndCount(
    char const* name,
    Func&& loader,
    uint32& loadedCount,
    uint32& failedCount)
{
    if (TryLoadScript(name, std::forward<Func>(loader)))
        ++loadedCount;
    else
        ++failedCount;
}

// The name of this function should match:
// void Add${NameOfDirectory}Scripts()
void AddDCScripts()
{
    uint32 loadedCount = 0;
    uint32 failedCount = 0;

    auto load = [&](char const* name, auto&& loader)
    {
        LoadAndCount(
            name,
            std::forward<decltype(loader)>(loader),
            loadedCount,
            failedCount);
    };

#define DC_LOAD(script) load(#script, []() { script(); })

    LOG_INFO("scripts.dc", "==============================================================");
    LOG_INFO("scripts.dc", "DarkChaos: DC script loader starting");
    LOG_INFO("scripts.dc", "==============================================================");

    LogSection("AoE Loot System");
    DC_LOAD(AddSC_dc_aoeloot_unified);

#undef DC_LOAD

    LOG_INFO("scripts.dc", "==============================================================");
    LOG_INFO(
        "scripts.dc",
        "DarkChaos: DC script loader complete (loaded: {}, failed: {})",
        loadedCount,
        failedCount);
    LOG_INFO("scripts.dc", "==============================================================");
}
