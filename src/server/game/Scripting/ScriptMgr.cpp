/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "AllScriptsObjects.h"
#include "InstanceScript.h"
#include "LFGScripts.h"
#include "ScriptSystem.h"
#include "SmartAI.h"
#include "SpellMgr.h"
#include "UnitAI.h"

namespace
{
    template<typename T>
    inline void SCR_CLEAR()
    {
        for (auto const& [scriptID, script] : ScriptRegistry<T>::ScriptPointerList)
        {
            delete script;
        }

        ScriptRegistry<T>::ScriptPointerList.clear();
    }
}

struct TSpellSummary
{
    uint8 Targets; // set of enum SelectTarget
    uint8 Effects; // set of enum SelectEffect
}*SpellSummary;

ScriptMgr::ScriptMgr()
    : _scriptCount(0),
    _scheduledScripts(0),
    _script_loader_callback(nullptr),
    _modules_loader_callback(nullptr) { }

ScriptMgr::~ScriptMgr() { }

ScriptMgr* ScriptMgr::instance()
{
    static ScriptMgr instance;
    return &instance;
}

PlayerbotScript::PlayerbotScript(const char* name) : ScriptObject(name)
{
    ScriptRegistry<PlayerbotScript>::AddScript(this);
}

void ScriptMgr::Initialize()
{
    LOG_INFO("server.loading", "> Loading C++ scripts");
    LOG_INFO("server.loading", " ");

    AddSC_SmartScripts();

    // LFGScripts
    lfg::AddSC_LFGScripts();

    ASSERT(_script_loader_callback,
        "Script loader callback wasn't registered!");

    ASSERT(_modules_loader_callback,
        "Modules loader callback wasn't registered!");

    _script_loader_callback();
    _modules_loader_callback();

    ScriptRegistry<AccountScript>::InitEnabledHooksIfNeeded(ACCOUNTHOOK_END);
    ScriptRegistry<AchievementScript>::InitEnabledHooksIfNeeded(ACHIEVEMENTHOOK_END);
    ScriptRegistry<ArenaScript>::InitEnabledHooksIfNeeded(ARENAHOOK_END);
    ScriptRegistry<ArenaTeamScript>::InitEnabledHooksIfNeeded(ARENATEAMHOOK_END);
    ScriptRegistry<AuctionHouseScript>::InitEnabledHooksIfNeeded(AUCTIONHOUSEHOOK_END);
    ScriptRegistry<BattlefieldScript>::InitEnabledHooksIfNeeded(BATTLEFIELDHOOK_END);
    ScriptRegistry<BGScript>::InitEnabledHooksIfNeeded(ALLBATTLEGROUNDHOOK_END);
    ScriptRegistry<CommandSC>::InitEnabledHooksIfNeeded(ALLCOMMANDHOOK_END);
    ScriptRegistry<DatabaseScript>::InitEnabledHooksIfNeeded(DATABASEHOOK_END);
    ScriptRegistry<FormulaScript>::InitEnabledHooksIfNeeded(FORMULAHOOK_END);
    ScriptRegistry<GameEventScript>::InitEnabledHooksIfNeeded(GAMEEVENTHOOK_END);
    ScriptRegistry<GlobalScript>::InitEnabledHooksIfNeeded(GLOBALHOOK_END);
    ScriptRegistry<GroupScript>::InitEnabledHooksIfNeeded(GROUPHOOK_END);
    ScriptRegistry<GuildScript>::InitEnabledHooksIfNeeded(GUILDHOOK_END);
    ScriptRegistry<LootScript>::InitEnabledHooksIfNeeded(LOOTHOOK_END);
    ScriptRegistry<MailScript>::InitEnabledHooksIfNeeded(MAILHOOK_END);
    ScriptRegistry<MiscScript>::InitEnabledHooksIfNeeded(MISCHOOK_END);
    ScriptRegistry<MovementHandlerScript>::InitEnabledHooksIfNeeded(MOVEMENTHOOK_END);
    ScriptRegistry<PetScript>::InitEnabledHooksIfNeeded(PETHOOK_END);
    ScriptRegistry<PlayerScript>::InitEnabledHooksIfNeeded(PLAYERHOOK_END);
    ScriptRegistry<ServerScript>::InitEnabledHooksIfNeeded(SERVERHOOK_END);
    ScriptRegistry<SpellSC>::InitEnabledHooksIfNeeded(ALLSPELLHOOK_END);
    ScriptRegistry<TicketScript>::InitEnabledHooksIfNeeded(TICKETHOOK_END);
    ScriptRegistry<UnitScript>::InitEnabledHooksIfNeeded(UNITHOOK_END);
    ScriptRegistry<WorldObjectScript>::InitEnabledHooksIfNeeded(WORLDOBJECTHOOK_END);
    ScriptRegistry<WorldScript>::InitEnabledHooksIfNeeded(WORLDHOOK_END);
    ScriptRegistry<AllMapScript>::InitEnabledHooksIfNeeded(ALLMAPHOOK_END);
}

void ScriptMgr::Unload()
{
    Acore::for_each<ScriptRegistryTypes>([]<typename Info>()
    {
        for (auto const& [scriptID, script] : ScriptRegistry<typename Info::type>::ScriptPointerList)
        {
            delete script;
        }

        ScriptRegistry<typename Info::type>::ScriptPointerList.clear();
    });

    delete[] SpellSummary;
}

void ScriptMgr::LoadDatabase()
{
    uint32 oldMSTime = getMSTime();

    sScriptSystemMgr->LoadScriptWaypoints();

    // Add all scripts that must be loaded after db/maps
    ScriptRegistry<WorldMapScript>::AddALScripts();
    ScriptRegistry<BattlegroundMapScript>::AddALScripts();
    ScriptRegistry<InstanceMapScript>::AddALScripts();
    ScriptRegistry<SpellScriptLoader>::AddALScripts();
    ScriptRegistry<ItemScript>::AddALScripts();
    ScriptRegistry<CreatureScript>::AddALScripts();
    ScriptRegistry<GameObjectScript>::AddALScripts();
    ScriptRegistry<AreaTriggerScript>::AddALScripts();
    ScriptRegistry<BattlegroundScript>::AddALScripts();
    ScriptRegistry<OutdoorPvPScript>::AddALScripts();
    ScriptRegistry<WeatherScript>::AddALScripts();
    ScriptRegistry<ConditionScript>::AddALScripts();
    ScriptRegistry<TransportScript>::AddALScripts();
    ScriptRegistry<AchievementCriteriaScript>::AddALScripts();

    FillSpellSummary();

    CheckIfScriptsInDatabaseExist();

    LOG_INFO("server.loading", ">> Loaded {} C++ scripts in {} ms", GetScriptCount(), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ScriptMgr::CheckIfScriptsInDatabaseExist()
{
    for (auto const& scriptName : sObjectMgr->GetScriptNames())
    {
        if (uint32 sid = sObjectMgr->GetScriptId(scriptName))
        {
            bool const hasRegisteredScript = Acore::any_of<ScriptRegistryTypes>([sid]<typename Info>()
            {
                if constexpr (Info::LegacyDbValidationCandidate)
                    return ScriptRegistry<typename Info::type>::GetScriptById(sid) != nullptr;

                return false;
            });

            if (!hasRegisteredScript)
                LOG_ERROR("sql.sql", "Script named '{}' is assigned in the database, but has no code!", scriptName);
        }
    }
}

void ScriptMgr::FillSpellSummary()
{
    UnitAI::FillAISpellInfo();

    SpellSummary = new TSpellSummary[sSpellMgr->GetSpellInfoStoreSize()];

    SpellInfo const* pTempSpell;

    for (uint32 i = 0; i < sSpellMgr->GetSpellInfoStoreSize(); ++i)
    {
        SpellSummary[i].Effects = 0;
        SpellSummary[i].Targets = 0;

        pTempSpell = sSpellMgr->GetSpellInfo(i);
        // This spell doesn't exist.
        if (!pTempSpell)
            continue;

        for (uint32 j = 0; j < MAX_SPELL_EFFECTS; ++j)
        {
            // Spell targets self.
            if (pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_CASTER)
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_SELF - 1);

            // Spell targets a single enemy.
            if (pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_TARGET_ENEMY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_DEST_TARGET_ENEMY)
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_SINGLE_ENEMY - 1);

            // Spell targets AoE at enemy.
            if (pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_SRC_AREA_ENEMY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_DEST_AREA_ENEMY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_SRC_CASTER ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_DEST_DYNOBJ_ENEMY)
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_AOE_ENEMY - 1);

            // Spell targets an enemy.
            if (pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_TARGET_ENEMY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_DEST_TARGET_ENEMY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_SRC_AREA_ENEMY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_DEST_AREA_ENEMY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_SRC_CASTER ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_DEST_DYNOBJ_ENEMY)
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_ANY_ENEMY - 1);

            // Spell targets a single friend (or self).
            if (pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_CASTER ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_TARGET_ALLY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_TARGET_PARTY)
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_SINGLE_FRIEND - 1);

            // Spell targets AoE friends.
            if (pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_CASTER_AREA_PARTY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_LASTTARGET_AREA_PARTY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_SRC_CASTER)
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_AOE_FRIEND - 1);

            // Spell targets any friend (or self).
            if (pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_CASTER ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_TARGET_ALLY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_TARGET_PARTY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_CASTER_AREA_PARTY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_LASTTARGET_AREA_PARTY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_SRC_CASTER)
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_ANY_FRIEND - 1);

            // Make sure that this spell includes a damage effect.
            if (pTempSpell->Effects[j].Effect == SPELL_EFFECT_SCHOOL_DAMAGE ||
                    pTempSpell->Effects[j].Effect == SPELL_EFFECT_INSTAKILL ||
                    pTempSpell->Effects[j].Effect == SPELL_EFFECT_ENVIRONMENTAL_DAMAGE ||
                    pTempSpell->Effects[j].Effect == SPELL_EFFECT_HEALTH_LEECH)
                SpellSummary[i].Effects |= 1 << (SELECT_EFFECT_DAMAGE - 1);

            // Make sure that this spell includes a healing effect (or an apply aura with a periodic heal).
            if (pTempSpell->Effects[j].Effect == SPELL_EFFECT_HEAL ||
                    pTempSpell->Effects[j].Effect == SPELL_EFFECT_HEAL_MAX_HEALTH ||
                    pTempSpell->Effects[j].Effect == SPELL_EFFECT_HEAL_MECHANICAL ||
                    (pTempSpell->Effects[j].Effect == SPELL_EFFECT_APPLY_AURA  && pTempSpell->Effects[j].ApplyAuraName == 8))
                SpellSummary[i].Effects |= 1 << (SELECT_EFFECT_HEALING - 1);

            // Make sure that this spell applies an aura.
            if (pTempSpell->Effects[j].Effect == SPELL_EFFECT_APPLY_AURA)
                SpellSummary[i].Effects |= 1 << (SELECT_EFFECT_AURA - 1);
        }
    }
}
