#pragma once

#include "../Action.h"
#include "../ItemVisitors.h"

namespace ai
{


    class InventoryAction : public Action {
    public:
        InventoryAction(PlayerbotAI* ai, string name) : Action(ai, name) {}

    protected:
        void IterateItems(IterateItemsVisitor* visitor, IterateItemsMask mask = ITERATE_ITEMS_IN_BAGS);
        void TellItems(map<uint32, int> items, map<uint32, bool> soulbound);
        void TellItem(ItemPrototype const * proto, int count, bool soulbound);
        list<Item*> parseItems(string text);
        uint32 GetItemCount(FindItemVisitor* visitor, IterateItemsMask mask = ITERATE_ITEMS_IN_BAGS);

    private:
        void IterateItemsInBags(IterateItemsVisitor* visitor);
        void IterateItemsInEquip(IterateItemsVisitor* visitor);
    };
}
