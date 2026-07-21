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

#ifndef MANGOS_H_PETMGR
#define MANGOS_H_PETMGR

#include "Platform/Define.h"
#include "Pet.h"  // PetSaveMode enum + MAX_PET_STABLES constant

class Player;

/**
 * PetMgr — owns a Player's pet-ownership metadata: the stable-slot
 * count persisted with the character row and the temporary-unsummon
 * tracking number used by transports / vehicles / mounts to bring the
 * same pet back after a short interruption.
 *
 * What this DOES NOT own:
 *  - The Pet creature itself (a Creature subclass living in the world,
 *    managed by Object/Pet.{cpp,h}). The pet object's lifecycle, AI,
 *    spells, leveling, and persistence to the `character_pet` table all
 *    stay in Pet.cpp.
 *  - The pet stable opcode handlers (`CMSG_STABLE_PET` and friends in
 *    WorldHandlers/NPCHandler.cpp). They keep reading and updating the
 *    slot count through Player's public accessors.
 *
 * What stays on Player.{cpp,h}:
 *  - The full public pet API (RemovePet / UnsummonPetTemporaryIfAny /
 *    ResummonPetTemporaryUnSummonedIfAny / RemovePetActionBar / temp-
 *    summon-number get/set + the new GetStableSlots) as inline
 *    delegates, so the ~25 external call sites in SpellAuras.cpp,
 *    Spell.cpp, BattleGround.cpp, Vehicle.cpp, MovementHandler.cpp,
 *    CharacterHandler.cpp, MiscHandler.cpp, WorldSession.cpp, Pet.cpp,
 *    NPCHandler.cpp, and Unit.cpp do not need to change.
 */
class PetMgr
{
    public:
        explicit PetMgr(Player* owner)
            : m_owner(owner), m_stableSlots(0), m_temporaryUnsummonedPetNumber(0)
        {
        }

        /// Number of paid stable slots the character has unlocked.
        /// Persisted to `characters`.stable_slots; clamped to
        /// MAX_PET_STABLES on load.
        uint32 GetStableSlots() const { return m_stableSlots; }
        void SetStableSlots(uint32 slots) { m_stableSlots = slots; }

        /// Called from Player::LoadFromDB with the raw column value.
        /// Clamps to MAX_PET_STABLES and logs a server error on
        /// out-of-range data so an operator can detect a tampered row.
        void LoadStableSlotsFromField(uint32 raw);

        /// Pet number that was active before a temporary unsummon (e.g.
        /// vehicle board / transport zone-in). Zero means no pending
        /// resummon. Read by Pet.cpp during save-mode dispatch and
        /// written by CharacterHandler on character creation when a
        /// hunter starts with a deity pet.
        uint32 GetTemporaryUnsummonedPetNumber() const { return m_temporaryUnsummonedPetNumber; }
        void SetTemporaryUnsummonedPetNumber(uint32 petnumber) { m_temporaryUnsummonedPetNumber = petnumber; }

        /// PET_SAVE_AS_CURRENT-style dismissal. If the owner currently
        /// has a controlled pet, asks it to unsummon with the given
        /// mode (see PetSaveMode in PetDefines.h).
        void Remove(PetSaveMode mode);

        /// SMSG_PET_SPELLS with an empty guid — clears the pet action
        /// bar UI on the client.
        void RemoveActionBar();

        /// If the owner has a controlled, non-temporary pet, stash its
        /// pet-number into m_temporaryUnsummonedPetNumber so we can
        /// bring it back later, then unsummon with PET_SAVE_AS_CURRENT.
        /// No-op if no eligible pet.
        void UnsummonTemporaryIfAny();

        /// Counterpart to UnsummonTemporaryIfAny: if we stashed a pet
        /// number AND it's now appropriate to resummon (no pending
        /// vehicle, mount, etc.), load that pet from DB. Clears the
        /// stash either way (success or "not yet").
        void ResummonTemporaryUnsummonedIfAny();

    private:
        Player* m_owner;
        uint32  m_stableSlots;
        uint32  m_temporaryUnsummonedPetNumber;
};

#endif // MANGOS_H_PETMGR
