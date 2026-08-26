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

#include "ArbiterModel.h"

namespace Arbiter
{
    namespace
    {
        const uint8 FIRST_COMMAND_LAYER = static_cast<uint8>(Layer::Scripted);
        const uint8 LAYER_COUNT = static_cast<uint8>(Layer::Count);
    }

    void ArbiterModel::InstallDefault(MoveKind kind)
    {
        const std::optional<Held> before = Selected();
        if (m_default)
        {
            m_events.push_back({MovementEvent::Kind::DefaultSwapped, m_default->kind, m_default->id, FinishReason::Superseded});
        }
        m_default = Held{kind, 0, ++m_seq};
        m_fallbackDefault.reset();
        Reselect(before);
    }

    void ArbiterModel::Request(MoveRequest const& request)
    {
        const std::optional<Held> before = Selected();
        const Layer layer = LayerOf(request.kind);
        const Policy policy = PolicyOf(request.kind, request.resumeCombat);
        const Held held{request.kind, request.id, ++m_seq};

        // MotionMaster::Mutate expires a HOME, DISTRACT or EFFECT top before pushing
        // anything else (MotionMaster.cpp:629-647). Same-layer requests supersede instead.
        if (before && SelfExpiring(before->kind) && LayerOf(before->kind) != layer)
        {
            Finish(m_commands[static_cast<uint8>(LayerOf(before->kind))], FinishReason::Cancelled);
        }

        if (layer == Layer::Default)
        {
            RequestDefault(request, held, policy);
        }
        else if (layer == Layer::Combat)
        {
            if (m_combat)
            {
                m_combat->id = held.id;   // D6: a chase on a chasing unit updates it
                m_combat->seq = held.seq;
            }
            else
            {
                m_combat = held;
            }
        }
        else
        {
            RequestCommand(request, held, layer, policy);
        }

        Reselect(before);
    }

    void ArbiterModel::RequestDefault(MoveRequest const& request, Held const& held, Policy policy)
    {
        if (request.kind == MoveKind::Idle && !Empty())
        {
            // MoveIdle on a non-empty stack pushes the idle singleton on top (MotionMaster.cpp:321-328)
            // and is a no-op when the top already is that singleton. Here it is a Scripted-layer
            // command: it masks combat and the default, and it supersedes a scripted one-shot
            // beneath it on purpose -- a stale point never resumes (design §1); the shadow
            // classifier reports that difference as SupersededOneShot.
            std::optional<Held>& scripted = m_commands[FIRST_COMMAND_LAYER];
            if (scripted && scripted->kind == MoveKind::Idle)
            {
                return;
            }
            Finish(scripted, FinishReason::Superseded);
            scripted = held;
            return;
        }

        if (request.kind == MoveKind::FollowTarget)
        {
            if (m_default && m_default->kind != MoveKind::FollowTarget)
            {
                m_fallbackDefault = m_default;
            }
        }
        else
        {
            m_fallbackDefault.reset();
        }

        if (m_default)
        {
            m_events.push_back({MovementEvent::Kind::DefaultSwapped, m_default->kind, m_default->id, FinishReason::Superseded});
        }
        m_default = held;

        if (policy == Policy::Override)
        {
            Finish(m_combat, FinishReason::Overridden);
            Finish(m_commands[static_cast<uint8>(Layer::Scripted)], FinishReason::Overridden);
            Finish(m_commands[static_cast<uint8>(Layer::Distract)], FinishReason::Overridden);
        }
    }

    void ArbiterModel::RequestCommand(MoveRequest const& request, Held const& held, Layer layer, Policy policy)
    {
        const uint8 index = static_cast<uint8>(layer);
        Finish(m_commands[index], FinishReason::Superseded);

        if (policy == Policy::Override)
        {
            for (uint8 lower = FIRST_COMMAND_LAYER; lower < index; ++lower)
            {
                // D8: a taxi masks a control instead of cancelling it.
                if (request.kind == MoveKind::Taxi && lower == static_cast<uint8>(Layer::Control))
                {
                    continue;
                }
                Finish(m_commands[lower], FinishReason::Overridden);
            }
            Finish(m_combat, FinishReason::Overridden);
        }

        m_commands[index] = held;
    }

    void ArbiterModel::Clear(bool all)
    {
        const std::optional<Held> before = Selected();
        for (uint8 i = FIRST_COMMAND_LAYER; i < LAYER_COUNT; ++i)
        {
            Finish(m_commands[i], FinishReason::Cleared);
        }
        Finish(m_combat, FinishReason::Cleared);
        if (all)
        {
            Finish(m_default, FinishReason::Cleared);
            m_fallbackDefault.reset();
        }
        Reselect(before);
    }

    void ArbiterModel::ExpireSelected()
    {
        const std::optional<Layer> layer = SelectedLayer();
        if (!layer)
        {
            return;
        }
        switch (*layer)
        {
            case Layer::Default:
                // MovementExpired at depth one is a no-op, except a Follow whose target is gone:
                // its Update() returns false and the bottom default beneath it resumes.
                if (m_default->kind == MoveKind::FollowTarget)
                {
                    FinishSelected(FinishReason::TargetLost);
                }
                return;
            case Layer::Combat:
                FinishSelected(FinishReason::TargetLost);
                return;
            default:
                FinishSelected(FinishReason::Expired);
                return;
        }
    }

    void ArbiterModel::Expire(MoveKind kind)
    {
        // The stack expires the generator on top, which is not always what the model
        // selects: a point pushed over a fear expires the point, never the fear.
        const std::optional<Held> before = Selected();

        for (uint8 i = LAYER_COUNT; i-- > FIRST_COMMAND_LAYER;)
        {
            if (m_commands[i] && m_commands[i]->kind == kind)
            {
                Finish(m_commands[i], FinishReason::Expired);
                Reselect(before);
                return;
            }
        }

        if (m_combat && m_combat->kind == kind)
        {
            Finish(m_combat, FinishReason::TargetLost);
            Reselect(before);
            return;
        }

        // A default only ends on its own when it is a Follow whose target is gone; any
        // other default, and a kind the model no longer holds, is a no-op.
        if (m_default && m_default->kind == kind && kind == MoveKind::FollowTarget)
        {
            Finish(m_default, FinishReason::TargetLost);
            m_default = m_fallbackDefault;
            m_fallbackDefault.reset();
            Reselect(before);
        }
    }

    void ArbiterModel::FinishSelected(FinishReason reason)
    {
        const std::optional<Held> before = Selected();
        const std::optional<Layer> layer = SelectedLayer();
        if (!layer)
        {
            return;
        }
        if (*layer == Layer::Default)
        {
            Finish(m_default, reason);
            m_default = m_fallbackDefault;
            m_fallbackDefault.reset();
        }
        else if (*layer == Layer::Combat)
        {
            Finish(m_combat, reason);
        }
        else
        {
            Finish(m_commands[static_cast<uint8>(*layer)], reason);
        }
        Reselect(before);
    }

    void ArbiterModel::CancelControl(MoveKind kind)
    {
        std::optional<Held>& control = m_commands[static_cast<uint8>(Layer::Control)];
        if (!control || control->kind != kind)
        {
            return;
        }
        const std::optional<Held> before = Selected();
        Finish(control, FinishReason::Cancelled);
        Reselect(before);
    }

    bool ArbiterModel::Empty() const
    {
        return !Selected();
    }

    std::optional<Layer> ArbiterModel::SelectedLayer() const
    {
        for (uint8 i = LAYER_COUNT; i-- > FIRST_COMMAND_LAYER;)
        {
            if (m_commands[i])
            {
                return static_cast<Layer>(i);
            }
        }
        if (m_combat)
        {
            return Layer::Combat;
        }
        if (m_default)
        {
            return Layer::Default;
        }
        return std::nullopt;
    }

    std::optional<Held> ArbiterModel::Selected() const
    {
        const std::optional<Layer> layer = SelectedLayer();
        if (!layer)
        {
            return std::nullopt;
        }
        if (*layer == Layer::Default)
        {
            return m_default;
        }
        if (*layer == Layer::Combat)
        {
            return m_combat;
        }
        return m_commands[static_cast<uint8>(*layer)];
    }

    std::vector<Held> ArbiterModel::Contents() const
    {
        std::vector<Held> out;
        if (m_default)
        {
            out.push_back(*m_default);
        }
        if (m_combat)
        {
            out.push_back(*m_combat);
        }
        for (uint8 i = FIRST_COMMAND_LAYER; i < LAYER_COUNT; ++i)
        {
            if (m_commands[i])
            {
                out.push_back(*m_commands[i]);
            }
        }
        return out;
    }

    std::vector<MovementEvent> ArbiterModel::DrainEvents()
    {
        std::vector<MovementEvent> out;
        out.swap(m_events);
        return out;
    }

    void ArbiterModel::Finish(std::optional<Held>& slot, FinishReason reason)
    {
        if (!slot)
        {
            return;
        }
        m_events.push_back({MovementEvent::Kind::Finished, slot->kind, slot->id, reason});
        slot.reset();
    }

    void ArbiterModel::Reselect(std::optional<Held> const& before)
    {
        const std::optional<Held> after = Selected();
        if (!before || !after || before->seq == after->seq)
        {
            return;
        }
        // The previous selection is still held somewhere: it was masked, not finished.
        for (Held const& h : Contents())
        {
            if (h.seq == before->seq)
            {
                m_events.push_back({MovementEvent::Kind::Suspended, before->kind, before->id, FinishReason::Cut});
                break;
            }
        }
        // The new selection existed before this operation: it resumes rather than starts.
        if (after->seq < m_seq || after->seq < before->seq)
        {
            if (after->seq != m_seq)
            {
                m_events.push_back({MovementEvent::Kind::Resumed, after->kind, after->id, FinishReason::Arrived});
            }
        }
    }
}
