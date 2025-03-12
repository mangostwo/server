#pragma once

#include "../Action.h"
#include "../../LootObjectStack.h"
#include "EquipAction.h"
#include "InventoryAction.h"

namespace ai
{
    class CheckInventoryAction : public EquipAction {
    public:
        CheckInventoryAction(PlayerbotAI* ai) : EquipAction(ai, "CheckInventory") {}
        virtual bool Execute(Event event);

    private:
        void HandleDisenchant(Item* item, bool randomBot);
        void HandleEquip(Item* item, bool randomBot);
        void HandleTrade(Item* item, bool randomBot);
        void HandleVendor(Item* item, bool randomBot);
        void HandleUse(Item* item, bool randomBot);
        void HandleNone(Item* item, bool randomBot);
        void HandleGray(Item* item, bool randomBot);
    };

}
