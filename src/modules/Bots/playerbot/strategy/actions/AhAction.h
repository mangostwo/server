#pragma once

#include "../Action.h"
#include "InventoryAction.h"

namespace ai
{
    class AhAction : public InventoryAction
    {
    public:
        AhAction(PlayerbotAI* ai) : InventoryAction(ai, "ah") {}
        virtual bool Execute(Event event);

    private:
        bool Execute(string text, Unit* auctioneer);
    };

}
