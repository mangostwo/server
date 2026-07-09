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
 * @file SpellMgrPet.cpp
 * @brief Cohesion split of SpellMgr.cpp -- pet aura / level-up spell map / default pet spell loaders.
 */

#include "SpellMgr.h"
#include "SpellAuraDefines.h"
#include "ObjectMgr.h"
#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "ProgressBar.h"
#include "DBCStores.h"
#include "SQLStorages.h"

/**
 * @brief Loads spell pet aura mappings from the database.
 */
void SpellMgr::LoadSpellPetAuras()
{
    mSpellPetAuraMap.clear();                               // need for reload case

    uint32 count = 0;

    //                                                 0        1           2      3
    QueryResult* result = WorldDatabase.Query("SELECT `spell`, `effectId`, `pet`, `aura` FROM `spell_pet_auras`");
    if (!result)
    {
        BarGoLink bar(1);
        bar.step();
        sLog.outString(">> Loaded %u spell pet auras", count);
        sLog.outString();
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();

        bar.step();

        uint32 spell = fields[0].GetUInt32();
        SpellEffectIndex eff = SpellEffectIndex(fields[1].GetUInt32());
        uint32 pet = fields[2].GetUInt32();
        uint32 aura = fields[3].GetUInt32();

        if (eff >= MAX_EFFECT_INDEX)
        {
            sLog.outErrorDb("Spell %u listed in `spell_pet_auras` with wrong spell effect index (%u)", spell, eff);
            continue;
        }

        SpellPetAuraMap::iterator itr = mSpellPetAuraMap.find((spell << 8) + eff);
        if (itr != mSpellPetAuraMap.end())
        {
            itr->second.AddAura(pet, aura);
        }
        else
        {
            SpellEntry const* spellInfo = sSpellStore.LookupEntry(spell);
            if (!spellInfo)
            {
                sLog.outErrorDb("Spell %u listed in `spell_pet_auras` does not exist", spell);
                continue;
            }

            if (spellInfo->Effect[eff] != SPELL_EFFECT_DUMMY &&
                    (spellInfo->Effect[eff] != SPELL_EFFECT_APPLY_AURA ||
                     spellInfo->EffectAura[eff] != SPELL_AURA_DUMMY))
            {
                sLog.outError("Spell %u listed in `spell_pet_auras` does not have dummy aura or dummy effect", spell);
                continue;
            }

            SpellEntry const* spellInfo2 = sSpellStore.LookupEntry(aura);
            if (!spellInfo2)
            {
                sLog.outErrorDb("Aura %u listed in `spell_pet_auras` does not exist", aura);
                continue;
            }

            PetAura pa(pet, aura, spellInfo->ImplicitTargetA[eff] == TARGET_PET, spellInfo->CalculateSimpleValue(eff));
            mSpellPetAuraMap[(spell << 8) + eff] = pa;
        }

        ++count;
    }
    while (result->NextRow());

    delete result;

    sLog.outString(">> Loaded %u spell pet auras", count);
    sLog.outString();
}

void SpellMgr::LoadPetLevelupSpellMap()
{
    uint32 count = 0;
    uint32 family_count = 0;

    for (uint32 i = 0; i < sCreatureFamilyStore.GetNumRows(); ++i)
    {
        CreatureFamilyEntry const* creatureFamily = sCreatureFamilyStore.LookupEntry(i);
        if (!creatureFamily)                                // not exist
        {
            continue;
        }

        for (uint32 j = 0; j < sSkillLineAbilityStore.GetNumRows(); ++j)
        {
            SkillLineAbilityEntry const* skillLine = sSkillLineAbilityStore.LookupEntry(j);
            if (!skillLine)
            {
                continue;
            }

            if (skillLine->SkillLine != creatureFamily->SkillLine[0] &&
                    (!creatureFamily->SkillLine[1] || skillLine->SkillLine != creatureFamily->SkillLine[1]))
                continue;

            if (skillLine->AcquireMethod != ABILITY_LEARNED_ON_GET_RACE_OR_CLASS_SKILL)
            {
                continue;
            }

            SpellEntry const* spell = sSpellStore.LookupEntry(skillLine->Spell);
            if (!spell)                                     // not exist
            {
                continue;
            }

            PetLevelupSpellSet& spellSet = mPetLevelupSpellMap[creatureFamily->ID];
            if (spellSet.empty())
            {
                ++family_count;
            }

            spellSet.insert(PetLevelupSpellSet::value_type(spell->SpellLevel, spell->ID));
            ++count;
        }
    }

    sLog.outString(">> Loaded %u pet levelup and default spells for %u families", count, family_count);
    sLog.outString();
}

bool SpellMgr::LoadPetDefaultSpells_helper(CreatureInfo const* cInfo, PetDefaultSpellsEntry& petDefSpells)
{
    // skip empty list;
    bool have_spell = false;
    for (int j = 0; j < MAX_CREATURE_SPELL_DATA_SLOT; ++j)
    {
        if (petDefSpells.spellid[j])
        {
            have_spell = true;
            break;
        }
    }
    if (!have_spell)
    {
        return false;
    }

    // remove duplicates with levelupSpells if any
    if (PetLevelupSpellSet const* levelupSpells = cInfo->Family ? GetPetLevelupSpellList(cInfo->Family) : NULL)
    {
        for (int j = 0; j < MAX_CREATURE_SPELL_DATA_SLOT; ++j)
        {
            if (!petDefSpells.spellid[j])
            {
                continue;
            }

            for (PetLevelupSpellSet::const_iterator itr = levelupSpells->begin(); itr != levelupSpells->end(); ++itr)
            {
                if (itr->second == petDefSpells.spellid[j])
                {
                    petDefSpells.spellid[j] = 0;
                    break;
                }
            }
        }
    }

    // skip empty list;
    have_spell = false;
    for (int j = 0; j < MAX_CREATURE_SPELL_DATA_SLOT; ++j)
    {
        if (petDefSpells.spellid[j])
        {
            have_spell = true;
            break;
        }
    }

    return have_spell;
}

void SpellMgr::LoadPetDefaultSpells()
{
    MANGOS_ASSERT(MAX_CREATURE_SPELL_DATA_SLOT <= CREATURE_MAX_SPELLS);

    mPetDefaultSpellsMap.clear();

    uint32 countCreature = 0;
    uint32 countData = 0;

    for (uint32 i = 0; i < sCreatureStorage.GetMaxEntry(); ++i)
    {
        CreatureInfo const* cInfo = sCreatureStorage.LookupEntry<CreatureInfo>(i);
        if (!cInfo)
        {
            continue;
        }

        if (!cInfo->PetSpellDataId)
        {
            continue;
        }

        // for creature with PetSpellDataId get default pet spells from dbc
        CreatureSpellDataEntry const* spellDataEntry = sCreatureSpellDataStore.LookupEntry(cInfo->PetSpellDataId);
        if (!spellDataEntry)
        {
            continue;
        }

        int32 petSpellsId = -(int32)cInfo->PetSpellDataId;
        PetDefaultSpellsEntry petDefSpells;
        for (int j = 0; j < MAX_CREATURE_SPELL_DATA_SLOT; ++j)
        {
            petDefSpells.spellid[j] = spellDataEntry->Spells[j];
        }

        if (LoadPetDefaultSpells_helper(cInfo, petDefSpells))
        {
            mPetDefaultSpellsMap[petSpellsId] = petDefSpells;
            ++countData;
        }
    }

    // different summon spells
    for (uint32 i = 0; i < sSpellStore.GetNumRows(); ++i)
    {
        SpellEntry const* spellEntry = sSpellStore.LookupEntry(i);
        if (!spellEntry)
        {
            continue;
        }

        for (int k = 0; k < MAX_EFFECT_INDEX; ++k)
        {
            if (spellEntry->Effect[k] == SPELL_EFFECT_SUMMON || spellEntry->Effect[k] == SPELL_EFFECT_SUMMON_PET)
            {
                uint32 creature_id = spellEntry->EffectMiscValue[k];
                CreatureInfo const* cInfo = sCreatureStorage.LookupEntry<CreatureInfo>(creature_id);
                if (!cInfo)
                {
                    continue;
                }

                // already loaded
                if (cInfo->PetSpellDataId)
                {
                    continue;
                }

                // for creature without PetSpellDataId get default pet spells from creature_template
                int32 petSpellsId = cInfo->Entry;
                if (mPetDefaultSpellsMap.find(cInfo->Entry) != mPetDefaultSpellsMap.end())
                {
                    continue;
                }

                PetDefaultSpellsEntry petDefSpells;
                if (CreatureTemplateSpells const* templateSpells = sCreatureTemplateSpellsStorage.LookupEntry<CreatureTemplateSpells>(cInfo->Entry))
                    for (int j = 0; j < MAX_CREATURE_SPELL_DATA_SLOT; ++j)
                    {
                        petDefSpells.spellid[j] = templateSpells->spells[j];
                    }

                if (LoadPetDefaultSpells_helper(cInfo, petDefSpells))
                {
                    mPetDefaultSpellsMap[petSpellsId] = petDefSpells;
                    ++countCreature;
                }
            }
        }
    }

    sLog.outString(">> Loaded addition spells for %u pet spell data entries and %u summonable creature templates", countData, countCreature);
    sLog.outString();
}

/// Some checks for spells, to prevent adding deprecated/broken spells for trainers, spell book, etc
bool SpellMgr::IsSpellValid(SpellEntry const* spellInfo, Player* pl, bool msg)
{
    // not exist
    if (!spellInfo)
    {
        return false;
    }

    bool need_check_reagents = false;

    // check effects
    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        switch (spellInfo->Effect[i])
        {
            case SPELL_EFFECT_NONE:
                continue;

                // craft spell for crafting nonexistent item (break client recipes list show)
            case SPELL_EFFECT_CREATE_ITEM:
            case SPELL_EFFECT_CREATE_ITEM_2:
            {
                if (spellInfo->EffectItemType[i] == 0)
                {
                    // skip auto-loot crafting spells, its not need explicit item info (but have special fake items sometime)
                    if (!IsLootCraftingSpell(spellInfo))
                    {
                        if (msg)
                        {
                            if (pl)
                            {
                                ChatHandler(pl).PSendSysMessage("Craft spell %u not have create item entry.", spellInfo->ID);
                            }
                            else
                            {
                                sLog.outErrorDb("Craft spell %u not have create item entry.", spellInfo->ID);
                            }
                        }
                        return false;
                    }
                }
                // also possible IsLootCraftingSpell case but fake item must exist anyway
                else if (!ObjectMgr::GetItemPrototype(spellInfo->EffectItemType[i]))
                {
                    if (msg)
                    {
                        if (pl)
                        {
                            ChatHandler(pl).PSendSysMessage("Craft spell %u create item (Entry: %u) but item does not exist in item_template.", spellInfo->ID, spellInfo->EffectItemType[i]);
                        }
                        else
                        {
                            sLog.outErrorDb("Craft spell %u create item (Entry: %u) but item does not exist in item_template.", spellInfo->ID, spellInfo->EffectItemType[i]);
                        }
                    }
                    return false;
                }

                need_check_reagents = true;
                break;
            }
            case SPELL_EFFECT_LEARN_SPELL:
            {
                SpellEntry const* spellInfo2 = sSpellStore.LookupEntry(spellInfo->EffectTriggerSpell[i]);
                if (!IsSpellValid(spellInfo2, pl, msg))
                {
                    if (msg)
                    {
                        if (pl)
                        {
                            ChatHandler(pl).PSendSysMessage("Spell %u learn to broken spell %u, and then...", spellInfo->ID, spellInfo->EffectTriggerSpell[i]);
                        }
                        else
                        {
                            sLog.outErrorDb("Spell %u learn to invalid spell %u, and then...", spellInfo->ID, spellInfo->EffectTriggerSpell[i]);
                        }
                    }
                    return false;
                }
                break;
            }
        }
    }

    if (need_check_reagents)
    {
        for (int j = 0; j < MAX_SPELL_REAGENTS; ++j)
        {
            if (spellInfo->Reagent[j] > 0 && !ObjectMgr::GetItemPrototype(spellInfo->Reagent[j]))
            {
                if (msg)
                {
                    if (pl)
                    {
                        ChatHandler(pl).PSendSysMessage("Craft spell %u requires reagent item (Entry: %u) but item does not exist in item_template.", spellInfo->ID, spellInfo->Reagent[j]);
                    }
                    else
                    {
                        sLog.outErrorDb("Craft spell %u requires reagent item (Entry: %u) but item does not exist in item_template.", spellInfo->ID, spellInfo->Reagent[j]);
                    }
                }
                return false;
            }
        }
    }

    return true;
}
