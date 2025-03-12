#include "botpch.h"
#include "../../playerbot.h"
#include "SellAction.h"
#include "../ItemVisitors.h"

using namespace ai;

class SellItemsVisitor : public IterateItemsVisitor
{
public:
    SellItemsVisitor(SellAction* action) : IterateItemsVisitor()
    {
        this->action = action;
    }

    virtual bool Visit(Item* item)
    {
        action->Sell(item);
        return true;
    }

private:
    SellAction* action;
};

class SellGrayItemsVisitor : public SellItemsVisitor
{
public:
    SellGrayItemsVisitor(SellAction* action) : SellItemsVisitor(action) {}

    virtual bool Visit(Item* item)
    {
        if (item->GetProto()->Quality != ITEM_QUALITY_POOR)
            return true;

        return SellItemsVisitor::Visit(item);
    }
};

class SellVendorItemsVisitor : public SellItemsVisitor
{
public:
    SellVendorItemsVisitor(SellAction* action, set<uint32>& itemIds) : SellItemsVisitor(action), itemIds(itemIds) {}

    virtual bool Visit(Item* item)
    {
        uint32 id = item->GetProto()->ItemId;

        if (itemIds.find(id) == itemIds.end())
            return true;

        return SellItemsVisitor::Visit(item);
    }

private:
    set<uint32>& itemIds;
};


bool SellAction::Execute(Event event)
{
    Player* master = GetMaster();
    if (!master)
        return false;

    string text = event.getParam();
    if (text.empty()) return false;

    if (text == "gray" || text == "*")
    {
        SellGrayItemsVisitor visitor(this);
        IterateItems(&visitor);

        if (text == "*")
        {
            set<uint32>& ids = ai->GetAiObjectContext()->GetValue<set<uint32>& >("vendor list")->Get();
            SellVendorItemsVisitor visitor2(this, ids);
            IterateItems(&visitor2);
        }
        return true;
    }

    list<Item*> items = parseItems(text, ITERATE_ITEMS_IN_BAGS);
    for (list<Item*>::iterator i = items.begin(); i != items.end(); ++i)
    {
        Sell(*i);
    }

    return true;
}


void SellAction::Sell(FindItemVisitor* visitor)
{
    IterateItems(visitor);
    list<Item*> items = visitor->GetResult();
    for (list<Item*>::iterator i = items.begin(); i != items.end(); ++i)
        Sell(*i);
}

void SellAction::Sell(Item* item)
{
    Player* master = GetMaster();
    list<ObjectGuid> vendors = ai->GetAiObjectContext()->GetValue<list<ObjectGuid> >("nearest npcs")->Get();
    bool bought = false;
    for (list<ObjectGuid>::iterator i = vendors.begin(); i != vendors.end(); ++i)
    {
        ObjectGuid vendorguid = *i;
        Creature *pCreature = bot->GetNPCIfCanInteractWith(vendorguid,UNIT_NPC_FLAG_VENDOR);
        if (!pCreature)
            continue;

        ObjectGuid itemguid = item->GetObjectGuid();
        uint32 count = item->GetCount();

        WorldPacket p;
        p << vendorguid << itemguid << count;
        bot->GetSession()->HandleSellItemOpcode(p);

        ostringstream out; out << "Selling " << chat->formatItem(item->GetProto());
        ai->TellMaster(out);
    }
}
