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
 * @file BattleGroundReward.cpp
 * @brief Cohesion split of BattleGround.cpp -- end-of-battleground rewards:
 *        honor/reputation to team, items/marks (incl. mail delivery), quest
 *        completion, reward spells, battlemaster lookup, bonus honor and
 *        world-state updates. Same `BattleGround` class; no behaviour change.
 */

#include "Object.h"
#include "Player.h"
#include "BattleGround.h"
#include "BattleGroundMgr.h"
#include "Creature.h"
#include "MapManager.h"
#include "Language.h"
#include "SpellAuras.h"
#include "ArenaTeam.h"
#include "World.h"
#include "Group.h"
#include "ObjectGuid.h"
#include "ObjectMgr.h"
#include "Mail.h"
#include "WorldPacket.h"
#include "Util.h"
#include "Formulas.h"
#include "GridNotifiersImpl.h"
#include "Chat.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /* ENABLE_ELUNA */

/// <summary>
/// Rewards the honor to team.
/// </summary>
/// <param name="Honor">The honor.</param>
/// <param name="teamId">The team id.</param>
void BattleGround::RewardHonorToTeam(uint32 Honor, Team teamId)
{
    for (BattleGroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
    {
        if (itr->second.OfflineRemoveTime)
        {
            continue;
        }

        Player* plr = sObjectMgr.GetPlayer(itr->first);

        if (!plr)
        {
            sLog.outError("BattleGround:RewardHonorToTeam: %s not found!", itr->first.GetString().c_str());
            continue;
        }

        Team team = itr->second.PlayerTeam;
        if (!team)
        {
            team = plr->GetTeam();
        }

        if (team == teamId)
        {
            UpdatePlayerScore(plr, SCORE_BONUS_HONOR, Honor);
        }
    }
}

/// <summary>
/// Rewards the reputation to team.
/// </summary>
/// <param name="faction_id">The faction_id.</param>
/// <param name="Reputation">The reputation.</param>
/// <param name="teamId">The team id.</param>
void BattleGround::RewardReputationToTeam(uint32 faction_id, uint32 Reputation, Team teamId)
{
    FactionEntry const* factionEntry = sFactionStore.LookupEntry(faction_id);

    if (!factionEntry)
    {
        return;
    }

    for (BattleGroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
    {
        if (itr->second.OfflineRemoveTime)
        {
            continue;
        }

        Player* plr = sObjectMgr.GetPlayer(itr->first);

        if (!plr)
        {
            sLog.outError("BattleGround:RewardReputationToTeam: %s not found!", itr->first.GetString().c_str());
            continue;
        }

        Team team = itr->second.PlayerTeam;
        if (!team)
        {
            team = plr->GetTeam();
        }

        if (team == teamId)
        {
            plr->GetReputationMgr().ModifyReputation(factionEntry, Reputation);
        }
    }
}

/// <summary>
/// Updates the state of the world.
/// </summary>
/// <param name="Field">The field.</param>
/// <param name="Value">The value.</param>
void BattleGround::UpdateWorldState(uint32 Field, uint32 Value)
{
    WorldPacket data;
    sBattleGroundMgr.BuildUpdateWorldStatePacket(&data, Field, Value);
    SendPacketToAll(&data);
}

/// <summary>
/// Updates the world state for player.
/// </summary>
/// <param name="Field">The field.</param>
/// <param name="Value">The value.</param>
/// <param name="Source">The source.</param>
void BattleGround::UpdateWorldStateForPlayer(uint32 Field, uint32 Value, Player* Source)
{
    WorldPacket data;
    sBattleGroundMgr.BuildUpdateWorldStatePacket(&data, Field, Value);
    Source->GetSession()->SendPacket(&data);
}

/// <summary>
/// Ends the battle ground.
/// </summary>
/// <param name="winner">The winner.</param>
void BattleGround::EndBattleGround(Team winner)
{
#ifdef ENABLE_ELUNA
    if (Eluna* e = GetBgMap()->GetEluna())
    {
        e->OnBGEnd(this, GetTypeID(), GetInstanceID(), winner);
    }
#endif /* ENABLE_ELUNA */
    this->RemoveFromBGFreeSlotQueue();

    ArenaTeam* winner_arena_team = NULL;
    ArenaTeam* loser_arena_team = NULL;
    uint32 loser_rating = 0;
    uint32 winner_rating = 0;
    WorldPacket data;
    int32 winmsg_id = 0;

    if (winner == ALLIANCE)
    {
        winmsg_id = isBattleGround() ? LANG_BG_A_WINS : LANG_ARENA_GOLD_WINS;
        PlaySoundToAll(SOUND_ALLIANCE_WINS);                // alliance wins sound
    }
    else if (winner == HORDE)
    {
        winmsg_id = isBattleGround() ? LANG_BG_H_WINS : LANG_ARENA_GREEN_WINS;
        PlaySoundToAll(SOUND_HORDE_WINS);                   // horde wins sound
    }

    SetWinner(winner);

    SetStatus(STATUS_WAIT_LEAVE);
    // we must set it this way, because end time is sent in packet!
    m_EndTime = TIME_TO_AUTOREMOVE;

    // arena rating calculation
    if (isArena() && isRated())
    {
        winner_arena_team = sObjectMgr.GetArenaTeamById(GetArenaTeamIdForTeam(winner));
        loser_arena_team = sObjectMgr.GetArenaTeamById(GetArenaTeamIdForTeam(GetOtherTeam(winner)));
        if (winner_arena_team && loser_arena_team)
        {
            loser_rating = loser_arena_team->GetStats().rating;
            winner_rating = winner_arena_team->GetStats().rating;
            int32 winner_change = winner_arena_team->WonAgainst(loser_rating);
            int32 loser_change = loser_arena_team->LostAgainst(winner_rating);
            DEBUG_LOG("--- Winner rating: %u, Loser rating: %u, Winner change: %i, Loser change: %i ---", winner_rating, loser_rating, winner_change, loser_change);
            SetArenaTeamRatingChangeForTeam(winner, winner_change);
            SetArenaTeamRatingChangeForTeam(GetOtherTeam(winner), loser_change);
        }
        else
        {
            SetArenaTeamRatingChangeForTeam(ALLIANCE, 0);
            SetArenaTeamRatingChangeForTeam(HORDE, 0);
        }
    }

    for (BattleGroundPlayerMap::iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
    {
        Team team = itr->second.PlayerTeam;

        if (itr->second.OfflineRemoveTime)
        {
            // if rated arena match - make member lost!
            if (isArena() && isRated() && winner_arena_team && loser_arena_team)
            {
                if (team == winner)
                {
                    winner_arena_team->OfflineMemberLost(itr->first, loser_rating);
                }
                else
                {
                    loser_arena_team->OfflineMemberLost(itr->first, winner_rating);
                }
            }
            continue;
        }

        Player* plr = sObjectMgr.GetPlayer(itr->first);
        if (!plr)
        {
            sLog.outError("BattleGround:EndBattleGround %s not found!", itr->first.GetString().c_str());
            continue;
        }

        // should remove spirit of redemption
        if (plr->HasAuraType(SPELL_AURA_SPIRIT_OF_REDEMPTION))
        {
            plr->RemoveSpellsCausingAura(SPELL_AURA_MOD_SHAPESHIFT);
        }

        if (!plr->IsAlive())
        {
            plr->ResurrectPlayer(1.0f);
            plr->SpawnCorpseBones();
        }
        else
        {
            // needed cause else in av some creatures will kill the players at the end
            plr->CombatStop();
            plr->GetHostileRefManager().deleteReferences();
        }

        // this line is obsolete - team is set ALWAYS
        // if (!team) team = plr->GetTeam();

        // per player calculation
        if (isArena() && isRated() && winner_arena_team && loser_arena_team)
        {
            if (team == winner)
            {
                // update achievement BEFORE personal rating update
                ArenaTeamMember* member = winner_arena_team->GetMember(plr->GetObjectGuid());
                if (member)
                {
                    plr->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA, member->personal_rating);
                }

                winner_arena_team->MemberWon(plr, loser_rating);

                if (member)
                {
                    plr->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_PERSONAL_RATING, GetArenaType(), member->personal_rating);
                    plr->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_TEAM_RATING, GetArenaType(), winner_arena_team->GetStats().rating);
                }
            }
            else
            {
                loser_arena_team->MemberLost(plr, winner_rating);

                // Arena lost => reset the win_rated_arena having the "no_loose" condition
                plr->GetAchievementMgr().ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA, ACHIEVEMENT_CRITERIA_CONDITION_NO_LOOSE);
            }
        }

        uint32 win_kills = plr->GetRandomWinner() ? BG_REWARD_WINNER_HONOR_LAST : BG_REWARD_WINNER_HONOR_FIRST;
        uint32 loos_kills = plr->GetRandomWinner() ? BG_REWARD_LOOSER_HONOR_LAST : BG_REWARD_LOOSER_HONOR_FIRST;
        uint32 win_arena = plr->GetRandomWinner() ? BG_REWARD_WINNER_ARENA_LAST : BG_REWARD_WINNER_ARENA_FIRST;

        if (team == winner)
        {
            RewardMark(plr, ITEM_WINNER_COUNT);
            RewardQuestComplete(plr);

            if (IsRandom() || BattleGroundMgr::IsBGWeekend(GetTypeID()))
            {
                UpdatePlayerScore(plr, SCORE_BONUS_HONOR, GetBonusHonorFromKill(win_kills*4));
                plr->ModifyArenaPoints(win_arena);
                if (!plr->GetRandomWinner())
                {
                    plr->SetRandomWinner(true);
                }
            }
            plr->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_WIN_BG, 1);
        }
        else
        {
            RewardMark(plr, ITEM_LOSER_COUNT);
            if (IsRandom() || BattleGroundMgr::IsBGWeekend(GetTypeID()))
            {
                UpdatePlayerScore(plr, SCORE_BONUS_HONOR, GetBonusHonorFromKill(loos_kills*4));
            }
        }

        plr->CombatStopWithPets(true);

        BlockMovement(plr);

        sBattleGroundMgr.BuildPvpLogDataPacket(&data, this);
        plr->GetSession()->SendPacket(&data);

        BattleGroundQueueTypeId bgQueueTypeId = BattleGroundMgr::BGQueueTypeId(GetTypeID(), GetArenaType());
        sBattleGroundMgr.BuildBattleGroundStatusPacket(&data, this, plr->GetBattleGroundQueueIndex(bgQueueTypeId), STATUS_IN_PROGRESS, TIME_TO_AUTOREMOVE, GetStartTime(), GetArenaType(), plr->GetBGTeam());
        plr->GetSession()->SendPacket(&data);
        plr->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_BATTLEGROUND, 1);
    }

    if (isArena() && isRated() && winner_arena_team && loser_arena_team)
    {
        // update arena points only after increasing the player's match count!
        // obsolete: winner_arena_team->UpdateArenaPointsHelper();
        // obsolete: loser_arena_team->UpdateArenaPointsHelper();
        // save the stat changes
        winner_arena_team->SaveToDB();
        loser_arena_team->SaveToDB();
        // send updated arena team stats to players
        // this way all arena team members will get notified, not only the ones who participated in this match
        winner_arena_team->NotifyStatsChanged();
        loser_arena_team->NotifyStatsChanged();
    }

    if (winmsg_id)
    {
        SendMessageToAll(winmsg_id, CHAT_MSG_BG_SYSTEM_NEUTRAL);
    }
}

/// <summary>
/// Gets the bonus honor from kill.
/// </summary>
/// <param name="kills">The kills.</param>
/// <returns></returns>
uint32 BattleGround::GetBonusHonorFromKill(uint32 kills) const
{
    // variable kills means how many honorable kills you scored (so we need kills * honor_for_one_kill)
    return (uint32)MaNGOS::Honor::hk_honor_at_level(GetMaxLevel(), kills);
}

/// <summary>
/// Gets the battlemaster entry.
/// </summary>
/// <returns></returns>
uint32 BattleGround::GetBattlemasterEntry() const
{
    switch (GetTypeID(true))
    {
        case BATTLEGROUND_AV: return 15972;
        case BATTLEGROUND_WS: return 14623;
        case BATTLEGROUND_AB: return 14879;
        case BATTLEGROUND_EY: return 22516;
        case BATTLEGROUND_NA: return 20200;
        default:              return 0;
    }
}

/// <summary>
/// Rewards the mark.
/// </summary>
/// <param name="plr">The PLR.</param>
/// <param name="count">The count.</param>
void BattleGround::RewardMark(Player* plr, uint32 count)
{
    switch (GetTypeID(true))
    {
        case BATTLEGROUND_AV:
            if (count == ITEM_WINNER_COUNT)
            {
                RewardSpellCast(plr, SPELL_AV_MARK_WINNER);
            }
            else
            {
                RewardSpellCast(plr, SPELL_AV_MARK_LOSER);
            }
            break;
        case BATTLEGROUND_WS:
            if (count == ITEM_WINNER_COUNT)
            {
                RewardSpellCast(plr, SPELL_WS_MARK_WINNER);
            }
            else
            {
                RewardSpellCast(plr, SPELL_WS_MARK_LOSER);
            }
            break;
        case BATTLEGROUND_AB:
            if (count == ITEM_WINNER_COUNT)
            {
                RewardSpellCast(plr, SPELL_AB_MARK_WINNER);
            }
            else
            {
                RewardSpellCast(plr, SPELL_AB_MARK_LOSER);
            }
            break;
        case BATTLEGROUND_EY:                               // no rewards
        default:
            break;
    }
}

/// <summary>
/// Rewards the spell cast.
/// </summary>
/// <param name="plr">The PLR.</param>
/// <param name="spell_id">The spell_id.</param>
void BattleGround::RewardSpellCast(Player* plr, uint32 spell_id)
{
    // 'Inactive' this aura prevents the player from gaining honor points and battleground tokens
    if (plr->GetDummyAura(SPELL_AURA_PLAYER_INACTIVE))
    {
        return;
    }

    SpellEntry const* spellInfo = sSpellStore.LookupEntry(spell_id);
    if (!spellInfo)
    {
        sLog.outError("Battleground reward casting spell %u not exist.", spell_id);
        return;
    }

    plr->CastSpell(plr, spellInfo, true);
}

/// <summary>
/// Rewards the item.
/// </summary>
/// <param name="plr">The PLR.</param>
/// <param name="item_id">The item_id.</param>
/// <param name="count">The count.</param>
void BattleGround::RewardItem(Player* plr, uint32 item_id, uint32 count)
{
    // 'Inactive' this aura prevents the player from gaining honor points and battleground tokens
    if (plr->GetDummyAura(SPELL_AURA_PLAYER_INACTIVE))
    {
        return;
    }

    ItemPosCountVec dest;
    uint32 no_space_count = 0;
    uint8 msg = plr->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, item_id, count, &no_space_count);

    if (msg == EQUIP_ERR_ITEM_NOT_FOUND)
    {
        sLog.outErrorDb("Battleground reward item (Entry %u) not exist in `item_template`.", item_id);
        return;
    }

    if (msg != EQUIP_ERR_OK)                                // convert to possible store amount
    {
        count -= no_space_count;
    }

    if (count != 0 && !dest.empty())                        // can add some
    {
        if (Item* item = plr->StoreNewItem(dest, item_id, true, 0))
        {
            plr->SendNewItem(item, count, true, false);
        }
    }

    if (no_space_count > 0)
    {
        SendRewardMarkByMail(plr, item_id, no_space_count);
    }
}

/// <summary>
/// Sends the reward mark by mail.
/// </summary>
/// <param name="plr">The PLR.</param>
/// <param name="mark">The mark.</param>
/// <param name="count">The count.</param>
void BattleGround::SendRewardMarkByMail(Player* plr, uint32 mark, uint32 count)
{
    uint32 bmEntry = GetBattlemasterEntry();
    if (!bmEntry)
    {
        return;
    }

    ItemPrototype const* markProto = ObjectMgr::GetItemPrototype(mark);
    if (!markProto)
    {
        return;
    }

    if (Item* markItem = Item::CreateItem(mark, count, plr))
    {
        // save new item before send
        markItem->SaveToDB();                               // save for prevent lost at next mail load, if send fail then item will deleted

        int loc_idx = plr->GetSession()->GetSessionDbLocaleIndex();

        // subject: item name
        std::string subject = markProto->Name1;
        sObjectMgr.GetItemLocaleStrings(markProto->ItemId, loc_idx, &subject);

        // text
        std::string textFormat = plr->GetSession()->GetMangosString(LANG_BG_MARK_BY_MAIL);
        char textBuf[300];
        snprintf(textBuf, 300, textFormat.c_str(), GetName(), GetName());

        MailDraft(subject, textBuf)
        .AddItem(markItem)
        .SendMailTo(plr, MailSender(MAIL_CREATURE, bmEntry));
    }
}

/// <summary>
/// Rewards the quest complete.
/// </summary>
/// <param name="plr">The PLR.</param>
void BattleGround::RewardQuestComplete(Player* plr)
{
    uint32 quest = 0;
    switch (GetTypeID(true))
    {
        case BATTLEGROUND_AV:
            quest = SPELL_AV_QUEST_REWARD;
            break;
        case BATTLEGROUND_WS:
            quest = SPELL_WS_QUEST_REWARD;
            break;
        case BATTLEGROUND_AB:
            quest = SPELL_AB_QUEST_REWARD;
            break;
        case BATTLEGROUND_EY:
            quest = SPELL_EY_QUEST_REWARD;
            break;
        default:
            return;
    }

    RewardSpellCast(plr, quest);
}
