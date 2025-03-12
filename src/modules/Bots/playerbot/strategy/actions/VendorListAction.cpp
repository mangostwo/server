#include "botpch.h"
#include "../../playerbot.h"
#include "VendorListAction.h"
#include "../values/LootStrategyValue.h"
#include "LootAction.h"

using namespace ai;


bool VendorListAction::Execute(Event event)
{
    string text = event.getParam();
    if (text == "?")
    {
        TellVendorList("vendor list");
    }
    else
    {
        ItemIds items = chat->parseItems(text);

        bool remove = text.size() > 1 && text.substr(0, 1) == "-";
        bool query = text.size() > 1 && text.substr(0, 1) == "?";
        bool add = !remove && !query;
        bool changes = false;
        set<uint32>& vendorItems = AI_VALUE(set<uint32>&, "vendor list");
        for (ItemIds::iterator i = items.begin(); i != items.end(); i++)
        {
            uint32 itemid = *i;
            if (query)
            {
                ItemPrototype const *proto = sObjectMgr.GetItemPrototype(itemid);
                if (proto)
                {
                    ostringstream out;
                    out << (StoreLootAction::IsLootAllowed(itemid, ai) ? "|cFF000000Will vendor " : "|c00FF0000Won't vendor ") << ChatHelper::formatItem(proto);
                    ai->TellMaster(out.str());
                }
            }

            if (remove)
            {
                set<uint32>::iterator j = vendorItems.find(itemid);
                if (j != vendorItems.end()) vendorItems.erase(j);
                changes = true;
            }

            if (add)
            {
                vendorItems.insert(itemid);
                changes = true;
            }
        }

        if (changes)
        {
            TellVendorList("vendor list");
        }
    }

    return true;
}

void VendorListAction::TellVendorList(string name)
{
    set<uint32>& alwaysLootItems = AI_VALUE(set<uint32>&, name);
    ostringstream out;
    out << "My " << name << ":";

    for (set<uint32>::iterator i = alwaysLootItems.begin(); i != alwaysLootItems.end(); i++)
    {
        ItemPrototype const *proto = sItemStorage.LookupEntry<ItemPrototype>(*i);
        if (!proto)
            continue;

        out << " " << chat->formatItem(proto);
    }
    ai->TellMaster(out);
}