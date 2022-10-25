#include "botpch.h"
#include "../../playerbot.h"
#include "ItemUsageValue.h"

#include "../../../ahbot/AhBot.h"
#include "../../GuildTaskMgr.h"
using namespace ai;

ItemUsage ItemUsageValue::Calculate()
{
    uint32 itemId = atoi(qualifier.c_str());
    if (!itemId)
    {
        return ITEM_USAGE_NONE;
    }

    const ItemPrototype* proto = sObjectMgr.GetItemPrototype(itemId);
    if (!proto)
    {
        return ITEM_USAGE_NONE;
    }

    if (IsItemUsefulForSkill(proto))
    {
        return ITEM_USAGE_SKILL;
    }

    switch (proto->Class)
    {
    case ITEM_CLASS_KEY:
    case ITEM_CLASS_CONSUMABLE:
        return ITEM_USAGE_USE;
    }

    if (bot->GetGuildId() && sGuildTaskMgr.IsGuildTaskItem(itemId, bot->GetGuildId()))
    {
        return ITEM_USAGE_GUILD_TASK;
    }

    ItemUsage equip = QueryItemUsageForEquip(proto);
    if (equip != ITEM_USAGE_NONE)
    {
        return equip;
    }

    if ((proto->Class == ITEM_CLASS_ARMOR || proto->Class == ITEM_CLASS_WEAPON) && proto->Bonding != BIND_WHEN_PICKED_UP &&
            ai->HasSkill(SKILL_ENCHANTING) && proto->Quality >= ITEM_QUALITY_UNCOMMON)
        return ITEM_USAGE_DISENCHANT;

    return ITEM_USAGE_NONE;
}

ItemUsage ItemUsageValue::QueryItemUsageForEquip(ItemPrototype const * item)
{
    if (bot->CanUseItem(item) != EQUIP_ERR_OK)
    {
        return ITEM_USAGE_NONE;
    }

    if (item->InventoryType == INVTYPE_NON_EQUIP)
    {
        return ITEM_USAGE_NONE;
    }

    Item *pItem = Item::CreateItem(item->ItemId, 1, bot);
    if (!pItem)
    {
        return ITEM_USAGE_NONE;
    }

    uint16 dest;
    InventoryResult result = bot->CanEquipItem(NULL_SLOT, dest, pItem, true, false);
    pItem->RemoveFromUpdateQueueOf(bot);
    delete pItem;

    if( result != EQUIP_ERR_OK )
    {
        return ITEM_USAGE_NONE;
    }

    Item* existingItem = bot->GetItemByPos(dest);
    if (!existingItem)
    {
        return ITEM_USAGE_EQUIP;
    }

    const ItemPrototype* oldItem = existingItem->GetProto();
    if (oldItem->ItemLevel < item->ItemLevel && oldItem->ItemId != item->ItemId)
    {
        switch (item->Class)
        {
        case ITEM_CLASS_ARMOR:
            if (oldItem->SubClass <= item->SubClass) {
            {
                return ITEM_USAGE_REPLACE;
            }
            }
            break;
        default:
            return ITEM_USAGE_EQUIP;
        }
    }

    return ITEM_USAGE_NONE;
}

bool ItemUsageValue::IsItemUsefulForSkill(ItemPrototype const * proto)
{
    switch (proto->Class)
    {
    case ITEM_CLASS_TRADE_GOODS:
    case ITEM_CLASS_MISC:
    case ITEM_CLASS_REAGENT:
        {
            if (ai->HasSkill(SKILL_TAILORING) && auctionbot.IsUsedBySkill(proto, SKILL_TAILORING))
            {
                return true;
            }
            if (ai->HasSkill(SKILL_LEATHERWORKING) && auctionbot.IsUsedBySkill(proto, SKILL_LEATHERWORKING))
            {
                return true;
            }
            if (ai->HasSkill(SKILL_ENGINEERING) && auctionbot.IsUsedBySkill(proto, SKILL_ENGINEERING))
            {
                return true;
            }
            if (ai->HasSkill(SKILL_BLACKSMITHING) && auctionbot.IsUsedBySkill(proto, SKILL_BLACKSMITHING))
            {
                return true;
            }
            if (ai->HasSkill(SKILL_ALCHEMY) && auctionbot.IsUsedBySkill(proto, SKILL_ALCHEMY))
            {
                return true;
            }
            if (ai->HasSkill(SKILL_ENCHANTING) && auctionbot.IsUsedBySkill(proto, SKILL_ENCHANTING))
            {
                return true;
            }
            if (ai->HasSkill(SKILL_FISHING) && auctionbot.IsUsedBySkill(proto, SKILL_FISHING))
            {
                return true;
            }
            if (ai->HasSkill(SKILL_FIRST_AID) && auctionbot.IsUsedBySkill(proto, SKILL_FIRST_AID))
            {
                return true;
            }
            if (ai->HasSkill(SKILL_COOKING) && auctionbot.IsUsedBySkill(proto, SKILL_COOKING))
            {
                return true;
            }
            if (ai->HasSkill(SKILL_MINING) &&
                    (auctionbot.IsUsedBySkill(proto, SKILL_MINING) || auctionbot.IsUsedBySkill(proto, SKILL_BLACKSMITHING) || auctionbot.IsUsedBySkill(proto, SKILL_ENGINEERING)))
                return true;
            if (ai->HasSkill(SKILL_SKINNING) &&
                    (auctionbot.IsUsedBySkill(proto, SKILL_SKINNING) || auctionbot.IsUsedBySkill(proto, SKILL_LEATHERWORKING)))
                return true;
            if (ai->HasSkill(SKILL_HERBALISM) &&
                    (auctionbot.IsUsedBySkill(proto, SKILL_HERBALISM) || auctionbot.IsUsedBySkill(proto, SKILL_ALCHEMY)))
                return true;

            return false;
        }
    case ITEM_CLASS_RECIPE:
        {
            if (bot->HasSpell(proto->Spells[2].SpellId))
            {
                break;
            }

            switch (proto->SubClass)
            {
            case ITEM_SUBCLASS_LEATHERWORKING_PATTERN:
                return ai->HasSkill(SKILL_LEATHERWORKING);
            case ITEM_SUBCLASS_TAILORING_PATTERN:
                return ai->HasSkill(SKILL_TAILORING);
            case ITEM_SUBCLASS_ENGINEERING_SCHEMATIC:
                return ai->HasSkill(SKILL_ENGINEERING);
            case ITEM_SUBCLASS_BLACKSMITHING:
                return ai->HasSkill(SKILL_BLACKSMITHING);
            case ITEM_SUBCLASS_COOKING_RECIPE:
                return ai->HasSkill(SKILL_COOKING);
            case ITEM_SUBCLASS_ALCHEMY_RECIPE:
                return ai->HasSkill(SKILL_ALCHEMY);
            case ITEM_SUBCLASS_FIRST_AID_MANUAL:
                return ai->HasSkill(SKILL_FIRST_AID);
            case ITEM_SUBCLASS_ENCHANTING_FORMULA:
                return ai->HasSkill(SKILL_ENCHANTING);
            case ITEM_SUBCLASS_FISHING_MANUAL:
                return ai->HasSkill(SKILL_FISHING);
            }
        }
    }
    return false;
}
