#pragma once

#include "../Strategy.h"
#include "../generic/CombatStrategy.h"

namespace ai
{
    class GenericWarlockStrategy : public CombatStrategy
    {
    public:
        GenericWarlockStrategy(PlayerbotAI* ai);
        virtual string getName() { return "warlock"; }

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
        virtual NextAction** getDefaultActions();
    };

    class WarlockBoostStrategy : public Strategy
    {
    public:
        WarlockBoostStrategy(PlayerbotAI* ai) : Strategy(ai) {};
        virtual string getName() { return "boost"; }

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
    };

    class WarlockCcStrategy : public Strategy
    {
    public:
        WarlockCcStrategy(PlayerbotAI* ai) : Strategy(ai) {};
        virtual string getName() { return "cc"; }

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
    };
}
