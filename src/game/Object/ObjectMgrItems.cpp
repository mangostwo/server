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
 * @file ObjectMgrItems.cpp
 * @brief Cohesion split of ObjectMgr.cpp -- item locale, prototype, convert,
 *        and required-target loaders. Same `ObjectMgr` class; no behaviour
 *        change. CMake `file(GLOB Object/*.cpp)` picks this file up
 *        automatically; ObjectMgr.h is unchanged.
 */

#include <string>
#include "Common/Locales.h"
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
/**
 * @brief Loads localized item names and descriptions.
 */
void ObjectMgr::LoadItemLocales()
{
    mItemLocaleMap.clear();                                 // need for reload case

    QueryResult* result = WorldDatabase.Query("SELECT `entry`,`name_loc1`,`description_loc1`,`name_loc2`,`description_loc2`,`name_loc3`,`description_loc3`,`name_loc4`,`description_loc4`,`name_loc5`,`description_loc5`,`name_loc6`,`description_loc6`,`name_loc7`,`description_loc7`,`name_loc8`,`description_loc8` FROM `locales_item`");

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();
        sLog.outString(">> Loaded 0 Item locale strings. DB table `locales_item` is empty.");
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();
        bar.step();

        uint32 entry = fields[0].GetUInt32();

        if (!GetItemPrototype(entry))
        {
            ERROR_DB_STRICT_LOG("Table `locales_item` has data for nonexistent item entry %u, skipped.", entry);
            continue;
        }

        ItemLocale& data = mItemLocaleMap[entry];

        for (int i = 1; i < MAX_LOCALE; ++i)
        {
            std::string str = fields[1 + 2 * (i - 1)].GetCppString();
            if (!str.empty())
            {
                int idx = GetOrNewIndexForLocale(LocaleConstant(i));
                if (idx >= 0)
                {
                    if ((int32)data.Name.size() <= idx)
                    {
                        data.Name.resize(idx + 1);
                    }

                    data.Name[idx] = str;
                }
            }

            str = fields[1 + 2 * (i - 1) + 1].GetCppString();
            if (!str.empty())
            {
                int idx = GetOrNewIndexForLocale(LocaleConstant(i));
                if (idx >= 0)
                {
                    if ((int32)data.Description.size() <= idx)
                    {
                        data.Description.resize(idx + 1);
                    }

                    data.Description[idx] = str;
                }
            }
        }
    }
    while (result->NextRow());

    delete result;

    sLog.outString(">> Loaded %zu Item locale strings", mItemLocaleMap.size());
    sLog.outString();
}

struct SQLItemLoader : public SQLStorageLoaderBase<SQLItemLoader, SQLStorage>
{
    template<class D>
    void convert_from_str(uint32 /*field_pos*/, char const* src, D& dst)
    {
        dst = D(sScriptMgr.GetScriptId(src));
    }
};

/**
 * @brief Loads item prototypes and validates core item metadata.
 */
void ObjectMgr::LoadItemPrototypes()
{
    SQLItemLoader loader;
    loader.Load(sItemStorage);
    sLog.outString(">> Loaded %u item prototypes", sItemStorage.GetRecordCount());
    sLog.outString();

    // check data correctness
    for (uint32 i = 1; i < sItemStorage.GetMaxEntry(); ++i)
    {
        ItemPrototype const* proto = sItemStorage.LookupEntry<ItemPrototype >(i);
        ItemEntry const* dbcitem = sItemStore.LookupEntry(i);
        if (!proto)
        {
            /* to many errors, and possible not all items really used in game
            if (dbcitem)
            {
                sLog.outErrorDb("Item (Entry: %u) doesn't exists in DB, but must exist.",i);
            }
            */
            continue;
        }

        if (dbcitem)
        {
            if (proto->Class != dbcitem->ClassID)
            {
                sLog.outErrorDb("Item (Entry: %u) not correct class %u, must be %u (still using DB value).", i, proto->Class, dbcitem->ClassID);
                // It safe let use Class from DB
            }
            /* disabled: have some strange wrong cases for Subclass values.
               for enable also uncomment Subclass field in ItemEntry structure and in Itemfmt[]
            if (proto->SubClass != dbcitem->SubClass)
            {
                sLog.outErrorDb("Item (Entry: %u) not correct (Class: %u, Sub: %u) pair, must be (Class: %u, Sub: %u) (still using DB value).",i,proto->Class,proto->SubClass,dbcitem->Class,dbcitem->SubClass);
                // It safe let use Subclass from DB
            }
            */

            if (proto->Unk0 != dbcitem->SoundOverrideSubclassID)
            {
                sLog.outErrorDb("Item (Entry: %u) not correct %i Unk0, must be %i (still using DB value).", i, proto->Unk0, dbcitem->SoundOverrideSubclassID);
                // It safe let use Unk0 from DB
            }

            if (proto->Material != dbcitem->Material)
            {
                sLog.outErrorDb("Item (Entry: %u) not correct %i material, must be %i (still using DB value).", i, proto->Material, dbcitem->Material);
                // It safe let use Material from DB
            }

            if (proto->InventoryType != dbcitem->InventoryType)
            {
                sLog.outErrorDb("Item (Entry: %u) not correct %u inventory type, must be %u (still using DB value).", i, proto->InventoryType, dbcitem->InventoryType);
                // It safe let use InventoryType from DB
            }

            if (proto->DisplayInfoID != dbcitem->DisplayInfoID)
            {
                sLog.outErrorDb("Item (Entry: %u) not correct %u display id, must be %u (using it).", i, proto->DisplayInfoID, dbcitem->DisplayInfoID);
                const_cast<ItemPrototype*>(proto)->DisplayInfoID = dbcitem->DisplayInfoID;
            }
            if (proto->Sheath != dbcitem->SheatheType)
            {
                sLog.outErrorDb("Item (Entry: %u) not correct %u sheath, must be %u  (using it).", i, proto->Sheath, dbcitem->SheatheType);
                const_cast<ItemPrototype*>(proto)->Sheath = dbcitem->SheatheType;
            }
        }
        else
        {
            sLog.outErrorDb("Item (Entry: %u) not correct (not listed in list of existing items).", i);
        }

        if (proto->Class >= MAX_ITEM_CLASS)
        {
            sLog.outErrorDb("Item (Entry: %u) has wrong Class value (%u)", i, proto->Class);
            const_cast<ItemPrototype*>(proto)->Class = ITEM_CLASS_MISC;
        }

        if (proto->SubClass >= MaxItemSubclassValues[proto->Class])
        {
            sLog.outErrorDb("Item (Entry: %u) has wrong Subclass value (%u) for class %u", i, proto->SubClass, proto->Class);
            const_cast<ItemPrototype*>(proto)->SubClass = 0;// exist for all item classes
        }

        if (proto->Quality >= MAX_ITEM_QUALITY)
        {
            sLog.outErrorDb("Item (Entry: %u) has wrong Quality value (%u)", i, proto->Quality);
            const_cast<ItemPrototype*>(proto)->Quality = ITEM_QUALITY_NORMAL;
        }

        if (proto->Flags2 & ITEM_FLAG2_HORDE_ONLY)
        {
            if (FactionEntry const* faction = sFactionStore.LookupEntry(HORDE))
                if ((proto->AllowableRace & faction->ReputationRaceMask[0]) == 0)
                    sLog.outErrorDb("Item (Entry: %u) have in `AllowableRace` races (%u) only not compatible with ITEM_FLAG2_HORDE_ONLY (%u) in Flags field, item any way will can't be equipped or use by this races.",
                                    i, proto->AllowableRace, ITEM_FLAG2_HORDE_ONLY);

            if (proto->Flags2 & ITEM_FLAG2_ALLIANCE_ONLY)
                sLog.outErrorDb("Item (Entry: %u) have in `Flags2` flags ITEM_FLAG2_ALLIANCE_ONLY (%u) and ITEM_FLAG2_HORDE_ONLY (%u) in Flags field, this is wrong combination.",
                                i, ITEM_FLAG2_ALLIANCE_ONLY, ITEM_FLAG2_HORDE_ONLY);
        }
        else if (proto->Flags2 & ITEM_FLAG2_ALLIANCE_ONLY)
        {
            if (FactionEntry const* faction = sFactionStore.LookupEntry(ALLIANCE))
                if ((proto->AllowableRace & faction->ReputationRaceMask[0]) == 0)
                    sLog.outErrorDb("Item (Entry: %u) have in `AllowableRace` races (%u) only not compatible with ITEM_FLAG2_ALLIANCE_ONLY (%u) in Flags field, item any way will can't be equipped or use by this races.",
                                    i, proto->AllowableRace, ITEM_FLAG2_ALLIANCE_ONLY);
        }

        if (proto->BuyCount <= 0)
        {
            sLog.outErrorDb("Item (Entry: %u) has wrong BuyCount value (%u), set to default(1).", i, proto->BuyCount);
            const_cast<ItemPrototype*>(proto)->BuyCount = 1;
        }

        if (proto->InventoryType >= MAX_INVTYPE)
        {
            sLog.outErrorDb("Item (Entry: %u) has wrong InventoryType value (%u)", i, proto->InventoryType);
            const_cast<ItemPrototype*>(proto)->InventoryType = INVTYPE_NON_EQUIP;
        }

        if (proto->InventoryType != INVTYPE_NON_EQUIP)
        {
            if (proto->Flags & ITEM_FLAG_LOOTABLE)
            {
                sLog.outErrorDb("Item container (Entry: %u) has not allowed for containers flag ITEM_FLAG_LOOTABLE (%u), flag removed.", i, ITEM_FLAG_LOOTABLE);
                const_cast<ItemPrototype*>(proto)->Flags &= ~ITEM_FLAG_LOOTABLE;
            }

            if (proto->Flags & ITEM_FLAG_MILLABLE)
            {
                sLog.outErrorDb("Item container (Entry: %u) has not allowed for containers flag ITEM_FLAG_MILLABLE (%u), flag removed.", i, ITEM_FLAG_MILLABLE);
                const_cast<ItemPrototype*>(proto)->Flags &= ~ITEM_FLAG_MILLABLE;
            }

            if (proto->Flags & ITEM_FLAG_PROSPECTABLE)
            {
                sLog.outErrorDb("Item container (Entry: %u) has not allowed for containers flag ITEM_FLAG_PROSPECTABLE (%u), flag removed.", i, ITEM_FLAG_PROSPECTABLE);
                const_cast<ItemPrototype*>(proto)->Flags &= ~ITEM_FLAG_PROSPECTABLE;
            }
        }
        else if (proto->InventoryType != INVTYPE_BAG)
        {
            if (proto->ContainerSlots > 0)
            {
                sLog.outErrorDb("Non-container item (Entry: %u) has ContainerSlots (%u), set to 0.", i, proto->ContainerSlots);
                const_cast<ItemPrototype*>(proto)->ContainerSlots = 0;
            }
        }

        if (proto->RequiredSkill >= MAX_SKILL_TYPE)
        {
            sLog.outErrorDb("Item (Entry: %u) has wrong RequiredSkill value (%u)", i, proto->RequiredSkill);
            const_cast<ItemPrototype*>(proto)->RequiredSkill = 0;
        }

        {
            // can be used in equip slot, as page read use in inventory, or spell casting at use
            bool req = proto->InventoryType != INVTYPE_NON_EQUIP || proto->PageText;
            if (!req)
            {
                for (int j = 0; j < MAX_ITEM_PROTO_SPELLS; ++j)
                {
                    if (proto->Spells[j].SpellId)
                    {
                        req = true;
                        break;
                    }
                }
            }

            if (req)
            {
                if (!(proto->AllowableClass & CLASSMASK_ALL_PLAYABLE))
                {
                    sLog.outErrorDb("Item (Entry: %u) not have in `AllowableClass` any playable classes (%u) and can't be equipped or use.", i, proto->AllowableClass);
                }

                if (!(proto->AllowableRace & RACEMASK_ALL_PLAYABLE))
                {
                    sLog.outErrorDb("Item (Entry: %u) not have in `AllowableRace` any playable races (%u) and can't be equipped or use.", i, proto->AllowableRace);
                }
            }
        }

        if (proto->RequiredSpell && !sSpellStore.LookupEntry(proto->RequiredSpell))
        {
            sLog.outErrorDb("Item (Entry: %u) have wrong (nonexistent) spell in RequiredSpell (%u)", i, proto->RequiredSpell);
            const_cast<ItemPrototype*>(proto)->RequiredSpell = 0;
        }

        if (proto->RequiredReputationRank >= MAX_REPUTATION_RANK)
        {
            sLog.outErrorDb("Item (Entry: %u) has wrong reputation rank in RequiredReputationRank (%u), item can't be used.", i, proto->RequiredReputationRank);
        }

        if (proto->RequiredReputationFaction)
        {
            if (!sFactionStore.LookupEntry(proto->RequiredReputationFaction))
            {
                sLog.outErrorDb("Item (Entry: %u) has wrong (not existing) faction in RequiredReputationFaction (%u)", i, proto->RequiredReputationFaction);
                const_cast<ItemPrototype*>(proto)->RequiredReputationFaction = 0;
            }

            if (proto->RequiredReputationRank == MIN_REPUTATION_RANK)
            {
                sLog.outErrorDb("Item (Entry: %u) has min. reputation rank in RequiredReputationRank (0) but RequiredReputationFaction > 0, faction setting is useless.", i);
            }
        }
        else if (proto->RequiredReputationRank > MIN_REPUTATION_RANK)
        {
            sLog.outErrorDb("Item (Entry: %u) has RequiredReputationFaction ==0 but RequiredReputationRank > 0, rank setting is useless.", i);
        }

        if (proto->MaxCount < -1)
        {
            sLog.outErrorDb("Item (Entry: %u) has too large negative in maxcount (%i), replace by value (-1) no storing limits.", i, proto->MaxCount);
            const_cast<ItemPrototype*>(proto)->MaxCount = -1;
        }

        if (proto->Stackable == 0)
        {
            sLog.outErrorDb("Item (Entry: %u) has wrong value in stackable (%u), replace by default 1.", i, proto->Stackable);
            const_cast<ItemPrototype*>(proto)->Stackable = 1;
        }
        else if (proto->Stackable < -1)
        {
            sLog.outErrorDb("Item (Entry: %u) has too large negative in stackable (%i), replace by value (-1) no stacking limits.", i, proto->Stackable);
            const_cast<ItemPrototype*>(proto)->Stackable = -1;
        }
        else if (proto->Stackable > 1000)
        {
            sLog.outErrorDb("Item (Entry: %u) has too large value in stackable (%u), replace by hardcoded upper limit (1000).", i, proto->Stackable);
            const_cast<ItemPrototype*>(proto)->Stackable = 1000;
        }

        if (proto->ContainerSlots)
        {
            if (proto->ContainerSlots > MAX_BAG_SIZE)
            {
                sLog.outErrorDb("Item (Entry: %u) has too large value in ContainerSlots (%u), replace by hardcoded limit (%u).", i, proto->ContainerSlots, MAX_BAG_SIZE);
                const_cast<ItemPrototype*>(proto)->ContainerSlots = MAX_BAG_SIZE;
            }
        }

        if (proto->StatsCount > MAX_ITEM_PROTO_STATS)
        {
            sLog.outErrorDb("Item (Entry: %u) has too large value in statscount (%u), replace by hardcoded limit (%u).", i, proto->StatsCount, MAX_ITEM_PROTO_STATS);
            const_cast<ItemPrototype*>(proto)->StatsCount = MAX_ITEM_PROTO_STATS;
        }

        for (int j = 0; j < MAX_ITEM_PROTO_STATS; ++j)
        {
            // for ItemStatValue != 0
            if (proto->ItemStat[j].ItemStatValue && proto->ItemStat[j].ItemStatType >= MAX_ITEM_MOD)
            {
                sLog.outErrorDb("Item (Entry: %u) has wrong stat_type%d (%u)", i, j + 1, proto->ItemStat[j].ItemStatType);
                const_cast<ItemPrototype*>(proto)->ItemStat[j].ItemStatType = 0;
            }

            switch (proto->ItemStat[j].ItemStatType)
            {
                case ITEM_MOD_SPELL_HEALING_DONE:
                case ITEM_MOD_SPELL_DAMAGE_DONE:
                    sLog.outErrorDb("Item (Entry: %u) has deprecated stat_type%d (%u)", i, j + 1, proto->ItemStat[j].ItemStatType);
                    break;
                default:
                    break;
            }
        }

        for (int j = 0; j < MAX_ITEM_PROTO_DAMAGES; ++j)
        {
            if (proto->Damage[j].DamageType >= MAX_SPELL_SCHOOL)
            {
                sLog.outErrorDb("Item (Entry: %u) has wrong dmg_type%d (%u)", i, j + 1, proto->Damage[j].DamageType);
                const_cast<ItemPrototype*>(proto)->Damage[j].DamageType = 0;
            }
        }

        // special format
        if ((proto->Spells[0].SpellId == SPELL_ID_GENERIC_LEARN) || (proto->Spells[0].SpellId == SPELL_ID_GENERIC_LEARN_PET))
        {
            // spell_1
            if (proto->Spells[0].SpellTrigger != ITEM_SPELLTRIGGER_ON_USE)
            {
                sLog.outErrorDb("Item (Entry: %u) has wrong item spell trigger value in spelltrigger_%d (%u) for special learning format", i, 0 + 1, proto->Spells[0].SpellTrigger);
                const_cast<ItemPrototype*>(proto)->Spells[0].SpellId = 0;
                const_cast<ItemPrototype*>(proto)->Spells[0].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
                const_cast<ItemPrototype*>(proto)->Spells[1].SpellId = 0;
                const_cast<ItemPrototype*>(proto)->Spells[1].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
            }

            // spell_2 have learning spell
            if (proto->Spells[1].SpellTrigger != ITEM_SPELLTRIGGER_LEARN_SPELL_ID)
            {
                sLog.outErrorDb("Item (Entry: %u) has wrong item spell trigger value in spelltrigger_%d (%u) for special learning format.", i, 1 + 1, proto->Spells[1].SpellTrigger);
                const_cast<ItemPrototype*>(proto)->Spells[0].SpellId = 0;
                const_cast<ItemPrototype*>(proto)->Spells[1].SpellId = 0;
                const_cast<ItemPrototype*>(proto)->Spells[1].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
            }
            else if (!proto->Spells[1].SpellId)
            {
                sLog.outErrorDb("Item (Entry: %u) not has expected spell in spellid_%d in special learning format.", i, 1 + 1);
                const_cast<ItemPrototype*>(proto)->Spells[0].SpellId = 0;
                const_cast<ItemPrototype*>(proto)->Spells[1].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
            }
            else
            {
                SpellEntry const* spellInfo = sSpellStore.LookupEntry(proto->Spells[1].SpellId);
                if (!spellInfo)
                {
                    sLog.outErrorDb("Item (Entry: %u) has wrong (not existing) spell in spellid_%d (%u)", i, 1 + 1, proto->Spells[1].SpellId);
                    const_cast<ItemPrototype*>(proto)->Spells[0].SpellId = 0;
                    const_cast<ItemPrototype*>(proto)->Spells[1].SpellId = 0;
                    const_cast<ItemPrototype*>(proto)->Spells[1].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
                }
                // allowed only in special format
                else if ((proto->Spells[1].SpellId == SPELL_ID_GENERIC_LEARN) || (proto->Spells[1].SpellId == SPELL_ID_GENERIC_LEARN_PET))
                {
                    sLog.outErrorDb("Item (Entry: %u) has broken spell in spellid_%d (%u)", i, 1 + 1, proto->Spells[1].SpellId);
                    const_cast<ItemPrototype*>(proto)->Spells[0].SpellId = 0;
                    const_cast<ItemPrototype*>(proto)->Spells[1].SpellId = 0;
                    const_cast<ItemPrototype*>(proto)->Spells[1].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
                }
            }

            // spell_3*,spell_4*,spell_5* is empty
            for (int j = 2; j < MAX_ITEM_PROTO_SPELLS; ++j)
            {
                if (proto->Spells[j].SpellTrigger != ITEM_SPELLTRIGGER_ON_USE)
                {
                    sLog.outErrorDb("Item (Entry: %u) has wrong item spell trigger value in spelltrigger_%d (%u)", i, j + 1, proto->Spells[j].SpellTrigger);
                    const_cast<ItemPrototype*>(proto)->Spells[j].SpellId = 0;
                    const_cast<ItemPrototype*>(proto)->Spells[j].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
                }
                else if (proto->Spells[j].SpellId != 0)
                {
                    sLog.outErrorDb("Item (Entry: %u) has wrong spell in spellid_%d (%u) for learning special format", i, j + 1, proto->Spells[j].SpellId);
                    const_cast<ItemPrototype*>(proto)->Spells[j].SpellId = 0;
                }
            }
        }
        // normal spell list
        else
        {
            for (int j = 0; j < MAX_ITEM_PROTO_SPELLS; ++j)
            {
                if (DisableMgr::IsDisabledFor(DISABLE_TYPE_SPELL, proto->Spells[j].SpellId))
                {
                    DEBUG_LOG("Spell %u on item %u (%s) is disabled.", proto->Spells[j].SpellId, proto->ItemId, proto->Name1);
                    const_cast<ItemPrototype*>(proto)->Spells[j].SpellId = 0;
                    continue;
                }

                if (proto->Spells[j].SpellTrigger >= MAX_ITEM_SPELLTRIGGER || proto->Spells[j].SpellTrigger == ITEM_SPELLTRIGGER_LEARN_SPELL_ID)
                {
                    sLog.outErrorDb("Item (Entry: %u) has wrong item spell trigger value in spelltrigger_%d (%u)", i, j + 1, proto->Spells[j].SpellTrigger);
                    const_cast<ItemPrototype*>(proto)->Spells[j].SpellId = 0;
                    const_cast<ItemPrototype*>(proto)->Spells[j].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
                }
                // on hit can be sued only at weapon
                else if (proto->Spells[j].SpellTrigger == ITEM_SPELLTRIGGER_CHANCE_ON_HIT)
                {
                    if (proto->Class != ITEM_CLASS_WEAPON)
                    {
                        sLog.outErrorDb("Item (Entry: %u) isn't weapon (Class: %u) but has on hit spelltrigger_%d (%u), it will not triggered.", i, proto->Class, j + 1, proto->Spells[j].SpellTrigger);
                    }
                }

                if (proto->Spells[j].SpellId)
                {
                    SpellEntry const* spellInfo = sSpellStore.LookupEntry(proto->Spells[j].SpellId);
                    if (!spellInfo)
                    {
                        sLog.outErrorDb("Item (Entry: %u) has wrong (not existing) spell in spellid_%d (%u)", i, j + 1, proto->Spells[j].SpellId);
                        const_cast<ItemPrototype*>(proto)->Spells[j].SpellId = 0;
                    }
                    // allowed only in special format
                    else if ((proto->Spells[j].SpellId == SPELL_ID_GENERIC_LEARN) || (proto->Spells[j].SpellId == SPELL_ID_GENERIC_LEARN_PET))
                    {
                        sLog.outErrorDb("Item (Entry: %u) has broken spell in spellid_%d (%u)", i, j + 1, proto->Spells[j].SpellId);
                        const_cast<ItemPrototype*>(proto)->Spells[j].SpellId = 0;
                    }
                }
            }
        }

        if (proto->Bonding >= MAX_BIND_TYPE)
        {
            sLog.outErrorDb("Item (Entry: %u) has wrong Bonding value (%u)", i, proto->Bonding);
        }

        if (proto->PageText)
        {
            if (!sPageTextStore.LookupEntry<PageText>(proto->PageText))
            {
                sLog.outErrorDb("Item (Entry: %u) has non existing first page (Id:%u)", i, proto->PageText);
            }
        }

        if (proto->LockID && !sLockStore.LookupEntry(proto->LockID))
        {
            sLog.outErrorDb("Item (Entry: %u) has wrong LockID (%u)", i, proto->LockID);
        }

        if (proto->Sheath >= MAX_SHEATHETYPE)
        {
            sLog.outErrorDb("Item (Entry: %u) has wrong Sheath (%u)", i, proto->Sheath);
            const_cast<ItemPrototype*>(proto)->Sheath = SHEATHETYPE_NONE;
        }

        if (proto->RandomProperty && !sItemRandomPropertiesStore.LookupEntry(GetItemEnchantMod(proto->RandomProperty)))
        {
            sLog.outErrorDb("Item (Entry: %u) has unknown (wrong or not listed in `item_enchantment_template`) RandomProperty (%u)", i, proto->RandomProperty);
            const_cast<ItemPrototype*>(proto)->RandomProperty = 0;
        }

        if (proto->RandomSuffix && !sItemRandomSuffixStore.LookupEntry(GetItemEnchantMod(proto->RandomSuffix)))
        {
            sLog.outErrorDb("Item (Entry: %u) has wrong RandomSuffix (%u)", i, proto->RandomSuffix);
            const_cast<ItemPrototype*>(proto)->RandomSuffix = 0;
        }

        // item can have not null only one from field values
        if (proto->RandomProperty && proto->RandomSuffix)
        {
            sLog.outErrorDb("Item (Entry: %u) have RandomProperty==%u and RandomSuffix==%u, but must have one from field = 0",
                            proto->ItemId, proto->RandomProperty, proto->RandomSuffix);
            const_cast<ItemPrototype*>(proto)->RandomSuffix = 0;
        }

        if (proto->ItemSet && !sItemSetStore.LookupEntry(proto->ItemSet))
        {
            sLog.outErrorDb("Item (Entry: %u) have wrong ItemSet (%u)", i, proto->ItemSet);
            const_cast<ItemPrototype*>(proto)->ItemSet = 0;
        }

        if (proto->Area && !GetAreaEntryByAreaID(proto->Area))
        {
            sLog.outErrorDb("Item (Entry: %u) has wrong Area (%u)", i, proto->Area);
        }

        if (proto->Map && !sMapStore.LookupEntry(proto->Map))
        {
            sLog.outErrorDb("Item (Entry: %u) has wrong Map (%u)", i, proto->Map);
        }

        if (proto->BagFamily)
        {
            // check bits
            for (uint32 j = 0; j < sizeof(proto->BagFamily) * 8; ++j)
            {
                uint32 mask = 1 << j;
                if (!(proto->BagFamily & mask))
                {
                    continue;
                }

                ItemBagFamilyEntry const* bf = sItemBagFamilyStore.LookupEntry(j + 1);
                if (!bf)
                {
                    sLog.outErrorDb("Item (Entry: %u) has bag family bit set not listed in ItemBagFamily.dbc, remove bit", i);
                    const_cast<ItemPrototype*>(proto)->BagFamily &= ~mask;
                    continue;
                }

                if (BAG_FAMILY_MASK_CURRENCY_TOKENS & mask)
                {
                    CurrencyTypesEntry const* ctEntry = sCurrencyTypesStore.LookupEntry(proto->ItemId);
                    if (!ctEntry)
                    {
                        sLog.outErrorDb("Item (Entry: %u) has currency bag family bit set in BagFamily but not listed in CurrencyTypes.dbc, remove bit", i);
                        const_cast<ItemPrototype*>(proto)->BagFamily &= ~mask;
                    }
                }
            }
        }

        if (proto->TotemCategory && !sTotemCategoryStore.LookupEntry(proto->TotemCategory))
        {
            sLog.outErrorDb("Item (Entry: %u) has wrong TotemCategory (%u)", i, proto->TotemCategory);
        }

        for (int j = 0; j < MAX_ITEM_PROTO_SOCKETS; ++j)
        {
            if (proto->Socket[j].Color && (proto->Socket[j].Color & SOCKET_COLOR_ALL) != proto->Socket[j].Color)
            {
                sLog.outErrorDb("Item (Entry: %u) has wrong socketColor_%d (%u)", i, j + 1, proto->Socket[j].Color);
                const_cast<ItemPrototype*>(proto)->Socket[j].Color = 0;
            }
        }

        if (proto->GemProperties && !sGemPropertiesStore.LookupEntry(proto->GemProperties))
        {
            sLog.outErrorDb("Item (Entry: %u) has wrong GemProperties (%u)", i, proto->GemProperties);
        }

        if (proto->RequiredDisenchantSkill < -1)
        {
            sLog.outErrorDb("Item (Entry: %u) has wrong RequiredDisenchantSkill (%i), set to (-1).", i, proto->RequiredDisenchantSkill);
            const_cast<ItemPrototype*>(proto)->RequiredDisenchantSkill = -1;
        }
        else if (proto->RequiredDisenchantSkill != -1)
        {
            if (proto->Quality > ITEM_QUALITY_EPIC || proto->Quality < ITEM_QUALITY_UNCOMMON)
            {
                ERROR_DB_STRICT_LOG("Item (Entry: %u) has unexpected RequiredDisenchantSkill (%u) for non-disenchantable quality (%u), reset it.",
                                    i, proto->RequiredDisenchantSkill, proto->Quality);
                const_cast<ItemPrototype*>(proto)->RequiredDisenchantSkill = -1;
            }
            else if (proto->Class != ITEM_CLASS_WEAPON && proto->Class != ITEM_CLASS_ARMOR)
            {
                // some wrong data in wdb for unused items
                ERROR_DB_STRICT_LOG("Item (Entry: %u) has unexpected RequiredDisenchantSkill (%u) for non-disenchantable item class (%u), reset it.",
                                    i, proto->RequiredDisenchantSkill, proto->Class);
                const_cast<ItemPrototype*>(proto)->RequiredDisenchantSkill = -1;
            }
        }

        if (proto->DisenchantID)
        {
            if (proto->Quality > ITEM_QUALITY_EPIC || proto->Quality < ITEM_QUALITY_UNCOMMON)
            {
                sLog.outErrorDb("Item (Entry: %u) has wrong quality (%u) for disenchanting, remove disenchanting loot id.", i, proto->Quality);
                const_cast<ItemPrototype*>(proto)->DisenchantID = 0;
            }
            else if (proto->Class != ITEM_CLASS_WEAPON && proto->Class != ITEM_CLASS_ARMOR)
            {
                sLog.outErrorDb("Item (Entry: %u) has wrong item class (%u) for disenchanting, remove disenchanting loot id.", i, proto->Class);
                const_cast<ItemPrototype*>(proto)->DisenchantID = 0;
            }
            else if (proto->RequiredDisenchantSkill < 0)
            {
                sLog.outErrorDb("Item (Entry: %u) marked as non-disenchantable by RequiredDisenchantSkill == -1, remove disenchanting loot id.", i);
                const_cast<ItemPrototype*>(proto)->DisenchantID = 0;
            }
        }
        else
        {
            // lot DB cases
            if (proto->RequiredDisenchantSkill >= 0)
            {
                ERROR_DB_STRICT_LOG("Item (Entry: %u) marked as disenchantable by RequiredDisenchantSkill, but not have disenchanting loot id.", i);
            }
        }

        if (proto->FoodType >= MAX_PET_DIET)
        {
            sLog.outErrorDb("Item (Entry: %u) has wrong FoodType value (%u)", i, proto->FoodType);
            const_cast<ItemPrototype*>(proto)->FoodType = 0;
        }

        if (proto->ItemLimitCategory && !sItemLimitCategoryStore.LookupEntry(proto->ItemLimitCategory))
        {
            sLog.outErrorDb("Item (Entry: %u) has wrong LimitCategory value (%u)", i, proto->ItemLimitCategory);
            const_cast<ItemPrototype*>(proto)->ItemLimitCategory = 0;
        }

        if (proto->HolidayId && !sHolidaysStore.LookupEntry(proto->HolidayId))
        {
            sLog.outErrorDb("Item (Entry: %u) has wrong HolidayId value (%u)", i, proto->HolidayId);
            const_cast<ItemPrototype*>(proto)->HolidayId = 0;
        }

        if (proto->ExtraFlags)
        {
            if (proto->ExtraFlags & ~ITEM_EXTRA_ALL)
            {
                sLog.outErrorDb("Item (Entry: %u) has wrong ExtraFlags (%u) with unused bits set", i, proto->ExtraFlags);
            }

            if (proto->ExtraFlags & ITEM_EXTRA_NON_CONSUMABLE)
            {
                bool can_be_need = false;
                for (int j = 0; j < MAX_ITEM_PROTO_SPELLS; ++j)
                {
                    if (proto->Spells[j].SpellCharges < 0)
                    {
                        can_be_need = true;
                        break;
                    }
                }

                if (!can_be_need)
                {
                    sLog.outErrorDb("Item (Entry: %u) has redundant non-consumable flag in ExtraFlags, item not have negative charges", i);
                    const_cast<ItemPrototype*>(proto)->ExtraFlags &= ~ITEM_EXTRA_NON_CONSUMABLE;
                }
            }

            if (proto->ExtraFlags & ITEM_EXTRA_REAL_TIME_DURATION)
            {
                if (proto->Duration == 0)
                {
                    sLog.outErrorDb("Item (Entry: %u) has redundant real-time duration flag in ExtraFlags, item not have duration", i);
                    const_cast<ItemPrototype*>(proto)->ExtraFlags &= ~ITEM_EXTRA_REAL_TIME_DURATION;
                }
            }
        }
    }

    // check some dbc referenced items (avoid duplicate reports)
    std::set<uint32> notFoundOutfit;
    for (uint32 i = 1; i < sCharStartOutfitStore.GetNumRows(); ++i)
    {
        CharStartOutfitEntry const* entry = sCharStartOutfitStore.LookupEntry(i);
        if (!entry)
        {
            continue;
        }

        for (int j = 0; j < MAX_OUTFIT_ITEMS; ++j)
        {
            if (entry->ItemID[j] <= 0)
            {
                continue;
            }

            uint32 item_id = entry->ItemID[j];

            if (!GetItemPrototype(item_id))
                if (item_id != 40582)                       // nonexistent item by default but referenced in DBC, skip it from errors
                {
                    notFoundOutfit.insert(item_id);
                }
        }
    }

    for (std::set<uint32>::const_iterator itr = notFoundOutfit.begin(); itr != notFoundOutfit.end(); ++itr)
    {
        sLog.outErrorDb("Item (Entry: %u) not exist in `item_template` but referenced in `CharStartOutfit.dbc`", *itr);
    }
}

void ObjectMgr::LoadItemConverts()
{
    m_ItemConvert.clear();                                  // needed for reload case

    uint32 count = 0;

    QueryResult* result = WorldDatabase.Query("SELECT `entry`,`item` FROM `item_convert`");

    if (!result)
    {
        BarGoLink bar(1);

        bar.step();

        sLog.outString();
        sLog.outErrorDb(">> Loaded 0 Item converts . DB table `item_convert` is empty.");
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();
        bar.step();

        uint32 itemEntry    = fields[0].GetUInt32();
        uint32 itemTargetId = fields[1].GetUInt32();

        ItemPrototype const* pItemEntryProto = sItemStorage.LookupEntry<ItemPrototype>(itemEntry);
        if (!pItemEntryProto)
        {
            sLog.outErrorDb("Table `item_convert`: Item %u not exist in `item_template`.", itemEntry);
            continue;
        }

        ItemPrototype const* pItemTargetProto = sItemStorage.LookupEntry<ItemPrototype>(itemTargetId);
        if (!pItemTargetProto)
        {
            sLog.outErrorDb("Table `item_convert`: Item target %u for original item %u not exist in `item_template`.", itemTargetId, itemEntry);
            continue;
        }

        // 2 cases when item convert used
        // Boa item with reputation requirement
        if ((!(pItemEntryProto->Flags & ITEM_FLAG_BOA) || !pItemEntryProto->RequiredReputationFaction) &&
                // convertion to another team/race
                (pItemTargetProto->AllowableRace & pItemEntryProto->AllowableRace))
        {
            sLog.outErrorDb("Table `item_convert` not appropriate item %u conversion to %u. Table can be used for BoA items requirement drop or for conversion to another race/team use.", itemEntry, itemTargetId);
            continue;
        }

        m_ItemConvert[itemEntry] = itemTargetId;

        ++count;
    }
    while (result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString(">> Loaded %u Item converts", count);
}

void ObjectMgr::LoadItemExpireConverts()
{
    m_ItemExpireConvert.clear();                            // needed for reload case

    uint32 count = 0;

    QueryResult* result = WorldDatabase.Query("SELECT `entry`,`item` FROM `item_expire_convert`");

    if (!result)
    {
        BarGoLink bar(1);

        bar.step();

        sLog.outString();
        sLog.outErrorDb(">> Loaded 0 Item expire converts . DB table `item_expire_convert` is empty.");
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();
        bar.step();

        uint32 itemEntry    = fields[0].GetUInt32();
        uint32 itemTargetId = fields[1].GetUInt32();

        ItemPrototype const* pItemEntryProto = sItemStorage.LookupEntry<ItemPrototype>(itemEntry);
        if (!pItemEntryProto)
        {
            sLog.outErrorDb("Table `item_expire_convert`: Item %u not exist in `item_template`.", itemEntry);
            continue;
        }

        ItemPrototype const* pItemTargetProto = sItemStorage.LookupEntry<ItemPrototype>(itemTargetId);
        if (!pItemTargetProto)
        {
            sLog.outErrorDb("Table `item_expire_convert`: Item target %u for original item %u not exist in `item_template`.", itemTargetId, itemEntry);
            continue;
        }

        // Expire convert possible only for items with duration
        if (pItemEntryProto->Duration == 0)
        {
            sLog.outErrorDb("Table `item_expire_convert` not appropriate item %u conversion to %u. Table can be used for items with duration.", itemEntry, itemTargetId);
            continue;
        }

        m_ItemExpireConvert[itemEntry] = itemTargetId;

        ++count;
    }
    while (result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString(">> Loaded %u Item expire converts", count);
}

/**
 * @brief Loads required target constraints for item use.
 */
void ObjectMgr::LoadItemRequiredTarget()
{
    m_ItemRequiredTarget.clear();                           // needed for reload case

    uint32 count = 0;

    QueryResult* result = WorldDatabase.Query("SELECT `entry`,`type`,`targetEntry` FROM `item_required_target`");

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();
        sLog.outErrorDb(">> Loaded 0 ItemRequiredTarget. DB table `item_required_target` is empty.");
        sLog.outString();
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();
        bar.step();

        uint32 uiItemId      = fields[0].GetUInt32();
        uint32 uiType        = fields[1].GetUInt32();
        uint32 uiTargetEntry = fields[2].GetUInt32();

        ItemPrototype const* pItemProto = sItemStorage.LookupEntry<ItemPrototype>(uiItemId);

        if (!pItemProto)
        {
            sLog.outErrorDb("Table `item_required_target`: Entry %u listed for TargetEntry %u does not exist in `item_template`.", uiItemId, uiTargetEntry);
            continue;
        }

        bool bIsItemSpellValid = false;

        for (int i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
        {
            if (SpellEntry const* pSpellInfo = sSpellStore.LookupEntry(pItemProto->Spells[i].SpellId))
            {
                if (pItemProto->Spells[i].SpellTrigger == ITEM_SPELLTRIGGER_ON_USE)
                {
                    SQLMultiStorage::SQLMSIteratorBounds<SpellTargetEntry> bounds = sSpellScriptTargetStorage.getBounds<SpellTargetEntry>(pSpellInfo->ID);
                    if (bounds.first != bounds.second)
                    {
                        break;
                    }

                    for (int j = 0; j < MAX_EFFECT_INDEX; ++j)
                    {
                        if (pSpellInfo->ImplicitTargetA[j] == TARGET_CHAIN_DAMAGE ||
                            pSpellInfo->ImplicitTargetB[j] == TARGET_CHAIN_DAMAGE ||
                            pSpellInfo->ImplicitTargetA[j] == TARGET_DUELVSPLAYER ||
                            pSpellInfo->ImplicitTargetB[j] == TARGET_DUELVSPLAYER)
                        {
                            bIsItemSpellValid = true;
                            break;
                        }
                    }
                    if (bIsItemSpellValid)
                    {
                        break;
                    }
                }
            }
        }

        if (!bIsItemSpellValid)
        {
            sLog.outErrorDb("Table `item_required_target`: Spell used by item %u does not have implicit target TARGET_CHAIN_DAMAGE(6), TARGET_DUELVSPLAYER(25), already listed in `spell_script_target` or doesn't have item spelltrigger.", uiItemId);
            continue;
        }

        if (!uiType || uiType > MAX_ITEM_REQ_TARGET_TYPE)
        {
            sLog.outErrorDb("Table `item_required_target`: Type %u for TargetEntry %u is incorrect.", uiType, uiTargetEntry);
            continue;
        }

        if (!uiTargetEntry)
        {
            sLog.outErrorDb("Table `item_required_target`: TargetEntry == 0 for Type (%u).", uiType);
            continue;
        }

        if (!sCreatureStorage.LookupEntry<CreatureInfo>(uiTargetEntry))
        {
            sLog.outErrorDb("Table `item_required_target`: creature template entry %u does not exist.", uiTargetEntry);
            continue;
        }

        m_ItemRequiredTarget.insert(ItemRequiredTargetMap::value_type(uiItemId, ItemRequiredTarget(ItemRequiredTargetType(uiType), uiTargetEntry)));

        ++count;
    }
    while (result->NextRow());

    delete result;

    sLog.outString(">> Loaded %u Item required targets", count);
    sLog.outString();
}

