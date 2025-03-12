#include "botpch.h"
#include "../../playerbot.h"
#include "DpsAssistStrategy.h"

using namespace ai;

void DpsAssistStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "not dps target active",
        NextAction::array(0, new NextAction("dps assist", 50.0f), NULL)));
}

void DpsAoeStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "not dps aoe target active",
        NextAction::array(0, new NextAction("dps aoe", 50.0f), NULL)));
}



