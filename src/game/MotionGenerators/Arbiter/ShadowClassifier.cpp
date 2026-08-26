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

#include "ShadowClassifier.h"

namespace Arbiter
{
    namespace
    {
        bool IsOneShot(MovementGeneratorType type)
        {
            switch (type)
            {
                case POINT_MOTION_TYPE:
                case EFFECT_MOTION_TYPE:
                case HOME_MOTION_TYPE:
                case DISTRACT_MOTION_TYPE:
                case ASSISTANCE_MOTION_TYPE:
                case ASSISTANCE_DISTRACT_MOTION_TYPE:
                    return true;
                default:
                    return false;
            }
        }

        /// The model-side twin of IsOneShot: kinds whose stack generator ends by itself.
        bool IsOneShotKind(MoveKind kind)
        {
            switch (kind)
            {
                case MoveKind::Point:
                case MoveKind::FlyLand:
                case MoveKind::Home:
                case MoveKind::AssistanceRun:
                case MoveKind::Distract:
                case MoveKind::AssistanceDistract:
                case MoveKind::Effect:
                    return true;
                default:
                    return false;
            }
        }

        bool IsTargeted(MovementGeneratorType type)
        {
            return type == CHASE_MOTION_TYPE || type == FOLLOW_MOTION_TYPE;
        }

        bool IsDefaultType(MovementGeneratorType type)
        {
            return type == IDLE_MOTION_TYPE || type == RANDOM_MOTION_TYPE || type == WAYPOINT_MOTION_TYPE;
        }

        /// True when the model holds an entry of the stack type's kind; reports the highest
        /// layer it is held on (an Idle command on Scripted, not the Idle default beneath it).
        bool ModelHolds(ArbiterModel const& model, MovementGeneratorType type, Layer* layerOut)
        {
            for (uint8 i = static_cast<uint8>(Layer::Count); i-- > static_cast<uint8>(Layer::Scripted);)
            {
                const Layer layer = static_cast<Layer>(i);
                std::optional<Held> const& command = model.Command(layer);
                if (command && SameKind(type, command->kind))
                {
                    if (layerOut)
                    {
                        *layerOut = layer;
                    }
                    return true;
                }
            }
            if (model.Combat() && SameKind(type, model.Combat()->kind))
            {
                if (layerOut)
                {
                    *layerOut = Layer::Combat;
                }
                return true;
            }
            if (model.Default() && SameKind(type, model.Default()->kind))
            {
                if (layerOut)
                {
                    *layerOut = Layer::Default;
                }
                return true;
            }
            return false;
        }

        size_t CountIf(StackTypes const& stack, bool (*pred)(MovementGeneratorType))
        {
            size_t n = 0;
            for (MovementGeneratorType t : stack)
            {
                if (pred(t))
                {
                    ++n;
                }
            }
            return n;
        }

        /// Number of one-shot moves the model holds: the only entries a stack one-shot
        /// can legitimately stand for. A standing behaviour masking others (an Idle
        /// command, a fear, a taxi) is not one, and must not pad the count.
        size_t ModelOneShotCount(ArbiterModel const& model)
        {
            size_t n = 0;
            for (Held const& h : model.Contents())
            {
                if (IsOneShotKind(h.kind))
                {
                    ++n;
                }
            }
            return n;
        }
    }

    Divergence Classify(StackTypes const& stack, ArbiterModel const& model)
    {
        const std::optional<Held> selected = model.Selected();
        if (!selected || stack.empty())
        {
            return Divergence::Unexpected;
        }

        const MovementGeneratorType top = stack.back();
        // Rules 3 and 4 compare positions inside the model, so they use the layer the selection is
        // held on (an Idle command sits on Scripted although LayerOf(Idle) is Default).
        const Layer heldLayer = *model.SelectedLayer();

        if (SameKind(top, selected->kind))
        {
            if (CountIf(stack, IsOneShot) > ModelOneShotCount(model))
            {
                return Divergence::SupersededOneShot;
            }
            if (CountIf(stack, IsTargeted) > 1)
            {
                return Divergence::TargetedMultiplicity;
            }
            return Divergence::None;
        }

        Layer heldAt = Layer::Default;
        // A default kind on top means everything above it was removed, not that it was
        // pushed later: only a combat or command entry held beneath the selection counts.
        if (ModelHolds(model, top, &heldAt) && heldAt >= Layer::Combat && heldAt < heldLayer)
        {
            return Divergence::LayerOrder;
        }

        const bool modelWantsTargeted = heldLayer == Layer::Combat || selected->kind == MoveKind::FollowTarget;
        if (modelWantsTargeted && CountIf(stack, IsTargeted) == 0 && IsDefaultType(top))
        {
            return Divergence::CombatCancelledEarly;
        }

        if (IsOneShot(top) && !ModelHolds(model, top, nullptr))
        {
            return Divergence::SupersededOneShot;
        }

        return Divergence::Unexpected;
    }

    const char* DivergenceName(Divergence divergence)
    {
        switch (divergence)
        {
            case Divergence::None:                 return "None";
            case Divergence::LayerOrder:           return "LayerOrder";
            case Divergence::SupersededOneShot:    return "SupersededOneShot";
            case Divergence::TargetedMultiplicity: return "TargetedMultiplicity";
            case Divergence::CombatCancelledEarly: return "CombatCancelledEarly";
            default:                               return "Unexpected";
        }
    }

    std::string DescribeStack(StackTypes const& stack)
    {
        std::string out = "[";
        for (size_t i = 0; i < stack.size(); ++i)
        {
            if (i)
            {
                out += ",";
            }
            out += KindName(KindOf(stack[i]));
        }
        return out + "]";
    }

    std::string DescribeModel(ArbiterModel const& model)
    {
        std::string out = "[";
        bool first = true;
        for (Held const& h : model.Contents())
        {
            if (!first)
            {
                out += "|";
            }
            first = false;
            out += KindName(h.kind);
            if (h.id)
            {
                out += "#" + std::to_string(h.id);
            }
        }
        return out + "]";
    }
}
