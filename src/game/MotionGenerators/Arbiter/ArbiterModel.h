/**
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2026 MaNGOS <https://www.getmangos.eu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

#ifndef MANGOS_ARBITER_ARBITERMODEL_H
#define MANGOS_ARBITER_ARBITERMODEL_H

#include "MoveContract.h"

#include <optional>
#include <vector>

namespace Arbiter
{
    /// One held move, wherever it sits in the model (default, combat or a command layer).
    struct Held
    {
        MoveKind kind;  ///< what is held
        uint32   id;    ///< MovementInform id, 0 when none
        uint32   seq;   ///< arrival order; newest wins ties and tells a resume from a fresh start
    };

    /// One selection-model event, drained by the caller after a mutating call.
    struct MovementEvent
    {
        /// What happened to `who`: it finished for good, it was masked out, it came back
        /// on top, or the default layer swapped to a different kind entirely.
        enum class Kind : uint8 { Finished, Suspended, Resumed, DefaultSwapped };
        Kind         kind;    ///< what happened
        MoveKind     who;     ///< the kind it happened to
        uint32       id;      ///< that entry's MovementInform id
        FinishReason reason;  ///< why, for Finished/Suspended/DefaultSwapped
    };

    /// The pure selection core (design §4): one default, one combat behaviour, one
    /// command per layer above them. No Unit, no driver, no clock: it only decides.
    class ArbiterModel
    {
        public:
            /// Factory default: swap, nothing cancelled.
            void InstallDefault(MoveKind kind);
            /// Generic request entry: derives layer and policy from the kind, applies
            /// self-expiry (Home/Distract/Effect) and the policy's cancellation effects.
            void Request(MoveRequest const& request);
            /// §5 Clear(reset, all) projection: drop every command and combat, and the
            /// default too when `all`.
            void Clear(bool all);
            /// MovementExpired / Update()==false on whatever is currently selected.
            void ExpireSelected();
            /// Finish whatever is currently selected, for the given reason.
            void FinishSelected(FinishReason reason);
            /// Cancel the Control command if it still holds `kind` (D3: by identity).
            void CancelControl(MoveKind kind);

            /// True when nothing is selected (no default, no combat, no command).
            bool Empty() const;
            /// The currently selected entry, if any.
            std::optional<Held> Selected() const;
            /// The layer the current selection lives on, if any.
            std::optional<Layer> SelectedLayer() const;
            /// The Default-layer entry, if any.
            std::optional<Held> const& Default() const { return m_default; }
            /// The Combat-layer entry, if any.
            std::optional<Held> const& Combat() const { return m_combat; }
            /// The entry held on the given command layer, if any.
            std::optional<Held> const& Command(Layer layer) const { return m_commands[static_cast<uint8>(layer)]; }
            /// Every held entry, ascending layer order (Default, Combat, then commands).
            std::vector<Held> Contents() const;
            /// Take and clear the accumulated event log.
            std::vector<MovementEvent> DrainEvents();

        private:
            /// Finish the entry in `slot`, if any, logging Finished and clearing it.
            void Finish(std::optional<Held>& slot, FinishReason reason);
            /// Compare the selection before and after a mutation and log Suspended/Resumed.
            void Reselect(std::optional<Held> const& before);
            /// Apply a Default-layer request (§4.2): swap, Idle-as-command, Follow fallback.
            void RequestDefault(MoveRequest const& request, Held const& held, Policy policy);
            /// Apply a command-layer request: supersede the layer, then Override cancels below it.
            void RequestCommand(MoveRequest const& request, Held const& held, Layer layer, Policy policy);

            std::optional<Held> m_default;         ///< the Default-layer entry
            std::optional<Held> m_fallbackDefault;  ///< the default FollowTarget replaced
            std::optional<Held> m_combat;          ///< the Combat-layer entry
            std::optional<Held> m_commands[static_cast<uint8>(Layer::Count)];  ///< per-layer commands
            uint32 m_seq = 0;                      ///< monotonic arrival counter
            std::vector<MovementEvent> m_events;   ///< accumulated since the last DrainEvents
    };
}

#endif
