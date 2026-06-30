/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2025 MaNGOS <https://www.getmangos.eu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

/**
 * @file PlayerItem.cpp
 * @brief Cohesion split of Player.cpp -- inventory storage: add/remove/swap/find item slots.
 *        Same `Player` class; no behaviour change.
 */

#include "Player.h"
#include "Language.h"
#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "Opcodes.h"
#include "SpellMgr.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "UpdateMask.h"
#include "SkillDiscovery.h"
#include "QuestDef.h"
#include "GossipDef.h"
#include "UpdateData.h"
#include "Channel.h"
#include "ChannelMgr.h"
#include "MapManager.h"
#include "MapPersistentStateMgr.h"
#include "InstanceData.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "CreatureAI.h"
#include "Formulas.h"
#include "Group.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Pet.h"
#include "Util.h"
#include "Transports.h"
#include "Weather.h"
#include "BattleGround/BattleGround.h"
#include "BattleGround/BattleGroundMgr.h"
#include "BattleGround/BattleGroundAV.h"
#include "OutdoorPvP/OutdoorPvP.h"
#include "ArenaTeam.h"
#include "Chat.h"
#include "revision_data.h"
#include "Database/DatabaseImpl.h"
#include "Spell.h"
#include "ScriptMgr.h"
#include "SocialMgr.h"
#include "AchievementMgr.h"
#include "Mail.h"
#include "SpellAuras.h"
#include "DBCStores.h"
#include "SQLStorages.h"
#include "Vehicle.h"
#include "Calendar.h"
#include "DisableMgr.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /* ENABLE_ELUNA */

/**
 * @brief Checks whether a bag and slot refer to normal inventory storage.
 *
 * @param bag The bag identifier.
 * @param slot The slot identifier.
 * @return True if the position is an inventory position; otherwise, false.
 */
bool Player::IsInventoryPos(uint8 bag, uint8 slot)
{
    if (bag == INVENTORY_SLOT_BAG_0 && slot == NULL_SLOT)
    {
        return true;
    }
    if (bag == INVENTORY_SLOT_BAG_0 && (slot >= INVENTORY_SLOT_ITEM_START && slot < INVENTORY_SLOT_ITEM_END))
    {
        return true;
    }
    if (bag >= INVENTORY_SLOT_BAG_START && bag < INVENTORY_SLOT_BAG_END)
    {
        return true;
    }
    if (bag == INVENTORY_SLOT_BAG_0 && (slot >= KEYRING_SLOT_START && slot < CURRENCYTOKEN_SLOT_END))
    {
        return true;
    }
    return false;
}

/**
 * @brief Checks whether a bag and slot refer to an equipment position.
 *
 * @param bag The bag identifier.
 * @param slot The slot identifier.
 * @return True if the position is an equipment position; otherwise, false.
 */
bool Player::IsEquipmentPos(uint8 bag, uint8 slot)
{
    if (bag == INVENTORY_SLOT_BAG_0 && (slot < EQUIPMENT_SLOT_END))
    {
        return true;
    }
    if (bag == INVENTORY_SLOT_BAG_0 && (slot >= INVENTORY_SLOT_BAG_START && slot < INVENTORY_SLOT_BAG_END))
    {
        return true;
    }
    return false;
}

/**
 * @brief Checks whether a bag and slot refer to bank storage.
 *
 * @param bag The bag identifier.
 * @param slot The slot identifier.
 * @return True if the position is in bank storage; otherwise, false.
 */
bool Player::IsBankPos(uint8 bag, uint8 slot)
{
    if (bag == INVENTORY_SLOT_BAG_0 && (slot >= BANK_SLOT_ITEM_START && slot < BANK_SLOT_ITEM_END))
    {
        return true;
    }
    if (bag == INVENTORY_SLOT_BAG_0 && (slot >= BANK_SLOT_BAG_START && slot < BANK_SLOT_BAG_END))
    {
        return true;
    }
    if (bag >= BANK_SLOT_BAG_START && bag < BANK_SLOT_BAG_END)
    {
        return true;
    }
    return false;
}

/**
 * @brief Checks whether a packed position refers to a bag slot.
 *
 * @param pos The packed bag and slot position.
 * @return True if the position is a bag slot; otherwise, false.
 */
bool Player::IsBagPos(uint16 pos)
{
    uint8 bag = pos >> 8;
    uint8 slot = pos & 255;
    if (bag == INVENTORY_SLOT_BAG_0 && (slot >= INVENTORY_SLOT_BAG_START && slot < INVENTORY_SLOT_BAG_END))
    {
        return true;
    }
    if (bag == INVENTORY_SLOT_BAG_0 && (slot >= BANK_SLOT_BAG_START && slot < BANK_SLOT_BAG_END))
    {
        return true;
    }
    return false;
}

/**
 * @brief Checks whether a bag and slot form a valid player storage position.
 *
 * @param bag The bag identifier.
 * @param slot The slot identifier.
 * @param explicit_pos True if the caller requires an explicit fixed position.
 * @return True if the position is valid; otherwise, false.
 */
bool Player::IsValidPos(uint8 bag, uint8 slot, bool explicit_pos) const
{
    // post selected
    if (bag == NULL_BAG && !explicit_pos)
    {
        return true;
    }

    if (bag == INVENTORY_SLOT_BAG_0)
    {
        // any post selected
        if (slot == NULL_SLOT && !explicit_pos)
        {
            return true;
        }

        // equipment
        if (slot < EQUIPMENT_SLOT_END)
        {
            return true;
        }

        // bag equip slots
        if (slot >= INVENTORY_SLOT_BAG_START && slot < INVENTORY_SLOT_BAG_END)
        {
            return true;
        }

        // backpack slots
        if (slot >= INVENTORY_SLOT_ITEM_START && slot < INVENTORY_SLOT_ITEM_END)
        {
            return true;
        }

        // keyring slots
        if (slot >= KEYRING_SLOT_START && slot < KEYRING_SLOT_END)
        {
            return true;
        }

        // bank main slots
        if (slot >= BANK_SLOT_ITEM_START && slot < BANK_SLOT_ITEM_END)
        {
            return true;
        }

        // bank bag slots
        if (slot >= BANK_SLOT_BAG_START && slot < BANK_SLOT_BAG_END)
        {
            return true;
        }

        return false;
    }

    // bag content slots
    if (bag >= INVENTORY_SLOT_BAG_START && bag < INVENTORY_SLOT_BAG_END)
    {
        Bag* pBag = reinterpret_cast<Bag*>(GetItemByPos(INVENTORY_SLOT_BAG_0, bag));
        if (!pBag)
        {
            return false;
        }

        // any post selected
        if (slot == NULL_SLOT && !explicit_pos)
        {
            return true;
        }

        return slot < pBag->GetBagSize();
    }

    // bank bag content slots
    if (bag >= BANK_SLOT_BAG_START && bag < BANK_SLOT_BAG_END)
    {
        Bag* pBag = reinterpret_cast<Bag*>(GetItemByPos(INVENTORY_SLOT_BAG_0, bag));
        if (!pBag)
        {
            return false;
        }

        // any post selected
        if (slot == NULL_SLOT && !explicit_pos)
        {
            return true;
        }

        return slot < pBag->GetBagSize();
    }

    // where this?
    return false;
}

/**
 * @brief Checks whether the player owns at least a given count of an item.
 *
 * @param item The item entry to count.
 * @param count The required quantity.
 * @param inBankAlso True to include bank storage in the search.
 * @return True if enough items are present; otherwise, false.
 */
bool Player::HasItemCount(uint32 item, uint32 count, bool inBankAlso) const
{
    uint32 tempcount = 0;
    for (int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        Item* pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (pItem && pItem->GetEntry() == item && !pItem->IsInTrade())
        {
            tempcount += pItem->GetCount();
            if (tempcount >= count)
            {
                return true;
            }
        }
    }
    for (int i = KEYRING_SLOT_START; i < CURRENCYTOKEN_SLOT_END; ++i)
    {
        Item* pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (pItem && pItem->GetEntry() == item && !pItem->IsInTrade())
        {
            tempcount += pItem->GetCount();
            if (tempcount >= count)
            {
                return true;
            }
        }
    }
    for (int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if (Bag* pBag = reinterpret_cast<Bag*>(GetItemByPos(INVENTORY_SLOT_BAG_0, i)))
        {
            for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
            {
                Item* pItem = GetItemByPos(i, j);
                if (pItem && pItem->GetEntry() == item && !pItem->IsInTrade())
                {
                    tempcount += pItem->GetCount();
                    if (tempcount >= count)
                    {
                        return true;
                    }
                }
            }
        }
    }

    if (inBankAlso)
    {
        for (int i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; ++i)
        {
            Item* pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, i);
            if (pItem && pItem->GetEntry() == item && !pItem->IsInTrade())
            {
                tempcount += pItem->GetCount();
                if (tempcount >= count)
                {
                    return true;
                }
            }
        }
        for (int i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
        {
            if (Bag* pBag = reinterpret_cast<Bag*>(GetItemByPos(INVENTORY_SLOT_BAG_0, i)))
            {
                for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
                {
                    Item* pItem = GetItemByPos(i, j);
                    if (pItem && pItem->GetEntry() == item && !pItem->IsInTrade())
                    {
                        tempcount += pItem->GetCount();
                        if (tempcount >= count)
                        {
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

bool Player::HasItemOrGemWithIdEquipped(uint32 item, uint32 count, uint8 except_slot) const
{
    uint32 tempcount = 0;
    for (int i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        if (i == int(except_slot))
        {
            continue;
        }

        Item* pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (pItem && pItem->GetEntry() == item)
        {
            tempcount += pItem->GetCount();
            if (tempcount >= count)
            {
                return true;
            }
        }
    }

    ItemPrototype const* pProto = ObjectMgr::GetItemPrototype(item);
    if (pProto && pProto->GemProperties)
    {
        for (int i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
        {
            if (i == int(except_slot))
            {
                continue;
            }

            Item* pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, i);
            if (pItem && pItem->GetProto()->Socket[0].Color)
            {
                tempcount += pItem->GetGemCountWithID(item);
                if (tempcount >= count)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

bool Player::HasItemOrGemWithLimitCategoryEquipped(uint32 limitCategory, uint32 count, uint8 except_slot) const
{
    uint32 tempcount = 0;
    for (int i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        if (i == int(except_slot))
        {
            continue;
        }

        Item* pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (!pItem)
        {
            continue;
        }

        ItemPrototype const* pProto = pItem->GetProto();
        if (!pProto)
        {
            continue;
        }

        if (pProto->ItemLimitCategory == limitCategory)
        {
            tempcount += pItem->GetCount();
            if (tempcount >= count)
            {
                return true;
            }
        }

        if (pProto->Socket[0].Color)
        {
            tempcount += pItem->GetGemCountWithLimitCategory(limitCategory);
            if (tempcount >= count)
            {
                return true;
            }
        }
    }

    return false;
}

/**
 * @brief Cancels the current trade and clears trade state for both players.
 *
 * @param sendback True to send a cancel notification to this player as well.
 */
void Player::TradeCancel(bool sendback)
{
    if (m_trade)
    {
        Player* trader = m_trade->GetTrader();

        // send yellow "Trade canceled" message to both traders
        if (sendback)
        {
            GetSession()->SendCancelTrade();
        }

        trader->GetSession()->SendCancelTrade();

        // cleanup
        delete m_trade;
        m_trade = NULL;
        delete trader->m_trade;
        trader->m_trade = NULL;
    }
}
