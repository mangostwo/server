/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2025 MaNGOS <https://www.getmangos.eu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

/**
 * @file ObjectMgrCreatures.cpp
 * @brief Cohesion split of ObjectMgr.cpp -- creature template, addon,
 *        model, and spawn loading helpers. Same `ObjectMgr` class; no
 *        behaviour change.
 */

#include "ObjectMgr.h"
#include "LivingWorldAnchorPolicy.h"
#include "MotionGenerators/MotionMaster.h"
#include "Database/DatabaseEnv.h"
#include "Policies/Singleton.h"

#include "SQLStorages.h"
#include "Log.h"
#include "MapManager.h"
#include "ObjectGuid.h"
#include "ScriptMgr.h"
#include "SpellMgr.h"
#include "UpdateMask.h"
#include "World.h"
#include "Group.h"
#include "ArenaTeam.h"
#include "Transports.h"
#include "ProgressBar.h"
#include "Language.h"
#include "PoolManager.h"
#include "GameEventMgr.h"
#include "Spell.h"
#include "Chat.h"
#include "AccountMgr.h"
#include "MapPersistentStateMgr.h"
#include "SpellAuras.h"
#include "Util.h"
#include "WaypointManager.h"
#include "GossipDef.h"
#include "Mail.h"
#include "InstanceData.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "DisableMgr.h"

#include <limits>
#include <set>
struct SQLCreatureLoader : public SQLStorageLoaderBase<SQLCreatureLoader, SQLStorage>
{
    template<class D>
    void convert_from_str(uint32 /*field_pos*/, char const* src, D& dst)
    {
        dst = D(sScriptMgr.GetScriptId(src));
    }
};

/**
 * @brief Loads creature template definitions and validates related data.
 */
void ObjectMgr::LoadCreatureTemplates()
{
    SQLCreatureLoader loader;
    loader.Load(sCreatureStorage);

    sLog.outString(">> Loaded %u creature definitions", sCreatureStorage.GetRecordCount());
    sLog.outString();

    std::set<uint32> difficultyEntries[MAX_DIFFICULTY - 1]; // already loaded difficulty 1 value in creatures
    std::set<uint32> hasDifficultyEntries[MAX_DIFFICULTY - 1]; // already loaded creatures with difficulty 1  values

    // check data correctness
    for (uint32 i = 1; i < sCreatureStorage.GetMaxEntry(); ++i)
    {
        CreatureInfo const* cInfo = sCreatureStorage.LookupEntry<CreatureInfo>(i);
        if (!cInfo)
        {
            continue;
        }

        bool ok = true;                                     // bool to allow continue outside this loop
        for (uint32 diff = 0; diff < MAX_DIFFICULTY - 1 && ok; ++diff)
        {
            if (!cInfo->DifficultyEntry[diff])
            {
                continue;
            }
            ok = false;                                     // will be set to true at the end of this loop again

            CreatureInfo const* difficultyInfo = GetCreatureTemplate(cInfo->DifficultyEntry[diff]);
            if (!difficultyInfo)
            {
                sLog.outErrorDb("Creature (Entry: %u) have `difficulty_entry_%u`=%u but creature entry %u not exist.",
                                i, diff + 1, cInfo->DifficultyEntry[diff], cInfo->DifficultyEntry[diff]);
                continue;
            }

            if (difficultyEntries[diff].find(i) != difficultyEntries[diff].end())
            {
                sLog.outErrorDb("Creature (Entry: %u) listed as difficulty %u but have value in `difficulty_entry_%u`.", i, diff + 1, diff + 1);
                continue;
            }

            bool ok2 = true;
            for (uint32 diff2 = 0; diff2 < MAX_DIFFICULTY - 1 && ok2; ++diff2)
            {
                ok2 = false;
                if (difficultyEntries[diff2].find(cInfo->DifficultyEntry[diff]) != difficultyEntries[diff2].end())
                {
                    sLog.outErrorDb("Creature (Entry: %u) already listed as difficulty %u for another entry.", cInfo->DifficultyEntry[diff], diff2 + 1);
                    continue;
                }

                if (hasDifficultyEntries[diff2].find(cInfo->DifficultyEntry[diff]) != hasDifficultyEntries[diff2].end())
                {
                    sLog.outErrorDb("Creature (Entry: %u) have `difficulty_entry_%u`=%u but creature entry %u have difficulty %u entry also.",
                                    i, diff + 1, cInfo->DifficultyEntry[diff], cInfo->DifficultyEntry[diff], diff2 + 1);
                    continue;
                }
                ok2 = true;
            }
            if (!ok2)
            {
                continue;
            }

            if (cInfo->UnitClass != difficultyInfo->UnitClass)
            {
                sLog.outErrorDb("Creature (Entry: %u, class %u) has different `UnitClass` in difficulty %u mode (Entry: %u, class %u).",
                                i, cInfo->UnitClass, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->UnitClass);
                continue;
            }

            if (cInfo->NpcFlags != difficultyInfo->NpcFlags)
            {
                sLog.outErrorDb("Creature (Entry: %u) has different `NpcFlags` in difficulty %u mode (Entry: %u).", i, diff + 1, cInfo->DifficultyEntry[diff]);
                continue;
            }

            if (cInfo->TrainerClass != difficultyInfo->TrainerClass)
            {
                sLog.outErrorDb("Creature (Entry: %u) has different `TrainerClass` in difficulty %u mode (Entry: %u).", i, diff + 1, cInfo->DifficultyEntry[diff]);
                continue;
            }

            if (cInfo->TrainerRace != difficultyInfo->TrainerRace)
            {
                sLog.outErrorDb("Creature (Entry: %u) has different `TrainerRace` in difficulty %u mode (Entry: %u).", i, diff + 1, cInfo->DifficultyEntry[diff]);
                continue;
            }

            if (cInfo->TrainerType != difficultyInfo->TrainerType)
            {
                sLog.outErrorDb("Creature (Entry: %u) has different `TrainerType` in difficulty %u mode (Entry: %u).", i, diff + 1, cInfo->DifficultyEntry[diff]);
                continue;
            }

            if (cInfo->TrainerSpell != difficultyInfo->TrainerSpell)
            {
                sLog.outErrorDb("Creature (Entry: %u) has different `TrainerSpell` in difficulty %u mode (Entry: %u).", i, diff + 1, cInfo->DifficultyEntry[diff]);
                continue;
            }

            if (difficultyInfo->AIName && *difficultyInfo->AIName)
            {
                sLog.outErrorDb("Difficulty %u mode creature (Entry: %u) has `AIName`, but in any case will used difficulty 0 mode creature (Entry: %u) AIName.",
                                diff + 1, cInfo->DifficultyEntry[diff], i);
                continue;
            }

            if (sScriptMgr.GetBoundScriptId(SCRIPTED_UNIT, difficultyInfo->Entry))
            {
                sLog.outErrorDb("Difficulty %u mode creature (Entry: %u) has `ScriptName`, but in any case will used difficulty 0 mode creature (Entry: %u) ScriptName.",
                                diff + 1, cInfo->DifficultyEntry[diff], i);
                continue;
            }

            hasDifficultyEntries[diff].insert(i);
            difficultyEntries[diff].insert(cInfo->DifficultyEntry[diff]);
            ok = true;
        }
        if (!ok)
        {
            continue;
        }

        FactionTemplateEntry const* factionTemplate = sFactionTemplateStore.LookupEntry(cInfo->FactionAlliance);
        if (!factionTemplate)
        {
            sLog.outErrorDb("Creature (Entry: %u) has nonexistent faction_A template (%u)", cInfo->Entry, cInfo->FactionAlliance);
        }

        factionTemplate = sFactionTemplateStore.LookupEntry(cInfo->FactionHorde);
        if (!factionTemplate)
        {
            sLog.outErrorDb("Creature (Entry: %u) has nonexistent faction_H template (%u)", cInfo->Entry, cInfo->FactionHorde);
        }

        for (int k = 0; k < MAX_KILL_CREDIT; ++k)
        {
            if (cInfo->KillCredit[k])
            {
                if (!GetCreatureTemplate(cInfo->KillCredit[k]))
                {
                    sLog.outErrorDb("Creature (Entry: %u) has nonexistent creature entry in `KillCredit%d` (%u)", cInfo->Entry, k + 1, cInfo->KillCredit[k]);
                    const_cast<CreatureInfo*>(cInfo)->KillCredit[k] = 0;
                }
            }
        }

        // used later for scale
        CreatureDisplayInfoEntry const* displayScaleEntry = NULL;

        for (int j = 0; j < MAX_CREATURE_MODEL; ++j)
        {
            if (cInfo->ModelId[j])
            {
                CreatureDisplayInfoEntry const* displayEntry = sCreatureDisplayInfoStore.LookupEntry(cInfo->ModelId[j]);
                if (!displayEntry)
                {
                    sLog.outErrorDb("Creature (Entry: %u) has nonexistent modelid_%d (%u), can crash client", cInfo->Entry, j + 1, cInfo->ModelId[j]);
                    const_cast<CreatureInfo*>(cInfo)->ModelId[j] = 0;
                }
                else if (!displayScaleEntry)
                {
                    displayScaleEntry = displayEntry;
                }

                CreatureModelInfo const* minfo = sCreatureModelStorage.LookupEntry<CreatureModelInfo>(cInfo->ModelId[j]);
                if (!minfo)
                {
                    sLog.outErrorDb("Creature (Entry: %u) are using modelid_%d (%u), but creature_model_info are missing for this model.", cInfo->Entry, j + 1, cInfo->ModelId[j]);
                }
            }
        }

        if (!displayScaleEntry)
        {
            sLog.outErrorDb("Creature (Entry: %u) has nonexistent modelid in modelid_1/modelid_2/modelid_3/modelid_4", cInfo->Entry);
        }

        if (!cInfo->MinLevel)
        {
            sLog.outErrorDb("Creature (Entry: %u) has invalid minlevel, set to 1", cInfo->Entry);
            const_cast<CreatureInfo*>(cInfo)->MinLevel = 1;
        }

        if (cInfo->MinLevel > cInfo->MaxLevel)
        {
            sLog.outErrorDb("Creature (Entry: %u) has invalid maxlevel, set to minlevel", cInfo->Entry);
            const_cast<CreatureInfo*>(cInfo)->MaxLevel = cInfo->MinLevel;
        }

        if (cInfo->MinLevel > DEFAULT_MAX_CREATURE_LEVEL)
        {
            sLog.outErrorDb("Creature (Entry: %u) `MinLevel` exceeds maximum allowed value of '%u'", cInfo->Entry, uint32(DEFAULT_MAX_CREATURE_LEVEL));
            const_cast<CreatureInfo*>(cInfo)->MinLevel = uint32(DEFAULT_MAX_CREATURE_LEVEL);
        }

        if (cInfo->MaxLevel > DEFAULT_MAX_CREATURE_LEVEL)
        {
            sLog.outErrorDb("Creature (Entry: %u) `MaxLevel` exceeds maximum allowed value of '%u'", cInfo->Entry, uint32(DEFAULT_MAX_CREATURE_LEVEL));
            const_cast<CreatureInfo*>(cInfo)->MaxLevel = uint32(DEFAULT_MAX_CREATURE_LEVEL);
        }

        if (cInfo->Expansion > MAX_EXPANSION)
        {
            sLog.outErrorDb("Creature (Entry: %u) `Expansion(%u)` is not correct", cInfo->Entry, uint32(MAX_EXPANSION));
            const_cast<CreatureInfo*>(cInfo)->Expansion = -1;
        }

        // use below code for 0-checks for unit_class
        if (!cInfo->UnitClass || (((1 << (cInfo->UnitClass - 1)) & CLASSMASK_ALL_CREATURES) == 0))
        {
            sLog.outErrorDb("Creature (Entry: %u) does not have proper `UnitClass(%u)` in creature_template", cInfo->Entry, cInfo->UnitClass);
           // Mark NPC as having improper data by his expansion
            const_cast<CreatureInfo*>(cInfo)->Expansion = -1;
        }

        if (!sLog.HasLogFilter(LOG_FILTER_DB_STRICTED_CHECK) && cInfo->Expansion >= 0) // TODO - Remove the DB_STRICTED_CHECK after a while
        {
            // check if ClassLevel data are available for all possible level of that creature
            for (uint32 level = cInfo->MinLevel; level <= cInfo->MaxLevel; ++level)
            {
                if (!GetCreatureClassLvlStats(level, cInfo->UnitClass, cInfo->Expansion))
                {
                    sLog.outErrorDb("Creature (Entry: %u), Class(%u), level(%u) has no data in `creature_template_classlevelstats`", cInfo->Entry, cInfo->UnitClass, level);
                         // Deactivate using ClassLevelStats for this NPC
                    const_cast<CreatureInfo*>(cInfo)->Expansion = -1;
                    break;
                }
            }
        }

        if (cInfo->DamageSchool >= MAX_SPELL_SCHOOL)
        {
            sLog.outErrorDb("Creature (Entry: %u) has invalid spell school value (%u) in `dmgschool`", cInfo->Entry, cInfo->DamageSchool);
            const_cast<CreatureInfo*>(cInfo)->DamageSchool = SPELL_SCHOOL_NORMAL;
        }

        if (cInfo->MeleeBaseAttackTime == 0)
        {
            const_cast<CreatureInfo*>(cInfo)->MeleeBaseAttackTime  = BASE_ATTACK_TIME;
        }

        if (cInfo->RangedBaseAttackTime == 0)
        {
            const_cast<CreatureInfo*>(cInfo)->RangedBaseAttackTime = BASE_ATTACK_TIME;
        }

        if (cInfo->NpcFlags & UNIT_NPC_FLAG_SPELLCLICK)
        {
            sLog.outErrorDb("Creature (Entry: %u) has dynamic flag UNIT_NPC_FLAG_SPELLCLICK (%u) set, it expect to be set by code base at `npc_spellclick_spells` content.", cInfo->Entry, UNIT_NPC_FLAG_SPELLCLICK);
            const_cast<CreatureInfo*>(cInfo)->NpcFlags &= ~UNIT_NPC_FLAG_SPELLCLICK;
        }

        if ((cInfo->NpcFlags & UNIT_NPC_FLAG_TRAINER) && cInfo->TrainerType >= MAX_TRAINER_TYPE)
        {
            sLog.outErrorDb("Creature (Entry: %u) has wrong trainer type %u", cInfo->Entry, cInfo->TrainerType);
        }

        if (cInfo->CreatureType && !sCreatureTypeStore.LookupEntry(cInfo->CreatureType))
        {
            sLog.outErrorDb("Creature (Entry: %u) has invalid creature type (%u) in `type`", cInfo->Entry, cInfo->CreatureType);
            const_cast<CreatureInfo*>(cInfo)->CreatureType = CREATURE_TYPE_HUMANOID;
        }

        // must exist or used hidden but used in data horse case
        if (cInfo->Family && !sCreatureFamilyStore.LookupEntry(cInfo->Family) && cInfo->Family != CREATURE_FAMILY_HORSE_CUSTOM)
        {
            sLog.outErrorDb("Creature (Entry: %u) has invalid creature family (%u) in `family`", cInfo->Entry, cInfo->Family);
            const_cast<CreatureInfo*>(cInfo)->Family = 0;
        }

        if (cInfo->InhabitType <= 0 || cInfo->InhabitType > INHABIT_ANYWHERE)
        {
            sLog.outErrorDb("Creature (Entry: %u) has wrong value (%u) in `InhabitType`, creature will not correctly walk/swim/fly", cInfo->Entry, cInfo->InhabitType);
            const_cast<CreatureInfo*>(cInfo)->InhabitType = INHABIT_ANYWHERE;
        }

        if (cInfo->PetSpellDataId)
        {
            CreatureSpellDataEntry const* spellDataId = sCreatureSpellDataStore.LookupEntry(cInfo->PetSpellDataId);
            if (!spellDataId)
            {
                sLog.outErrorDb("Creature (Entry: %u) has non-existing PetSpellDataId (%u)", cInfo->Entry, cInfo->PetSpellDataId);
            }
        }

        if (cInfo->MovementType >= MAX_DB_MOTION_TYPE)
        {
            sLog.outErrorDb("Creature (Entry: %u) has wrong movement generator type (%u), ignore and set to IDLE.", cInfo->Entry, cInfo->MovementType);
            const_cast<CreatureInfo*>(cInfo)->MovementType = IDLE_MOTION_TYPE;
        }

        if (cInfo->VehicleTemplateId && !sVehicleStore.LookupEntry(cInfo->VehicleTemplateId))
        {
            sLog.outErrorDb("Creature (Entry: %u) has non-existing vehicle_id (%u), set to 0.", cInfo->Entry, cInfo->VehicleTemplateId);
            const_cast<CreatureInfo*>(cInfo)->VehicleTemplateId = 0;
        }

        if (cInfo->EquipmentTemplateId > 0)                         // 0 no equipment
        {
            if (!GetEquipmentInfo(cInfo->EquipmentTemplateId))
            {
                sLog.outErrorDb("Table `creature_template` have creature (Entry: %u) with EquipmentTemplateId %u not found in table `creature_equip_template`, set to no equipment.", cInfo->Entry, cInfo->EquipmentTemplateId);
                const_cast<CreatureInfo*>(cInfo)->EquipmentTemplateId = 0;
            }
        }

        if (cInfo->VendorTemplateId > 0)
        {
            if (!(cInfo->NpcFlags & UNIT_NPC_FLAG_VENDOR))
            {
                sLog.outErrorDb("Table `creature_template` have creature (Entry: %u) with VendorTemplateId %u but not have flag UNIT_NPC_FLAG_VENDOR (%u), vendor items will ignored.", cInfo->Entry, cInfo->VendorTemplateId, UNIT_NPC_FLAG_VENDOR);
            }
        }

        /// if not set custom creature scale then load scale from CreatureDisplayInfo.dbc
        if (cInfo->Scale <= 0.0f)
        {
            if (displayScaleEntry)
            {
                const_cast<CreatureInfo*>(cInfo)->Scale = displayScaleEntry->CreatureModelScale;
            }
            else
            {
                const_cast<CreatureInfo*>(cInfo)->Scale = DEFAULT_OBJECT_SCALE;
            }
        }
    }

    sLog.outString(">> Loaded %u creature definitions", sCreatureStorage.GetRecordCount());
    sLog.outString();
}

/**
 * @brief Converts serialized creature addon aura text into stored spell ids.
 *
 * @param addon The addon record being converted.
 * @param table The source table name.
 * @param guidEntryStr The identifier label used in log output.
 */
void ObjectMgr::ConvertCreatureAddonAuras(CreatureDataAddon* addon, char const* table, char const* guidEntryStr)
{
    // Now add the auras, format "spell1 spell2 ..."
    char* p, *s;
    std::vector<int> val;
    s = p = (char*)reinterpret_cast<char const*>(addon->auras);
    if (p)
    {
        while (p[0] != 0)
        {
            ++p;
            if (p[0] == ' ')
            {
                val.push_back(atoi(s));
                s = ++p;
            }
        }
        if (p != s)
        {
            val.push_back(atoi(s));
        }

        // free char* loaded memory
        delete[](char*)reinterpret_cast<char const*>(addon->auras);
    }

    // empty list
    if (val.empty())
    {
        addon->auras = NULL;
        return;
    }

    // replace by new structures array
    const_cast<uint32*&>(addon->auras) = new uint32[val.size() + 1];

    uint32 i = 0;
    for (uint32 j = 0; j < val.size(); ++j)
    {
        uint32& cAura = const_cast<uint32&>(addon->auras[i]);
        cAura = uint32(val[j]);

        SpellEntry const* AdditionalSpellInfo = sSpellStore.LookupEntry(cAura);
        if (!AdditionalSpellInfo)
        {
            sLog.outErrorDb("Creature (%s: %u) has wrong spell %u defined in `auras` field in `%s`.", guidEntryStr, addon->guidOrEntry, cAura, table);
            continue;
        }

        // Must be Aura, but also allow dummy/script effect spells, as they are used sometimes to select a random aura or similar
        if (!IsSpellAppliesAura(AdditionalSpellInfo) && !IsSpellHaveEffect(AdditionalSpellInfo, SPELL_EFFECT_DUMMY) && !IsSpellHaveEffect(AdditionalSpellInfo, SPELL_EFFECT_SCRIPT_EFFECT) && !IsSpellHaveEffect(AdditionalSpellInfo, SPELL_EFFECT_TRIGGER_SPELL))
        {
            sLog.outErrorDb("Creature (%s: %u) has spell %u defined in `auras` field in `%s, but spell doesn't apply an aura`.", guidEntryStr, addon->guidOrEntry, cAura, table);
            continue;
        }

        // TODO: Remove LogFilter check after more research
        if (!sLog.HasLogFilter(LOG_FILTER_DB_STRICTED_CHECK) && !IsOnlySelfTargeting(AdditionalSpellInfo))
        {
            sLog.outErrorDb("Creature (%s: %u) has spell %u defined in `auras` field in `%s, but spell is no self-only spell`.", guidEntryStr, addon->guidOrEntry, cAura, table);
            continue;
        }

        if (IsSpellHaveAura(AdditionalSpellInfo, SPELL_AURA_CONTROL_VEHICLE))
        {
            sLog.outErrorDb("Creature (%s: %u) has spell %u defined in `auras` field in `%s, but vehicle control spells are not suitable for _addon.auras handling`.", guidEntryStr, addon->guidOrEntry, cAura, table);
            continue;
        }

        if (std::find(&addon->auras[0], &addon->auras[i], cAura) != &addon->auras[i])
        {
            sLog.outErrorDb("Creature (%s: %u) has duplicate spell %u defined in `auras` field in `%s`.", guidEntryStr, addon->guidOrEntry, cAura, table);
            continue;
        }

        ++i;
    }

    // fill terminator element (after last added)
    const_cast<uint32&>(addon->auras[i]) = 0;
}

/**
 * @brief Loads creature addon records from a storage and validates their fields.
 *
 * @param creatureaddons The addon storage to load.
 * @param entryName The entry label used in log output.
 * @param comment The descriptive text used in the load summary.
 */
void ObjectMgr::LoadCreatureAddons(SQLStorage& creatureaddons, char const* entryName, char const* comment)
{
    creatureaddons.Load();

    // check data correctness and convert 'auras'
    for (uint32 i = 1; i < creatureaddons.GetMaxEntry(); ++i)
    {
        CreatureDataAddon const* addon = creatureaddons.LookupEntry<CreatureDataAddon>(i);
        if (!addon)
        {
            continue;
        }

        if (addon->mount)
        {
            if (!sCreatureDisplayInfoStore.LookupEntry(addon->mount))
            {
                sLog.outErrorDb("Creature (%s %u) have invalid displayInfoId for mount (%u) defined in `%s`.", entryName, addon->guidOrEntry, addon->mount, creatureaddons.GetTableName());
                const_cast<CreatureDataAddon*>(addon)->mount = 0;
            }
        }

        if (addon->sheath_state > SHEATH_STATE_RANGED)
        {
            sLog.outErrorDb("Creature (%s %u) has unknown sheath state (%u) defined in `%s`.", entryName, addon->guidOrEntry, addon->sheath_state, creatureaddons.GetTableName());
        }

        if (!sEmotesStore.LookupEntry(addon->emote))
        {
            sLog.outErrorDb("Creature (%s %u) have invalid emote (%u) defined in `%s`.", entryName, addon->guidOrEntry, addon->emote, creatureaddons.GetTableName());
            const_cast<CreatureDataAddon*>(addon)->emote = 0;
        }

        ConvertCreatureAddonAuras(const_cast<CreatureDataAddon*>(addon), creatureaddons.GetTableName(), entryName);
    }

    sLog.outString(">> Loaded %u %s", creatureaddons.GetRecordCount(), comment);
}

/**
 * @brief Loads creature template and spawn addon records.
 */
void ObjectMgr::LoadCreatureAddons()
{
    LoadCreatureAddons(sCreatureInfoAddonStorage, "Entry", "creature template addons");

    // check entry ids
    for (uint32 i = 1; i < sCreatureInfoAddonStorage.GetMaxEntry(); ++i)
    {
        if (CreatureDataAddon const* addon = sCreatureInfoAddonStorage.LookupEntry<CreatureDataAddon>(i))
        {
            if (!sCreatureStorage.LookupEntry<CreatureInfo>(addon->guidOrEntry))
            {
                sLog.outErrorDb("Creature (Entry: %u) does not exist but has a record in `%s`", addon->guidOrEntry, sCreatureInfoAddonStorage.GetTableName());
            }
        }
    }

    LoadCreatureAddons(sCreatureDataAddonStorage, "GUID", "creature addons");

    // check entry ids
    for (uint32 i = 1; i < sCreatureDataAddonStorage.GetMaxEntry(); ++i)
    {
        if (CreatureDataAddon const* addon = sCreatureDataAddonStorage.LookupEntry<CreatureDataAddon>(i))
        {
            if (mCreatureDataMap.find(addon->guidOrEntry) == mCreatureDataMap.end())
            {
                sLog.outErrorDb("Creature (GUID: %u) does not exist but has a record in `creature_addon`", addon->guidOrEntry);
            }
        }
    }
}

/**
 * @brief Loads creature class and level base stat data.
 */
void ObjectMgr::LoadCreatureClassLvlStats()
{
    // initialize data array
    memset(&m_creatureClassLvlStats, 0, sizeof(m_creatureClassLvlStats));

    std::string queryStr = "SELECT `Class`, `Level`, `BaseMana`, `BaseMeleeAttackPower`, `BaseRangedAttackPower`, `BaseArmor`";

    for (int i = 0; i <= MAX_EXPANSION; i++)
    {
        std::ostringstream str;
        str << ", `BaseHealthExp" << i << "`, `BaseDamageExp" << i << "`";
        queryStr.append(str.str().c_str());
    }

    queryStr.append(" FROM `creature_template_classlevelstats` ORDER BY `Class`, `Level`");

    QueryResult* result = WorldDatabase.Query(queryStr.c_str());

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();
        sLog.outErrorDb("DB table `creature_template_classlevelstats` is empty.");
        sLog.outString();
        return;
    }

    BarGoLink bar(result->GetRowCount());
    uint32 DataCount = 0;

    do
    {
        Field* fields = result->Fetch();
        bar.step();

        uint32 creatureClass               = fields[0].GetUInt32();
        uint32 creatureLevel               = fields[1].GetUInt32();

        if (creatureLevel == 0 || creatureLevel > DEFAULT_MAX_CREATURE_LEVEL)
        {
            sLog.outErrorDb("Found stats for creature level [%u], incorrect level for this core. Skip!", creatureLevel);
            continue;
        }

        if (((1 << (creatureClass - 1)) & CLASSMASK_ALL_CREATURES) == 0)
        {
            sLog.outErrorDb("Found stats for creature class [%u], incorrect class for this core. Skip!", creatureClass);
            continue;
        }

        uint32  baseMana                   = fields[2].GetUInt32();
        float   baseMeleeAttackPower       = fields[3].GetFloat();
        float   baseRangedAttackPower      = fields[4].GetFloat();
        uint32  baseArmor                  = fields[5].GetUInt32();

        for (uint8 i = 0; i <= MAX_EXPANSION; ++i)
        {
            CreatureClassLvlStats &cCLS = m_creatureClassLvlStats[creatureLevel][classToIndex[creatureClass]][i - 1];   // values should start from 0
            cCLS.BaseMana                   = baseMana;
            cCLS.BaseMeleeAttackPower       = baseMeleeAttackPower;
            cCLS.BaseRangedAttackPower      = baseRangedAttackPower;
            cCLS.BaseArmor                  = baseArmor;

            cCLS.BaseHealth = fields[6 + (i * 2)].GetUInt32();
            cCLS.BaseDamage = fields[7 + (i * 2)].GetFloat();
        }
        ++DataCount;
    }
    while (result->NextRow());

    delete result;

    sLog.outString(">> Found %u creature stats definitions.", DataCount);
    sLog.outString();
}

/**
 * @brief Gets base class-level stats for a creature level and class.
 *
 * @param level The creature level.
 * @param unitClass The creature unit class.
 * @return The matching stats record, or null if unavailable.
 */
CreatureClassLvlStats const* ObjectMgr::GetCreatureClassLvlStats(uint32 level, uint32 unitClass, int32 expansion) const
{
    if (expansion < 0)
    {
        return NULL;
    }

    CreatureClassLvlStats const* cCLS = &m_creatureClassLvlStats[level][classToIndex[unitClass]][expansion];

    if (cCLS->BaseHealth != 0 && cCLS->BaseDamage > 0.1f)
    {
        return cCLS;
    }

    return NULL;
}

/**
 * @brief Loads creature equipment templates in current and deprecated formats.
 */
void ObjectMgr::LoadEquipmentTemplates()
{
    sEquipmentStorage.Load();

    for (uint32 i = 0; i < sEquipmentStorage.GetMaxEntry(); ++i)
    {
        EquipmentInfo const* eqInfo = sEquipmentStorage.LookupEntry<EquipmentInfo>(i);

        if (!eqInfo)
        {
            continue;
        }

        for (uint8 j = 0; j < 3; ++j)
        {
            if (!eqInfo->equipentry[j])
            {
                continue;
            }

            ItemEntry const* dbcitem = sItemStore.LookupEntry(eqInfo->equipentry[j]);
            if (!dbcitem)
            {
                sLog.outErrorDb("Unknown item (entry=%u) in creature_equip_template.equipentry%u for entry = %u, forced to 0.", eqInfo->equipentry[j], j + 1, i);
                const_cast<EquipmentInfo*>(eqInfo)->equipentry[j] = 0;
                continue;
            }

            if (dbcitem->InventoryType != INVTYPE_WEAPON &&
                    dbcitem->InventoryType != INVTYPE_SHIELD &&
                    dbcitem->InventoryType != INVTYPE_RANGED &&
                    dbcitem->InventoryType != INVTYPE_2HWEAPON &&
                    dbcitem->InventoryType != INVTYPE_WEAPONMAINHAND &&
                    dbcitem->InventoryType != INVTYPE_WEAPONOFFHAND &&
                    dbcitem->InventoryType != INVTYPE_HOLDABLE &&
                    dbcitem->InventoryType != INVTYPE_THROWN &&
                    dbcitem->InventoryType != INVTYPE_RANGEDRIGHT &&
                    dbcitem->InventoryType != INVTYPE_RELIC)
            {
                sLog.outErrorDb("Item (entry=%u) in creature_equip_template.equipentry%u for entry = %u is not equipable in a hand, forced to 0.", eqInfo->equipentry[j], j + 1, i);
                const_cast<EquipmentInfo*>(eqInfo)->equipentry[j] = 0;
            }
        }
    }

    sLog.outString(">> Loaded %u equipment template", sEquipmentStorage.GetRecordCount());
    sLog.outString();
}

// generally models that does not have a gender(2), or has alternative model for same gender
uint32 ObjectMgr::GetCreatureModelAlternativeModel(uint32 modelId) const
{
    if (const CreatureModelInfo* modelInfo = GetCreatureModelInfo(modelId))
    {
        return modelInfo->modelid_alternative;
    }

    return 0;
}

/**
 * @brief Gets a creature model info record, optionally swapping to the other gender.
 *
 * @param display_id The creature display id.
 * @return The selected model info, or null if none exists.
 */
CreatureModelInfo const* ObjectMgr::GetCreatureModelRandomGender(uint32 display_id) const
{
    CreatureModelInfo const* minfo = GetCreatureModelInfo(display_id);
    if (!minfo)
    {
        return NULL;
    }

    // If a model for another gender exists, 50% chance to use it
    if (minfo->modelid_other_gender != 0 && urand(0, 1) == 0)
    {
        CreatureModelInfo const* minfo_tmp = GetCreatureModelInfo(minfo->modelid_other_gender);
        if (!minfo_tmp)
        {
            sLog.outErrorDb("Model (Entry: %u) has modelid_other_gender %u not found in table `creature_model_info`. ", minfo->modelid, minfo->modelid_other_gender);
            return minfo;                                   // not fatal, just use the previous one
        }
        else
        {
            return minfo_tmp;
        }
    }
    else
    {
        return minfo;
    }
}

uint32 ObjectMgr::GetModelForRace(uint32 sourceModelId, uint32 racemask)
{
    uint32 modelId = 0;

    CreatureModelRaceMapBounds bounds = m_mCreatureModelRaceMap.equal_range(sourceModelId);

    for (CreatureModelRaceMap::const_iterator itr = bounds.first; itr != bounds.second; ++itr)
    {
        if (!(itr->second.racemask & racemask))
        {
            continue;
        }

        if (itr->second.creature_entry)
        {
            const CreatureInfo* cInfo = GetCreatureTemplate(itr->second.creature_entry);
            modelId = Creature::ChooseDisplayId(cInfo);
        }
        else
        {
            modelId = itr->second.modelid_racial;
        }
    }

    return modelId;
}

/**
 * @brief Loads creature model info records and validates playable race models.
 */
void ObjectMgr::LoadCreatureModelInfo()
{
    sCreatureModelStorage.Load();

    // post processing
    for (uint32 i = 1; i < sCreatureModelStorage.GetMaxEntry(); ++i)
    {
        CreatureModelInfo const* minfo = sCreatureModelStorage.LookupEntry<CreatureModelInfo>(i);
        if (!minfo)
        {
            continue;
        }

        if (!sCreatureDisplayInfoStore.LookupEntry(minfo->modelid))
        {
            sLog.outErrorDb("Table `creature_model_info` has model for nonexistent model id (%u).", minfo->modelid);
        }

        if (minfo->gender >= MAX_GENDER)
        {
            sLog.outErrorDb("Table `creature_model_info` has invalid gender (%u) for model id (%u).", uint32(minfo->gender), minfo->modelid);
            const_cast<CreatureModelInfo*>(minfo)->gender = GENDER_MALE;
        }

        if (minfo->modelid_other_gender)
        {
            if (minfo->modelid_other_gender == minfo->modelid)
            {
                sLog.outErrorDb("Table `creature_model_info` has redundant modelid_other_gender model (%u) defined for model id %u.", minfo->modelid_other_gender, minfo->modelid);
                const_cast<CreatureModelInfo*>(minfo)->modelid_other_gender = 0;
            }
            else if (!sCreatureDisplayInfoStore.LookupEntry(minfo->modelid_other_gender))
            {
                sLog.outErrorDb("Table `creature_model_info` has nonexistent modelid_other_gender model (%u) defined for model id %u.", minfo->modelid_other_gender, minfo->modelid);
                const_cast<CreatureModelInfo*>(minfo)->modelid_other_gender = 0;
            }
        }

        if (minfo->modelid_alternative)
        {
            if (minfo->modelid_alternative == minfo->modelid)
            {
                sLog.outErrorDb("Table `creature_model_info` has redundant modelid_alternative model (%u) defined for model id %u.", minfo->modelid_alternative, minfo->modelid);
                const_cast<CreatureModelInfo*>(minfo)->modelid_alternative = 0;
            }
            else if (!sCreatureDisplayInfoStore.LookupEntry(minfo->modelid_alternative))
            {
                sLog.outErrorDb("Table `creature_model_info` has nonexistent modelid_alternative model (%u) defined for model id %u.", minfo->modelid_alternative, minfo->modelid);
                const_cast<CreatureModelInfo*>(minfo)->modelid_alternative = 0;
            }
        }
    }

    // character races expected have model info data in table
    for (uint32 race = 1; race < sChrRacesStore.GetNumRows(); ++race)
    {
        ChrRacesEntry const* raceEntry = sChrRacesStore.LookupEntry(race);
        if (!raceEntry)
        {
            continue;
        }

        if (!((1 << (race - 1)) & RACEMASK_ALL_PLAYABLE))
        {
            continue;
        }

        if (CreatureModelInfo const* minfo = GetCreatureModelInfo(raceEntry->model_f))
        {
            if (minfo->gender != GENDER_FEMALE)
            {
                sLog.outErrorDb("Table `creature_model_info` have wrong gender %u for character race %u female model id %u", minfo->gender, race, raceEntry->model_f);
            }

            if (minfo->modelid_other_gender != raceEntry->model_m)
            {
                sLog.outErrorDb("Table `creature_model_info` have wrong other gender model id %u for character race %u female model id %u", minfo->modelid_other_gender, race, raceEntry->model_f);
            }

            if (minfo->bounding_radius <= 0.0f)
            {
                sLog.outErrorDb("Table `creature_model_info` have wrong bounding_radius %f for character race %u female model id %u, use %f instead", minfo->bounding_radius, race, raceEntry->model_f, DEFAULT_WORLD_OBJECT_SIZE);
                const_cast<CreatureModelInfo*>(minfo)->bounding_radius = DEFAULT_WORLD_OBJECT_SIZE;
            }

            if (minfo->combat_reach != 1.5f)
            {
                sLog.outErrorDb("Table `creature_model_info` have wrong combat_reach %f for character race %u female model id %u, expected always 1.5f", minfo->combat_reach, race, raceEntry->model_f);
                const_cast<CreatureModelInfo*>(minfo)->combat_reach = 1.5f;
            }
        }
        else
        {
            sLog.outErrorDb("Table `creature_model_info` expect have data for character race %u female model id %u", race, raceEntry->model_f);
        }

        if (CreatureModelInfo const* minfo = GetCreatureModelInfo(raceEntry->model_m))
        {
            if (minfo->gender != GENDER_MALE)
            {
                sLog.outErrorDb("Table `creature_model_info` have wrong gender %u for character race %u male model id %u", minfo->gender, race, raceEntry->model_m);
            }

            if (minfo->modelid_other_gender != raceEntry->model_f)
            {
                sLog.outErrorDb("Table `creature_model_info` have wrong other gender model id %u for character race %u male model id %u", minfo->modelid_other_gender, race, raceEntry->model_m);
            }

            if (minfo->bounding_radius <= 0.0f)
            {
                sLog.outErrorDb("Table `creature_model_info` have wrong bounding_radius %f for character race %u male model id %u, use %f instead", minfo->bounding_radius, race, raceEntry->model_f, DEFAULT_WORLD_OBJECT_SIZE);
                const_cast<CreatureModelInfo*>(minfo)->bounding_radius = DEFAULT_WORLD_OBJECT_SIZE;
            }

            if (minfo->combat_reach != 1.5f)
            {
                sLog.outErrorDb("Table `creature_model_info` have wrong combat_reach %f for character race %u male model id %u, expected always 1.5f", minfo->combat_reach, race, raceEntry->model_m);
                const_cast<CreatureModelInfo*>(minfo)->combat_reach = 1.5f;
            }
        }
        else
        {
            sLog.outErrorDb("Table `creature_model_info` expect have data for character race %u male model id %u", race, raceEntry->model_m);
        }
    }

    sLog.outString(">> Loaded %u creature model based info", sCreatureModelStorage.GetRecordCount());
    sLog.outString();
}

/**
 * @brief Loads creature spell templates and referenced script bindings.
 */
void ObjectMgr::LoadCreatureModelRace()
{
    m_mCreatureModelRaceMap.clear();                        // can be used for reload

    QueryResult* result = WorldDatabase.Query("SELECT `modelid`, `racemask`, `creature_entry`, `modelid_racial` FROM `creature_model_race`");

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outString();
        sLog.outErrorDb(">> Loaded creature_model_race, table is empty!");
        return;
    }


    BarGoLink bar(result->GetRowCount());

    uint32 count = 0;

    // model, racemask
    std::map<uint32, uint32> model2raceMask;

    do
    {
        bar.step();
        Field* fields = result->Fetch();

        CreatureModelRace raceData;

        raceData.modelid            = fields[0].GetUInt32();
        raceData.racemask           = fields[1].GetUInt32();
        raceData.creature_entry     = fields[2].GetUInt32();
        raceData.modelid_racial     = fields[3].GetUInt32();

        if (!sCreatureDisplayInfoStore.LookupEntry(raceData.modelid))
        {
            sLog.outErrorDb("Table `creature_model_race` has model for nonexistent model id (%u), skipping", raceData.modelid);
            continue;
        }

        if (!sCreatureModelStorage.LookupEntry<CreatureModelInfo>(raceData.modelid))
        {
            sLog.outErrorDb("Table `creature_model_race` modelid %u does not exist in creature_model_info, skipping", raceData.modelid);
            continue;
        }

        if (!raceData.racemask)
        {
            sLog.outErrorDb("Table `creature_model_race` modelid %u has no racemask defined, skipping", raceData.modelid);
            continue;
        }

        if (!(raceData.racemask & RACEMASK_ALL_PLAYABLE))
        {
            sLog.outErrorDb("Table `creature_model_race` modelid %u include invalid racemask, skipping", raceData.modelid);
            continue;
        }

        std::map<uint32, uint32>::const_iterator model2Race = model2raceMask.find(raceData.modelid);

        // can't have same mask for same model several times
        if (model2Race != model2raceMask.end())
        {
            if (model2Race->second & raceData.racemask)
            {
                sLog.outErrorDb("Table `creature_model_race` modelid %u with racemask %u has mask already included for same modelid, skipping", raceData.modelid, raceData.racemask);
                continue;
            }
        }

        model2raceMask[raceData.modelid] |= raceData.racemask;

        // creature_entry is the prefered way
        if (raceData.creature_entry)
        {
            if (raceData.modelid_racial)
                sLog.outErrorDb("Table `creature_model_race` modelid %u has modelid_racial for modelid %u but a creature_entry are already defined, modelid_racial will never be used.", raceData.modelid, raceData.modelid_racial);

            if (!sCreatureStorage.LookupEntry<CreatureInfo>(raceData.creature_entry))
            {
                sLog.outErrorDb("Table `creature_model_race` modelid %u has creature_entry for nonexistent creature_template (%u), skipping", raceData.modelid, raceData.creature_entry);
                continue;
            }
        }
        else if (raceData.modelid_racial)
        {
            if (!sCreatureDisplayInfoStore.LookupEntry(raceData.modelid_racial))
            {
                sLog.outErrorDb("Table `creature_model_race` modelid %u has modelid_racial for nonexistent model id (%u), skipping", raceData.modelid, raceData.modelid_racial);
                continue;
            }

            if (!sCreatureModelStorage.LookupEntry<CreatureModelInfo>(raceData.modelid_racial))
            {
                sLog.outErrorDb("Table `creature_model_race` modelid %u has modelid_racial %u, but are not defined in creature_model_info, skipping", raceData.modelid, raceData.modelid_racial);
                continue;
            }
        }
        else
        {
            sLog.outErrorDb("Table `creature_model_race` modelid %u does not have either creature_entry or modelid_racial defined, skipping", raceData.modelid);
            continue;
        }

        m_mCreatureModelRaceMap.insert(CreatureModelRaceMap::value_type(raceData.modelid, raceData));

        ++count;
    }
    while (result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString(">> Loaded %u creature_model_race entries", count);
}

/**
 * @brief Loads creature spawn records and validates their database data.
 */
void ObjectMgr::LoadCreatures()
{
    uint32 count = 0;
    //                                                0                       1   2    3
    QueryResult* result = WorldDatabase.Query("SELECT `creature`.`guid`, `creature`.`id`, `map`, `modelid`,"
                          //   4             5           6           7           8            9              10         11
                          "`equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`,"
                          //   12        13         14            15              16           17           18
                          "`curhealth`, `curmana`, `DeathState`, `MovementType`, `spawnMask`, `phaseMask`, `event`,"
                          //                19                                     20
                          "`pool_creature`.`pool_entry`, `pool_creature_template`.`pool_entry` "
                          "FROM `creature` "
                          "LEFT OUTER JOIN `game_event_creature` ON `creature`.`guid` = `game_event_creature`.`guid` "
                          "LEFT OUTER JOIN `pool_creature` ON `creature`.`guid` = `pool_creature`.`guid` "
                          "LEFT OUTER JOIN `pool_creature_template` ON `creature`.`id` = `pool_creature_template`.`id`");

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();
        sLog.outErrorDb(">> Loaded 0 creature. DB table `creature` is empty.");
        sLog.outString();
        return;
    }

    // build single time for check creature data
    std::set<uint32> difficultyCreatures[MAX_DIFFICULTY - 1];
    for (uint32 i = 0; i < sCreatureStorage.GetMaxEntry(); ++i)
    {
        if (CreatureInfo const* cInfo = sCreatureStorage.LookupEntry<CreatureInfo>(i))
            for (uint32 diff = 0; diff < MAX_DIFFICULTY - 1; ++diff)
            {
                if (cInfo->DifficultyEntry[diff])
                {
                    difficultyCreatures[diff].insert(cInfo->DifficultyEntry[diff]);
                }
            }
    }

    // build single time for check spawnmask
    std::map<uint32, uint32> spawnMasks;
    for (uint32 i = 0; i < sMapStore.GetNumRows(); ++i)
    {
        if (sMapStore.LookupEntry(i))
            for (int k = 0; k < MAX_DIFFICULTY; ++k)
            {
                if (GetMapDifficultyData(i, Difficulty(k)))
                {
                    spawnMasks[i] |= (1 << k);
                }
            }
    }

    BarGoLink bar(result->GetRowCount());

    const uint32 lwAnchorMask = sWorld.getConfig(CONFIG_UINT32_LIVINGWORLD_ANCHOR_MASK);
    uint32 lwWorldBossLeaderCount = 0;
    uint32 lwFlightMasterCount = 0;
    uint32 lwSettlementDefenderCount = 0;
    uint32 lwAnchorTotal = 0;

    do
    {
        Field* fields = result->Fetch();
        bar.step();

        uint32 guid         = fields[ 0].GetUInt32();
        uint32 entry        = fields[ 1].GetUInt32();

        if (DisableMgr::IsDisabledFor(DISABLE_TYPE_CREATURE_SPAWN, guid))
        {
            sLog.outDebug("Creature guid %u (entry %u) spawning is disabled.", guid, entry);
            continue;
        }

        CreatureInfo const* cInfo = GetCreatureTemplate(entry);
        if (!cInfo)
        {
            sLog.outErrorDb("Table `creature` has creature (GUID: %u) with non existing creature entry %u, skipped.", guid, entry);
            continue;
        }

        CreatureData& data = mCreatureDataMap[guid];

        data.id                 = entry;
        data.mapid              = fields[ 2].GetUInt32();
        data.modelid_override   = fields[ 3].GetUInt32();
        data.equipmentId        = fields[ 4].GetUInt32();
        data.posX               = fields[ 5].GetFloat();
        data.posY               = fields[ 6].GetFloat();
        data.posZ               = fields[ 7].GetFloat();
        data.orientation        = fields[ 8].GetFloat();
        data.spawntimesecs      = fields[ 9].GetUInt32();
        data.spawndist          = fields[10].GetFloat();
        data.currentwaypoint    = fields[11].GetUInt32();
        data.curhealth          = fields[12].GetUInt32();
        data.curmana            = fields[13].GetUInt32();
        data.is_dead            = fields[14].GetBool();
        data.movementType       = fields[15].GetUInt8();
        data.spawnMask          = fields[16].GetUInt8();
        data.phaseMask          = fields[17].GetUInt16();
        int16 gameEvent         = fields[18].GetInt16();
        int16 GuidPoolId        = fields[19].GetInt16();
        int16 EntryPoolId       = fields[20].GetInt16();

        MapEntry const* mapEntry = sMapStore.LookupEntry(data.mapid);
        if (!mapEntry)
        {
            sLog.outErrorDb("Table `creature` have creature (GUID: %u) that spawned at nonexistent map (Id: %u), skipped.", guid, data.mapid);
            continue;
        }

        if (data.spawnMask & ~spawnMasks[data.mapid])
            sLog.outErrorDb("Table `creature` have creature (GUID: %u) that have wrong spawn mask %u including not supported difficulty modes for map (Id: %u).", guid, data.spawnMask, data.mapid);

        bool ok = true;
        for (uint32 diff = 0; diff < MAX_DIFFICULTY - 1 && ok; ++diff)
        {
            if (difficultyCreatures[diff].find(data.id) != difficultyCreatures[diff].end())
            {
                sLog.outErrorDb("Table `creature` have creature (GUID: %u) that listed as difficulty %u template (entry: %u) in `creature_template`, skipped.",
                                guid, diff + 1, data.id);
                ok = false;
            }
        }
        if (!ok)
        {
            continue;
        }

        if (data.modelid_override > 0 && !sCreatureDisplayInfoStore.LookupEntry(data.modelid_override))
        {
            sLog.outErrorDb("Table `creature` GUID %u (entry %u) has model for nonexistent model id (%u), set to 0.", guid, data.id, data.modelid_override);
            data.modelid_override = 0;
        }

        if (data.equipmentId > 0)                           // -1 no equipment, 0 use default
        {
            if (!GetEquipmentInfo(data.equipmentId))
            {
                sLog.outErrorDb("Table `creature` have creature (Entry: %u) with equipment_id %u not found in table `creature_equip_template`, set to no equipment.", data.id, data.equipmentId);
                data.equipmentId = -1;
            }
        }

        if (cInfo->RegenerateStats & REGEN_FLAG_HEALTH && data.curhealth < cInfo->MinLevelHealth)
        {
            sLog.outErrorDb("Table `creature` have creature (GUID: %u Entry: %u) with `creature_template`.`RegenerateStats` & REGEN_FLAG_HEALTH and low current health (%u), `creature_template`.`minhealth`=%u.", guid, data.id, data.curhealth, cInfo->MinLevelHealth);
            data.curhealth = cInfo->MinLevelHealth;
        }

        if (cInfo->ExtraFlags & CREATURE_FLAG_EXTRA_INSTANCE_BIND)
        {
            if (!mapEntry || !mapEntry->IsDungeon())
                sLog.outErrorDb("Table `creature` have creature (GUID: %u Entry: %u) with `creature_template`.`ExtraFlags` including CREATURE_FLAG_EXTRA_INSTANCE_BIND (%u) but creature are not in instance.",
                                guid, data.id, CREATURE_FLAG_EXTRA_INSTANCE_BIND);
        }

        if (cInfo->ExtraFlags & CREATURE_FLAG_EXTRA_AGGRO_ZONE)
        {
            if (!mapEntry || !mapEntry->IsDungeon())
                sLog.outErrorDb("Table `creature` have creature (GUID: %u Entry: %u) with `creature_template`.`flags_extra` including CREATURE_FLAG_EXTRA_AGGRO_ZONE (%u) but creature are not in instance.",
                                guid, data.id, CREATURE_FLAG_EXTRA_AGGRO_ZONE);
        }

        if (data.curmana < cInfo->MinLevelMana)
        {
            sLog.outErrorDb("Table `creature` have creature (GUID: %u Entry: %u) with low current mana (%u), `creature_template`.`minmana`=%u.", guid, data.id, data.curmana, cInfo->MinLevelMana);
            data.curmana = cInfo->MinLevelMana;
        }

        if (data.spawndist < 0.0f)
        {
            sLog.outErrorDb("Table `creature` have creature (GUID: %u Entry: %u) with `spawndist`< 0, set to 0.", guid, data.id);
            data.spawndist = 0.0f;
        }
        else if (data.movementType == RANDOM_MOTION_TYPE)
        {
            if (data.spawndist == 0.0f)
            {
                sLog.outErrorDb("Table `creature` have creature (GUID: %u Entry: %u) with `MovementType`=1 (random movement) but with `spawndist`=0, replace by idle movement type (0).", guid, data.id);
                data.movementType = IDLE_MOTION_TYPE;
            }
        }
        else if (data.movementType == IDLE_MOTION_TYPE)
        {
            if (data.spawndist != 0.0f)
            {
                sLog.outErrorDb("Table `creature` have creature (GUID: %u Entry: %u) with `MovementType`=0 (idle) have `spawndist`<>0, set to 0.", guid, data.id);
                data.spawndist = 0.0f;
            }
        }

        if (data.phaseMask == 0)
        {
            sLog.outErrorDb("Table `creature` have creature (GUID: %u Entry: %u) with `phaseMask`=0 (not visible for anyone), set to 1.", guid, data.id);
            data.phaseMask = 1;
        }

        if (gameEvent == 0 && GuidPoolId == 0 && EntryPoolId == 0) // if not this is to be managed by GameEvent System or Pool system
        {
            AddCreatureToGrid(guid, &data);

            const bool lwIsWaypoint = (data.movementType == WAYPOINT_MOTION_TYPE);
            uint32 lwCats = (GetLivingWorldAnchorCategories(cInfo, mapEntry)
                          |  GetLivingWorldDefenderCategory(cInfo, mapEntry, lwIsWaypoint)) & lwAnchorMask;
            if ((cInfo->ExtraFlags & CREATURE_FLAG_EXTRA_ACTIVE) || lwCats != 0)
            {
                BASIC_FILTER_LOG(LOG_FILTER_MAP_LOADING, "Adding `creature` with Active Flag: Map: %u, Guid %u", data.mapid, guid);
                m_activeCreatures.insert(ActiveCreatureGuidsOnMap::value_type(data.mapid, guid));

                if (lwCats != 0)
                {
                    ++lwAnchorTotal;
                    if (lwCats & LW_ANCHOR_WORLD_BOSS_OR_LEADER)
                    {
                        ++lwWorldBossLeaderCount;
                    }
                    if (lwCats & LW_ANCHOR_FLIGHT_MASTER)
                    {
                        ++lwFlightMasterCount;
                    }
                    if (lwCats & LW_ANCHOR_SETTLEMENT_DEFENDER)
                    {
                        ++lwSettlementDefenderCount;
                    }
                }
            }
        }

        ++count;
    }
    while (result->NextRow());

    delete result;

    sLog.outString(">> Loaded %zu creatures", mCreatureDataMap.size());
    sLog.outString("[LivingWorld] anchor policy mask=0x%X: world-boss/leaders=%u, flight-masters=%u, settlement-defenders=%u, total-anchors=%u (continent-only; instance spawns excluded)",
        lwAnchorMask, lwWorldBossLeaderCount, lwFlightMasterCount, lwSettlementDefenderCount, lwAnchorTotal);
    sLog.outString();
}

/**
 * @brief Adds a creature spawn GUID to the grid lookup for its map cell.
 *
 * @param guid The creature spawn GUID.
 * @param data The creature spawn data.
 */
void ObjectMgr::AddCreatureToGrid(uint32 guid, CreatureData const* data)
{
    uint8 mask = data->spawnMask;
    for (uint8 i = 0; mask != 0; ++i, mask >>= 1)
    {
        if (mask & 1)
        {
            CellPair cell_pair = MaNGOS::ComputeCellPair(data->posX, data->posY);
            uint32 cell_id = (cell_pair.y_coord * TOTAL_NUMBER_OF_CELLS_PER_MAP) + cell_pair.x_coord;

            CellObjectGuids& cell_guids = mMapObjectGuids[MAKE_PAIR32(data->mapid, i)][cell_id];
            cell_guids.creatures.insert(guid);
        }
    }
}

/**
 * @brief Removes a creature spawn GUID from the grid lookup for its map cell.
 *
 * @param guid The creature spawn GUID.
 * @param data The creature spawn data.
 */
void ObjectMgr::RemoveCreatureFromGrid(uint32 guid, CreatureData const* data)
{
    uint8 mask = data->spawnMask;
    for (uint8 i = 0; mask != 0; ++i, mask >>= 1)
    {
        if (mask & 1)
        {
            CellPair cell_pair = MaNGOS::ComputeCellPair(data->posX, data->posY);
            uint32 cell_id = (cell_pair.y_coord * TOTAL_NUMBER_OF_CELLS_PER_MAP) + cell_pair.x_coord;

            CellObjectGuids& cell_guids = mMapObjectGuids[MAKE_PAIR32(data->mapid, i)][cell_id];
            cell_guids.creatures.erase(guid);
        }
    }
}



