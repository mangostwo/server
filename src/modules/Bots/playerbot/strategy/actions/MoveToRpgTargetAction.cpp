#include "botpch.h"
#include "../../playerbot.h"
#include "MoveToRpgTargetAction.h"
#include "../../PlayerbotAIConfig.h"
#include "../../ServerFacade.h"
#include "../values/PossibleRpgTargetsValue.h"

using namespace ai;

bool MoveToRpgTargetAction::Execute(Event event)
{
    Unit* target = ai->GetUnit(AI_VALUE(ObjectGuid, "rpg target"));
    if (!target) return false;

    if (sServerFacade.IsTaxiFlying(bot)) return false;

    float distance = AI_VALUE2(float, "distance", "rpg target");
    if (distance > 180.0f)
    {
        context->GetValue<ObjectGuid>("rpg target")->Set(ObjectGuid());
        return false;
    }

    float x = target->GetPositionX();
    float y = target->GetPositionY();
    float z = target->GetPositionZ();
    float mapId = target->GetMapId();

    if (bot->IsWithinLOS(x, y, z))
    {
        ai->SetWalkMode(true);
        return MoveNear(target, sPlayerbotAIConfig.followDistance);
    }

    WaitForReach(distance);

    if (bot->IsSitState())
        bot->SetStandState(UNIT_STAND_STATE_STAND);

    if (bot->IsNonMeleeSpellCasted(true))
    {
        bot->CastStop();
        ai->InterruptSpell();
    }

    ai->SetWalkMode(true);
    bool generatePath = !bot->IsFlying() && !sServerFacade.IsUnderwater(bot);
    MotionMaster &mm = *bot->GetMotionMaster();
    mm.MovePoint(mapId, x, y, z, generatePath);

    AI_VALUE(LastMovement&, "last movement").Set(x, y, z, bot->GetOrientation());
    return true;
}

bool MoveToRpgTargetAction::isUseful()
{
    return context->GetValue<ObjectGuid>("rpg target")->Get() && AI_VALUE2(float, "distance", "rpg target") > sPlayerbotAIConfig.followDistance;
}
