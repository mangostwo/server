#pragma once

#include "../Strategy.h"
#include "PaladinAiObjectContext.h"
#include "../generic/CombatStrategy.h"

namespace ai
{
    class GenericPaladinStrategy : public CombatStrategy
    {
    public:
        GenericPaladinStrategy(PlayerbotAI* ai);

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
        virtual string getName() { return "paladin"; }
    };

    class PaladinCureStrategy : public Strategy
    {
    public:
        PaladinCureStrategy(PlayerbotAI* ai) : Strategy(ai) {}

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
        virtual string getName() { return "cure"; }
    };

    class PaladinBoostStrategy : public Strategy
    {
    public:
        PaladinBoostStrategy(PlayerbotAI* ai) : Strategy(ai) {}

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
        virtual string getName() { return "boost"; }
    };

    class PaladinCcStrategy : public Strategy
    {
    public:
        PaladinCcStrategy(PlayerbotAI* ai) : Strategy(ai) {}

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
        virtual string getName() { return "cc"; }
    };
}
