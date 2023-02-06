#include "botpch.h"
#include "../../playerbot.h"
#include "../values/LastMovementValue.h"
#include "MovementActions.h"
#include "MotionMaster.h"
#include "MovementGenerator.h"
#include "../../FleeManager.h"
#include "../../LootObjectStack.h"
#include "../../PlayerbotAIConfig.h"
#include "MotionGenerators/TargetedMovementGenerator.h"

using namespace ai;

bool MovementAction::MoveNear(uint32 mapId, float x, float y, float z, float distance)
{
    float angle = GetFollowAngle();
    return MoveTo(mapId, x + cos(angle) * distance, y + sin(angle) * distance, z);
}

bool MovementAction::MoveNear(WorldObject* target, float distance)
{
    if (!target)
    {
        return false;
    }

    distance += target->GetObjectBoundingRadius();

    float followAngle = GetFollowAngle();
    for (float angle = followAngle; angle <= followAngle + 2 * M_PI; angle += M_PI / 4)
    {
        float x = target->GetPositionX() + cos(angle) * distance,
             y = target->GetPositionY()+ sin(angle) * distance,
             z = target->GetPositionZ();
        if (!bot->IsWithinLOS(x, y, z))
        {
            continue;
        }
        bool moved = MoveTo(target->GetMapId(), x, y, z);
        if (moved)
        {
            return true;
        }
    }
    return false;
}

bool MovementAction::MoveTo(uint32 mapId, float x, float y, float z)
{
    if (!bot->IsUnderWater())
    {
        bot->UpdateGroundPositionZ(x, y, z);
    }

    if (!IsMovingAllowed(mapId, x, y, z))
    {
        return false;
    }

    float distance = bot->GetDistance2d(x, y);
    if (distance > sPlayerbotAIConfig.contactDistance)
    {
        WaitForReach(distance);

        if (bot->IsSitState())
        {
            bot->SetStandState(UNIT_STAND_STATE_STAND);
        }

        if (bot->IsNonMeleeSpellCasted(true))
        {
            bot->CastStop();
            ai->InterruptSpell();
        }

        bool generatePath = !bot->IsFlying() && !bot->IsUnderWater();
        MotionMaster &mm = *bot->GetMotionMaster();
        mm.MovePoint(mapId, x, y, z, generatePath);

        AI_VALUE(LastMovement&, "last movement").Set(x, y, z, bot->GetOrientation());
        return true;
    }

    return false;
}

bool MovementAction::MoveTo(Unit* target, float distance)
{
    if (!IsMovingAllowed(target))
    {
        return false;
    }

    float bx = bot->GetPositionX();
    float by = bot->GetPositionY();
    float bz = bot->GetPositionZ();

    float tx = target->GetPositionX();
    float ty = target->GetPositionY();
    float tz = target->GetPositionZ();

    float distanceToTarget = bot->GetDistance2d(target);
    float angle = bot->GetAngle(target);
    float needToGo = distanceToTarget - distance;

    float maxDistance = sPlayerbotAIConfig.spellDistance;
    if (needToGo > 0 && needToGo > maxDistance)
    {
        needToGo = maxDistance;
    }
    else if (needToGo < 0 && needToGo < -maxDistance)
    {
        needToGo = -maxDistance;
    }

    float dx = cos(angle) * needToGo + bx;
    float dy = sin(angle) * needToGo + by;

    return MoveTo(target->GetMapId(), dx, dy, tz);
}

float MovementAction::GetFollowAngle()
{
    Player* master = GetMaster();
    Group* group = master ? master->GetGroup() : bot->GetGroup();
    if (!group)
    {
        return 0.0f;
    }

    int index = 1;
    for (GroupReference *ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        if( ref->getSource() == master)
        {
            continue;
        }

        if( ref->getSource() == bot)
        {
            return 2 * M_PI / (group->GetMembersCount() -1) * index;
        }

        index++;
    }
    return 0;
}

bool MovementAction::IsMovingAllowed(Unit* target)
{
    if (!target)
    {
        return false;
    }

    if (bot->GetMapId() != target->GetMapId())
    {
        return false;
    }

    float distance = bot->GetDistance(target);
    if (distance > sPlayerbotAIConfig.reactDistance)
    {
        return false;
    }

    return IsMovingAllowed();
}

bool MovementAction::IsMovingAllowed(uint32 mapId, float x, float y, float z)
{
    float distance = bot->GetDistance(x, y, z);
    if (distance > sPlayerbotAIConfig.reactDistance)
    {
        return false;
    }

    return IsMovingAllowed();
}

bool MovementAction::IsMovingAllowed()
{
    if (bot->IsFrozen() || bot->IsPolymorphed() ||
            (bot->IsDead() && !bot->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST)) ||
            bot->IsBeingTeleported() ||
            bot->IsInRoots() ||
            bot->HasAuraType(SPELL_AURA_MOD_CONFUSE) || bot->IsCharmed() ||
            bot->HasAuraType(SPELL_AURA_MOD_STUN) || bot->IsFlying())
        return false;

    MotionMaster &mm = *bot->GetMotionMaster();
    return mm.GetCurrentMovementGeneratorType() != FLIGHT_MOTION_TYPE;
}

bool MovementAction::Follow(Unit* target, float distance)
{
    return Follow(target, distance, GetFollowAngle());
}

bool MovementAction::Follow(Unit* target, float distance, float angle)
{
    MotionMaster &mm = *bot->GetMotionMaster();

    if (!target)
    {
        return false;
    }

    if (bot->GetDistance2d(target->GetPositionX(), target->GetPositionY()) <= sPlayerbotAIConfig.sightDistance &&
            abs(bot->GetPositionZ() - target->GetPositionZ()) >= sPlayerbotAIConfig.spellDistance)
    {
        mm.Clear();
        float x = bot->GetPositionX(), y = bot->GetPositionY(), z = target->GetPositionZ();
        if (target->GetMapId() && bot->GetMapId() != target->GetMapId())
        {
            bot->TeleportTo(target->GetMapId(), x, y, z, bot->GetOrientation());
        }
        else
        {
            bot->Relocate(x, y, z, bot->GetOrientation());
        }
        AI_VALUE(LastMovement&, "last movement").Set(target);
        return true;
    }

    if (!IsMovingAllowed(target))
    {
        return false;
    }

    if (target->IsFriendlyTo(bot) && bot->IsMounted() && AI_VALUE(list<ObjectGuid>, "possible targets").empty())
    {
        distance += angle;
    }

    if (bot->GetDistance2d(target) <= sPlayerbotAIConfig.followDistance)
    {
        return false;
    }

    if (bot->IsSitState())
    {
        bot->SetStandState(UNIT_STAND_STATE_STAND);
    }

    if (bot->IsNonMeleeSpellCasted(true))
    {
        bot->CastStop();
        ai->InterruptSpell();
    }

    AI_VALUE(LastMovement&, "last movement").Set(target);

    if (bot->GetMotionMaster()->GetCurrentMovementGeneratorType() == FOLLOW_MOTION_TYPE)
    {
        Unit *currentTarget = static_cast<ChaseMovementGenerator<Player> const*>(bot->GetMotionMaster()->GetCurrent())->GetTarget();
        if (currentTarget && currentTarget->GetObjectGuid() == target->GetObjectGuid()) return false;
    }

    mm.MoveFollow(target, distance, angle);
    return true;
}

void MovementAction::WaitForReach(float distance)
{
    float delay = 1000.0f * distance / bot->GetSpeed(MOVE_RUN) + sPlayerbotAIConfig.reactDelay;

    if (delay > sPlayerbotAIConfig.maxWaitForMove)
    {
        delay = sPlayerbotAIConfig.maxWaitForMove;
    }

    Unit* target = *ai->GetAiObjectContext()->GetValue<Unit*>("current target");
    Unit* player = *ai->GetAiObjectContext()->GetValue<Unit*>("enemy player target");
    if ((player || target) && delay > sPlayerbotAIConfig.globalCoolDown)
    {
        delay = sPlayerbotAIConfig.globalCoolDown;
    }

    ai->SetNextCheckDelay((uint32)delay);
}

bool MovementAction::Flee(Unit *target)
{
    Player* master = GetMaster();
    if (!target)
    {
        target = master;
    }

    if (!target)
    {
        return false;
    }

    if (!sPlayerbotAIConfig.fleeingEnabled)
    {
        return false;
    }

    if (!IsMovingAllowed())
    {
        return false;
    }

    FleeManager manager(bot, sPlayerbotAIConfig.fleeDistance, bot->GetAngle(target) + M_PI);

    float rx, ry, rz;
    if (!manager.CalculateDestination(&rx, &ry, &rz))
    {
        return false;
    }

    return MoveTo(target->GetMapId(), rx, ry, rz);
}

bool FleeAction::Execute(Event event)
{
    return Flee(AI_VALUE(Unit*, "current target"));
}

bool FleeAction::isUseful()
{
    return AI_VALUE(uint8, "attacker count") > 0 &&
            AI_VALUE2(float, "distance", "current target") <= sPlayerbotAIConfig.shootDistance;
}

bool RunAwayAction::Execute(Event event)
{
    return Flee(AI_VALUE(Unit*, "master target"));
}

bool MoveRandomAction::Execute(Event event)
{
    vector<WorldLocation> locs;
    list<ObjectGuid> npcs = AI_VALUE(list<ObjectGuid>, "nearest npcs");
    for (list<ObjectGuid>::iterator i = npcs.begin(); i != npcs.end(); i++)
    {
        WorldObject* target = ai->GetUnit(*i);
        if (target && bot->GetDistance(target) > sPlayerbotAIConfig.tooCloseDistance)
        {
            WorldLocation loc;
            target->GetPosition(loc);
            locs.push_back(loc);
        }
    }

    list<ObjectGuid> players = AI_VALUE(list<ObjectGuid>, "nearest friendly players");
    for (list<ObjectGuid>::iterator i = players.begin(); i != players.end(); i++)
    {
        WorldObject* target = ai->GetUnit(*i);
        if (target && bot->GetDistance(target) > sPlayerbotAIConfig.tooCloseDistance)
        {
            WorldLocation loc;
            target->GetPosition(loc);
            locs.push_back(loc);
        }
    }

    list<ObjectGuid> gos = AI_VALUE(list<ObjectGuid>, "nearest game objects");
    for (list<ObjectGuid>::iterator i = gos.begin(); i != gos.end(); i++)
    {
        WorldObject* target = ai->GetGameObject(*i);

        if (target && bot->GetDistance(target) > sPlayerbotAIConfig.tooCloseDistance)
        {
            WorldLocation loc;
            target->GetPosition(loc);
            locs.push_back(loc);
        }
    }

    float distance = sPlayerbotAIConfig.grindDistance;
    Map* map = bot->GetMap();
    for (int i = 0; i < 10; ++i)
    {
        float x = bot->GetPositionX();
        float y = bot->GetPositionY();
        float z = bot->GetPositionZ();
        x += urand(0, distance) - distance / 2;
        y += urand(0, distance) - distance / 2;
        bot->UpdateGroundPositionZ(x, y, z);

        const TerrainInfo* terrain = map->GetTerrain();
        if (terrain->IsUnderWater(x, y, z) ||
            terrain->IsInWater(x, y, z))
            continue;

        float ground = map->GetHeight(bot->GetPhaseMask(), x, y, z + 0.5f);
        if (ground <= INVALID_HEIGHT)
        {
            continue;
        }

        z = 0.05f + ground;
        if (abs(z - bot->GetPositionZ()) > sPlayerbotAIConfig.tooCloseDistance)
        {
            continue;
        }

        WorldLocation loc(bot->GetMapId(), x, y, z);
        locs.push_back(loc);
    }

    if (locs.empty())
    {
        return false;
    }

    WorldLocation target = locs[urand(0, locs.size() - 1)];
    return MoveNear(target.mapid, target.coord_x, target.coord_y, target.coord_z);
}

bool MoveToLootAction::Execute(Event event)
{
    LootObject loot = AI_VALUE(LootObject, "loot target");
    if (!loot.IsLootPossible(bot))
    {
        return false;
    }

    WorldObject *wo = loot.GetWorldObject(bot);
    return MoveNear(wo);
}

bool MoveOutOfEnemyContactAction::Execute(Event event)
{
    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target)
    {
        return false;
    }

    return MoveNear(target, sPlayerbotAIConfig.meleeDistance);
}

bool MoveOutOfEnemyContactAction::isUseful()
{
    return AI_VALUE2(float, "distance", "current target") < (sPlayerbotAIConfig.meleeDistance + sPlayerbotAIConfig.contactDistance);
}

bool SetFacingTargetAction::Execute(Event event)
{
    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target)
    {
        return false;
    }

    if (bot->IsTaxiFlying())
    {
        return true;
    }

    bot->SetFacingTo(bot->GetAngle(target));
    ai->SetNextCheckDelay(sPlayerbotAIConfig.globalCoolDown);
    return true;
}

bool SetFacingTargetAction::isUseful()
{
    return !AI_VALUE2(bool, "facing", "current target");
}
