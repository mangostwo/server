#include "botpch.h"
#include "../../playerbot.h"
#include "InvalidTargetValue.h"
#include "AttackersValue.h"
#include "../../PlayerbotAIConfig.h"
#include "../../ServerFacade.h"

using namespace ai;


bool InvalidTargetValue::Calculate()
{
    Unit* target = AI_VALUE(Unit*, qualifier);
    if (qualifier == "current target")
    {
        return !AttackersValue::IsValidTarget(target, bot);
    }

    return !target;
}
