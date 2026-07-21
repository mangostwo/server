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

#include "Utilities/Util.h"
#include "Utilities/PackedValues.h"
#include "Common/TimeConstants.h"
#include "HonorMgr.h"
#include "Player.h"
#include "Creature.h"
#include "WorldSession.h"
#include "World.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "ObjectGuid.h"
#include "UpdateFields.h"
#include "Formulas.h"

void HonorMgr::UpdateKills()
{
    /// called when rewarding honor and at each save
    time_t now = time(NULL);
    time_t today = (time(NULL) / DAY) * DAY;

    if (m_lastUpdateTime < today)
    {
        time_t yesterday = today - DAY;

        uint16 kills_today = PAIR32_LOPART(m_owner->GetUInt32Value(PLAYER_FIELD_KILLS));

        // update yesterday's contribution
        if (m_lastUpdateTime >= yesterday)
        {
            m_owner->SetUInt32Value(PLAYER_FIELD_YESTERDAY_CONTRIBUTION, m_owner->GetUInt32Value(PLAYER_FIELD_TODAY_CONTRIBUTION));

            // this is the first update today, reset today's contribution
            m_owner->SetUInt32Value(PLAYER_FIELD_TODAY_CONTRIBUTION, 0);
            m_owner->SetUInt32Value(PLAYER_FIELD_KILLS, MAKE_PAIR32(0, kills_today));
        }
        else
        {
            // no honor/kills yesterday or today, reset
            m_owner->SetUInt32Value(PLAYER_FIELD_YESTERDAY_CONTRIBUTION, 0);
            m_owner->SetUInt32Value(PLAYER_FIELD_KILLS, 0);
        }
    }

    m_lastUpdateTime = now;
}

bool HonorMgr::Reward(Unit* uVictim, uint32 groupsize, float honor)
{
    // do not reward honor in arenas, but enable onkill spellproc
    if (m_owner->InArena())
    {
        if (!uVictim || uVictim == m_owner || uVictim->GetTypeId() != TYPEID_PLAYER)
        {
            return false;
        }

        if (m_owner->GetBGTeam() == (reinterpret_cast<Player*>(uVictim))->GetBGTeam())
        {
            return false;
        }

        return true;
    }

    // 'Inactive' this aura prevents the player from gaining honor points and battleground tokens
    if (m_owner->GetDummyAura(SPELL_AURA_PLAYER_INACTIVE))
    {
        return false;
    }

    ObjectGuid victim_guid;
    uint32 victim_rank = 0;

    // need call before fields update to have chance move yesterday data to appropriate fields before today data change.
    UpdateKills();

    if (honor <= 0)
    {
        if (!uVictim || uVictim == m_owner || uVictim->HasAuraType(SPELL_AURA_NO_PVP_CREDIT))
        {
            return false;
        }

        victim_guid = uVictim->GetObjectGuid();

        if (uVictim->GetTypeId() == TYPEID_PLAYER)
        {
            Player* pVictim = reinterpret_cast<Player*>(uVictim);

            if (m_owner->GetTeam() == pVictim->GetTeam() && !sWorld.IsFFAPvPRealm())
            {
                return false;
            }

            float f = 1;                                    // need for total kills (?? need more info)
            uint32 k_grey = 0;
            uint32 k_level = m_owner->getLevel();
            uint32 v_level = pVictim->getLevel();

            {
                // PLAYER_CHOSEN_TITLE VALUES DESCRIPTION
                //  [0]      Just name
                //  [1..14]  Alliance honor titles and player name
                //  [15..28] Horde honor titles and player name
                //  [29..38] Other title and player name
                //  [39+]    Nothing
                uint32 victim_title = pVictim->GetUInt32Value(PLAYER_CHOSEN_TITLE);
                // Get Killer titles, CharTitlesEntry::bit_index
                // Ranks:
                //  title[1..14]  -> rank[5..18]
                //  title[15..28] -> rank[5..18]
                //  title[other]  -> 0
                if (victim_title == 0)
                {
                    victim_guid.Clear();                    // Don't show HK: <rank> message, only log.
                }
                else if (victim_title < 15)
                {
                    victim_rank = victim_title + 4;
                }
                else if (victim_title < 29)
                {
                    victim_rank = victim_title - 14 + 4;
                }
                else
                {
                    victim_guid.Clear();                    // Don't show HK: <rank> message, only log.
                }
            }

            k_grey = MaNGOS::XP::GetGrayLevel(k_level);

            if (v_level <= k_grey)
            {
                return false;
            }

            float diff_level = (k_level == k_grey) ? 1 : ((float(v_level) - float(k_grey)) / (float(k_level) - float(k_grey)));

            int32 v_rank = 1;                               // need more info

            honor = ((f * diff_level * (190 + v_rank * 10)) / 6);
            honor *= float(k_level) / 70.0f;                // factor of dependence on levels of the killer

            // count the number of playerkills in one day
            m_owner->ApplyModUInt32Value(PLAYER_FIELD_KILLS, 1, true);
            // and those in a lifetime
            m_owner->ApplyModUInt32Value(PLAYER_FIELD_LIFETIME_HONORABLE_KILLS, 1, true);
            m_owner->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_EARN_HONORABLE_KILL);
            m_owner->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HK_CLASS, pVictim->getClass());
            m_owner->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HK_RACE, pVictim->getRace());
        }
        else
        {
            Creature* cVictim = reinterpret_cast<Creature*>(uVictim);

            if (!cVictim->IsRacialLeader())
            {
                return false;
            }

            honor = 100;                                    // ??? need more info
            victim_rank = 19;                               // HK: Leader
        }
    }

    if (uVictim != NULL)
    {
        honor *= sWorld.getConfig(CONFIG_FLOAT_RATE_HONOR);
        honor *= (m_owner->GetMaxPositiveAuraModifier(SPELL_AURA_MOD_HONOR_GAIN) + 100.0f) / 100.0f;

        if (groupsize > 1)
        {
            honor /= groupsize;
        }

        honor *= (((float)urand(8, 12)) / 10);              // approx honor: 80% - 120% of real honor
    }

    // honor - for show honor points in log
    // victim_guid - for show victim name in log
    // victim_rank [1..4]  HK: <dishonored rank>
    // victim_rank [5..19] HK: <alliance\horde rank>
    // victim_rank [0,20+] HK: <>
    WorldPacket data(SMSG_PVP_CREDIT, 4 + 8 + 4);
    data << uint32(honor);
    data << ObjectGuid(victim_guid);
    data << uint32(victim_rank);
    m_owner->GetSession()->SendPacket(&data);

    // add honor points
    m_owner->ModifyHonorPoints(int32(honor));

    m_owner->ApplyModUInt32Value(PLAYER_FIELD_TODAY_CONTRIBUTION, uint32(honor), true);
    return true;
}
