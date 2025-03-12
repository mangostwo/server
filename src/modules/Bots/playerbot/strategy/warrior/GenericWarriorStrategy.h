#pragma once

#include "../Strategy.h"
#include "../generic/CombatStrategy.h"

namespace ai
{
    class AiObjectContext;

    class GenericWarriorStrategy : public CombatStrategy
    {
    public:
        GenericWarriorStrategy(PlayerbotAI* ai);

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
        virtual string getName() { return "warrior"; }
    };
}
