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

#include "ArbiterShadow.h"

#include "Unit.h"
#include "Log/Log.h"

void ArbiterShadow::OnFactoryDefault(MovementGeneratorType type)
{
    m_model.InstallDefault(Arbiter::KindOf(type));
}

void ArbiterShadow::OnRequest(Arbiter::MoveKind kind, uint32 id)
{
    Arbiter::MoveRequest request;
    request.kind = kind;
    request.id = id;
    request.resumeCombat = false;   // no facade carries the flag yet (PR3)
    m_model.Request(request);
}

void ArbiterShadow::OnClear(bool all)
{
    m_model.Clear(all);
}

void ArbiterShadow::OnExpired(MovementGeneratorType type)
{
    Arbiter::MoveKind kind = Arbiter::KindOf(type);

    // FlyOrLand reports POINT: tell the two apart by what the scripted layer holds.
    if (type == POINT_MOTION_TYPE)
    {
        std::optional<Arbiter::Held> const& scripted = m_model.Command(Arbiter::Layer::Scripted);
        if (scripted && scripted->kind == Arbiter::MoveKind::FlyLand)
        {
            kind = Arbiter::MoveKind::FlyLand;
        }
    }

    m_model.Expire(kind);
}

void ArbiterShadow::Compare(Arbiter::StackTypes const& stack)
{
    m_model.DrainEvents();   // PR1 consumes no events; keep the queue bounded

    const Arbiter::Divergence divergence = Arbiter::Classify(stack, m_model);
    if (divergence == Arbiter::Divergence::None)
    {
        m_lastReport.clear();
        return;
    }

    std::string report = std::string(Arbiter::DivergenceName(divergence)) + " stack=" +
                         Arbiter::DescribeStack(stack) + " model=" + Arbiter::DescribeModel(m_model);
    if (report == m_lastReport)
    {
        return;
    }

    m_lastReport = report;
    sLog.outBasic("ArbiterShadow: %s %s", m_owner.GetGuidStr().c_str(), report.c_str());
}
