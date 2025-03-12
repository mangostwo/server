#include "botpch.h"
#include "../../playerbot.h"
#include "OpenItemAction.h"

#include "../../PlayerbotAIConfig.h"
#include "../../ServerFacade.h"
using namespace ai;

bool OpenItemAction::Execute(Event event)
{
    string name = event.getParam();
    if (name.empty())
        name = getName();

    list<Item*> items = AI_VALUE2(list<Item*>, "inventory items", name);

    if (items.empty())
    {
        ai->TellError("No items available");
        return false;
    }

    Item* item = *items.begin();
    uint8 bagIndex = item->GetBagSlot();
    uint8 slot = item->GetSlot();

    WorldPacket p;
    p << bagIndex << slot;
    bot->GetSession()->HandleOpenItemOpcode(p);
    ostringstream out;
    out << "Opening " << chat->formatItem(item->GetProto());
    ai->TellMaster(out.str());

    if (!item->loot.empty())
    {
        uint32 itemindex = 0;
        for (LootItemList::iterator i = item->loot.items.begin(); i != item->loot.items.end(); ++i)
        {
            ItemPrototype const *proto = sItemStorage.LookupEntry<ItemPrototype>(i->itemid);
            if (!proto)
                continue;

            WorldPacket packet(CMSG_AUTOSTORE_LOOT_ITEM, 1);
            packet << itemindex++;
            bot->GetSession()->HandleAutostoreLootItemOpcode(packet);

            ostringstream out;
            out << "Looting " << chat->formatItem(proto);
            ai->TellMaster(out.str());
        }
    }


    ai->SetNextCheckDelay(sPlayerbotAIConfig.globalCoolDown);
    ai->TellMasterNoFacing(out.str());
    return true;
}
