#pragma once

#include "../Strategy.h"
#include "../generic/CombatStrategy.h"

namespace ai
{
    class DpsRogueStrategy : public CombatStrategy
    {
    public:
        DpsRogueStrategy(PlayerbotAI* ai);

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
        virtual string getName() { return "dps"; }
        virtual NextAction** getDefaultActions();
    };

    class RogueAoeStrategy : public Strategy
    {
    public:
        RogueAoeStrategy(PlayerbotAI* ai) : Strategy(ai) {}

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
        virtual string getName() { return "aoe"; }
    };

    class RogueBoostStrategy : public Strategy
    {
    public:
        RogueBoostStrategy(PlayerbotAI* ai) : Strategy(ai) {}

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
        virtual string getName() { return "boost"; }
    };
}
