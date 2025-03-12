#pragma once

#include "../Action.h"

namespace ai
{
    class RtiAction : public Action
    {
    public:
        RtiAction(PlayerbotAI* ai) : Action(ai, "rti")
        {}

        virtual bool Execute(Event event);

    private:
        void AppendRti(ostringstream & out, string type);

    };

    class MarkRtiAction : public Action
    {
    public:
        MarkRtiAction(PlayerbotAI* ai) : Action(ai, "mark rti")
        {}

        virtual bool Execute(Event event);
    };
}
