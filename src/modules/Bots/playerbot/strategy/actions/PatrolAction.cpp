#include "botpch.h"
#include "../../playerbot.h"
#include "PatrolAction.h"
#include "../../PlayerbotAIConfig.h"
#include "../../ServerFacade.h"
#include "../values/Formations.h"
#include "../values/PositionValue.h"
#include "../values/PossibleRpgTargetsValue.h"

using namespace ai;

bool PatrolAction::Execute(Event event)
{
    float bx = bot->GetPositionX();
    float by = bot->GetPositionY();
    float bz = bot->GetPositionZ();
    float mapId = bot->GetMapId();

    ai::Position pos = context->GetValue<ai::PositionMap&>("position")->Get()["patrol"];
    if (pos.isSet())
    {
        float distanceToTarget = sServerFacade.GetDistance2d(bot, pos.x, pos.y);
        float distance = sPlayerbotAIConfig.spellDistance;
        if (sServerFacade.IsDistanceGreaterThan(distanceToTarget, distance))
        {
            float angle = bot->GetAngle(pos.x, pos.y);
            float needToGo = distanceToTarget - distance;

            float maxDistance = ai->GetRange("spell");
            if (needToGo > 0 && needToGo > maxDistance)
                needToGo = maxDistance;
            else if (needToGo < 0 && needToGo < -maxDistance)
                needToGo = -maxDistance;

            float dx = cos(angle) * needToGo + bx;
            float dy = sin(angle) * needToGo + by;
            float dz = bz + (pos.z - bz) * needToGo / distanceToTarget;

            return MoveTo(pos.mapId, dx, dy, dz);
        }
    }

    UpdateDestination();
    return true;
}

bool PatrolAction::isUseful()
{
    return !context->GetValue<Unit*>("grind target")->Get() && !context->GetValue<ObjectGuid>("rpg target")->Get();
}

void PatrolAction::UpdateDestination()
{
    float bx = bot->GetPositionX();
    float by = bot->GetPositionY();
    float bz = bot->GetPositionZ();
    float mapId = bot->GetMapId();

    ai::PositionMap& posMap = context->GetValue<ai::PositionMap&>("position")->Get();
    ai::Position pos = posMap["patrol"];

    float angle = urand(1000, M_PI * 2000) / 1000.0f;
    float dist = sPlayerbotAIConfig.patrolDistance;
    float tx = bx + cos(angle) * dist;
    float ty = by + sin(angle) * dist;
    float tz = bz;
    Formation::UpdateAllowedPositionZ(bot, tx, ty, tz);

    pos.Set(tx, ty, tz, mapId);
    posMap[name] = pos;
}