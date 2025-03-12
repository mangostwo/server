#include "../generic/NonCombatStrategy.h"
#pragma once

namespace ai
{
    class PatrolStrategy : public NonCombatStrategy
    {
    public:
        PatrolStrategy(PlayerbotAI* ai) : NonCombatStrategy(ai) {}
        virtual string getName() { return "patrol"; }
        NextAction** getDefaultActions();

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
    };



}
