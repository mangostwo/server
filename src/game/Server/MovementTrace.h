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

#ifndef MANGOS_MOVEMENTTRACE_H
#define MANGOS_MOVEMENTTRACE_H

#include "Platform/Define.h"

#include <cstddef>
#include <string>

class Creature;
class Map;
class MovementInfo;
class WorldSession;

/**
 * @brief A CSV record of every movement packet a client sends, for offline analysis.
 *
 * Written for the question this fork keeps having to answer: what does the client
 * ACTUALLY tell us about a player standing on a global transport? The packet carries two
 * coordinate systems and two timelines, and the server's estimate of the vessel agrees
 * with none of them -- so the only way to reason about it is to write both sides of every
 * packet down and subtract them afterwards.
 *
 * Deliberately stateless: no per-session history, no deltas computed here. Every row
 * carries its own absolute timestamps and the vessel state as of that instant, so rate,
 * jitter and drift are columns you subtract in a spreadsheet rather than a registry that
 * could fall out of step with the session it describes.
 */
namespace MovementTrace
{
    /// Cheap enough to call on the hot path before building anything.
    bool Enabled();

    /**
     * @brief Open (or close) the trace.
     *
     * Two files, named by `MovementTrace.File` and `MovementTrace.DeckFile` and landing
     * in `LogsDir`: what the clients send, and how the crew of ONE deck map moves. Both
     * append, so a run can be resumed.
     *
     * `deckMapId` is that deck. A creature has no client and no packet, so its trace can
     * only ever be the server's own relocations -- and a whole server's worth of those is
     * noise. Naming the one deck under study is what makes the file readable.
     */
    bool SetEnabled(bool on, uint32 deckMapId = 0);

    /// Path of the packet trace, empty when off.
    std::string const& FileName();

    /// Path of the deck-crew trace, empty when off or when no deck is named.
    std::string const& DeckFileName();

    /// The deck map whose crew is traced, 0 for none. Read on the relocation hot path.
    uint32 DeckMapId();

    /// Open the trace at start-up if the config asks for it.
    void Initialize();

    /**
     * @brief One row for one client packet.
     *
     * `stage` names the moment: "recv" before the packet is applied -- the vessel columns
     * then still hold the pose we were guessing with -- and "post" after, when they hold
     * what the client's claim solved to. Subtracting the pair is the whole point.
     */
    void Packet(WorldSession const* session, uint16 opcode, MovementInfo const& mi,
                char const* stage, size_t bytes);

    /// One row for a state change that carries no MovementInfo of its own: boarding,
    /// leaving, a worldport landing, a rejected packet.
    void Event(WorldSession const* session, char const* event, char const* detail);

    /**
     * @brief One row per server-driven move of a creature on the traced deck.
     *
     * The crew's counterpart to Packet(): nobody sends these, the server decides them, so
     * the row records who decided -- which movement generator, and whether a spline was
     * still running -- alongside the position. Coordinates are deck-local, because on this
     * map that is all a position is.
     */
    void DeckMove(Map* on, Creature* creature, float x, float y, float z, float o);
}

#endif
