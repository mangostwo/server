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

/**
 * @file SpellAreaTargets.cpp
 * @brief Cohesion split of Spell.cpp -- area/raid target selection.
 *        Same `Spell` class; no behaviour change.
 */

#include "Geometry/Placement.h"
#include "Utilities/MathDefines.h"
#include <vector>
#include <queue>
#include "Spell.h"
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Opcodes.h"
#include "Log.h"
#include "UpdateMask.h"
#include "World.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "Pet.h"
#include "Unit.h"
#include "DynamicObject.h"
#include "Group.h"
#include "UpdateData.h"
#include "MapManager.h"
#include "Transports.h"
#include "TransportMap.h"

#include <cmath>
#include "CellImpl.h"
#include "Policies/Singleton.h"
#include "SharedDefines.h"
#include "LootMgr.h"
#include "BattleGround/BattleGround.h"
#include "Util.h"
#include "Chat.h"
#include "Vehicle.h"
#include "TemporarySummon.h"
#include "SQLStorages.h"
#include "DisableMgr.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /*ENABLE_ELUNA*/

class PrioritizeManaUnitWraper
{
    public:
        explicit PrioritizeManaUnitWraper(Unit* unit) : i_unit(unit)
        {
            uint32 maxmana = unit->GetMaxPower(POWER_MANA);
            i_percent = maxmana ? unit->GetPower(POWER_MANA) * 100 / maxmana : 101;
        }
        Unit* getUnit() const { return i_unit; }
        uint32 getPercent() const { return i_percent; }
    private:
        Unit* i_unit;
        uint32 i_percent;
};

struct PrioritizeMana
{
    int operator()(PrioritizeManaUnitWraper const& x, PrioritizeManaUnitWraper const& y) const
    {
        return x.getPercent() > y.getPercent();
    }
};

typedef std::priority_queue<PrioritizeManaUnitWraper, std::vector<PrioritizeManaUnitWraper>, PrioritizeMana> PrioritizeManaUnitQueue;

class PrioritizeHealthUnitWraper
{
    public:
        explicit PrioritizeHealthUnitWraper(Unit* unit) : i_unit(unit)
        {
            i_percent = unit->GetHealth() * 100 / unit->GetMaxHealth();
        }
        Unit* getUnit() const { return i_unit; }
        uint32 getPercent() const { return i_percent; }
    private:
        Unit* i_unit;
        uint32 i_percent;
};

struct PrioritizeHealth
{
    int operator()(PrioritizeHealthUnitWraper const& x, PrioritizeHealthUnitWraper const& y) const
    {
        return x.getPercent() > y.getPercent();
    }
};

typedef std::priority_queue<PrioritizeHealthUnitWraper, std::vector<PrioritizeHealthUnitWraper>, PrioritizeHealth> PrioritizeHealthUnitQueue;

/**
 * Fill target list by units around (x,y) points at radius distance

 * @param targetUnitMap        Reference to target list that filled by function
 * @param x                    X coordinates of center point for target search
 * @param y                    Y coordinates of center point for target search
 * @param radius               Radius around (x,y) for target search
 * @param pushType             Additional rules for target area selection (in front, angle, etc)
 * @param spellTargets         Additional rules for target selection base at hostile/friendly state to original spell caster
 * @param originalCaster       If provided set alternative original caster, if =NULL then used Spell::GetAffectiveObject() return
 */
namespace
{
    // The half-width of the cone each forward/back push selects, matching the arcs the
    // world path passes to IsInFront/IsInBack.
    float ConeHalfArc(SpellNotifyPushType push)
    {
        switch (push)
        {
            case PUSH_IN_FRONT:    return (2 * M_PI_F / 3) * 0.5f;
            case PUSH_IN_FRONT_90: return (M_PI_F / 2) * 0.5f;
            case PUSH_IN_FRONT_30: return (M_PI_F / 6) * 0.5f;
            case PUSH_IN_FRONT_15: return (M_PI_F / 12) * 0.5f;
            case PUSH_IN_BACK:     return (2 * M_PI_F / 3) * 0.5f;
            default:               return 0.0f;
        }
    }

    bool IsConePush(SpellNotifyPushType push)
    {
        return push == PUSH_IN_FRONT || push == PUSH_IN_FRONT_90 || push == PUSH_IN_FRONT_30 ||
               push == PUSH_IN_FRONT_15 || push == PUSH_IN_BACK;
    }
}

void Spell::FillAreaTargets(UnitList& targetUnitMap, float radius, SpellNotifyPushType pushType, SpellTargets spellTargets, WorldObject* originalCaster /*=NULL*/)
{
    MaNGOS::SpellNotifierCreatureAndPlayer notifier(*this, targetUnitMap, radius, pushType, spellTargets, originalCaster);

    // Where is the area's centre, and is it a point on a vessel? A caster-centred push
    // sits on the caster; a dest-centred one on the ground-target the client sent; a
    // target-centred one on the unit. If that anchor is aboard a transport, the whole
    // search runs in the vessel's local space -- exact, and never asking where the hull is.
    TransportMap* vessel = NULL;
    float lx = 0.0f, ly = 0.0f, lz = 0.0f, lo = 0.0f;

    switch (pushType)
    {
        case PUSH_SELF_CENTER:
        case PUSH_IN_FRONT:
        case PUSH_IN_FRONT_90:
        case PUSH_IN_FRONT_30:
        case PUSH_IN_FRONT_15:
        case PUSH_IN_BACK:
            if (WorldObject* castingObject = GetCastingObject())
            {
                if ((vessel = castingObject->GetMap()->AsTransport()))
                {
                    if (const auto p = vessel->PositionOf(*castingObject))
                    {
                        lx = p->X(); ly = p->Y(); lz = p->Z(); lo = p->Facing();
                    }
                    else
                    {
                        vessel = NULL;
                    }
                }
            }
            break;

        case PUSH_DEST_CENTER:
        {
            const bool src = (m_targets.m_targetMask & TARGET_FLAG_SOURCE_LOCATION) != 0;
            const ObjectGuid guid = src ? m_targets.getSrcTransportGuid()
                                        : m_targets.getDestTransportGuid();
            Transport* named = guid ? Transport::GetTransport(m_caster->GetMap(), guid) : NULL;
            if (named && (vessel = named->AsMap()))
            {
                if (src)
                {
                    m_targets.getSrcTransportOffset(lx, ly, lz);
                }
                else
                {
                    m_targets.getDestTransportOffset(lx, ly, lz);
                }
            }
            break;
        }

        case PUSH_TARGET_CENTER:
            if (Unit* target = m_targets.getUnitTarget())
            {
                if ((vessel = target->GetMap()->AsTransport()))
                {
                    if (const auto p = vessel->PositionOf(*target))
                    {
                        lx = p->X(); ly = p->Y(); lz = p->Z(); lo = p->Facing();
                    }
                    else
                    {
                        vessel = NULL;
                    }
                }
            }
            break;

        default:
            break;
    }

    // The ordinary sweep runs either way: on a deck it walks the deck map's own cells, in
    // the deck's own coordinates, and nothing in it knows a ship is involved.
    Cell::VisitAllObjects(notifier.GetCenterX(), notifier.GetCenterY(), m_caster->GetMap(), notifier, radius);

    if (!vessel)
    {
        return;
    }

    // The deck search. Everyone aboard is a boarded creature or minion (the crew grid) or a
    // boarded player; each one's LOCAL separation from the centre is exact, because a rigid
    // transform preserves distances and angles -- so this needs, and touches, no world
    // coordinate at all.
    const float r2 = radius * radius;
    const bool cone = IsConePush(pushType);
    const float half = ConeHalfArc(pushType);
    const float ref = (pushType == PUSH_IN_BACK) ? Geometry::Placement::NormalizeOrientation(lo + M_PI_F) : lo;

    const auto consider = [&](Unit* unit)
    {
        if (!notifier.PassesTargetFilter(unit))
        {
            return;
        }

        const auto tl = vessel->PositionOf(*unit);
        if (!tl)
        {
            return;                                     // not aboard this vessel
        }

        const float dx = tl->X() - lx;
        const float dy = tl->Y() - ly;
        const float dz = tl->Z() - lz;

        if (dx * dx + dy * dy + dz * dz > r2)
        {
            return;
        }

        if (cone)
        {
            float diff = std::atan2(dy, dx) - ref;
            diff = Geometry::Placement::NormalizeOrientation(diff);
            if (diff > M_PI_F)
            {
                diff -= 2 * M_PI_F;
            }
            if (std::fabs(diff) > half)
            {
                return;
            }
        }

        targetUnitMap.push_back(unit);
    };

}

/**
 * @brief Fills a target list with party or raid members around a reference unit.
 *
 * @param targetUnitMap The target list being populated.
 * @param member The reference member.
 * @param radius The search radius.
 * @param raid True to include the whole raid; false to limit to the subgroup.
 * @param withPets True to include pets.
 * @param withcaster True to include the caster when applicable.
 */
void Spell::FillRaidOrPartyTargets(UnitList& targetUnitMap, Unit* member, Unit* center, float radius, bool raid, bool withPets, bool withcaster)
{
    Player* pMember = member->GetCharmerOrOwnerPlayerOrPlayerItself();
    Group* pGroup = pMember ? pMember->GetGroup() : NULL;

    if (pGroup)
    {
        uint8 subgroup = pMember->GetSubGroup();

        for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* Target = itr->getSource();

            // IsHostileTo check duel and controlled by enemy
            if (Target && (raid || subgroup == Target->GetSubGroup())
                && !m_caster->IsHostileTo(Target))
            {
                if ((Target == center || InReach(*center, *Target, radius)) &&
                        (withcaster || Target != m_caster))
                {
                    targetUnitMap.push_back(Target);
                }

                if (withPets)
                {
                    if (Pet* pet = Target->GetPet())
                    {
                        if ((pet == center || InReach(*center, *pet, radius)) &&
                                (withcaster || pet != m_caster))
                        {
                            targetUnitMap.push_back(pet);
                        }
                    }
                }
            }
        }
    }
    else
    {
        Unit* ownerOrSelf = pMember ? pMember : member->GetCharmerOrOwnerOrSelf();
        if ((ownerOrSelf == center || InReach(*center, *ownerOrSelf, radius)) &&
                (withcaster || ownerOrSelf != m_caster))
        {
            targetUnitMap.push_back(ownerOrSelf);
        }

        if (withPets)
        {
            if (Pet* pet = ownerOrSelf->GetPet())
            {
                if ((pet == center || InReach(*center, *pet, radius)) &&
                        (withcaster || pet != m_caster))
                {
                    targetUnitMap.push_back(pet);
                }
            }
        }
    }
}

void Spell::FillRaidOrPartyManaPriorityTargets(UnitList& targetUnitMap, Unit* member, Unit* center, float radius, uint32 count, bool raid, bool withPets, bool withCaster)
{
    FillRaidOrPartyTargets(targetUnitMap, member, center, radius, raid, withPets, withCaster);

    PrioritizeManaUnitQueue manaUsers;
    for (UnitList::const_iterator itr = targetUnitMap.begin(); itr != targetUnitMap.end(); ++itr)
    {
        if ((*itr)->GetPowerType() == POWER_MANA && !(*itr)->IsDead())
        {
            manaUsers.push(PrioritizeManaUnitWraper(*itr));
        }
    }
    targetUnitMap.clear();
    while (!manaUsers.empty() && targetUnitMap.size() < count)
    {
        targetUnitMap.push_back(manaUsers.top().getUnit());
        manaUsers.pop();
    }
}

void Spell::FillRaidOrPartyHealthPriorityTargets(UnitList& targetUnitMap, Unit* member, Unit* center, float radius, uint32 count, bool raid, bool withPets, bool withCaster)
{
    FillRaidOrPartyTargets(targetUnitMap, member, center, radius, raid, withPets, withCaster);

    PrioritizeHealthUnitQueue healthQueue;
    for (UnitList::const_iterator itr = targetUnitMap.begin(); itr != targetUnitMap.end(); ++itr)
    {
        if (!(*itr)->IsDead())
        {
            healthQueue.push(PrioritizeHealthUnitWraper(*itr));
        }
    }

    targetUnitMap.clear();
    while (!healthQueue.empty() && targetUnitMap.size() < count)
    {
        targetUnitMap.push_back(healthQueue.top().getUnit());
        healthQueue.pop();
    }
}
