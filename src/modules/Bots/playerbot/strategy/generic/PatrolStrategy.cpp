#include "botpch.h"
#include "../../playerbot.h"
#include "PatrolStrategy.h"

using namespace ai;


NextAction** PatrolStrategy::getDefaultActions()
{
    return NextAction::array(0, new NextAction("patrol", 1.0f), NULL);
}

void PatrolStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
}

