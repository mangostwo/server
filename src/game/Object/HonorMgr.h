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

#ifndef MANGOS_H_HONORMGR
#define MANGOS_H_HONORMGR

#include "Common.h"

class Player;
class Unit;

/**
 * @brief Owns a Player's honor-system bookkeeping and per-kill reward calc.
 *
 * Scope: today / yesterday kill-and-contribution rollover, and the
 * calculation that converts a kill into honor points + an SMSG_PVP_CREDIT
 * packet to the client.
 *
 * What stays on Player and is NOT in HonorMgr:
 *  - isHonorOrXPTarget(): a victim-eligibility filter that gates BOTH honor
 *    and XP rewards, so it does not belong in an honor-specific module.
 *  - The honor-points balance (PLAYER_FIELD_HONOR_CURRENCY) and its
 *    Get/Set/ModifyHonorPoints accessors — HonorMgr awards honor through
 *    Player::ModifyHonorPoints but does not own the long-term balance.
 *  - The kill-count entity fields (PLAYER_FIELD_KILLS,
 *    PLAYER_FIELD_LIFETIME_HONORABLE_KILLS) — those live on Player's
 *    update mask, mutated via Player methods called through m_owner.
 */
class HonorMgr
{
    public:
        /**
         * @brief Constructs a HonorMgr bound to its owning Player.
         *
         * @param owner The Player whose honor bookkeeping this manager owns.
         */
        explicit HonorMgr(Player* owner) : m_owner(owner), m_lastUpdateTime(time(NULL)) {}

        /**
         * @brief Rolls today's kills/contribution over to yesterday once a calendar day has passed.
         *
         * Called from Reward before crediting a new kill, at each character
         * save, and indirectly on character load.
         */
        void UpdateKills();

        /**
         * @brief Compute honor points for a kill and credit them to the player.
         *
         * Sends an SMSG_PVP_CREDIT packet and adds honor through
         * Player::ModifyHonorPoints when the kill is eligible. In arenas no
         * honor flows but on-kill spell procs are still allowed to fire.
         *
         * @param uVictim   The killed unit (player or racial-leader creature). May be NULL.
         * @param groupsize Number of group members to split the honor across; 1 means solo.
         * @param honor     Explicit honor value to award; pass a non-positive value to have it computed.
         * @return True if honor was awarded or arena on-kill procs should fire; false otherwise.
         */
        bool Reward(Unit* uVictim, uint32 groupsize, float honor);

        /**
         * @brief Reset the timestamp that the daily rollover compares against.
         *
         * Called from Player::LoadFromDB with the saved logout time so a long
         * absence triggers the correct yesterday-shift on first save.
         *
         * @param t The new last-update timestamp.
         */
        void SetLastKillUpdate(time_t t) { m_lastUpdateTime = t; }

    private:
        Player* m_owner;            ///< Non-owning pointer to the Player this manager belongs to.
        time_t  m_lastUpdateTime;   ///< Timestamp the daily kill rollover compares against.
};

#endif // MANGOS_H_HONORMGR
