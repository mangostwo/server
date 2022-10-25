#include "botpch.h"
#include "../../playerbot.h"
#include "BuyAction.h"
#include "../ItemVisitors.h"
#include "../values/ItemCountValue.h"

using namespace ai;

bool BuyAction::Execute(Event event)
{
    string link = event.getParam();

    ItemIds itemIds = chat->parseItems(link);
    if (itemIds.empty())
    {
        return false;
    }

    Player* master = GetMaster();

    if (!master)
    {
        return false;
    }

    list<ObjectGuid> vendors = ai->GetAiObjectContext()->GetValue<list<ObjectGuid> >("nearest npcs")->Get();
    bool bought = false;
    for (list<ObjectGuid>::iterator i = vendors.begin(); i != vendors.end(); ++i)
    {
        ObjectGuid vendorguid = *i;
        Creature *pCreature = bot->GetNPCIfCanInteractWith(vendorguid,UNIT_NPC_FLAG_VENDOR);
        if (!pCreature)
        {
            continue;
        }

        VendorItemData const* tItems = pCreature->GetVendorItems();
        if (!tItems)
        {
            continue;
        }

        for (ItemIds::iterator i = itemIds.begin(); i != itemIds.end(); i++)
        {
            for (uint32 slot = 0; slot < tItems->GetItemCount(); slot++)
            {
                const ItemPrototype* proto = sObjectMgr.GetItemPrototype(*i);
                if (proto && tItems->GetItem(slot)->item == *i)
                {
                    bot->BuyItemFromVendorSlot(vendorguid, 0, *i, 1, NULL_BAG, NULL_SLOT);
                    ostringstream out; out << "Buying " << ChatHelper::formatItem(proto);
                    ai->TellMaster(out.str());
                    bought = true;
                }
            }
        }
    }

    return bought;
}
