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
 * @file PlayerTalent.cpp
 * @brief Cohesion split of Player.cpp -- talent learn/reset and talent-point accounting.
 *        Same `Player` class; no behaviour change.
 */

#include "Player.h"
#include "Language.h"
#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "Opcodes.h"
#include "SpellMgr.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "UpdateMask.h"
#include "SkillDiscovery.h"
#include "QuestDef.h"
#include "GossipDef.h"
#include "UpdateData.h"
#include "Channel.h"
#include "ChannelMgr.h"
#include "MapManager.h"
#include "MapPersistentStateMgr.h"
#include "InstanceData.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "CreatureAI.h"
#include "Formulas.h"
#include "Group.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Pet.h"
#include "Util.h"
#include "Transports.h"
#include "Weather.h"
#include "BattleGround/BattleGround.h"
#include "BattleGround/BattleGroundMgr.h"
#include "BattleGround/BattleGroundAV.h"
#include "OutdoorPvP/OutdoorPvP.h"
#include "ArenaTeam.h"
#include "Chat.h"
#include "revision_data.h"
#include "Database/DatabaseImpl.h"
#include "Spell.h"
#include "ScriptMgr.h"
#include "SocialMgr.h"
#include "AchievementMgr.h"
#include "Mail.h"
#include "SpellAuras.h"
#include "DBCStores.h"
#include "SQLStorages.h"
#include "Vehicle.h"
#include "Calendar.h"
#include "DisableMgr.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /* ENABLE_ELUNA */
#include <cmath>

/**
 * @brief Learns a specific talent rank if all requirements are satisfied.
 *
 * @param talentId The talent entry identifier.
 * @param talentRank The requested talent rank index.
 */
void Player::LearnTalent(uint32 talentId, uint32 talentRank)
{
    uint32 CurTalentPoints = GetFreeTalentPoints();

    if (CurTalentPoints == 0)
    {
        return;
    }

    if (talentRank >= MAX_TALENT_RANK)
    {
        return;
    }

    TalentEntry const* talentInfo = sTalentStore.LookupEntry(talentId);

    if (!talentInfo)
    {
        return;
    }

    TalentTabEntry const* talentTabInfo = sTalentTabStore.LookupEntry(talentInfo->TabID);

    if (!talentTabInfo)
    {
        return;
    }

    // prevent learn talent for different class (cheating)
    if ((getClassMask() & talentTabInfo->ClassMask) == 0)
    {
        return;
    }

    // find current max talent rank
    uint32 curtalent_maxrank = 0;
    if (PlayerTalent const* talent = GetKnownTalentById(talentId))
    {
        curtalent_maxrank = talent->currentRank + 1;
    }

    // we already have same or higher talent rank learned
    if (curtalent_maxrank >= (talentRank + 1))
    {
        return;
    }

    // check if we have enough talent points
    if (CurTalentPoints < (talentRank - curtalent_maxrank + 1))
    {
        return;
    }

    // Check if it requires another talent
    if (talentInfo->PrereqTalent_0 > 0)
    {
        if (TalentEntry const* depTalentInfo = sTalentStore.LookupEntry(talentInfo->PrereqTalent_0))
        {
            bool hasEnoughRank = false;
            PlayerTalentMap::iterator dependsOnTalent = m_talents[m_activeSpec].find(depTalentInfo->ID);
            if (dependsOnTalent != m_talents[m_activeSpec].end() && dependsOnTalent->second.state != PLAYERSPELL_REMOVED)
            {
                PlayerTalent depTalent = (*dependsOnTalent).second;
                if (depTalent.currentRank >= talentInfo->PrereqRank_0)
                {
                    hasEnoughRank = true;
                }
            }

            if (!hasEnoughRank)
            {
                return;
            }
        }
    }

    // Find out how many points we have in this field
    uint32 spentPoints = 0;

    uint32 tTab = talentInfo->TabID;
    if (talentInfo->TierID > 0)
    {
        for (PlayerTalentMap::const_iterator iter = m_talents[m_activeSpec].begin(); iter != m_talents[m_activeSpec].end(); ++iter)
        {
            if (iter->second.state != PLAYERSPELL_REMOVED && iter->second.talentEntry->TabID == tTab)
            {
                spentPoints += iter->second.currentRank + 1;
            }
        }
    }

    // not have required min points spent in talent tree
    if (spentPoints < (talentInfo->TierID * MAX_TALENT_RANK))
    {
        return;
    }

    // spell not set in talent.dbc
    uint32 spellid = talentInfo->SpellRank[talentRank];
    if (spellid == 0)
    {
        sLog.outError("Talent.dbc have for talent: %u Rank: %u spell id = 0", talentId, talentRank);
        return;
    }

    // already known
    if (HasSpell(spellid))
    {
        return;
    }

    // learn! (other talent ranks will unlearned at learning)
    learnSpell(spellid, false);
    DETAIL_LOG("TalentID: %u Rank: %u Spell: %u\n", talentId, talentRank, spellid);

#ifdef ENABLE_ELUNA
    if (Eluna* e = GetEluna())
    {
        e->OnLearnTalents(this, talentId, talentRank, spellid);
    }
#endif /*ENABLE_ELUNA*/
}

void Player::LearnPetTalent(ObjectGuid petGuid, uint32 talentId, uint32 talentRank)
{
    Pet* pet = GetPet();
    if (!pet)
    {
        return;
    }

    if (petGuid != pet->GetObjectGuid())
    {
        return;
    }

    uint32 CurTalentPoints = pet->GetFreeTalentPoints();

    if (CurTalentPoints == 0)
    {
        return;
    }

    if (talentRank >= MAX_PET_TALENT_RANK)
    {
        return;
    }

    TalentEntry const* talentInfo = sTalentStore.LookupEntry(talentId);

    if (!talentInfo)
    {
        return;
    }

    TalentTabEntry const* talentTabInfo = sTalentTabStore.LookupEntry(talentInfo->TabID);

    if (!talentTabInfo)
    {
        return;
    }

    CreatureInfo const* ci = pet->GetCreatureInfo();

    if (!ci)
    {
        return;
    }

    CreatureFamilyEntry const* pet_family = sCreatureFamilyStore.LookupEntry(ci->Family);

    if (!pet_family)
    {
        return;
    }

    if (pet_family->PetTalentType < 0)                      // not hunter pet
    {
        return;
    }

    // prevent learn talent for different family (cheating)
    if (!((1 << pet_family->PetTalentType) & talentTabInfo->PetTalentMask))
    {
        return;
    }

    // find current max talent rank
    int32 curtalent_maxrank = 0;
    for (int32 k = MAX_TALENT_RANK - 1; k > -1; --k)
    {
        if (talentInfo->SpellRank[k] && pet->HasSpell(talentInfo->SpellRank[k]))
        {
            curtalent_maxrank = k + 1;
            break;
        }
    }

    // we already have same or higher talent rank learned
    if (curtalent_maxrank >= int32(talentRank + 1))
    {
        return;
    }

    // check if we have enough talent points
    if (CurTalentPoints < (talentRank - curtalent_maxrank + 1))
    {
        return;
    }

    // Check if it requires another talent
    if (talentInfo->PrereqTalent_0 > 0)
    {
        if (TalentEntry const* depTalentInfo = sTalentStore.LookupEntry(talentInfo->PrereqTalent_0))
        {
            bool hasEnoughRank = false;
            for (int i = talentInfo->PrereqRank_0; i < MAX_TALENT_RANK; ++i)
            {
                if (depTalentInfo->SpellRank[i] != 0)
                    if (pet->HasSpell(depTalentInfo->SpellRank[i]))
                    {
                        hasEnoughRank = true;
                    }
            }
            if (!hasEnoughRank)
            {
                return;
            }
        }
    }

    // Find out how many points we have in this field
    uint32 spentPoints = 0;

    uint32 tTab = talentInfo->TabID;
    if (talentInfo->TierID > 0)
    {
        unsigned int numRows = sTalentStore.GetNumRows();
        for (unsigned int i = 0; i < numRows; ++i)          // Loop through all talents.
        {
            // Someday, someone needs to revamp
            const TalentEntry* tmpTalent = sTalentStore.LookupEntry(i);
            if (tmpTalent)                                  // the way talents are tracked
            {
                if (tmpTalent->TabID == tTab)
                {
                    for (int j = 0; j < MAX_TALENT_RANK; ++j)
                    {
                        if (tmpTalent->SpellRank[j] != 0)
                        {
                            if (pet->HasSpell(tmpTalent->SpellRank[j]))
                            {
                                spentPoints += j + 1;
                            }
                        }
                    }
                }
            }
        }
    }

    // not have required min points spent in talent tree
    if (spentPoints < (talentInfo->TierID * MAX_PET_TALENT_RANK))
    {
        return;
    }

    // spell not set in talent.dbc
    uint32 spellid = talentInfo->SpellRank[talentRank];
    if (spellid == 0)
    {
        sLog.outError("Talent.dbc have for talent: %u Rank: %u spell id = 0", talentId, talentRank);
        return;
    }

    // already known
    if (pet->HasSpell(spellid))
    {
        return;
    }

    // learn! (other talent ranks will unlearned at learning)
    pet->learnSpell(spellid);
    DETAIL_LOG("PetTalentID: %u Rank: %u Spell: %u\n", talentId, talentRank, spellid);
}

/**
 * @brief Refreshes stored fall tracking data when movement indicates a new fall state.
 *
 * @param minfo The current movement information.
 * @param opcode The movement opcode being processed.
 */
void Player::UpdateFallInformationIfNeed(MovementInfo const& minfo, uint16 opcode)
{
    if (m_lastFallTime >= minfo.GetFallTime() || m_lastFallZ <= minfo.GetPos()->z || opcode == MSG_MOVE_FALL_LAND)
    {
        SetFallInformation(minfo.GetFallTime(), minfo.GetPos()->z);
    }
}

/**
 * @brief Checks whether the player can currently see spell-click interaction on a creature.
 *
 * @param c The creature to evaluate.
 * @return True if spell-click should be visible; otherwise, false.
 */
bool Player::canSeeSpellClickOn(Creature const* c) const
{
    if (!c->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK))
    {
        return false;
    }

    SpellClickInfoMapBounds clickPair = sObjectMgr.GetSpellClickInfoMapBounds(c->GetEntry());
    for (SpellClickInfoMap::const_iterator itr = clickPair.first; itr != clickPair.second; ++itr)
    {
        if (itr->second.IsFitToRequirements(this, c))
        {
            return true;
        }
    }

    return false;
}

void Player::BuildPlayerTalentsInfoData(WorldPacket* data)
{
    *data << uint32(GetFreeTalentPoints());                 // unspentTalentPoints
    *data << uint8(m_specsCount);                           // talent group count (0, 1 or 2)
    *data << uint8(m_activeSpec);                           // talent group index (0 or 1)

    if (m_specsCount)
    {
        // loop through all specs (only 1 for now)
        for (uint32 specIdx = 0; specIdx < m_specsCount; ++specIdx)
        {
            uint8 talentIdCount = 0;
            size_t pos = data->wpos();
            *data << uint8(talentIdCount);                  // [PH], talentIdCount

            // find class talent tabs (all players have 3 talent tabs)
            uint32 const* talentTabIds = GetTalentTabPages(getClass());

            for (uint32 i = 0; i < 3; ++i)
            {
                uint32 talentTabId = talentTabIds[i];
                for (PlayerTalentMap::iterator iter = m_talents[specIdx].begin(); iter != m_talents[specIdx].end(); ++iter)
                {
                    PlayerTalent talent = (*iter).second;

                    if (talent.state == PLAYERSPELL_REMOVED)
                    {
                        continue;
                    }

                    // skip another tab talents
                    if (talent.talentEntry->TabID != talentTabId)
                    {
                        continue;
                    }

                    *data << uint32(talent.talentEntry->ID);  // Talent.dbc
                    *data << uint8(talent.currentRank);     // talentMaxRank (0-4)

                    ++talentIdCount;
                }
            }

            data->put<uint8>(pos, talentIdCount);           // put real count

            *data << uint8(MAX_GLYPH_SLOT_INDEX);           // glyphs count

            // GlyphProperties.dbc
            for (uint8 i = 0; i < MAX_GLYPH_SLOT_INDEX; ++i)
            {
                *data << uint16(m_glyphMgr.GetGlyph(specIdx, i));
            }
        }
    }
}

void Player::BuildPetTalentsInfoData(WorldPacket* data)
{
    uint32 unspentTalentPoints = 0;
    size_t pointsPos = data->wpos();
    *data << uint32(unspentTalentPoints);                   // [PH], unspentTalentPoints

    uint8 talentIdCount = 0;
    size_t countPos = data->wpos();
    *data << uint8(talentIdCount);                          // [PH], talentIdCount

    Pet* pet = GetPet();
    if (!pet)
    {
        return;
    }

    unspentTalentPoints = pet->GetFreeTalentPoints();

    data->put<uint32>(pointsPos, unspentTalentPoints);      // put real points

    CreatureInfo const* ci = pet->GetCreatureInfo();
    if (!ci)
    {
        return;
    }

    CreatureFamilyEntry const* pet_family = sCreatureFamilyStore.LookupEntry(ci->Family);
    if (!pet_family || pet_family->PetTalentType < 0)
    {
        return;
    }

    for (uint32 talentTabId = 1; talentTabId < sTalentTabStore.GetNumRows(); ++talentTabId)
    {
        TalentTabEntry const* talentTabInfo = sTalentTabStore.LookupEntry(talentTabId);
        if (!talentTabInfo)
        {
            continue;
        }

        if (!((1 << pet_family->PetTalentType) & talentTabInfo->PetTalentMask))
        {
            continue;
        }

        for (uint32 talentId = 0; talentId < sTalentStore.GetNumRows(); ++talentId)
        {
            TalentEntry const* talentInfo = sTalentStore.LookupEntry(talentId);
            if (!talentInfo)
            {
                continue;
            }

            // skip another tab talents
            if (talentInfo->TabID != talentTabId)
            {
                continue;
            }

            // find max talent rank
            int32 curtalent_maxrank = -1;
            for (int32 k = 4; k > -1; --k)
            {
                if (talentInfo->SpellRank[k] && pet->HasSpell(talentInfo->SpellRank[k]))
                {
                    curtalent_maxrank = k;
                    break;
                }
            }

            // not learned talent
            if (curtalent_maxrank < 0)
            {
                continue;
            }

            *data << uint32(talentInfo->ID);          // Talent.dbc
            *data << uint8(curtalent_maxrank);              // talentMaxRank (0-4)

            ++talentIdCount;
        }

        data->put<uint8>(countPos, talentIdCount);          // put real count

        break;
    }
}

void Player::SendTalentsInfoData(bool pet)
{
    WorldPacket data(SMSG_TALENT_UPDATE, 50);
    data << uint8(pet ? 1 : 0);
    if (pet)
    {
        BuildPetTalentsInfoData(&data);
    }
    else
    {
        BuildPlayerTalentsInfoData(&data);
    }
    GetSession()->SendPacket(&data);
}

void Player::BuildEnchantmentsInfoData(WorldPacket* data)
{
    uint32 slotUsedMask = 0;
    size_t slotUsedMaskPos = data->wpos();
    *data << uint32(slotUsedMask);                          // slotUsedMask < 0x80000

    for (uint32 i = 0; i < EQUIPMENT_SLOT_END; ++i)
    {
        Item* item = GetItemByPos(INVENTORY_SLOT_BAG_0, i);

        if (!item)
        {
            continue;
        }

        slotUsedMask |= (1 << i);

        *data << uint32(item->GetEntry());                  // item entry

        uint16 enchantmentMask = 0;
        size_t enchantmentMaskPos = data->wpos();
        *data << uint16(enchantmentMask);                   // enchantmentMask < 0x1000

        for (uint32 j = 0; j < MAX_ENCHANTMENT_SLOT; ++j)
        {
            uint32 enchId = item->GetEnchantmentId(EnchantmentSlot(j));

            if (!enchId)
            {
                continue;
            }

            enchantmentMask |= (1 << j);

            *data << uint16(enchId);                        // enchantmentId?
        }

        data->put<uint16>(enchantmentMaskPos, enchantmentMask);

        *data << uint16(item->GetItemRandomPropertyId());
        *data << item->GetGuidValue(ITEM_FIELD_CREATOR).WriteAsPacked();
        *data << uint32(item->GetItemSuffixFactor());
    }

    data->put<uint32>(slotUsedMaskPos, slotUsedMask);
}

void Player::SendEquipmentSetList()
{
    uint32 count = 0;
    WorldPacket data(SMSG_LOAD_EQUIPMENT_SET, 4);
    size_t count_pos = data.wpos();
    data << uint32(count);                                  // count placeholder
    for (EquipmentSets::iterator itr = m_EquipmentSets.begin(); itr != m_EquipmentSets.end(); ++itr)
    {
        if (itr->second.state == EQUIPMENT_SET_DELETED)
        {
            continue;
        }
        data.appendPackGUID(itr->second.Guid);
        data << uint32(itr->first);
        data << itr->second.Name;
        data << itr->second.IconName;
        for (uint32 i = 0; i < EQUIPMENT_SLOT_END; ++i)
        {
            // ignored slots stored in IgnoreMask, client wants "1" as raw GUID, so no HIGHGUID_ITEM
            if (itr->second.IgnoreMask & (1 << i))
            {
                data << ObjectGuid(uint64(1)).WriteAsPacked();
            }
            else
            {
                data << ObjectGuid(HIGHGUID_ITEM, itr->second.Items[i]).WriteAsPacked();
            }
        }

        ++count;                                            // client have limit but it checked at loading and set
    }
    data.put<uint32>(count_pos, count);
    GetSession()->SendPacket(&data);
}

void Player::SetEquipmentSet(uint32 index, EquipmentSet eqset)
{
    if (eqset.Guid != 0)
    {
        bool found = false;

        for (EquipmentSets::iterator itr = m_EquipmentSets.begin(); itr != m_EquipmentSets.end(); ++itr)
        {
            if ((itr->second.Guid == eqset.Guid) && (itr->first == index))
            {
                found = true;
                break;
            }
        }

        if (!found)                                         // something wrong...
        {
            sLog.outError("Player %s tried to save equipment set " UI64FMTD " (index %u), but that equipment set not found!", GetName(), eqset.Guid, index);
            return;
        }
    }

    EquipmentSet& eqslot = m_EquipmentSets[index];

    EquipmentSetUpdateState old_state = eqslot.state;

    eqslot = eqset;

    if (eqset.Guid == 0)
    {
        eqslot.Guid = sObjectMgr.GenerateEquipmentSetGuid();

        WorldPacket data(SMSG_EQUIPMENT_SET_ID, 4 + 1);
        data << uint32(index);
        data.appendPackGUID(eqslot.Guid);
        GetSession()->SendPacket(&data);
    }

    eqslot.state = old_state == EQUIPMENT_SET_NEW ? EQUIPMENT_SET_NEW : EQUIPMENT_SET_CHANGED;
}

void Player::DeleteEquipmentSet(uint64 setGuid)
{
    for (EquipmentSets::iterator itr = m_EquipmentSets.begin(); itr != m_EquipmentSets.end(); ++itr)
    {
        if (itr->second.Guid == setGuid)
        {
            if (itr->second.state == EQUIPMENT_SET_NEW)
            {
                m_EquipmentSets.erase(itr);
            }
            else
            {
                itr->second.state = EQUIPMENT_SET_DELETED;
            }
            break;
        }
    }
}

void Player::ActivateSpec(uint8 specNum)
{
    if (GetActiveSpec() == specNum)
    {
        return;
    }

    if (specNum >= GetSpecsCount())
    {
        return;
    }

    UnsummonPetTemporaryIfAny();

    // prevent deletion of action buttons by client at spell unlearn or by player while spec change in progress
    SendLockActionButtons();

    ApplyGlyphs(false);

    // copy of new talent spec (we will use it as model for converting current tlanet state to new)
    PlayerTalentMap tempSpec = m_talents[specNum];

    // copy old spec talents to new one, must be before spec switch to have previous spec num(as m_activeSpec)
    m_talents[specNum] = m_talents[m_activeSpec];

    SetActiveSpec(specNum);

    // remove all talent spells that don't exist in next spec but exist in old
    for (PlayerTalentMap::iterator specIter = m_talents[m_activeSpec].begin(); specIter != m_talents[m_activeSpec].end();)
    {
        PlayerTalent& talent = specIter->second;

        if (talent.state == PLAYERSPELL_REMOVED)
        {
            ++specIter;
            continue;
        }

        PlayerTalentMap::iterator iterTempSpec = tempSpec.find(specIter->first);

        // remove any talent rank if talent not listed in temp spec
        if (iterTempSpec == tempSpec.end() || iterTempSpec->second.state == PLAYERSPELL_REMOVED)
        {
            TalentEntry const* talentInfo = talent.talentEntry;

            for (int r = 0; r < MAX_TALENT_RANK; ++r)
            {
                if (talentInfo->SpellRank[r])
                {
                    removeSpell(talentInfo->SpellRank[r], !IsPassiveSpell(talentInfo->SpellRank[r]), false);
                }
            }

            specIter = m_talents[m_activeSpec].begin();
        }
        else
        {
            ++specIter;
        }
    }

    // now new spec data have only talents (maybe different rank) as in temp spec data, sync ranks then.
    for (PlayerTalentMap::const_iterator tempIter = tempSpec.begin(); tempIter != tempSpec.end(); ++tempIter)
    {
        PlayerTalent const& talent = tempIter->second;

        // removed state talent already unlearned in prev. loop
        // but we need restore it if it deleted for finish removed-marked data in DB
        if (talent.state == PLAYERSPELL_REMOVED)
        {
            m_talents[m_activeSpec][tempIter->first] = talent;
            continue;
        }

        uint32 talentSpellId = talent.talentEntry->SpellRank[talent.currentRank];

        // learn talent spells if they not in new spec (old spec copy)
        // and if they have different rank
        if (PlayerTalent const* cur_talent = GetKnownTalentById(tempIter->first))
        {
            if (cur_talent->currentRank != talent.currentRank)
            {
                learnSpell(talentSpellId, false);
            }
        }
        else
        {
            learnSpell(talentSpellId, false);
        }

        // sync states - original state is changed in addSpell that learnSpell calls
        PlayerTalentMap::iterator specIter = m_talents[m_activeSpec].find(tempIter->first);
        if (specIter != m_talents[m_activeSpec].end())
        {
            specIter->second.state = talent.state;
        }
        else
        {
            sLog.outError("ActivateSpec: Talent spell %u expected to learned at spec switch but not listed in talents at final check!", talentSpellId);

            // attempt resync DB state (deleted lost spell from DB)
            if (talent.state != PLAYERSPELL_NEW)
            {
                PlayerTalent& talentNew = m_talents[m_activeSpec][tempIter->first];
                talentNew = talent;
                talentNew.state = PLAYERSPELL_REMOVED;
            }
        }
    }

    InitTalentForLevel();

    // recheck action buttons (not checked at loading/spec copy)
    ActionButtonList const& currentActionButtonList = m_actionButtons[m_activeSpec];
    for (ActionButtonList::const_iterator itr = currentActionButtonList.begin(); itr != currentActionButtonList.end();)
    {
        if (itr->second.uState != ACTIONBUTTON_DELETED)
        {
            // remove broken without any output (it can be not correct because talents not copied at spec creating)
            if (!IsActionButtonDataValid(itr->first, itr->second.GetAction(), itr->second.GetType(), this, false))
            {
                removeActionButton(m_activeSpec, itr->first);
                itr = currentActionButtonList.begin();
                continue;
            }
        }
        ++itr;
    }

    ResummonPetTemporaryUnSummonedIfAny();

    ApplyGlyphs(true);

    SendInitialActionButtons();

    Powers powerType = GetPowerType();
    if (powerType != POWER_MANA)
    {
        SetPower(POWER_MANA, 0);
    }

    SetPower(powerType, 0);
}

void Player::UpdateSpecCount(uint8 count)
{
    uint8 curCount = GetSpecsCount();
    if (curCount == count)
    {
        return;
    }

    // maybe current spec data must be copied to 0 spec?
    if (m_activeSpec >= count)
    {
        ActivateSpec(0);
    }

    // copy spec data from new specs
    if (count > curCount)
    {
        // copy action buttons from active spec (more easy in this case iterate first by button)
        ActionButtonList const& currentActionButtonList = m_actionButtons[m_activeSpec];

        for (ActionButtonList::const_iterator itr = currentActionButtonList.begin(); itr != currentActionButtonList.end(); ++itr)
        {
            if (itr->second.uState != ACTIONBUTTON_DELETED)
            {
                for (uint8 spec = curCount; spec < count; ++spec)
                {
                    addActionButton(spec, itr->first, itr->second.GetAction(), itr->second.GetType());
                }
            }
        }
    }
    // delete spec data for removed specs
    else if (count < curCount)
    {
        // delete action buttons for removed spec
        for (uint8 spec = count; spec < curCount; ++spec)
        {
            // delete action buttons for removed spec
            for (uint8 button = 0; button < MAX_ACTION_BUTTONS; ++button)
            {
                removeActionButton(spec, button);
            }
        }
    }

    SetSpecsCount(count);

    SendTalentsInfoData(false);
}
