#include "botpch.h"
#include "../../playerbot.h"
#include "CheckInventoryAction.h"
#include "../values/ItemUsageValue.h"

using namespace ai;

bool CheckInventoryAction::Execute(Event event)
{
    uint32 account = sObjectMgr.GetPlayerAccountIdByGUID(bot->GetObjectGuid());
    bool randomBot = sPlayerbotAIConfig.IsInRandomAccountList(account) && !bot->GetGroup();
    if (!randomBot && !ai->GetMaster()) return true;

    list<Item*> all = parseItems("");
    if (all.empty()) return true;

    vector<Item*> found;
    for (list<Item*>::iterator i = all.begin(); i != all.end(); ++i) found.push_back(*i);

    Item* item = found[urand(0, found.size() - 1)];
    ItemPrototype const* proto = item->GetProto();

    if (proto->Class == ITEM_CLASS_QUEST) return true;

    ostringstream out; out << proto->ItemId;
    ItemUsage usage = AI_VALUE2(ItemUsage, "item usage", out.str());

    switch (usage)
    {
    case ITEM_USAGE_DISENCHANT:
        HandleDisenchant(item, randomBot);
        break;
    case ITEM_USAGE_REPLACE:
    case ITEM_USAGE_EQUIP:
        HandleEquip(item, randomBot);
        break;
    case ITEM_USAGE_SKILL:
        HandleTrade(item, randomBot);
        break;
    case ITEM_USAGE_USE:
        HandleUse(item, randomBot);
        break;
    case ITEM_USAGE_NONE:
        HandleNone(item, randomBot);
        break;
    }

    if (proto->Quality == ITEM_QUALITY_POOR) HandleGray(item, randomBot);

    return true;
}

void CheckInventoryAction::HandleDisenchant(Item* item, bool randomBot)
{
    if (!bot->HasSkill(SKILL_ENCHANTING) || bot->GetSkillValue(SKILL_ENCHANTING) == 1)
    {
        HandleTrade(item, randomBot);
        return;
    }

    if (!randomBot)
    {
        ostringstream out; out << "I think I should disenchant " << chat->formatItem(item->GetProto());
        ai->TellMaster(out.str());
        return;
    }

    if (ai->GetAiObjectContext()->GetValue<uint8>("bag space")->Get() < 80)
    {
        ai->CastSpell("disenchant", NULL, item);
    }
}

void CheckInventoryAction::HandleEquip(Item* item, bool randomBot)
{
    uint16 dest;
    InventoryResult result = bot->CanEquipItem(NULL_SLOT, dest, item, true, false);
    Item* existingItem = bot->GetItemByPos(dest);
    if (!existingItem)
        return;

    if (!randomBot)
    {
        ostringstream out; out << "I think " << chat->formatItem(item->GetProto())
                << " is better than my " << chat->formatItem(existingItem->GetProto());
        ai->TellMaster(out.str());
        return;
    }

    EquipItem(*item);
}

void CheckInventoryAction::HandleTrade(Item* item, bool randomBot)
{
    uint32 price = item->GetCount();
    if (price <= item->GetProto()->SellPrice)
    {
        HandleVendor(item, randomBot);
        return;
    }

    if (!randomBot)
    {
        ostringstream out; out << "I think it is a good idea to post " << chat->formatItem(item->GetProto(), item->GetCount())
                << " to the AH for " << chat->formatMoney(price);
        ai->TellMaster(out.str());
        return;
    }

    bot->DestroyItem(item->GetBagSlot(),item->GetSlot(), true);
    bot->ModifyMoney(price);
}

void CheckInventoryAction::HandleVendor(Item* item, bool randomBot)
{
    uint32 price = item->GetProto()->SellPrice;
    if (!price)
    {
        HandleNone(item, randomBot);
        return;
    }

    if (!randomBot)
    {
        ostringstream out; out << "I don't think " << chat->formatItem(item->GetProto(), item->GetCount())
                << " is worth anything other than selling it for " << chat->formatMoney(price);
        ai->TellMaster(out.str());
        return;
    }

    bot->DestroyItem(item->GetBagSlot(),item->GetSlot(), true);
    bot->ModifyMoney(price);
}

void CheckInventoryAction::HandleGray(Item* item, bool randomBot)
{
    uint32 price = item->GetProto()->SellPrice;
    if (!price)
    {
        HandleNone(item, randomBot);
        return;
    }

    if (!randomBot)
    {
        ostringstream out; out << "I have gray items I should sell";
        ai->TellMaster(out.str());
        return;
    }

    bot->DestroyItem(item->GetBagSlot(),item->GetSlot(), true);
    bot->ModifyMoney(price);
}

void CheckInventoryAction::HandleUse(Item* item, bool randomBot)
{
    if (randomBot)
        return;

    ostringstream out; out << "I think I should use " << chat->formatItem(item->GetProto());
    if (item->GetCount() > 1) out << " more often";
    else out << " when needed";
    ai->TellMaster(out.str());
}

void CheckInventoryAction::HandleNone(Item* item, bool randomBot)
{
    if (sPlayerbotAIConfig.IsInRandomItemKeepList(item->GetProto()->ItemId))
        return;

    if (!randomBot)
    {
        ostringstream out; out << "I don't see any use of " << chat->formatItem(item->GetProto(), item->GetCount());
        ai->TellMaster(out.str());
        return;
    }

    bot->DestroyItem(item->GetBagSlot(),item->GetSlot(), true);
}

