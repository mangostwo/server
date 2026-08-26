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

#ifndef MANGOS_ARBITER_SHADOWCLASSIFIER_H
#define MANGOS_ARBITER_SHADOWCLASSIFIER_H

#include "ArbiterModel.h"

#include <string>
#include <vector>

/// Pure diagnostics: compares the legacy generator stack against the
/// arbiter's selection model and names how, if at all, they disagree
/// (design §7, PR1). No Unit, no driver -- the game-side shadow (Task 5)
/// calls this once per update and only ever logs the result.
namespace Arbiter
{
    /// Why the stack's top and the model's selection differ (design §7, PR1).
    enum class Divergence : uint8
    {
        None,                   ///< the stack agrees with the model
        LayerOrder,             ///< the stack runs the last push, the model the highest layer
        SupersededOneShot,      ///< a stale one-shot the model superseded still sits in (or resumed from) the stack
        TargetedMultiplicity,   ///< the stack holds more than one chase/follow
        CombatCancelledEarly,   ///< the stack deleted the chase/follow beneath a non-effect expiry
        Unexpected              ///< no rule matched; the stack and the model disagree for another reason
    };

    typedef std::vector<MovementGeneratorType> StackTypes;   ///< bottom to top

    /// Compares the stack's top against the model's current selection and
    /// names the divergence, applying the rules in order; the first match wins.
    Divergence Classify(StackTypes const& stack, ArbiterModel const& model);
    /// Human-readable name for a Divergence value, for traces and tests.
    const char* DivergenceName(Divergence divergence);
    /// Renders a stack as "[Kind,Kind,...]", bottom to top.
    std::string DescribeStack(StackTypes const& stack);
    /// Renders a model's held entries as "[Kind|Kind#id|...]", default to highest layer.
    std::string DescribeModel(ArbiterModel const& model);
}

#endif
