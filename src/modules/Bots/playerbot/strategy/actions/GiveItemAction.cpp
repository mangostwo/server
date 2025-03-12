#include "botpch.h"
#include "../../playerbot.h"
#include "GiveItemAction.h"

#include "../values/ItemCountValue.h"

using namespace ai;

vector<string> split(const string &s, char delim);

bool GiveItemAction::Execute(Event event)
{
    Unit* target = GetTarget();
    if (!target) return false;

    Player* receiver = dynamic_cast<Player*>(target);
    if (!receiver) return false;

    PlayerbotAI *receiverAi = receiver->GetPlayerbotAI();
    if (!receiverAi)
        return false;

    if (receiverAi->GetAiObjectContext()->GetValue<uint8>("item count", item)->Get())
        return true;

    bool moved = false;
    list<Item*> items = InventoryAction::parseItems(item, ITERATE_ITEMS_IN_BAGS);
    for (list<Item*>::iterator j = items.begin(); j != items.end(); j++)
    {
        Item* item = *j;

        ItemPosCountVec dest;
        InventoryResult msg = receiver->CanStoreItem(NULL_BAG, NULL_SLOT, dest, item, false);
        if (msg == EQUIP_ERR_OK)
        {
            bot->MoveItemFromInventory(item->GetBagSlot(), item->GetSlot(), true);
            item->SetOwnerGuid(target->GetObjectGuid());
            receiver->MoveItemToInventory(dest, item, true);
            moved = true;

            ostringstream out;
            out << "Got " << chat->formatItem(item->GetProto(), item->GetCount()) << " from " << bot->GetName();
            receiverAi->TellMasterNoFacing(out.str());
        }
        else
        {
            ostringstream out;
            out << "Cannot get " << chat->formatItem(item->GetProto(), item->GetCount()) << " from " << bot->GetName() << "- my bags are full";
            receiverAi->TellError(out.str());
        }
    }

    return true;
}

Unit* GiveItemAction::GetTarget()
{
    return AI_VALUE2(Unit*, "party member without item", item);
}

Unit* GiveFoodAction::GetTarget()
{
    return AI_VALUE(Unit*, "party member without food");
}

Unit* GiveWaterAction::GetTarget()
{
    return AI_VALUE(Unit*, "party member without water");
}
