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
 * @file PlayerSpell.cpp
 * @brief Cohesion split of Player.cpp -- spellbook, talent, and cooldown
 *        management methods. Same `Player` class; no behaviour change.
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
#include "CinematicFlyover.h"
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
#include "LFGMgr.h"
#include "DisableMgr.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /* ENABLE_ELUNA */

#include <cmath>
/**
 * @brief Adds or updates a spell entry in the player's spellbook.
 *
 * @param spell_id The spell identifier to add.
 * @param active True if the spell should be active in the spellbook.
 * @param learning True if the spell is being learned now rather than loaded.
 * @param dependent True if the spell is learned as a dependency.
 * @param disabled True if the spell should remain disabled.
 * @return True if the spell should be reported to the client as newly learned.
 */
bool Player::addSpell(uint32 spell_id, bool active, bool learning, bool dependent, bool disabled)
{
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(spell_id);
    if (!spellInfo)
    {
        // do character spell book cleanup (all characters)
        if (!IsInWorld() && !learning)                      // spell load case
        {
            sLog.outError("Player::addSpell: nonexistent in SpellStore spell #%u request, deleting for all characters in `character_spell`.", spell_id);
            CharacterDatabase.PExecute("DELETE FROM `character_spell` WHERE `spell` = '%u'", spell_id);
        }
        else
        {
            sLog.outError("Player::addSpell: nonexistent in SpellStore spell #%u request.", spell_id);
        }

        return false;
    }

    if (!SpellMgr::IsSpellValid(spellInfo, this, false))
    {
        // do character spell book cleanup (all characters)
        if (!IsInWorld() && !learning)                      // spell load case
        {
            sLog.outError("Player::addSpell: Broken spell #%u learning not allowed, deleting for all characters in `character_spell`.", spell_id);
            CharacterDatabase.PExecute("DELETE FROM `character_spell` WHERE `spell` = '%u'", spell_id);
        }
        else
        {
            sLog.outError("Player::addSpell: Broken spell #%u learning not allowed.", spell_id);
        }

        return false;
    }

    PlayerSpellState state = learning ? PLAYERSPELL_NEW : PLAYERSPELL_UNCHANGED;

    bool dependent_set = false;
    bool disabled_case = false;
    bool superceded_old = false;

    PlayerSpellMap::iterator itr = m_spells.find(spell_id);
    if (itr != m_spells.end())
    {
        uint32 next_active_spell_id = 0;
        bool dependent_set = false;

        // fix activate state for non-stackable low rank (and find next spell for !active case)
        if (sSpellMgr.IsRankedSpellNonStackableInSpellBook(spellInfo))
        {
            SpellChainMapNext const& nextMap = sSpellMgr.GetSpellChainNext();
            for (SpellChainMapNext::const_iterator next_itr = nextMap.lower_bound(spell_id); next_itr != nextMap.upper_bound(spell_id); ++next_itr)
            {
                if (HasSpell(next_itr->second))
                {
                    // high rank already known so this must !active
                    active = false;
                    next_active_spell_id = next_itr->second;
                    break;
                }
            }
        }

        PlayerSpell& playerSpell = itr->second;

        // not do anything if already known in expected state
        if (playerSpell.state != PLAYERSPELL_REMOVED && playerSpell.active == active &&
                playerSpell.dependent == dependent && playerSpell.disabled == disabled)
        {
            if (!IsInWorld() && !learning)                  // explicitly load from DB and then exist in it already and set correctly
            {
                playerSpell.state = PLAYERSPELL_UNCHANGED;
            }

            return false;
        }

        // dependent spell known as not dependent, overwrite state
        if (playerSpell.state != PLAYERSPELL_REMOVED && !playerSpell.dependent && dependent)
        {
            playerSpell.dependent = dependent;
            if (playerSpell.state != PLAYERSPELL_NEW)
            {
                playerSpell.state = PLAYERSPELL_CHANGED;
            }
            dependent_set = true;
        }

        // update active state for known spell
        if (playerSpell.active != active && playerSpell.state != PLAYERSPELL_REMOVED && !playerSpell.disabled)
        {
            playerSpell.active = active;

            if (!IsInWorld() && !learning && !dependent_set)// explicitly load from DB and then exist in it already and set correctly
            {
                playerSpell.state = PLAYERSPELL_UNCHANGED;
            }
            else if (playerSpell.state != PLAYERSPELL_NEW)
            {
                playerSpell.state = PLAYERSPELL_CHANGED;
            }

            if (active)
            {
                if (IsNeedCastPassiveLikeSpellAtLearn(spellInfo))
                {
                    CastSpell(this, spell_id, true);
                }
            }
            else if (IsInWorld())
            {
                if (next_active_spell_id)
                {
                    // update spell ranks in spellbook and action bar
                    WorldPacket data(SMSG_SUPERCEDED_SPELL, 4 + 4);
                    data << uint32(spell_id);
                    data << uint32(next_active_spell_id);
                    GetSession()->SendPacket(&data);
                }
                else
                {
                    WorldPacket data(SMSG_REMOVED_SPELL, 4);
                    data << uint32(spell_id);
                    GetSession()->SendPacket(&data);
                }
            }

            return active;                                  // learn (show in spell book if active now)
        }

        if (playerSpell.disabled != disabled && playerSpell.state != PLAYERSPELL_REMOVED)
        {
            if (playerSpell.state != PLAYERSPELL_NEW)
            {
                playerSpell.state = PLAYERSPELL_CHANGED;
            }
            playerSpell.disabled = disabled;

            if (disabled)
            {
                return false;
            }

            disabled_case = true;
        }
        else switch (playerSpell.state)
            {
                case PLAYERSPELL_UNCHANGED:                 // known saved spell
                    return false;
                case PLAYERSPELL_REMOVED:                   // re-learning removed not saved spell
                {
                    m_spells.erase(itr);
                    state = PLAYERSPELL_CHANGED;
                    break;                                  // need re-add
                }
                default:                                    // known not saved yet spell (new or modified)
                {
                    // can be in case spell loading but learned at some previous spell loading
                    if (!IsInWorld() && !learning && !dependent_set)
                    {
                        playerSpell.state = PLAYERSPELL_UNCHANGED;
                    }

                    return false;
                }
            }
    }

    TalentSpellPos const* talentPos = GetTalentSpellPos(spell_id);

    if (!disabled_case) // skip new spell adding if spell already known (disabled spells case)
    {
        // talent: unlearn all other talent ranks (high and low)
        if (talentPos)
        {
            if (TalentEntry const* talentInfo = sTalentStore.LookupEntry(talentPos->talent_id))
            {
                for (int i = 0; i < MAX_TALENT_RANK; ++i)
                {
                    // skip learning spell and no rank spell case
                    uint32 rankSpellId = talentInfo->SpellRank[i];
                    if (!rankSpellId || rankSpellId == spell_id)
                    {
                        continue;
                    }

                    removeSpell(rankSpellId, false, false);
                }
            }
        }
        // non talent spell: learn low ranks (recursive call)
        else if (uint32 prev_spell = sSpellMgr.GetPrevSpellInChain(spell_id))
        {
            if (!IsInWorld() || disabled)                   // at spells loading, no output, but allow save
            {
                addSpell(prev_spell, active, true, true, disabled);
            }
            else                                            // at normal learning
            {
                learnSpell(prev_spell, true);
            }
        }

        PlayerSpell newspell;
        newspell.state     = state;
        newspell.active    = active;
        newspell.dependent = dependent;
        newspell.disabled  = disabled;

        // replace spells in action bars and spellbook to bigger rank if only one spell rank must be accessible
        if (newspell.active && !newspell.disabled && sSpellMgr.IsRankedSpellNonStackableInSpellBook(spellInfo))
        {
            for (PlayerSpellMap::iterator itr2 = m_spells.begin(); itr2 != m_spells.end(); ++itr2)
            {
                if (itr2->second.state == PLAYERSPELL_REMOVED)
                {
                    continue;
                }
                SpellEntry const* i_spellInfo = sSpellStore.LookupEntry(itr2->first);
                if (!i_spellInfo)
                {
                    continue;
                }
                if (sSpellMgr.IsRankSpellDueToSpell(spellInfo, itr2->first))
                {
                    if (itr2->second.active)
                    {
                        if (sSpellMgr.IsHighRankOfSpell(spell_id, itr2->first))
                        {
                            if (IsInWorld())                // not send spell (re-/over-)learn packets at loading
                            {
                                WorldPacket data(SMSG_SUPERCEDED_SPELL, 4 + 4);
                                data << uint32(itr2->first);
                                data << uint32(spell_id);
                                GetSession()->SendPacket(&data);
                            }

                            // mark old spell as disable (SMSG_SUPERCEDED_SPELL replace it in client by new)
                            itr2->second.active = false;
                            if (itr2->second.state != PLAYERSPELL_NEW)
                            {
                                itr2->second.state = PLAYERSPELL_CHANGED;
                            }
                            superceded_old = true;          // new spell replace old in action bars and spell book.
                        }
                        else if (sSpellMgr.IsHighRankOfSpell(itr2->first, spell_id))
                        {
                            if (IsInWorld())                // not send spell (re-/over-)learn packets at loading
                            {
                                WorldPacket data(SMSG_SUPERCEDED_SPELL, 4 + 4);
                                data << uint32(spell_id);
                                data << uint32(itr2->first);
                                GetSession()->SendPacket(&data);
                            }

                            // mark new spell as disable (not learned yet for client and will not learned)
                            newspell.active = false;
                            if (newspell.state != PLAYERSPELL_NEW)
                            {
                                newspell.state = PLAYERSPELL_CHANGED;
                            }
                        }
                    }
                }
            }
        }

        m_spells[spell_id] = newspell;

        // return false if spell disabled or spell is non-stackable with lower-ranks
        if (newspell.disabled)
        {
            return false;
        }
    }

    if (talentPos)
    {
        // update talent map
        PlayerTalentMap::iterator iter = m_talents[m_activeSpec].find(talentPos->talent_id);
        if (iter != m_talents[m_activeSpec].end())
        {
            // check if ranks different or removed
            if ((*iter).second.state == PLAYERSPELL_REMOVED || talentPos->rank != (*iter).second.currentRank)
            {
                (*iter).second.currentRank = talentPos->rank;

                if ((*iter).second.state != PLAYERSPELL_NEW)
                {
                    (*iter).second.state = PLAYERSPELL_CHANGED;
                }
            }
        }
        else
        {
            PlayerTalent talent;
            talent.currentRank = talentPos->rank;
            talent.talentEntry = sTalentStore.LookupEntry(talentPos->talent_id);
            talent.state       = IsInWorld() ? PLAYERSPELL_NEW : PLAYERSPELL_UNCHANGED;
            m_talents[m_activeSpec][talentPos->talent_id] = talent;
        }

        // update used talent points count
        m_usedTalentCount += GetTalentSpellCost(talentPos);
        UpdateFreeTalentPoints(false);
    }

    // update free primary prof.points (if any, can be none in case GM .learn prof. learning)
    if (uint32 freeProfs = GetFreePrimaryProfessionPoints())
    {
        if (sSpellMgr.IsPrimaryProfessionFirstRankSpell(spell_id))
        {
            SetFreePrimaryProfessions(freeProfs - 1);
        }
    }

    // cast talents with SPELL_EFFECT_LEARN_SPELL (other dependent spells will learned later as not auto-learned)
    // note: all spells with SPELL_EFFECT_LEARN_SPELL isn't passive
    if (talentPos && IsSpellHaveEffect(spellInfo, SPELL_EFFECT_LEARN_SPELL))
    {
        // ignore stance requirement for talent learn spell (stance set for spell only for client spell description show)
        CastSpell(this, spell_id, true);
    }
    // also cast passive (and passive like) spells (including all talents without SPELL_EFFECT_LEARN_SPELL) with additional checks
    else if (IsNeedCastPassiveLikeSpellAtLearn(spellInfo))
    {
        CastSpell(this, spell_id, true);
    }
    else if (IsSpellHaveEffect(spellInfo, SPELL_EFFECT_SKILL_STEP))
    {
        CastSpell(this, spell_id, true);
        return false;
    }

    // add dependent skills
    uint16 maxskill = GetMaxSkillValueForLevel();

    SpellLearnSkillNode const* spellLearnSkill = sSpellMgr.GetSpellLearnSkill(spell_id);

    SkillLineAbilityMapBounds skill_bounds = sSpellMgr.GetSkillLineAbilityMapBounds(spell_id);

    if (spellLearnSkill)
    {
        uint32 skill_value = GetPureSkillValue(spellLearnSkill->skill);
        uint32 skill_max_value = GetPureMaxSkillValue(spellLearnSkill->skill);

        if (skill_value < spellLearnSkill->value)
        {
            skill_value = spellLearnSkill->value;
        }

        uint32 new_skill_max_value = spellLearnSkill->maxvalue == 0 ? maxskill : spellLearnSkill->maxvalue;

        if (skill_max_value < new_skill_max_value)
        {
            skill_max_value =  new_skill_max_value;
        }

        SetSkill(spellLearnSkill->skill, skill_value, skill_max_value, spellLearnSkill->step);
    }
    else
    {
        // not ranked skills
        for (SkillLineAbilityMap::const_iterator _spell_idx = skill_bounds.first; _spell_idx != skill_bounds.second; ++_spell_idx)
        {
            SkillLineEntry const* pSkill = sSkillLineStore.LookupEntry(_spell_idx->second->SkillLine);
            if (!pSkill)
            {
                continue;
            }

            if (HasSkill(pSkill->id))
            {
                continue;
            }

            if (_spell_idx->second->AcquireMethod == ABILITY_LEARNED_ON_GET_RACE_OR_CLASS_SKILL ||
                    // lockpicking/runeforging special case, not have ABILITY_LEARNED_ON_GET_RACE_OR_CLASS_SKILL
                    ((pSkill->id == SKILL_LOCKPICKING || pSkill->id == SKILL_RUNEFORGING) && _spell_idx->second->TrivialSkillLineRankHigh == 0))
            {
                switch (GetSkillRangeType(pSkill, _spell_idx->second->RaceMask != 0))
                {
                    case SKILL_RANGE_LANGUAGE:
                        SetSkill(pSkill->id, 300, 300);
                        break;
                    case SKILL_RANGE_LEVEL:
                        SetSkill(pSkill->id, 1, GetMaxSkillValueForLevel());
                        break;
                    case SKILL_RANGE_MONO:
                        SetSkill(pSkill->id, 1, 1);
                        break;
                    default:
                        break;
                }
            }
        }
    }

    // learn dependent spells
    SpellLearnSpellMapBounds spell_bounds = sSpellMgr.GetSpellLearnSpellMapBounds(spell_id);

    for (SpellLearnSpellMap::const_iterator itr2 = spell_bounds.first; itr2 != spell_bounds.second; ++itr2)
    {
        SpellLearnSpellNode const& spellLearn = itr2->second;
        if (!spellLearn.autoLearned)
        {
            if (!IsInWorld() || !spellLearn.active)       // at spells loading, no output, but allow save
            {
                addSpell(spellLearn.spell, spellLearn.active, true, true, false);
            }
            else                                            // at normal learning
            {
                learnSpell(spellLearn.spell, true);
            }
        }
    }

    if (!GetSession()->PlayerLoading())
    {
        // not ranked skills
        for (SkillLineAbilityMap::const_iterator _spell_idx = skill_bounds.first; _spell_idx != skill_bounds.second; ++_spell_idx)
        {
            GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LINE, _spell_idx->second->SkillLine);
            GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS, _spell_idx->second->SkillLine);
        }

        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL, spell_id);
    }

    // return true (for send learn packet) only if spell active (in case ranked spells) and not replace old spell
    return active && !disabled && !superceded_old;
}

/**
 * @brief Determines whether a learned spell must be cast immediately like a passive.
 *
 * @param spellInfo The spell being evaluated.
 * @return True if the spell should be cast on learn; otherwise, false.
 */
bool Player::IsNeedCastPassiveLikeSpellAtLearn(SpellEntry const* spellInfo) const
{
    ShapeshiftForm form = GetShapeshiftForm();

    if (IsNeedCastSpellAtFormApply(spellInfo, form))        // SPELL_ATTR_PASSIVE | SPELL_ATTR_HIDE_SPELL spells
    {
        return true;                                         // all stance req. cases, not have auarastate cases
    }

    if (!spellInfo->HasAttribute(SPELL_ATTR_PASSIVE))
    {
        return false;
    }

    // note: form passives activated with shapeshift spells be implemented by HandleShapeshiftBoosts instead of spell_learn_spell
    // talent dependent passives activated at form apply have proper stance data
    bool need_cast = !spellInfo->ShapeshiftMask || (!form && spellInfo->HasAttribute(SPELL_ATTR_EX2_NOT_NEED_SHAPESHIFT));

    // Check CasterAuraStates
    return need_cast && (!spellInfo->CasterAuraState || HasAuraState(AuraState(spellInfo->CasterAuraState)));
}

/**
 * @brief Learns a spell and notifies the client when appropriate.
 *
 * @param spell_id The spell identifier to learn.
 * @param dependent True if the spell is being learned as a dependency.
 */
void Player::learnSpell(uint32 spell_id, bool dependent)
{
    PlayerSpellMap::iterator itr = m_spells.find(spell_id);

    bool disabled = (itr != m_spells.end()) ? itr->second.disabled : false;
    bool active = disabled ? itr->second.active : true;

    bool learning = addSpell(spell_id, active, true, dependent, false);

    // prevent duplicated entires in spell book, also not send if not in world (loading)
    if (learning && IsInWorld())
    {
        WorldPacket data(SMSG_LEARNED_SPELL, 6);
        data << uint32(spell_id);
        data << uint16(0);                                  // 3.3.3 unk
        GetSession()->SendPacket(&data);
    }

    // learn all disabled higher ranks (recursive)
    if (disabled)
    {
        SpellChainMapNext const& nextMap = sSpellMgr.GetSpellChainNext();
        for (SpellChainMapNext::const_iterator i = nextMap.lower_bound(spell_id); i != nextMap.upper_bound(spell_id); ++i)
        {
            PlayerSpellMap::iterator iter = m_spells.find(i->second);
            if (iter != m_spells.end() && iter->second.disabled)
            {
                learnSpell(i->second, false);
            }
        }
    }
}

/**
 * @brief Removes or disables a spell and updates dependent spell state.
 *
 * @param spell_id The spell identifier to remove.
 * @param disabled True to disable the spell instead of fully removing it.
 * @param learn_low_rank True to reactivate lower ranks when appropriate.
 */
void Player::removeSpell(uint32 spell_id, bool disabled, bool learn_low_rank, bool sendUpdate)
{
    PlayerSpellMap::iterator itr = m_spells.find(spell_id);
    if (itr == m_spells.end())
    {
        return;
    }

    PlayerSpell& playerSpell = itr->second;
    if (playerSpell.state == PLAYERSPELL_REMOVED || (disabled && playerSpell.disabled))
    {
        return;
    }

    // unlearn non talent higher ranks (recursive)
    SpellChainMapNext const& nextMap = sSpellMgr.GetSpellChainNext();
    for (SpellChainMapNext::const_iterator itr2 = nextMap.lower_bound(spell_id); itr2 != nextMap.upper_bound(spell_id); ++itr2)
    {
        if (HasSpell(itr2->second) && !GetTalentSpellPos(itr2->second))
        {
            removeSpell(itr2->second, disabled, false);
        }
    }

    // re-search, it can be corrupted in prev loop
    itr = m_spells.find(spell_id);
    if (itr == m_spells.end() || playerSpell.state == PLAYERSPELL_REMOVED)
    {
        return; // already unleared
    }

    bool cur_active = playerSpell.active;
    bool cur_dependent = playerSpell.dependent;

    if (disabled)
    {
        playerSpell.disabled = disabled;
        if (playerSpell.state != PLAYERSPELL_NEW)
        {
            playerSpell.state = PLAYERSPELL_CHANGED;
        }
    }
    else
    {
        if (playerSpell.state == PLAYERSPELL_NEW)
        {
            m_spells.erase(itr);
        }
        else
        {
            playerSpell.state = PLAYERSPELL_REMOVED;
        }
    }

    RemoveAurasDueToSpell(spell_id);

    // remove pet auras
    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        if (PetAura const* petSpell = sSpellMgr.GetPetAura(spell_id, SpellEffectIndex(i)))
        {
            RemovePetAura(petSpell);
        }
    }

    TalentSpellPos const* talentPos = GetTalentSpellPos(spell_id);
    if (talentPos)
    {
        // update talent map
        PlayerTalentMap::iterator iter = m_talents[m_activeSpec].find(talentPos->talent_id);
        if (iter != m_talents[m_activeSpec].end())
        {
            if ((*iter).second.state != PLAYERSPELL_NEW)
            {
                (*iter).second.state = PLAYERSPELL_REMOVED;
            }
            else
            {
                m_talents[m_activeSpec].erase(iter);
            }
        }
        else
        {
            sLog.outError("removeSpell: Player (GUID: %u) has talent spell (id: %u) but doesn't have talent", GetGUIDLow(), spell_id);
        }

        // free talent points
        uint32 talentCosts = GetTalentSpellCost(talentPos);

        if (talentCosts < m_usedTalentCount)
        {
            m_usedTalentCount -= talentCosts;
        }
        else
        {
            m_usedTalentCount = 0;
        }

        UpdateFreeTalentPoints(false);
    }

    // update free primary prof.points (if not overflow setting, can be in case GM use before .learn prof. learning)
    if (sSpellMgr.IsPrimaryProfessionFirstRankSpell(spell_id))
    {
        uint32 freeProfs = GetFreePrimaryProfessionPoints() + 1;
        if (freeProfs <= sWorld.getConfig(CONFIG_UINT32_MAX_PRIMARY_TRADE_SKILL))
        {
            SetFreePrimaryProfessions(freeProfs);
        }
    }

    // remove dependent skill
    SpellLearnSkillNode const* spellLearnSkill = sSpellMgr.GetSpellLearnSkill(spell_id);
    if (spellLearnSkill)
    {
        uint32 prev_spell = sSpellMgr.GetPrevSpellInChain(spell_id);
        if (!prev_spell)                                    // first rank, remove skill
        {
            SetSkill(spellLearnSkill->skill, 0, 0);
        }
        else
        {
            // search prev. skill setting by spell ranks chain
            SpellLearnSkillNode const* prevSkill = sSpellMgr.GetSpellLearnSkill(prev_spell);
            while (!prevSkill && prev_spell)
            {
                prev_spell = sSpellMgr.GetPrevSpellInChain(prev_spell);
                prevSkill = sSpellMgr.GetSpellLearnSkill(sSpellMgr.GetFirstSpellInChain(prev_spell));
            }

            if (!prevSkill)                                 // not found prev skill setting, remove skill
            {
                SetSkill(spellLearnSkill->skill, 0, 0);
            }
            else                                            // set to prev. skill setting values
            {
                uint32 skill_value = GetPureSkillValue(prevSkill->skill);
                uint32 skill_max_value = GetPureMaxSkillValue(prevSkill->skill);

                if (skill_value >  prevSkill->value)
                {
                    skill_value = prevSkill->value;
                }

                uint32 new_skill_max_value = prevSkill->maxvalue == 0 ? GetMaxSkillValueForLevel() : prevSkill->maxvalue;

                if (skill_max_value > new_skill_max_value)
                {
                    skill_max_value =  new_skill_max_value;
                }

                SetSkill(prevSkill->skill, skill_value, skill_max_value, prevSkill->step);
            }
        }
    }
    else
    {
        // not ranked skills
        SkillLineAbilityMapBounds bounds = sSpellMgr.GetSkillLineAbilityMapBounds(spell_id);

        for (SkillLineAbilityMap::const_iterator _spell_idx = bounds.first; _spell_idx != bounds.second; ++_spell_idx)
        {
            SkillLineAbilityEntry const* skillAbility = _spell_idx->second;
            SkillLineEntry const* pSkill = sSkillLineStore.LookupEntry(skillAbility->SkillLine);
            if (!pSkill)
            {
                continue;
            }

            if ((skillAbility->AcquireMethod == ABILITY_LEARNED_ON_GET_RACE_OR_CLASS_SKILL &&
                    pSkill->categoryId != SKILL_CATEGORY_CLASS) ||// not unlearn class skills (spellbook/talent pages)
                    // lockpicking/runeforging special case, not have ABILITY_LEARNED_ON_GET_RACE_OR_CLASS_SKILL
                    ((pSkill->id == SKILL_LOCKPICKING || pSkill->id == SKILL_RUNEFORGING) && skillAbility->TrivialSkillLineRankHigh == 0))
            {
                // not reset skills for professions and racial abilities
                if ((pSkill->categoryId == SKILL_CATEGORY_SECONDARY || pSkill->categoryId == SKILL_CATEGORY_PROFESSION) &&
                        (IsProfessionSkill(pSkill->id) || skillAbility->RaceMask != 0))
                {
                    continue;
                }

                SetSkill(pSkill->id, 0, 0);
            }
        }
    }

    // remove dependent spells
    SpellLearnSpellMapBounds spell_bounds = sSpellMgr.GetSpellLearnSpellMapBounds(spell_id);

    for (SpellLearnSpellMap::const_iterator itr2 = spell_bounds.first; itr2 != spell_bounds.second; ++itr2)
    {
        removeSpell(itr2->second.spell, disabled);
    }

    // activate lesser rank in spellbook/action bar, and cast it if need
    bool prev_activate = false;

    if (uint32 prev_id = sSpellMgr.GetPrevSpellInChain(spell_id))
    {
        SpellEntry const* spellInfo = sSpellStore.LookupEntry(spell_id);

        // if talent then lesser rank also talent and need learn
        if (talentPos)
        {
            if (learn_low_rank)
            {
                learnSpell(prev_id, false);
            }
        }
        // if ranked non-stackable spell: need activate lesser rank and update dependence state
        else if (cur_active && sSpellMgr.IsRankedSpellNonStackableInSpellBook(spellInfo))
        {
            // need manually update dependence state (learn spell ignore like attempts)
            PlayerSpellMap::iterator prev_itr = m_spells.find(prev_id);
            if (prev_itr != m_spells.end())
            {
                PlayerSpell& spell = prev_itr->second;
                if (spell.dependent != cur_dependent)
                {
                    spell.dependent = cur_dependent;
                    if (spell.state != PLAYERSPELL_NEW)
                    {
                        spell.state = PLAYERSPELL_CHANGED;
                    }
                }

                // now re-learn if need re-activate
                if (cur_active && !spell.active && learn_low_rank)
                {
                    if (addSpell(prev_id, true, false, spell.dependent, spell.disabled))
                    {
                        // downgrade spell ranks in spellbook and action bar
                        WorldPacket data(SMSG_SUPERCEDED_SPELL, 4 + 4);
                        data << uint32(spell_id);
                        data << uint32(prev_id);
                        GetSession()->SendPacket(&data);
                        prev_activate = true;
                    }
                }
            }
        }
    }

    // for Titan's Grip and shaman Dual-wield
    if (CanDualWield() || CanTitanGrip())
    {
        SpellEntry const* spellInfo = sSpellStore.LookupEntry(spell_id);

        if (CanDualWield() && IsSpellHaveEffect(spellInfo, SPELL_EFFECT_DUAL_WIELD))
        {
            SetCanDualWield(false);
        }

        if (CanTitanGrip() && IsSpellHaveEffect(spellInfo, SPELL_EFFECT_TITAN_GRIP))
        {
            SetCanTitanGrip(false);
            // Remove Titan's Grip damage penalty now
            RemoveAurasDueToSpell(49152);
        }
    }

    // for talents and normal spell unlearn that allow offhand use for some weapons
    if (sWorld.getConfig(CONFIG_BOOL_OFFHAND_CHECK_AT_TALENTS_RESET))
    {
        AutoUnequipOffhandIfNeed();
    }

    // remove from spell book if not replaced by lesser rank
    if (!prev_activate && sendUpdate)
    {
        WorldPacket data(SMSG_REMOVED_SPELL, 4);
        data << uint32(spell_id);
        GetSession()->SendPacket(&data);
    }
}


/**
 * @brief Calculates the current cost to reset the player's talents.
 *
 * @return The reset cost in copper.
 */
uint32 Player::resetTalentsCost() const
{
    // The first time reset costs 1 gold
    if (m_resetTalentsCost < 1 * GOLD)
    {
        return 1 * GOLD;
    }
    // then 5 gold
    else if (m_resetTalentsCost < 5 * GOLD)
    {
        return 5 * GOLD;
    }
    // After that it increases in increments of 5 gold
    else if (m_resetTalentsCost < 10 * GOLD)
    {
        return 10 * GOLD;
    }
    else
    {
        time_t months = (sWorld.GetGameTime() - m_resetTalentsTime) / MONTH;
        if (months > 0)
        {
            // This cost will be reduced by a rate of 5 gold per month
            int32 new_cost = int32((m_resetTalentsCost) - 5 * GOLD * months);
            // to a minimum of 10 gold.
            return uint32(new_cost < 10 * GOLD ? 10 * GOLD : new_cost);
        }
        else
        {
            // After that it increases in increments of 5 gold
            int32 new_cost = m_resetTalentsCost + 5 * GOLD;
            // until it hits a cap of 50 gold.
            if (new_cost > 50 * GOLD)
            {
                new_cost = 50 * GOLD;
            }
            return new_cost;
        }
    }
}

/**
 * @brief Resets all learned talents for the player's class.
 *
 * @param no_cost True to skip charging the reset fee.
 * @return True if talents were reset; otherwise, false.
 */
bool Player::resetTalents(bool no_cost, bool all_specs)
{
    // Used by Eluna
#ifdef ENABLE_ELUNA
    if (Eluna* e = GetEluna())
    {
        e->OnTalentsReset(this, no_cost);
    }
#endif /* ENABLE_ELUNA */

    // not need after this call
    if (HasAtLoginFlag(AT_LOGIN_RESET_TALENTS) && all_specs)
    {
        RemoveAtLoginFlag(AT_LOGIN_RESET_TALENTS, true);
    }

    if (m_usedTalentCount == 0 && !all_specs)
    {
        UpdateFreeTalentPoints(false);                      // for fix if need counter
        return false;
    }

    uint32 cost = 0;

    if (!no_cost)
    {
        cost = resetTalentsCost();

        if (GetMoney() < cost)
        {
            SendBuyError(BUY_ERR_NOT_ENOUGHT_MONEY, 0, 0, 0);
            return false;
        }
    }

    for (PlayerTalentMap::iterator iter = m_talents[m_activeSpec].begin(); iter != m_talents[m_activeSpec].end();)
    {
        if (iter->second.state == PLAYERSPELL_REMOVED)
        {
            ++iter;
            continue;
        }

        TalentEntry const* talentInfo = iter->second.talentEntry;

        if (!talentInfo)
        {
            m_talents[m_activeSpec].erase(iter++);
            continue;
        }

        TalentTabEntry const* talentTabInfo = sTalentTabStore.LookupEntry(talentInfo->TabID);

        if (!talentTabInfo)
        {
            m_talents[m_activeSpec].erase(iter++);
            continue;
        }

        // unlearn only talents for character class
        // some spell learned by one class as normal spells or know at creation but another class learn it as talent,
        // to prevent unexpected lost normal learned spell skip another class talents
        if ((getClassMask() & talentTabInfo->ClassMask) == 0)
        {
            ++iter;
            continue;
        }

        for (int j = 0; j < MAX_TALENT_RANK; ++j)
        {
            if (talentInfo->SpellRank[j])
            {
                removeSpell(talentInfo->SpellRank[j], !IsPassiveSpell(talentInfo->SpellRank[j]), false);
            }
        }

        iter = m_talents[m_activeSpec].begin();
    }

    // for not current spec just mark removed all saved to DB case and drop not saved
    if (all_specs)
    {
        for (uint8 spec = 0; spec < MAX_TALENT_SPEC_COUNT; ++spec)
        {
            if (spec == m_activeSpec)
            {
                continue;
            }

            for (PlayerTalentMap::iterator iter = m_talents[spec].begin(); iter != m_talents[spec].end();)
            {
                switch (iter->second.state)
                {
                    case PLAYERSPELL_REMOVED:
                        ++iter;
                        break;
                    case PLAYERSPELL_NEW:
                        m_talents[spec].erase(iter++);
                        break;
                    default:
                        iter->second.state = PLAYERSPELL_REMOVED;
                        ++iter;
                        break;
                }
            }
        }
    }

    UpdateFreeTalentPoints(false);

    if (!no_cost)
    {
        ModifyMoney(-(int32)cost);
        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_TALENTS, cost);
        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_NUMBER_OF_TALENT_RESETS, 1);

        m_resetTalentsCost = cost;
        m_resetTalentsTime = time(NULL);
    }

    // FIXME: remove pet before or after unlearn spells? for now after unlearn to allow removing of talent related, pet affecting auras
    RemovePet(PET_SAVE_REAGENTS);
    /* when prev line will dropped use next line
    if (Pet* pet = GetPet())
    {
        if (pet->getPetType()==HUNTER_PET && !pet->GetCreatureInfo()->isTameable(CanTameExoticPets()))
        {
            pet->Unsummon(PET_SAVE_REAGENTS, this);
        }
    }
    */
    return true;
}

/**
 * @brief Finds a mail entry by message identifier.
 *
 * @param id The message identifier to search for.
 * @return The matching mail entry, or null if none exists.
 */
Mail* Player::GetMail(uint32 id)
{
    for (PlayerMails::iterator itr = m_mail.begin(); itr != m_mail.end(); ++itr)
    {
        if ((*itr)->messageID == id)
        {
            return (*itr);
        }
    }
    return NULL;
}

/**
 * @brief Populates the create update mask for a target player.
 *
 * @param updateMask The update mask being built.
 * @param target The player receiving the create data.
 */
void Player::_SetCreateBits(UpdateMask* updateMask, Player* target) const
{
    if (target == this)
    {
        Object::_SetCreateBits(updateMask, target);
    }
    else
    {
        for (uint16 index = 0; index < m_valuesCount; ++index)
        {
            if (GetUInt32Value(index) != 0 && updateVisualBits.GetBit(index))
            {
                updateMask->SetBit(index);
            }
        }
    }
}

/**
 * @brief Populates the incremental update mask for a target player.
 *
 * @param updateMask The update mask being built.
 * @param target The player receiving the update data.
 */
void Player::_SetUpdateBits(UpdateMask* updateMask, Player* target) const
{
    if (target == this)
    {
        Object::_SetUpdateBits(updateMask, target);
    }
    else
    {
        Object::_SetUpdateBits(updateMask, target);
        *updateMask &= updateVisualBits;
    }
}

/**
 * @brief Initializes the set of player fields visible to other clients.
 */
void Player::InitVisibleBits()
{
    updateVisualBits.SetCount(PLAYER_END);

    updateVisualBits.SetBit(OBJECT_FIELD_GUID);
    updateVisualBits.SetBit(OBJECT_FIELD_TYPE);
    updateVisualBits.SetBit(OBJECT_FIELD_ENTRY);
    updateVisualBits.SetBit(OBJECT_FIELD_SCALE_X);
    updateVisualBits.SetBit(UNIT_FIELD_CHARM + 0);
    updateVisualBits.SetBit(UNIT_FIELD_CHARM + 1);
    updateVisualBits.SetBit(UNIT_FIELD_SUMMON + 0);
    updateVisualBits.SetBit(UNIT_FIELD_SUMMON + 1);
    updateVisualBits.SetBit(UNIT_FIELD_CHARMEDBY + 0);
    updateVisualBits.SetBit(UNIT_FIELD_CHARMEDBY + 1);
    updateVisualBits.SetBit(UNIT_FIELD_TARGET + 0);
    updateVisualBits.SetBit(UNIT_FIELD_TARGET + 1);
    updateVisualBits.SetBit(UNIT_FIELD_CHANNEL_OBJECT + 0);
    updateVisualBits.SetBit(UNIT_FIELD_CHANNEL_OBJECT + 1);
    updateVisualBits.SetBit(UNIT_FIELD_BYTES_0);
    updateVisualBits.SetBit(UNIT_FIELD_HEALTH);
    updateVisualBits.SetBit(UNIT_FIELD_POWER1);
    updateVisualBits.SetBit(UNIT_FIELD_POWER2);
    updateVisualBits.SetBit(UNIT_FIELD_POWER3);
    updateVisualBits.SetBit(UNIT_FIELD_POWER4);
    updateVisualBits.SetBit(UNIT_FIELD_POWER5);
    updateVisualBits.SetBit(UNIT_FIELD_POWER6);
    updateVisualBits.SetBit(UNIT_FIELD_POWER7);
    updateVisualBits.SetBit(UNIT_FIELD_MAXHEALTH);
    updateVisualBits.SetBit(UNIT_FIELD_MAXPOWER1);
    updateVisualBits.SetBit(UNIT_FIELD_MAXPOWER2);
    updateVisualBits.SetBit(UNIT_FIELD_MAXPOWER3);
    updateVisualBits.SetBit(UNIT_FIELD_MAXPOWER4);
    updateVisualBits.SetBit(UNIT_FIELD_MAXPOWER5);
    updateVisualBits.SetBit(UNIT_FIELD_MAXPOWER6);
    updateVisualBits.SetBit(UNIT_FIELD_MAXPOWER7);
    updateVisualBits.SetBit(UNIT_FIELD_LEVEL);
    updateVisualBits.SetBit(UNIT_FIELD_FACTIONTEMPLATE);
    updateVisualBits.SetBit(UNIT_VIRTUAL_ITEM_SLOT_ID + 0);
    updateVisualBits.SetBit(UNIT_VIRTUAL_ITEM_SLOT_ID + 1);
    updateVisualBits.SetBit(UNIT_VIRTUAL_ITEM_SLOT_ID + 2);
    updateVisualBits.SetBit(UNIT_FIELD_FLAGS);
    updateVisualBits.SetBit(UNIT_FIELD_FLAGS_2);
    updateVisualBits.SetBit(UNIT_FIELD_AURASTATE);
    updateVisualBits.SetBit(UNIT_FIELD_BASEATTACKTIME + 0);
    updateVisualBits.SetBit(UNIT_FIELD_BASEATTACKTIME + 1);
    updateVisualBits.SetBit(UNIT_FIELD_BOUNDINGRADIUS);
    updateVisualBits.SetBit(UNIT_FIELD_COMBATREACH);
    updateVisualBits.SetBit(UNIT_FIELD_DISPLAYID);
    updateVisualBits.SetBit(UNIT_FIELD_NATIVEDISPLAYID);
    updateVisualBits.SetBit(UNIT_FIELD_MOUNTDISPLAYID);
    updateVisualBits.SetBit(UNIT_FIELD_BYTES_1);
    updateVisualBits.SetBit(UNIT_FIELD_PETNUMBER);
    updateVisualBits.SetBit(UNIT_FIELD_PET_NAME_TIMESTAMP);
    updateVisualBits.SetBit(UNIT_DYNAMIC_FLAGS);
    updateVisualBits.SetBit(UNIT_CHANNEL_SPELL);
    updateVisualBits.SetBit(UNIT_MOD_CAST_SPEED);
    updateVisualBits.SetBit(UNIT_NPC_FLAGS);
    updateVisualBits.SetBit(UNIT_FIELD_BASE_MANA);
    updateVisualBits.SetBit(UNIT_FIELD_BYTES_2);
    updateVisualBits.SetBit(UNIT_FIELD_HOVERHEIGHT);

    updateVisualBits.SetBit(PLAYER_DUEL_ARBITER + 0);
    updateVisualBits.SetBit(PLAYER_DUEL_ARBITER + 1);
    updateVisualBits.SetBit(PLAYER_FLAGS);
    updateVisualBits.SetBit(PLAYER_GUILDID);
    updateVisualBits.SetBit(PLAYER_GUILDRANK);
    updateVisualBits.SetBit(PLAYER_BYTES);
    updateVisualBits.SetBit(PLAYER_BYTES_2);
    updateVisualBits.SetBit(PLAYER_BYTES_3);
    updateVisualBits.SetBit(PLAYER_DUEL_TEAM);
    updateVisualBits.SetBit(PLAYER_GUILD_TIMESTAMP);

    // PLAYER_QUEST_LOG_x also visible bit on official (but only on party/raid)...
    for (uint16 i = PLAYER_QUEST_LOG_1_1; i < PLAYER_QUEST_LOG_25_2; i += MAX_QUEST_OFFSET)
    {
        updateVisualBits.SetBit(i);
    }

    // Players visible items are not inventory stuff
    for (uint16 i = 0; i < EQUIPMENT_SLOT_END; ++i)
    {
        uint32 offset = i * 2;

        // item entry
        updateVisualBits.SetBit(PLAYER_VISIBLE_ITEM_1_ENTRYID + offset);
        // enchant
        updateVisualBits.SetBit(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + offset);
    }

    updateVisualBits.SetBit(PLAYER_CHOSEN_TITLE);
}

/**
 * @brief Builds the create update block for a player observer.
 *
 * @param data The update data buffer to append to.
 * @param target The player receiving the create block.
 */
void Player::BuildCreateUpdateBlockForPlayer(UpdateData* data, Player* target) const
{
    if (target == this)
    {
        for (int i = 0; i < EQUIPMENT_SLOT_END; ++i)
        {
            if (m_items[i] == NULL)
            {
                continue;
            }

            m_items[i]->BuildCreateUpdateBlockForPlayer(data, target);
        }
        for (int i = INVENTORY_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
        {
            if (m_items[i] == NULL)
            {
                continue;
            }

            m_items[i]->BuildCreateUpdateBlockForPlayer(data, target);
        }
        for (int i = KEYRING_SLOT_START; i < CURRENCYTOKEN_SLOT_END; ++i)
        {
            if (m_items[i] == NULL)
            {
                continue;
            }

            m_items[i]->BuildCreateUpdateBlockForPlayer(data, target);
        }
    }

    Unit::BuildCreateUpdateBlockForPlayer(data, target);
}

/**
 * @brief Builds destroy updates for the player and visible inventory objects.
 *
 * @param target The player that should receive the destroy updates.
 */
void Player::DestroyForPlayer(Player* target, bool anim) const
{
    Unit::DestroyForPlayer(target, anim);

    for (int i = 0; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if (m_items[i] == NULL)
        {
            continue;
        }

        m_items[i]->DestroyForPlayer(target);
    }

    if (target == this)
    {
        for (int i = INVENTORY_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
        {
            if (m_items[i] == NULL)
            {
                continue;
            }

            m_items[i]->DestroyForPlayer(target);
        }
        for (int i = KEYRING_SLOT_START; i < CURRENCYTOKEN_SLOT_END; ++i)
        {
            if (m_items[i] == NULL)
            {
                continue;
            }

            m_items[i]->DestroyForPlayer(target);
        }
    }
}

/**
 * @brief Checks whether the player knows a spell.
 *
 * @param spell The spell identifier to test.
 * @return True if the spell exists and is not removed or disabled; otherwise, false.
 */
bool Player::HasSpell(uint32 spell) const
{
    PlayerSpellMap::const_iterator itr = m_spells.find(spell);
    return (itr != m_spells.end() && itr->second.state != PLAYERSPELL_REMOVED &&
            !itr->second.disabled);
}

bool Player::HasTalent(uint32 spell, uint8 spec) const
{
    PlayerTalentMap::const_iterator itr = m_talents[spec].find(spell);
    return (itr != m_talents[spec].end() && itr->second.state != PLAYERSPELL_REMOVED);
}

/**
 * @brief Checks whether the player has a spell active in the spellbook.
 *
 * @param spell The spell identifier to test.
 * @return True if the spell is known, active, and not disabled; otherwise, false.
 */
bool Player::HasActiveSpell(uint32 spell) const
{
    PlayerSpellMap::const_iterator itr = m_spells.find(spell);
    return (itr != m_spells.end() && itr->second.state != PLAYERSPELL_REMOVED &&
            itr->second.active && !itr->second.disabled);
}

/**
 * @brief Evaluates whether a trainer spell can be learned by the player.
 *
 * @param trainer_spell The trainer spell entry being checked.
 * @param reqLevel An optional override for the required level.
 * @return The trainer spell state to display to the client.
 */
TrainerSpellState Player::GetTrainerSpellState(TrainerSpell const* trainer_spell, uint32 reqLevel) const
{
    if (!trainer_spell)
    {
        return TRAINER_SPELL_RED;
    }

    if (!trainer_spell->learnedSpell)
    {
        return TRAINER_SPELL_RED;
    }

    // known spell
    if (HasSpell(trainer_spell->learnedSpell))
    {
        return TRAINER_SPELL_GRAY;
    }

    // check race/class requirement
    if (!IsSpellFitByClassAndRace(trainer_spell->learnedSpell))
    {
        return TRAINER_SPELL_RED;
    }

    bool prof = SpellMgr::IsProfessionSpell(trainer_spell->learnedSpell);

    // check level requirement
    if (!prof || GetSession()->GetSecurity() < AccountTypes(sWorld.getConfig(CONFIG_UINT32_TRADE_SKILL_GMIGNORE_LEVEL)))
    {
        if (getLevel() < reqLevel)
        {
            return TRAINER_SPELL_RED;
        }
    }

    if (SpellChainNode const* spell_chain = sSpellMgr.GetSpellChainNode(trainer_spell->learnedSpell))
    {
        // check prev.rank requirement
        if (spell_chain->prev && !HasSpell(spell_chain->prev))
        {
            return TRAINER_SPELL_RED;
        }

        // check additional spell requirement
        if (spell_chain->req && !HasSpell(spell_chain->req))
        {
            return TRAINER_SPELL_RED;
        }
    }

    // check skill requirement
    if (!prof || GetSession()->GetSecurity() < AccountTypes(sWorld.getConfig(CONFIG_UINT32_TRADE_SKILL_GMIGNORE_SKILL)))
        if (trainer_spell->reqSkill && GetBaseSkillValue(trainer_spell->reqSkill) < trainer_spell->reqSkillValue)
        {
            return TRAINER_SPELL_RED;
        }

    // exist, already checked at loading
    SpellEntry const* spell = sSpellStore.LookupEntry(trainer_spell->learnedSpell);

    // secondary prof. or not prof. spell
    uint32 skill = spell->EffectMiscValue[1];

    if (spell->Effect[1] != SPELL_EFFECT_SKILL || !IsPrimaryProfessionSkill(skill))
    {
        return TRAINER_SPELL_GREEN;
    }

    // check primary prof. limit
    if (sSpellMgr.IsPrimaryProfessionFirstRankSpell(spell->ID) && GetFreePrimaryProfessionPoints() == 0)
    {
        return TRAINER_SPELL_GREEN_DISABLED;
    }

    return TRAINER_SPELL_GREEN;
}

