#pragma once

#include "../Strategy.h"
#include "../generic/NonCombatStrategy.h"

namespace ai
{
    class PriestNonCombatStrategy : public NonCombatStrategy
    {
    public:
        PriestNonCombatStrategy(PlayerbotAI* ai);

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
        virtual string getName() { return "nc"; }
    };

    class PriestBuffStrategy : public NonCombatStrategy
    {
    public:
        PriestBuffStrategy(PlayerbotAI* ai) : NonCombatStrategy(ai) {}

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
        virtual string getName() { return "buff"; }
    };

    class PriestShadowResistanceStrategy : public NonCombatStrategy
    {
    public:
        PriestShadowResistanceStrategy(PlayerbotAI* ai) : NonCombatStrategy(ai) {}

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
        virtual string getName() { return "rshadow"; }
    };
}
