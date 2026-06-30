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

#ifndef MANGOS_H_GLYPHMGR
#define MANGOS_H_GLYPHMGR

#include "Common.h"
#include "SharedDefines.h"                                  // MAX_GLYPH_SLOT_INDEX, MAX_TALENT_SPEC_COUNT

class Player;
class QueryResult;

/**
 * @brief Lifecycle state of a glyph slot's dirty flag.
 *
 * Used by GlyphMgr::Save to decide INSERT vs UPDATE vs DELETE on the
 * character_glyphs row.
 */
enum GlyphUpdateState
{
    GLYPH_UNCHANGED = 0,    ///< Slot matches the persisted row, no DB work needed.
    GLYPH_CHANGED   = 1,    ///< Slot was modified, emit UPDATE.
    GLYPH_NEW       = 2,    ///< Slot has no persisted row yet, emit INSERT.
    GLYPH_DELETED   = 3     ///< Slot was cleared, emit DELETE.
};

/**
 * @brief Per-slot glyph state with dirty-tracking state machine.
 *
 * SetId encodes the transitions between GlyphUpdateState values and decides
 * what DB operation Save should emit for this slot.
 */
struct Glyph
{
    uint32 id;                  ///< DBC glyph property id, 0 when slot is empty.
    GlyphUpdateState uState;    ///< Current dirty state for DB persistence.

    /**
     * @brief Default-constructs an empty, unchanged glyph slot.
     */
    Glyph() : id(0), uState(GLYPH_UNCHANGED) {}

    /**
     * @brief Returns the slot's current glyph id.
     *
     * @return The glyph property id, or 0 if the slot is empty.
     */
    uint32 GetId() const { return id; }

    /**
     * @brief Sets the slot's glyph id and updates the dirty state.
     *
     * Encodes the transitions between glyph dirty states so that Save emits
     * the correct INSERT / UPDATE / DELETE for this row.
     *
     * @param newId The new glyph property id, or 0 to clear the slot.
     */
    void SetId(uint32 newId)
    {
        if (newId == id)
        {
            return;
        }

        if (id == 0 && uState == GLYPH_UNCHANGED)           // not yet in db and not yet saved
        {
            uState = GLYPH_NEW;
        }
        else if (newId == 0)
        {
            if (uState == GLYPH_NEW)                        // delete before add new -> no change
            {
                uState = GLYPH_UNCHANGED;
            }
            else                                            // delete existing data
            {
                uState = GLYPH_DELETED;
            }
        }
        else if (uState != GLYPH_NEW)                       // if not new data, change current data
        {
            uState = GLYPH_CHANGED;
        }

        id = newId;
    }
};

/**
 * @brief Owns a Player's per-spec glyph state and lifecycle methods.
 *
 * Lifecycle methods include Init / Apply / Load / Save. Player exposes
 * the public glyph API as thin delegating accessors so external callers
 * (CharacterHandler, SpellEffects::EffectApplyGlyph, the GM .modify
 * command) drive glyphs through Player without referencing GlyphMgr.
 *
 * GlyphMgr stores a non-owning pointer to its Player. The pointer is used
 * to read Player's level / active spec / spec count and to call Player
 * methods that touch update fields, the spell system, or the character
 * database. GlyphMgr fully owns m_glyphs; Player accesses it only through
 * GlyphMgr's API.
 */
class GlyphMgr
{
    public:
        /**
         * @brief Constructs a GlyphMgr bound to its owning Player.
         *
         * @param owner The Player whose glyphs this manager owns.
         */
        explicit GlyphMgr(Player* owner) : m_owner(owner) {}

        /**
         * @brief Refreshes glyph slot types and unlock mask for the owner's level.
         *
         * Resets the slot type bitmap based on the owner's level and sets
         * PLAYER_GLYPHS_ENABLED with the bitmask of slots unlocked at this
         * level. Called on level change and on character creation.
         */
        void InitGlyphsForLevel();

        /**
         * @brief Apply or remove the spell from a single glyph slot on the owner.
         *
         * @param slot  The glyph slot index.
         * @param apply True to cast and write the slot's spell, false to remove it.
         */
        void ApplyGlyph(uint8 slot, bool apply);

        /**
         * @brief Apply or remove all glyphs in the active spec.
         *
         * @param apply True to apply all slots, false to remove them.
         */
        void ApplyAll(bool apply);

        /**
         * @brief Load glyph rows from a SELECT result against character_glyphs.
         *
         * @param result The DB query result holding (spec, slot, glyph) rows.
         */
        void Load(QueryResult* result);

        /**
         * @brief Persist dirty glyph slots to character_glyphs.
         *
         * Emits INSERT / UPDATE / DELETE for each dirty slot, then clears the
         * dirty flags.
         */
        void Save();

        /**
         * @brief Returns the glyph id stored in a given spec / slot.
         *
         * @param spec The talent spec index.
         * @param slot The glyph slot index.
         * @return The glyph property id in that slot, or 0 if empty.
         */
        uint32 GetGlyph(uint8 spec, uint8 slot) const { return m_glyphs[spec][slot].GetId(); }

        /**
         * @brief Writes a glyph id into a given spec / slot.
         *
         * @param spec The talent spec index.
         * @param slot The glyph slot index.
         * @param id   The new glyph property id, or 0 to clear the slot.
         */
        void   SetGlyph(uint8 spec, uint8 slot, uint32 id) { m_glyphs[spec][slot].SetId(id); }

    private:
        Player* m_owner;                                            ///< Non-owning pointer to the Player this manager belongs to.
        Glyph   m_glyphs[MAX_TALENT_SPEC_COUNT][MAX_GLYPH_SLOT_INDEX];  ///< Per-spec, per-slot glyph state owned by this manager.
};

#endif // MANGOS_H_GLYPHMGR
