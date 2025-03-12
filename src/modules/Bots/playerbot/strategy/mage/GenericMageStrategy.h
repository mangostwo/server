#pragma once

#include "../Strategy.h"
#include "../generic/CombatStrategy.h"

namespace ai
{
    class GenericMageStrategy : public CombatStrategy
    {
    public:
        GenericMageStrategy(PlayerbotAI* ai);
        virtual string getName() { return "mage"; }

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
    };

    class MageCureStrategy : public Strategy
    {
    public:
        MageCureStrategy(PlayerbotAI* ai) : Strategy(ai) {}

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
        virtual string getName() { return "cure"; }
    };

    class MageBoostStrategy : public Strategy
    {
    public:
        MageBoostStrategy(PlayerbotAI* ai) : Strategy(ai) {}

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
        virtual string getName() { return "boost"; }
    };

    class MageCcStrategy : public Strategy
    {
    public:
        MageCcStrategy(PlayerbotAI* ai) : Strategy(ai) {}

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
        virtual string getName() { return "cc"; }
    };
}
