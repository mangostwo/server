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

#ifndef MANGOS_ARBITER_ARBITERSHADOW_H
#define MANGOS_ARBITER_ARBITERSHADOW_H

#include "Arbiter/ShadowClassifier.h"

#include <string>

class Unit;

/// PR1 shadow mode: mirrors every MotionMaster facade call into an ArbiterModel and,
/// once per update, classifies where the model's selection differs from the stack's top.
/// Owned by MotionMaster only while Movement.ArbiterShadow is on; it never moves anything.
class ArbiterShadow
{
    public:
        /// \arg \c owner the unit whose generator stack is being shadowed.
        explicit ArbiterShadow(Unit& owner) : m_owner(owner) {}

        /// Mirrors the default generator MotionMaster::Initialize just pushed.
        void OnFactoryDefault(MovementGeneratorType type);
        /// Mirrors one facade request; \c id is the MovementInform id, 0 when none.
        void OnRequest(Arbiter::MoveKind kind, uint32 id);
        /// Mirrors Clear(reset, all); \c all also drops the default.
        void OnClear(bool all);
        /// Mirrors MovementExpired on the generator the stack is about to pop, by kind.
        void OnExpired(MovementGeneratorType type);
        /// Classifies stack against model and logs a divergence once per distinct report.
        void Compare(Arbiter::StackTypes const& stack);

    private:
        Unit& m_owner;              ///< the shadowed unit, for the trace's guid
        Arbiter::ArbiterModel m_model;   ///< the selection model run beside the stack
        std::string m_lastReport;   ///< one line per distinct divergence, not one per tick
};

#endif
