#include "botpch.h"
#include "../../playerbot.h"
#include "CombatStrategy.h"

using namespace ai;

void CombatStrategy::InitTriggers(list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "enemy out of spell",
        NextAction::array(0, new NextAction("reach spell", ACTION_NORMAL + 9), NULL)));

    triggers.push_back(new TriggerNode(
        "invalid target",
        NextAction::array(0, new NextAction("drop target", 55), NULL)));
}
