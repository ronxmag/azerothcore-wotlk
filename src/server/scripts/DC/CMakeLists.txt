# CMakeLists.txt for DC scripts

# AoE Loot System (Unified)
set(SCRIPTS_DC_AoELoot
    QOL/dc_aoeloot_unified.cpp
)

# Main AC scripts
set(SCRIPTS_DC_AC
    dc_script_loader.cpp
)

# DC Addon Extension System - Unified client/server protocol
set(SCRIPTS_DC_AddonExtension
    AddonExtension/dc_addon_extension_loader.cpp
    AddonExtension/dc_addon_aoeloot.cpp
)

# Add sources to AzerothCore scripts
set(SCRIPTS_WORLD
    ${SCRIPTS_WORLD}
    ${SCRIPTS_DC_AoELoot}
    ${SCRIPTS_DC_QOL}
)

# The DC HinterlandBG config was merged into the consolidated
# custom config (Custom/Config files/darkchaos-custom.conf.dist).
# Do not copy the legacy hinterlandbg.conf.dist here because it may
# not exist in this repository layout.
