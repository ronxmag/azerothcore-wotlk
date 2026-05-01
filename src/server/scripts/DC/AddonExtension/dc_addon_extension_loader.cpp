/*
 * Dark Chaos - Addon Extension Loader
 * ====================================
 *
 * Loads all addon extension scripts for the DC namespace.
 *
 * Copyright (C) 2024-2025 Dark Chaos Development Team
 */

// Script declarations
void AddSC_dc_addon_aoeloot();

namespace DCAddon { void AddTeleportScripts(); }

void AddDCAddonExtensionScripts()
{

    // Module handlers
    AddSC_dc_addon_aoeloot();

}
