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
 * @file CharacterHandlerCustomize.cpp
 * @brief Cohesion split of CharacterHandler.cpp -- character customization and
 *        equipment-set opcodes: appearance alter, customize/rename (with the
 *        rename DB callback), declined names, glyph removal, and equipment-set
 *        save/use/delete. Same `WorldSession` handlers; no behaviour change.
 */

#include "Common/ServerDefines.h"
#include "Platform/Define.h"
#include <string>
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "SharedDefines.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "World.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "CinematicFlyover.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "UpdateMask.h"
#include "Group.h"
#include "Database/DatabaseImpl.h"
#include "PlayerDump.h"
#include "SocialMgr.h"
#include "Util.h"
#include "ArenaTeam.h"
#include "Language.h"
#include "SpellMgr.h"
#include "Calendar.h"
#include "GameTime.h"
#include "Timer.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /* ENABLE_ELUNA */

/**
 * @brief Validates and starts the asynchronous character rename flow.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleCharRenameOpcode(WorldPacket& recv_data)
{
    ObjectGuid guid;
    std::string newname;

    recv_data >> guid;
    recv_data >> newname;

    // prevent character rename to invalid name
    if (!normalizePlayerName(newname))
    {
        WorldPacket data(SMSG_CHAR_RENAME, 1);
        data << uint8(CHAR_NAME_NO_NAME);
        SendPacket(&data);
        return;
    }

    uint8 res = ObjectMgr::CheckPlayerName(newname, true);
    if (res != CHAR_NAME_SUCCESS)
    {
        WorldPacket data(SMSG_CHAR_RENAME, 1);
        data << uint8(res);
        SendPacket(&data);
        return;
    }

    // check name limitations
    if (GetSecurity() == SEC_PLAYER && sObjectMgr.IsReservedName(newname))
    {
        WorldPacket data(SMSG_CHAR_RENAME, 1);
        data << uint8(CHAR_NAME_RESERVED);
        SendPacket(&data);
        return;
    }

    std::string escaped_newname = newname;
    CharacterDatabase.escape_string(escaped_newname);

    // make sure that the character belongs to the current account, that rename at login is enabled
    // and that there is no character with the desired new name
    CharacterDatabase.AsyncPQuery(&WorldSession::HandleChangePlayerNameOpcodeCallBack,
                                  GetAccountId(), newname,
                                  "SELECT `guid`, `name` FROM `characters` WHERE `guid` = %u AND `account` = %u AND (`at_login` & %u) = %u AND NOT EXISTS (SELECT NULL FROM `characters` WHERE `name` = '%s')",
                                  guid.GetCounter(), GetAccountId(), AT_LOGIN_RENAME, AT_LOGIN_RENAME, escaped_newname.c_str()
                                 );
}

/**
 * @brief Finalizes a character rename after the database validation query completes.
 *
 * @param result The rename validation query result.
 * @param accountId The session account id.
 * @param newname The requested new character name.
 */
void WorldSession::HandleChangePlayerNameOpcodeCallBack(QueryResult* result, uint32 accountId, std::string newname)
{
    WorldSession* session = sWorld.FindSession(accountId);
    if (!session)
    {
        if (result)
        {
            delete result;
        }
        return;
    }

    if (!result)
    {
        WorldPacket data(SMSG_CHAR_RENAME, 1);
        data << uint8(CHAR_CREATE_ERROR);
        session->SendPacket(&data);
        return;
    }

    uint32 guidLow = result->Fetch()[0].GetUInt32();
    ObjectGuid guid = ObjectGuid(HIGHGUID_PLAYER, guidLow);
    std::string oldname = result->Fetch()[1].GetCppString();

    delete result;

    CharacterDatabase.BeginTransaction();
    CharacterDatabase.PExecute("UPDATE `characters` SET `name` = '%s', `at_login` = `at_login` & ~ %u WHERE `guid` ='%u'", newname.c_str(), uint32(AT_LOGIN_RENAME), guidLow);
    CharacterDatabase.PExecute("DELETE FROM `character_declinedname` WHERE `guid` ='%u'", guidLow);
    CharacterDatabase.CommitTransaction();

    sLog.outChar("Account: %d (IP: %s) Character:[%s] (guid:%u) Changed name to: %s", session->GetAccountId(), session->GetRemoteAddress().c_str(), oldname.c_str(), guidLow, newname.c_str());

    WorldPacket data(SMSG_CHAR_RENAME, 1 + 8 + (newname.size() + 1));
    data << uint8(RESPONSE_SUCCESS);
    data << guid;
    data << newname;
    session->SendPacket(&data);

    sWorld.InvalidatePlayerDataToAllClient(guid);
}

void WorldSession::HandleSetPlayerDeclinedNamesOpcode(WorldPacket& recv_data)
{
    ObjectGuid guid;

    recv_data >> guid;

    // not accept declined names for unsupported languages
    std::string name;
    if (!sObjectMgr.GetPlayerNameByGUID(guid, name))
    {
        WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4 + 8);
        data << uint32(1);
        data << ObjectGuid(guid);
        SendPacket(&data);
        return;
    }

    std::wstring wname;
    if (!Utf8toWStr(name, wname))
    {
        WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4 + 8);
        data << uint32(1);
        data << ObjectGuid(guid);
        SendPacket(&data);
        return;
    }

    if (!isCyrillicCharacter(wname[0]))                     // name already stored as only single alphabet using
    {
        WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4 + 8);
        data << uint32(1);
        data << ObjectGuid(guid);
        SendPacket(&data);
        return;
    }

    std::string name2;
    DeclinedName declinedname;

    recv_data >> name2;

    if (name2 != name)                                      // character have different name
    {
        WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4 + 8);
        data << uint32(1);
        data << ObjectGuid(guid);
        SendPacket(&data);
        return;
    }

    for (int i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
    {
        recv_data >> declinedname.name[i];
        if (!normalizePlayerName(declinedname.name[i]))
        {
            WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4 + 8);
            data << uint32(1);
            data << ObjectGuid(guid);
            SendPacket(&data);
            return;
        }
    }

    if (!ObjectMgr::CheckDeclinedNames(GetMainPartOfName(wname, 0), declinedname))
    {
        WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4 + 8);
        data << uint32(1);
        data << ObjectGuid(guid);
        SendPacket(&data);
        return;
    }

    for (int i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
    {
        CharacterDatabase.escape_string(declinedname.name[i]);
    }

    CharacterDatabase.BeginTransaction();
    CharacterDatabase.PExecute("DELETE FROM `character_declinedname` WHERE `guid` = '%u'", guid.GetCounter());
    CharacterDatabase.PExecute("INSERT INTO `character_declinedname` (`guid`, `genitive`, `dative`, `accusative`, `instrumental`, `prepositional`) VALUES ('%u','%s','%s','%s','%s','%s')",
                               guid.GetCounter(), declinedname.name[0].c_str(), declinedname.name[1].c_str(), declinedname.name[2].c_str(), declinedname.name[3].c_str(), declinedname.name[4].c_str());
    CharacterDatabase.CommitTransaction();

    WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4 + 8);
    data << uint32(0);                                      // OK
    data << ObjectGuid(guid);
    SendPacket(&data);
}

void WorldSession::HandleAlterAppearanceOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("CMSG_ALTER_APPEARANCE");

    uint32 Hair, Color, FacialHair, skinTone;
    recv_data >> Hair >> Color >> FacialHair >> skinTone;

    uint32 skinTone_id = -1;
    if (_player->getRace() == RACE_TAUREN)
    {
        BarberShopStyleEntry const* bs_skinTone = sBarberShopStyleStore.LookupEntry(skinTone);
        if (!bs_skinTone || bs_skinTone->Type != 3 || bs_skinTone->Race != _player->getRace() || bs_skinTone->Sex != _player->getGender())
        {
            return;
        }
        skinTone_id = bs_skinTone->Data;
    }

    BarberShopStyleEntry const* bs_hair = sBarberShopStyleStore.LookupEntry(Hair);

    if (!bs_hair || bs_hair->Type != 0 || bs_hair->Race != _player->getRace() || bs_hair->Sex != _player->getGender())
    {
        return;
    }

    BarberShopStyleEntry const* bs_facialHair = sBarberShopStyleStore.LookupEntry(FacialHair);

    if (!bs_facialHair || bs_facialHair->Type != 2 || bs_facialHair->Race != _player->getRace() || bs_facialHair->Sex != _player->getGender())
    {
        return;
    }

    uint32 Cost = _player->GetBarberShopCost(bs_hair->Data, Color, bs_facialHair->Data, skinTone_id);

    // 0 - ok
    // 1,3 - not enough money
    // 2 - you have to seat on barber chair
    if (_player->GetMoney() < Cost)
    {
        WorldPacket data(SMSG_BARBER_SHOP_RESULT, 4);
        data << uint32(1);                                  // no money
        SendPacket(&data);
        return;
    }
    else
    {
        WorldPacket data(SMSG_BARBER_SHOP_RESULT, 4);
        data << uint32(0);                                  // ok
        SendPacket(&data);
    }

    _player->ModifyMoney(-int32(Cost));                     // it isn't free
    _player->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_AT_BARBER, Cost);

    _player->SetByteValue(PLAYER_BYTES, 2, uint8(bs_hair->Data));
    _player->SetByteValue(PLAYER_BYTES, 3, uint8(Color));
    _player->SetByteValue(PLAYER_BYTES_2, 0, uint8(bs_facialHair->Data));
    if (_player->getRace() == RACE_TAUREN)
    {
        _player->SetByteValue(PLAYER_BYTES, 0, uint8(skinTone_id));
    }

    _player->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_VISIT_BARBER_SHOP, 1);

    _player->SetStandState(0);                              // stand up
}

void WorldSession::HandleRemoveGlyphOpcode(WorldPacket& recv_data)
{
    uint32 slot;
    recv_data >> slot;

    if (slot >= MAX_GLYPH_SLOT_INDEX)
    {
        DEBUG_LOG("Client sent wrong glyph slot number in opcode CMSG_REMOVE_GLYPH %u", slot);
        return;
    }

    if (_player->GetGlyph(slot))
    {
        _player->ApplyGlyph(slot, false);
        _player->SetGlyph(slot, 0);
        _player->SendTalentsInfoData(false);
    }
}

void WorldSession::HandleCharCustomizeOpcode(WorldPacket& recv_data)
{
    ObjectGuid guid;
    std::string newname;

    recv_data >> guid;
    recv_data >> newname;

    uint8 gender, skin, face, hairStyle, hairColor, facialHair;
    recv_data >> gender >> skin >> hairColor >> hairStyle >> facialHair >> face;

    QueryResult* result = CharacterDatabase.PQuery("SELECT `at_login` FROM `characters` WHERE `guid` = '%u'", guid.GetCounter());
    if (!result)
    {
        WorldPacket data(SMSG_CHAR_CUSTOMIZE, 1);
        data << uint8(CHAR_CREATE_ERROR);
        SendPacket(&data);
        return;
    }

    Field* fields = result->Fetch();
    uint32 at_loginFlags = fields[0].GetUInt32();
    delete result;

    if (!(at_loginFlags & AT_LOGIN_CUSTOMIZE))
    {
        WorldPacket data(SMSG_CHAR_CUSTOMIZE, 1);
        data << uint8(CHAR_CREATE_ERROR);
        SendPacket(&data);
        return;
    }

    // prevent character rename to invalid name
    if (!normalizePlayerName(newname))
    {
        WorldPacket data(SMSG_CHAR_CUSTOMIZE, 1);
        data << uint8(CHAR_NAME_NO_NAME);
        SendPacket(&data);
        return;
    }

    uint8 res = ObjectMgr::CheckPlayerName(newname, true);
    if (res != CHAR_NAME_SUCCESS)
    {
        WorldPacket data(SMSG_CHAR_CUSTOMIZE, 1);
        data << uint8(res);
        SendPacket(&data);
        return;
    }

    // check name limitations
    if (GetSecurity() == SEC_PLAYER && sObjectMgr.IsReservedName(newname))
    {
        WorldPacket data(SMSG_CHAR_CUSTOMIZE, 1);
        data << uint8(CHAR_NAME_RESERVED);
        SendPacket(&data);
        return;
    }

    // character with this name already exist
    ObjectGuid newguid = sObjectMgr.GetPlayerGuidByName(newname);
    if (newguid && newguid != guid)
    {
        WorldPacket data(SMSG_CHAR_CUSTOMIZE, 1);
        data << uint8(CHAR_CREATE_NAME_IN_USE);
        SendPacket(&data);
        return;
    }

    CharacterDatabase.escape_string(newname);
    Player::Customize(guid, gender, skin, face, hairStyle, hairColor, facialHair);
    CharacterDatabase.PExecute("UPDATE `characters` SET `name` = '%s', `at_login` = `at_login` & ~ %u WHERE `guid` ='%u'", newname.c_str(), uint32(AT_LOGIN_CUSTOMIZE), guid.GetCounter());
    CharacterDatabase.PExecute("DELETE FROM `character_declinedname` WHERE `guid` ='%u'", guid.GetCounter());

    std::string IP_str = GetRemoteAddress();
    sLog.outChar("Account: %d (IP: %s), Character %s customized to: %s", GetAccountId(), IP_str.c_str(), guid.GetString().c_str(), newname.c_str());

    WorldPacket data(SMSG_CHAR_CUSTOMIZE, 1 + 8 + (newname.size() + 1) + 6);
    data << uint8(RESPONSE_SUCCESS);
    data << ObjectGuid(guid);
    data << newname;
    data << uint8(gender);
    data << uint8(skin);
    data << uint8(face);
    data << uint8(hairStyle);
    data << uint8(hairColor);
    data << uint8(facialHair);
    SendPacket(&data);

    sWorld.InvalidatePlayerDataToAllClient(guid);
}

void WorldSession::HandleEquipmentSetSaveOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("CMSG_EQUIPMENT_SET_SAVE");

    ObjectGuid setGuid;
    uint32 index;
    std::string name;
    std::string iconName;

    recv_data >> setGuid.ReadAsPacked();
    recv_data >> index;
    recv_data >> name;
    recv_data >> iconName;

    if (index >= MAX_EQUIPMENT_SET_INDEX)                   // client set slots amount
    {
        return;
    }

    EquipmentSet eqSet;

    eqSet.Guid      = setGuid.GetRawValue();
    eqSet.Name      = name;
    eqSet.IconName  = iconName;
    eqSet.state     = EQUIPMENT_SET_NEW;

    for (uint32 i = 0; i < EQUIPMENT_SLOT_END; ++i)
    {
        ObjectGuid itemGuid;

        recv_data >> itemGuid.ReadAsPacked();

        // equipment manager sends "1" (as raw GUID) for slots set to "ignore" (not touch slot at equip set)
        if (itemGuid.GetRawValue() == 1)
        {
            // ignored slots saved as bit mask because we have no free special values for Items[i]
            eqSet.IgnoreMask |= 1 << i;
            continue;
        }

        Item* item = _player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);

        if (!item && itemGuid)                              // cheating check 1
        {
            return;
        }

        if (item && item->GetObjectGuid() != itemGuid)      // cheating check 2
        {
            return;
        }

        eqSet.Items[i] = itemGuid.GetCounter();
    }

    _player->SetEquipmentSet(index, eqSet);
}

void WorldSession::HandleEquipmentSetDeleteOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("CMSG_EQUIPMENT_SET_DELETE");

    ObjectGuid setGuid;

    recv_data >> setGuid.ReadAsPacked();

    _player->DeleteEquipmentSet(setGuid.GetRawValue());
}

void WorldSession::HandleEquipmentSetUseOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("CMSG_EQUIPMENT_SET_USE");
    recv_data.hexlike();

    for (uint32 i = 0; i < EQUIPMENT_SLOT_END; ++i)
    {
        ObjectGuid itemGuid;
        uint8 srcbag, srcslot;

        recv_data >> itemGuid.ReadAsPacked();
        recv_data >> srcbag >> srcslot;

        DEBUG_LOG("Item (%s): srcbag %u, srcslot %u", itemGuid.GetString().c_str(), srcbag, srcslot);

        // check if item slot is set to "ignored" (raw value == 1), must not be unequipped then
        if (itemGuid.GetRawValue() == 1)
        {
            continue;
        }

        Item* item = _player->GetItemByGuid(itemGuid);

        uint16 dstpos = i | (INVENTORY_SLOT_BAG_0 << 8);

        if (!item)
        {
            Item* uItem = _player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
            if (!uItem)
            {
                continue;
            }

            ItemPosCountVec sDest;
            InventoryResult msg = _player->CanStoreItem(NULL_BAG, NULL_SLOT, sDest, uItem, false);
            if (msg == EQUIP_ERR_OK)
            {
                _player->RemoveItem(INVENTORY_SLOT_BAG_0, i, true);
                _player->StoreItem(sDest, uItem, true);
            }
            else
            {
                _player->SendEquipError(msg, uItem, NULL);
            }

            continue;
        }

        if (item->GetPos() == dstpos)
        {
            continue;
        }

        _player->SwapItem(item->GetPos(), dstpos);
    }

    WorldPacket data(SMSG_USE_EQUIPMENT_SET_RESULT, 1);
    data << uint8(0);                                       // 4 - equipment swap failed - inventory is full
    SendPacket(&data);
}
