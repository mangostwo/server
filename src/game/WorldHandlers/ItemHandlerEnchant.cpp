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
 * @file ItemHandlerEnchant.cpp
 * @brief Cohesion split of ItemHandler.cpp -- enchant/socket opcodes:
 *        socketing, temp-enchant cancel, item wrapping, and enchant log /
 *        time-update senders. Same `WorldSession` handlers; no behaviour
 *        change.
 */

#include "Platform/Define.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Item.h"
#include "UpdateData.h"
#include "Chat.h"

/**
 * @brief Sends an enchantment log packet to the client.
 *
 * @param targetGuid The enchanted target guid.
 * @param casterGuid The caster guid.
 * @param itemId The item entry id.
 * @param spellId The enchantment spell id.
 */
void WorldSession::SendEnchantmentLog(ObjectGuid targetGuid, ObjectGuid casterGuid, uint32 itemId, uint32 spellId)
{
    WorldPacket data(SMSG_ENCHANTMENTLOG, (8 + 8 + 4 + 4 + 1)); // last check 2.0.10
    data << ObjectGuid(targetGuid);
    data << ObjectGuid(casterGuid);
    data << uint32(itemId);
    data << uint32(spellId);
    data << uint8(0);
    SendPacket(&data);
}

/**
 * @brief Sends a temporary enchantment timer update.
 *
 * @param playerGuid The owning player guid.
 * @param itemGuid The enchanted item guid.
 * @param slot The equipment slot index.
 * @param duration The remaining duration in milliseconds.
 */
void WorldSession::SendItemEnchantTimeUpdate(ObjectGuid playerGuid, ObjectGuid itemGuid, uint32 slot, uint32 duration)
{
    // last check 2.0.10
    WorldPacket data(SMSG_ITEM_ENCHANT_TIME_UPDATE, (8 + 4 + 4 + 8));
    data << ObjectGuid(itemGuid);
    data << uint32(slot);
    data << uint32(duration);
    data << ObjectGuid(playerGuid);
    SendPacket(&data);
}

/**
 * @brief Wraps an item using wrapping paper.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleWrapItemOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("Received opcode CMSG_WRAP_ITEM");

    uint8 gift_bag, gift_slot, item_bag, item_slot;
    // recv_data.hexlike();

    recv_data >> gift_bag >> gift_slot;                     // paper
    recv_data >> item_bag >> item_slot;                     // item

    DEBUG_LOG("WRAP: receive gift_bag = %u, gift_slot = %u, item_bag = %u, item_slot = %u", gift_bag, gift_slot, item_bag, item_slot);

    Item* gift = _player->GetItemByPos(gift_bag, gift_slot);
    if (!gift)
    {
        _player->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, gift, NULL);
        return;
    }

    // cheating: non-wrapper wrapper (all empty wrappers is stackable)
    if (!(gift->GetProto()->Flags & ITEM_FLAG_WRAPPER) || gift->GetMaxStackCount() == 1)
    {
        _player->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, gift, NULL);
        return;
    }

    Item* item = _player->GetItemByPos(item_bag, item_slot);

    if (!item)
    {
        _player->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, item, NULL);
        return;
    }

    if (item == gift)                                       // not possible with packet from real client
    {
        _player->SendEquipError(EQUIP_ERR_WRAPPED_CANT_BE_WRAPPED, item, NULL);
        return;
    }

    if (item->IsEquipped())
    {
        _player->SendEquipError(EQUIP_ERR_EQUIPPED_CANT_BE_WRAPPED, item, NULL);
        return;
    }

    if (item->GetGuidValue(ITEM_FIELD_GIFTCREATOR))         // HasFlag(ITEM_FIELD_FLAGS, ITEM_DYNFLAG_WRAPPED);
    {
        _player->SendEquipError(EQUIP_ERR_WRAPPED_CANT_BE_WRAPPED, item, NULL);
        return;
    }

    if (item->IsBag())
    {
        _player->SendEquipError(EQUIP_ERR_BAGS_CANT_BE_WRAPPED, item, NULL);
        return;
    }

    if (item->IsSoulBound())
    {
        _player->SendEquipError(EQUIP_ERR_BOUND_CANT_BE_WRAPPED, item, NULL);
        return;
    }

    if (item->GetMaxStackCount() != 1)
    {
        _player->SendEquipError(EQUIP_ERR_STACKABLE_CANT_BE_WRAPPED, item, NULL);
        return;
    }

    // maybe not correct check  (it is better than nothing)
    if (item->GetProto()->MaxCount > 0)
    {
        _player->SendEquipError(EQUIP_ERR_UNIQUE_CANT_BE_WRAPPED, item, NULL);
        return;
    }

    CharacterDatabase.BeginTransaction();
    CharacterDatabase.PExecute("INSERT INTO `character_gifts` VALUES ('%u', '%u', '%u', '%u')", item->GetOwnerGuid().GetCounter(), item->GetGUIDLow(), item->GetEntry(), item->GetUInt32Value(ITEM_FIELD_FLAGS));
    item->SetEntry(gift->GetEntry());

    switch (item->GetEntry())
    {
        case 5042:  item->SetEntry(5043); break;
        case 5048:  item->SetEntry(5044); break;
        case 17303: item->SetEntry(17302); break;
        case 17304: item->SetEntry(17305); break;
        case 17307: item->SetEntry(17308); break;
        case 21830: item->SetEntry(21831); break;
    }
    item->SetGuidValue(ITEM_FIELD_GIFTCREATOR, _player->GetObjectGuid());
    item->SetUInt32Value(ITEM_FIELD_FLAGS, ITEM_DYNFLAG_WRAPPED);
    item->SetState(ITEM_CHANGED, _player);

    if (item->GetState() == ITEM_NEW)                       // save new item, to have alway for `character_gifts` record in `item_instance`
    {
        // after save it will be impossible to remove the item from the queue
        item->RemoveFromUpdateQueueOf(_player);
        item->SaveToDB();                                   // item gave inventory record unchanged and can be save standalone
    }
    CharacterDatabase.CommitTransaction();

    uint32 count = 1;
    _player->DestroyItemCount(gift, count, true);
}

void WorldSession::HandleSocketOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: CMSG_SOCKET_GEMS");

    ObjectGuid itemGuid;
    ObjectGuid gemGuids[MAX_GEM_SOCKETS];

    recv_data >> itemGuid;
    if (!itemGuid.IsItem())
    {
        return;
    }

    for (int i = 0; i < MAX_GEM_SOCKETS; ++i)
    {
        recv_data >> gemGuids[i];
    }

    // cheat -> tried to socket same gem multiple times
    for (int i = 0; i < MAX_GEM_SOCKETS; ++i)
    {
        ObjectGuid gemGuid = gemGuids[i];
        if (!gemGuid)
        {
            continue;
        }

        if (!gemGuid.IsItem())
        {
            return;
        }

        for (int j = i + 1; j < MAX_GEM_SOCKETS; ++j)
        {
            if (gemGuids[j] == gemGuid)
            {
                return;
            }
        }
    }

    Item* itemTarget = _player->GetItemByGuid(itemGuid);
    if (!itemTarget)                                        // missing item to socket
    {
        return;
    }

    ItemPrototype const* itemProto = itemTarget->GetProto();
    if (!itemProto)
    {
        return;
    }

    // this slot is excepted when applying / removing meta gem bonus
    uint8 slot = itemTarget->IsEquipped() ? itemTarget->GetSlot() : uint8(NULL_SLOT);

    Item* Gems[MAX_GEM_SOCKETS];
    for (int i = 0; i < MAX_GEM_SOCKETS; ++i)
    {
        Gems[i] = gemGuids[i] ? _player->GetItemByGuid(gemGuids[i]) : NULL;
    }

    GemPropertiesEntry const* GemProps[MAX_GEM_SOCKETS];
    for (int i = 0; i < MAX_GEM_SOCKETS; ++i)               // get geminfo from dbc storage
    {
        GemProps[i] = (Gems[i]) ? sGemPropertiesStore.LookupEntry(Gems[i]->GetProto()->GemProperties) : NULL;
    }

    for (int i = 0; i < MAX_GEM_SOCKETS; ++i)               // check for hack maybe
    {
        if (!GemProps[i])
        {
            continue;
        }

        // tried to put gem in socket where no socket exists (take care about prismatic sockets)
        if (!itemProto->Socket[i].Color)
        {
            // no prismatic socket
            if (!itemTarget->GetEnchantmentId(PRISMATIC_ENCHANTMENT_SLOT))
            {
                return;
            }

            // not first not-colored (not normally used) socket
            if (i != 0 && !itemProto->Socket[i - 1].Color && (i + 1 >= MAX_GEM_SOCKETS || itemProto->Socket[i + 1].Color))
            {
                return;
            }

            // ok, this is first not colored socket for item with prismatic socket
        }

        // tried to put normal gem in meta socket
        if (itemProto->Socket[i].Color == SOCKET_COLOR_META && GemProps[i]->Type != SOCKET_COLOR_META)
        {
            return;
        }

        // tried to put meta gem in normal socket
        if (itemProto->Socket[i].Color != SOCKET_COLOR_META && GemProps[i]->Type == SOCKET_COLOR_META)
        {
            return;
        }
    }

    uint32 GemEnchants[MAX_GEM_SOCKETS];
    uint32 OldEnchants[MAX_GEM_SOCKETS];
    for (int i = 0; i < MAX_GEM_SOCKETS; ++i)               // get new and old enchantments
    {
        GemEnchants[i] = (GemProps[i]) ? GemProps[i]->EnchantID : 0;
        OldEnchants[i] = itemTarget->GetEnchantmentId(EnchantmentSlot(SOCK_ENCHANTMENT_SLOT + i));
    }

    // check unique-equipped conditions
    for (int i = 0; i < MAX_GEM_SOCKETS; ++i)
    {
        if (!Gems[i])
        {
            continue;
        }

        // continue check for case when attempt add 2 similar unique equipped gems in one item.
        ItemPrototype const* iGemProto = Gems[i]->GetProto();

        // unique item (for new and already placed bit removed enchantments
        if (iGemProto->Flags & ITEM_FLAG_UNIQUE_EQUIPPED)
        {
            for (int j = 0; j < MAX_GEM_SOCKETS; ++j)
            {
                if (i == j)                                 // skip self
                {
                    continue;
                }

                if (Gems[j])
                {
                    if (iGemProto->ItemId == Gems[j]->GetEntry())
                    {
                        _player->SendEquipError(EQUIP_ERR_ITEM_UNIQUE_EQUIPPABLE_SOCKETED, itemTarget, NULL);
                        return;
                    }
                }
                else if (OldEnchants[j])
                {
                    if (SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(OldEnchants[j]))
                    {
                        if (iGemProto->ItemId == enchantEntry->SrcItemID)
                        {
                            _player->SendEquipError(EQUIP_ERR_ITEM_UNIQUE_EQUIPPABLE_SOCKETED, itemTarget, NULL);
                            return;
                        }
                    }
                }
            }
        }

        // unique limit type item
        int32 limit_newcount = 0;
        if (iGemProto->ItemLimitCategory)
        {
            if (ItemLimitCategoryEntry const* limitEntry = sItemLimitCategoryStore.LookupEntry(iGemProto->ItemLimitCategory))
            {
                // NOTE: limitEntry->mode not checked because if item have have-limit then it applied and to equip case

                for (int j = 0; j < MAX_GEM_SOCKETS; ++j)
                {
                    if (Gems[j])
                    {
                        // destroyed gem
                        if (OldEnchants[j])
                        {
                            if (SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(OldEnchants[j]))
                                if (ItemPrototype const* jProto = ObjectMgr::GetItemPrototype(enchantEntry->SrcItemID))
                                    if (iGemProto->ItemLimitCategory == jProto->ItemLimitCategory)
                                    {
                                        --limit_newcount;
                                    }
                        }

                        // new gem
                        if (iGemProto->ItemLimitCategory == Gems[j]->GetProto()->ItemLimitCategory)
                        {
                            ++limit_newcount;
                        }
                    }
                    // existing gem
                    else if (OldEnchants[j])
                    {
                        if (SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(OldEnchants[j]))
                            if (ItemPrototype const* jProto = ObjectMgr::GetItemPrototype(enchantEntry->SrcItemID))
                                if (iGemProto->ItemLimitCategory == jProto->ItemLimitCategory)
                                {
                                    ++limit_newcount;
                                }
                    }
                }

                if (limit_newcount > 0 && uint32(limit_newcount) > limitEntry->Quantity)
                {
                    _player->SendEquipError(EQUIP_ERR_ITEM_UNIQUE_EQUIPPABLE_SOCKETED, itemTarget, NULL);
                    return;
                }
            }
        }

        // for equipped item check all equipment for duplicate equipped gems
        if (itemTarget->IsEquipped())
        {
            if (InventoryResult res = _player->CanEquipUniqueItem(Gems[i], slot, limit_newcount >= 0 ? limit_newcount : 0))
            {
                _player->SendEquipError(res, itemTarget, NULL);
                return;
            }
        }
    }

    bool SocketBonusActivated = itemTarget->GemsFitSockets();    // save state of socketbonus
    _player->ToggleMetaGemsActive(slot, false);             // turn off all metagems (except for the target item)

    // if a meta gem is being equipped, all information has to be written to the item before testing if the conditions for the gem are met

    // remove ALL enchants
    for (uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT + MAX_GEM_SOCKETS; ++enchant_slot)
    {
        _player->ApplyEnchantment(itemTarget, EnchantmentSlot(enchant_slot), false);
    }

    for (int i = 0; i < MAX_GEM_SOCKETS; ++i)
    {
        if (GemEnchants[i])
        {
            itemTarget->SetEnchantment(EnchantmentSlot(SOCK_ENCHANTMENT_SLOT + i), GemEnchants[i], 0, 0);
            if (Item* guidItem = gemGuids[i] ? _player->GetItemByGuid(gemGuids[i]) : NULL)
            {
                _player->DestroyItem(guidItem->GetBagSlot(), guidItem->GetSlot(), true);
            }
        }
    }

    for (uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT + MAX_GEM_SOCKETS; ++enchant_slot)
    {
        _player->ApplyEnchantment(itemTarget, EnchantmentSlot(enchant_slot), true);
    }

    bool SocketBonusToBeActivated = itemTarget->GemsFitSockets();// current socketbonus state
    if (SocketBonusActivated != SocketBonusToBeActivated)   // if there was a change...
    {
        _player->ApplyEnchantment(itemTarget, BONUS_ENCHANTMENT_SLOT, false);
        itemTarget->SetEnchantment(BONUS_ENCHANTMENT_SLOT, (SocketBonusToBeActivated ? itemTarget->GetProto()->socketBonus : 0), 0, 0);
        _player->ApplyEnchantment(itemTarget, BONUS_ENCHANTMENT_SLOT, true);
        // it is not displayed, client has an inbuilt system to determine if the bonus is activated
    }

    _player->ToggleMetaGemsActive(slot, true);              // turn on all metagems (except for target item)
}

/**
 * @brief Cancels a temporary weapon enchantment.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleCancelTempEnchantmentOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: CMSG_CANCEL_TEMP_ENCHANTMENT");

    uint32 eslot;

    recv_data >> eslot;

    // apply only to equipped item
    if (!Player::IsEquipmentPos(INVENTORY_SLOT_BAG_0, eslot))
    {
        return;
    }

    Item* item = GetPlayer()->GetItemByPos(INVENTORY_SLOT_BAG_0, eslot);

    if (!item)
    {
        return;
    }

    if (!item->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT))
    {
        return;
    }

    GetPlayer()->ApplyEnchantment(item, TEMP_ENCHANTMENT_SLOT, false);
    item->ClearEnchantment(TEMP_ENCHANTMENT_SLOT);
}
