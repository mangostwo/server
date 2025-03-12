#pragma once

#include "../Action.h"
#include "../../LootObjectStack.h"

namespace ai
{
    class FlagAction : public Action {
    public:
        FlagAction(PlayerbotAI* ai) : Action(ai, "flag") {}
        virtual bool Execute(Event event);

    private:
        bool TellUsage();
    };

}
