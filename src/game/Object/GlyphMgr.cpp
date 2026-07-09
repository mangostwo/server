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

#include "GlyphMgr.h"
#include "Player.h"
#include "DBCStores.h"
#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "UpdateFields.h"

void GlyphMgr::InitGlyphsForLevel()
{
    for (uint32 i = 0; i < sGlyphSlotStore.GetNumRows(); ++i)
    {
        if (GlyphSlotEntry const* gs = sGlyphSlotStore.LookupEntry(i))
            if (gs->Tooltip)
            {
                m_owner->SetGlyphSlot(gs->Tooltip - 1, gs->Id);
            }
    }

    uint32 level = m_owner->getLevel();
    uint32 value = 0;

    // 0x3F = 0x01 | 0x02 | 0x04 | 0x08 | 0x10 | 0x20 for 80 level
    if (level >= 15)
    {
        value |= (0x01 | 0x02);
    }
    if (level >= 30)
    {
        value |= 0x08;
    }
    if (level >= 50)
    {
        value |= 0x04;
    }
    if (level >= 70)
    {
        value |= 0x10;
    }
    if (level >= 80)
    {
        value |= 0x20;
    }

    m_owner->SetUInt32Value(PLAYER_GLYPHS_ENABLED, value);
}

void GlyphMgr::ApplyGlyph(uint8 slot, bool apply)
{
    if (uint32 glyph = GetGlyph(m_owner->GetActiveSpec(), slot))
    {
        if (GlyphPropertiesEntry const* gp = sGlyphPropertiesStore.LookupEntry(glyph))
        {
            if (apply)
            {
                m_owner->CastSpell(m_owner, gp->SpellID, true);
                m_owner->SetUInt32Value(PLAYER_FIELD_GLYPHS_1 + slot, glyph);
            }
            else
            {
                m_owner->RemoveAurasDueToSpell(gp->SpellID);
                m_owner->SetUInt32Value(PLAYER_FIELD_GLYPHS_1 + slot, 0);
            }
        }
    }
}

void GlyphMgr::ApplyAll(bool apply)
{
    for (uint8 i = 0; i < MAX_GLYPH_SLOT_INDEX; ++i)
    {
        ApplyGlyph(i, apply);
    }
}

void GlyphMgr::Load(QueryResult* result)
{
    if (!result)
    {
        return;
    }

    //         0     1     2
    // "SELECT spec, slot, glyph FROM character_glyphs WHERE guid='%u'"

    do
    {
        Field* fields = result->Fetch();
        uint8 spec = fields[0].GetUInt8();
        uint8 slot = fields[1].GetUInt8();
        uint32 glyph = fields[2].GetUInt32();

        GlyphPropertiesEntry const* gp = sGlyphPropertiesStore.LookupEntry(glyph);
        if (!gp)
        {
            sLog.outError("Player %s has not existing glyph entry %u on index %u, spec %u", m_owner->GetName(), glyph, slot, spec);
            CharacterDatabase.PExecute("DELETE FROM `character_glyphs` WHERE `glyph` = %u", glyph);
            continue;
        }

        GlyphSlotEntry const* gs = sGlyphSlotStore.LookupEntry(m_owner->GetGlyphSlot(slot));
        if (!gs)
        {
            sLog.outError("Player %s has not existing glyph slot entry %u on index %u, spec %u", m_owner->GetName(), m_owner->GetGlyphSlot(slot), slot, spec);
            CharacterDatabase.PExecute("DELETE FROM `character_glyphs` WHERE `slot` = %u AND `spec` = %u AND `guid` = %u", slot, spec, m_owner->GetGUIDLow());
            continue;
        }

        if (gp->GlyphSlotFlags != gs->Type)
        {
            sLog.outError("Player %s has glyph with typeflags %u in slot with typeflags %u, removing.", m_owner->GetName(), gp->GlyphSlotFlags, gs->Type);
            CharacterDatabase.PExecute("DELETE FROM `character_glyphs` WHERE `slot` = %u AND `spec` = %u AND `guid` = %u", slot, spec, m_owner->GetGUIDLow());
            continue;
        }

        m_glyphs[spec][slot].id = glyph;
    }
    while (result->NextRow());

    delete result;
}

void GlyphMgr::Save()
{
    static SqlStatementID insertGlyph ;
    static SqlStatementID updateGlyph ;
    static SqlStatementID deleteGlyph ;

    for (uint8 spec = 0; spec < m_owner->GetSpecsCount(); ++spec)
    {
        for (uint8 slot = 0; slot < MAX_GLYPH_SLOT_INDEX; ++slot)
        {
            switch (m_glyphs[spec][slot].uState)
            {
                case GLYPH_NEW:
                {
                    SqlStatement stmt = CharacterDatabase.CreateStatement(insertGlyph, "INSERT INTO `character_glyphs` (`guid`, `spec`, `slot`, `glyph`) VALUES (?, ?, ?, ?)");
                    stmt.PExecute(m_owner->GetGUIDLow(), spec, slot, m_glyphs[spec][slot].GetId());
                }
                break;
                case GLYPH_CHANGED:
                {
                    SqlStatement stmt = CharacterDatabase.CreateStatement(updateGlyph, "UPDATE `character_glyphs` SET `glyph` = ? WHERE `guid` = ? AND `spec` = ? AND `slot` = ?");
                    stmt.PExecute(m_glyphs[spec][slot].GetId(), m_owner->GetGUIDLow(), spec, slot);
                }
                break;
                case GLYPH_DELETED:
                {
                    SqlStatement stmt = CharacterDatabase.CreateStatement(deleteGlyph, "DELETE FROM `character_glyphs` WHERE `guid` = ? AND `spec` = ? AND `slot` = ?");
                    stmt.PExecute(m_owner->GetGUIDLow(), spec, slot);
                }
                break;
                case GLYPH_UNCHANGED:
                    break;
            }
            m_glyphs[spec][slot].uState = GLYPH_UNCHANGED;
        }
    }
}
