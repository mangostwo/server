#pragma once

#include "../Action.h"

namespace ai
{
    class RemoveAuraAction : public Action
    {
    public:
        RemoveAuraAction(PlayerbotAI* ai);
        virtual bool Execute(Event event);
    };
}
