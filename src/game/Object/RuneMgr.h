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

#ifndef MANGOS_H_RUNEMGR
#define MANGOS_H_RUNEMGR

#include "Common.h"

class Player;

#define MAX_RUNES               6
#define RUNE_COOLDOWN           (2*5*IN_MILLISECONDS)       // msec

enum RuneType
{
    RUNE_BLOOD                  = 0,
    RUNE_UNHOLY                 = 1,
    RUNE_FROST                  = 2,
    RUNE_DEATH                  = 3,
    NUM_RUNE_TYPES              = 4
};

struct RuneInfo
{
    uint8  BaseRune;
    uint8  CurrentRune;
    uint16 Cooldown;                                        // msec
};

struct Runes
{
    RuneInfo runes[MAX_RUNES];
    uint8 runeState;                                        // mask of available runes

    void SetRuneState(uint8 index, bool set = true)
    {
        if (set)
        {
            runeState |= (1 << index);                      // usable
        }
        else
        {
            runeState &= ~(1 << index);                     // on cooldown
        }
    }
};

/**
 * @brief Owns a Player's death-knight rune state and lifecycle.
 *
 * The per-slot rune data (base/current type + cooldown) is only allocated
 * for death knights, via InitRunes; m_runes stays NULL for every other
 * class. Player exposes the public rune API as thin delegating accessors so
 * external callers (spell/aura code, the rune-power UI) drive runes through
 * Player without referencing RuneMgr.
 *
 * RuneMgr stores a non-owning pointer to its Player (used to read the class
 * and to send rune packets / write rune-regen update fields) and fully owns
 * the heap-allocated Runes block, which it frees in its destructor.
 */
class RuneMgr
{
    public:
        /**
         * @brief Constructs a RuneMgr bound to its owning Player.
         *
         * @param owner The Player whose rune state this manager owns.
         */
        explicit RuneMgr(Player* owner) : m_owner(owner), m_runes(NULL) {}

        /// Frees the heap-allocated rune block (NULL for non-death-knights).
        ~RuneMgr() { delete m_runes; }

        // Owns a raw pointer; copying would double-free.
        RuneMgr(const RuneMgr&) = delete;
        RuneMgr& operator=(const RuneMgr&) = delete;

        uint8 GetRunesState() const { return m_runes->runeState; }
        RuneType GetBaseRune(uint8 index) const { return RuneType(m_runes->runes[index].BaseRune); }
        RuneType GetCurrentRune(uint8 index) const { return RuneType(m_runes->runes[index].CurrentRune); }
        uint16 GetRuneCooldown(uint8 index) const { return m_runes->runes[index].Cooldown; }
        bool IsBaseRuneSlotsOnCooldown(RuneType runeType) const;
        void SetBaseRune(uint8 index, RuneType baseRune) { m_runes->runes[index].BaseRune = baseRune; }
        void SetCurrentRune(uint8 index, RuneType currentRune) { m_runes->runes[index].CurrentRune = currentRune; }
        void SetRuneCooldown(uint8 index, uint16 cooldown) { m_runes->runes[index].Cooldown = cooldown; m_runes->SetRuneState(index, (cooldown == 0) ? true : false); }
        void ConvertRune(uint8 index, RuneType newType);
        bool ActivateRunes(RuneType type, uint32 count);
        void ResyncRunes();
        void AddRunePower(uint8 index);
        void InitRunes();

    private:
        Player* m_owner;    ///< Non-owning pointer to the Player this manager belongs to.
        Runes*  m_runes;    ///< Heap-allocated rune block, owned here; NULL until InitRunes for a death knight.
};

#endif // MANGOS_H_RUNEMGR
