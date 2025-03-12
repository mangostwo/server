#pragma once

namespace ai
{
    class MarkRtiStrategy : public Strategy
    {
    public:
        MarkRtiStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        virtual string getName() { return "mark rti"; }

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
    };

}
