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
 * @file PlayerStateCommands.cpp
 * @brief Cohesion split of PlayerCommands.cpp -- player-state GM commands:
 *        revive/levelup/start, save(all), skills, cooldowns, combat stop,
 *        explore/area reveal, item add/move, repair, grave link and taxi/
 *        dismount cheats. Same `ChatHandler` commands; no behaviour change.
 */

#include <string>
#include "Common/TimeConstants.h"
#include "Common/ServerDefines.h"
#include "PlayerRegistry.h"
#include "CorpseManager.h"
#include "Chat.h"
#include "ObjectMgr.h"
#include "World.h"
#include "AccountMgr.h"
#include "SQLStorages.h"

/**
 * @brief Handler for HandleReviveCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleReviveCommand(char* args)
{
    Player* target;
    ObjectGuid target_guid;
    if (!ExtractPlayerTarget(&args, &target, &target_guid))
    {
        return false;
    }

    if (target)
    {
        target->ResurrectPlayer(0.5f);
        target->SpawnCorpseBones();
    }
    else // will resurrected at login without corpse
    {
        sCorpseManager.ConvertCorpseForPlayer(target_guid);
    }

    return true;
}

/**
 * @brief Handler for HandleDismountCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleDismountCommand(char* /*args*/)
{
    Player* player = m_session->GetPlayer();

    // If player is not mounted, so go out :)
    if (!player->IsMounted())
    {
        SendSysMessage(LANG_CHAR_NON_MOUNTED);
        SetSentErrorMessage(true);
        return false;
    }

    if (player->IsTaxiFlying())
    {
        SendSysMessage(LANG_YOU_IN_FLIGHT);
        SetSentErrorMessage(true);
        return false;
    }

    player->Unmount();
    player->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);
    return true;
}

/**
 * @brief Handler for HandleLinkGraveCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleLinkGraveCommand(char* args)
{
    uint32 g_id;
    if (!ExtractUInt32(&args, g_id))
    {
        return false;
    }

    char* teamStr = ExtractLiteralArg(&args);

    Team g_team;
    if (!teamStr)
    {
        g_team = TEAM_BOTH_ALLOWED;
    }
    else if (strncmp(teamStr, "horde", strlen(teamStr)) == 0)
    {
        g_team = HORDE;
    }
    else if (strncmp(teamStr, "alliance", strlen(teamStr)) == 0)
    {
        g_team = ALLIANCE;
    }
    else
    {
        return false;
    }

    WorldSafeLocsEntry const* graveyard = sWorldSafeLocsStore.LookupEntry(g_id);
    if (!graveyard)
    {
        PSendSysMessage(LANG_COMMAND_GRAVEYARDNOEXIST, g_id);
        SetSentErrorMessage(true);
        return false;
    }

    Player* player = m_session->GetPlayer();

    uint32 zoneId = player->GetZoneId();

    AreaTableEntry const* areaEntry = GetAreaEntryByAreaID(zoneId);
    if (!areaEntry || areaEntry->ParentAreaID != 0)
    {
        PSendSysMessage(LANG_COMMAND_GRAVEYARDWRONGZONE, g_id, zoneId);
        SetSentErrorMessage(true);
        return false;
    }

    if (sObjectMgr.AddGraveYardLink(g_id, zoneId, g_team))
    {
        PSendSysMessage(LANG_COMMAND_GRAVEYARDLINKED, g_id, zoneId);
    }
    else
    {
        PSendSysMessage(LANG_COMMAND_GRAVEYARDALRLINKED, g_id, zoneId);
    }

    return true;
}

/**
 * @brief Handler for HandleItemMoveCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleItemMoveCommand(char* args)
{
    if (!*args)
    {
        return false;
    }
    uint8 srcslot, dstslot;

    char* pParam1 = strtok(args, " ");
    if (!pParam1)
    {
        return false;
    }

    char* pParam2 = strtok(NULL, " ");
    if (!pParam2)
    {
        return false;
    }

    srcslot = (uint8)atoi(pParam1);
    dstslot = (uint8)atoi(pParam2);

    if (srcslot == dstslot)
    {
        return true;
    }

    Player* player = m_session->GetPlayer();
    if (!player->IsValidPos(INVENTORY_SLOT_BAG_0, srcslot, true))
    {
        return false;
    }

    // can be autostore pos
    if (!m_session->GetPlayer()->IsValidPos(INVENTORY_SLOT_BAG_0, dstslot, false))
    {
        return false;
    }

    uint16 src = ((INVENTORY_SLOT_BAG_0 << 8) | srcslot);
    uint16 dst = ((INVENTORY_SLOT_BAG_0 << 8) | dstslot);

    player->SwapItem(src, dst);

    return true;
}

/**
 * @brief Handler for HandleCooldownCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleCooldownCommand(char* args)
{
    Player* target = getSelectedPlayer();
    if (!target)
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    std::string tNameLink = GetNameLink(target);

    if (!*args)
    {
        target->RemoveAllSpellCooldown();
        PSendSysMessage(LANG_REMOVEALL_COOLDOWN, tNameLink.c_str());
    }
    else
    {
        // number or [name] Shift-click form |color|Hspell:spell_id|h[name]|h|r or Htalent form
        uint32 spell_id = ExtractSpellIdFromLink(&args);
        if (!spell_id)
        {
            return false;
        }

        if (!sSpellStore.LookupEntry(spell_id))
        {
            PSendSysMessage(LANG_UNKNOWN_SPELL, target == m_session->GetPlayer() ? GetMangosString(LANG_YOU) : tNameLink.c_str());
            SetSentErrorMessage(true);
            return false;
        }

        target->RemoveSpellCooldown(spell_id, true);
        PSendSysMessage(LANG_REMOVE_COOLDOWN, spell_id, target == m_session->GetPlayer() ? GetMangosString(LANG_YOU) : tNameLink.c_str());
    }
    return true;
}

/**
 * @brief Handler for HandleSaveCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleSaveCommand(char* /*args*/)
{
    Player* player = m_session->GetPlayer();

    // save GM account without delay and output message (testing, etc)
    if (GetAccessLevel() > SEC_PLAYER)
    {
        player->SaveToDB();
        SendSysMessage(LANG_PLAYER_SAVED);
        return true;
    }

    // save or plan save after 20 sec (logout delay) if current next save time more this value and _not_ output any messages to prevent cheat planning
    uint32 save_interval = sWorld.getConfig(CONFIG_UINT32_INTERVAL_SAVE);
    if (save_interval == 0 || (save_interval > 20 * IN_MILLISECONDS && player->GetSaveTimer() <= save_interval - 20 * IN_MILLISECONDS))
    {
        player->SaveToDB();
    }

    return true;
}

/**
 * @brief Handler for HandleSaveAllCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleSaveAllCommand(char* /*args*/)
{
    sPlayerRegistry.SaveAll();
    SendSysMessage(LANG_PLAYERS_SAVED);
    return true;
}

/**
 * @brief Handler for HandleStartCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleStartCommand(char* /*args*/)
{
    Player* chr = m_session->GetPlayer();

    if (chr->IsTaxiFlying())
    {
        SendSysMessage(LANG_YOU_IN_FLIGHT);
        SetSentErrorMessage(true);
        return false;
    }

    if (chr->IsInCombat())
    {
        SendSysMessage(LANG_YOU_IN_COMBAT);
        SetSentErrorMessage(true);
        return false;
    }

    // cast spell Stuck
    chr->CastSpell(chr, 7355, false);
    return true;
}

/**
 * @brief Handler for HandleTaxiCheatCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleTaxiCheatCommand(char* args)
{
    bool value;
    if (!ExtractOnOff(&args, value))
    {
        SendSysMessage(LANG_USE_BOL);
        SetSentErrorMessage(true);
        return false;
    }

    Player* chr = getSelectedPlayer();
    if (!chr)
    {
        chr = m_session->GetPlayer();
    }
    // check online security
    else if (HasLowerSecurity(chr))
    {
        return false;
    }

    if (value)
    {
        chr->SetTaxiCheater(true);
        PSendSysMessage(LANG_YOU_GIVE_TAXIS, GetNameLink(chr).c_str());
        if (needReportToTarget(chr))
        {
            ChatHandler(chr).PSendSysMessage(LANG_YOURS_TAXIS_ADDED, GetNameLink().c_str());
        }
    }
    else
    {
        chr->SetTaxiCheater(false);
        PSendSysMessage(LANG_YOU_REMOVE_TAXIS, GetNameLink(chr).c_str());
        if (needReportToTarget(chr))
        {
            ChatHandler(chr).PSendSysMessage(LANG_YOURS_TAXIS_REMOVED, GetNameLink().c_str());
        }
    }

    return true;
}

/**
 * @brief Handler for HandleExploreCheatCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleExploreCheatCommand(char* args)
{
    if (!*args)
    {
        return false;
    }

    int flag = atoi(args);

    Player* chr = getSelectedPlayer();
    if (chr == NULL)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    if (flag != 0)
    {
        PSendSysMessage(LANG_YOU_SET_EXPLORE_ALL, GetNameLink(chr).c_str());
        if (needReportToTarget(chr))
        {
            ChatHandler(chr).PSendSysMessage(LANG_YOURS_EXPLORE_SET_ALL, GetNameLink().c_str());
        }
    }
    else
    {
        PSendSysMessage(LANG_YOU_SET_EXPLORE_NOTHING, GetNameLink(chr).c_str());
        if (needReportToTarget(chr))
        {
            ChatHandler(chr).PSendSysMessage(LANG_YOURS_EXPLORE_SET_NOTHING, GetNameLink().c_str());
        }
    }

    for (uint8 i = 0; i < PLAYER_EXPLORED_ZONES_SIZE; ++i)
    {
        if (flag != 0)
        {
            m_session->GetPlayer()->SetFlag(PLAYER_EXPLORED_ZONES_1 + i, 0xFFFFFFFF);
        }
        else
        {
            m_session->GetPlayer()->SetFlag(PLAYER_EXPLORED_ZONES_1 + i, 0);
        }
    }

    return true;
}

/**
 * @brief Handler for HandleLevelUpCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleLevelUpCommand(char* args)
{
    int32 addlevel = 1;
    char* nameStr = NULL;

    if (*args)
    {
        nameStr = ExtractOptNotLastArg(&args);

        // exception opt second arg: .levelup $name
        if (!ExtractInt32(&args, addlevel))
        {
            if (!nameStr)
            {
                nameStr = ExtractArg(&args);
            }
            else
            {
                return false;
            }
        }
    }

    Player* target;
    ObjectGuid target_guid;
    std::string target_name;
    if (!ExtractPlayerTarget(&nameStr, &target, &target_guid, &target_name))
    {
        return false;
    }

    int32 oldlevel = target ? target->getLevel() : Player::GetLevelFromDB(target_guid);
    int32 newlevel = oldlevel + addlevel;

    if (newlevel < 1)
    {
        newlevel = 1;
    }

    if (newlevel > STRONG_MAX_LEVEL)                        // hardcoded maximum level
    {
        newlevel = STRONG_MAX_LEVEL;
    }

    HandleCharacterLevel(target, target_guid, oldlevel, newlevel);

    if (!m_session || m_session->GetPlayer() != target)     // including chr==NULL
    {
        std::string nameLink = playerLink(target_name);
        PSendSysMessage(LANG_YOU_CHANGE_LVL, nameLink.c_str(), newlevel);
    }

    return true;
}

/**
 * @brief Handler for HandleShowAreaCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleShowAreaCommand(char* args)
{
    if (!*args)
    {
        return false;
    }

    Player* chr = getSelectedPlayer();
    if (chr == NULL)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    int area = GetAreaFlagByAreaID(atoi(args));
    int offset = area / 32;
    uint32 val = (uint32)(1 << (area % 32));

    if (area < 0 || offset >= PLAYER_EXPLORED_ZONES_SIZE)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    uint32 currFields = chr->GetUInt32Value(PLAYER_EXPLORED_ZONES_1 + offset);
    chr->SetUInt32Value(PLAYER_EXPLORED_ZONES_1 + offset, (uint32)(currFields | val));

    SendSysMessage(LANG_EXPLORE_AREA);
    return true;
}

/**
 * @brief Handler for HandleHideAreaCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleHideAreaCommand(char* args)
{
    if (!*args)
    {
        return false;
    }

    Player* chr = getSelectedPlayer();
    if (chr == NULL)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    int area = GetAreaFlagByAreaID(atoi(args));
    int offset = area / 32;
    uint32 val = (uint32)(1 << (area % 32));

    if (area < 0 || offset >= PLAYER_EXPLORED_ZONES_SIZE)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    uint32 currFields = chr->GetUInt32Value(PLAYER_EXPLORED_ZONES_1 + offset);
    chr->SetUInt32Value(PLAYER_EXPLORED_ZONES_1 + offset, (uint32)(currFields ^ val));

    SendSysMessage(LANG_UNEXPLORE_AREA);
    return true;
}

/**
 * @brief Handler for HandleAddItemCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleAddItemCommand(char* args)
{
    char* cId = ExtractKeyFromLink(&args, "Hitem");
    if (!cId)
    {
        return false;
    }

    uint32 itemId = 0;
    if (!ExtractUInt32(&cId, itemId))                       // [name] manual form
    {
        std::string itemName = cId;
        WorldDatabase.escape_string(itemName);
        QueryResult* result = WorldDatabase.PQuery("SELECT `entry` FROM `item_template` WHERE `name` = '%s'", itemName.c_str());
        if (!result)
        {
            PSendSysMessage(LANG_COMMAND_COULDNOTFIND, cId);
            SetSentErrorMessage(true);
            return false;
        }
        itemId = result->Fetch()->GetUInt16();
        delete result;
    }

    int32 count;
    if (!ExtractOptInt32(&args, count, 1))
    {
        return false;
    }

    Player* pl = m_session->GetPlayer();
    Player* plTarget = getSelectedPlayer();
    if (!plTarget)
    {
        plTarget = pl;
    }

    DETAIL_LOG(GetMangosString(LANG_ADDITEM), itemId, count);

    ItemPrototype const* pProto = ObjectMgr::GetItemPrototype(itemId);
    if (!pProto)
    {
        PSendSysMessage(LANG_COMMAND_ITEMIDINVALID, itemId);
        SetSentErrorMessage(true);
        return false;
    }

    // Subtract
    if (count < 0)
    {
        plTarget->DestroyItemCount(itemId, -count, true, false);
        PSendSysMessage(LANG_REMOVEITEM, itemId, -count, GetNameLink(plTarget).c_str());
        return true;
    }

    // Adding items
    uint32 noSpaceForCount = 0;

    // check space and find places
    ItemPosCountVec dest;
    uint8 msg = plTarget->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, count, &noSpaceForCount);
    if (msg != EQUIP_ERR_OK)                                // convert to possible store amount
    {
        count -= noSpaceForCount;
    }

    if (count == 0 || dest.empty())                         // can't add any
    {
        PSendSysMessage(LANG_ITEM_CANNOT_CREATE, itemId, noSpaceForCount);
        SetSentErrorMessage(true);
        return false;
    }

    Item* item = plTarget->StoreNewItem(dest, itemId, true, Item::GenerateItemRandomPropertyId(itemId));

    // Remove binding (let GM give it to another player later)
    if (pl == plTarget)
    {
        for (ItemPosCountVec::const_iterator itr = dest.begin(); itr != dest.end(); ++itr)
        {
            if (Item* item1 = pl->GetItemByPos(itr->pos))
            {
                item1->SetBinding(false);
            }
        }
    }

    if (count > 0 && item)
    {
        pl->SendNewItem(item, count, false, true);
        if (pl != plTarget)
        {
            plTarget->SendNewItem(item, count, true, false);
        }
    }

    if (noSpaceForCount > 0)
    {
        PSendSysMessage(LANG_ITEM_CANNOT_CREATE, itemId, noSpaceForCount);
    }

    return true;
}

/**
 * @brief Handler for HandleAddItemSetCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleAddItemSetCommand(char* args)
{
    uint32 itemsetId;
    if (!ExtractUint32KeyFromLink(&args, "Hitemset", itemsetId))
    {
        return false;
    }

    // prevent generation all items with itemset field value '0'
    if (itemsetId == 0)
    {
        PSendSysMessage(LANG_NO_ITEMS_FROM_ITEMSET_FOUND, itemsetId);
        SetSentErrorMessage(true);
        return false;
    }

    Player* pl = m_session->GetPlayer();
    Player* plTarget = getSelectedPlayer();
    if (!plTarget)
    {
        plTarget = pl;
    }

    DETAIL_LOG(GetMangosString(LANG_ADDITEMSET), itemsetId);

    bool found = false;
    for (uint32 id = 0; id < sItemStorage.GetMaxEntry(); ++id)
    {
        ItemPrototype const* pProto = sItemStorage.LookupEntry<ItemPrototype>(id);
        if (!pProto)
        {
            continue;
        }

        if (pProto->ItemSet == itemsetId)
        {
            found = true;
            ItemPosCountVec dest;
            InventoryResult msg = plTarget->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, pProto->ItemId, 1);
            if (msg == EQUIP_ERR_OK)
            {
                Item* item = plTarget->StoreNewItem(dest, pProto->ItemId, true);

                // remove binding (let GM give it to another player later)
                if (pl == plTarget)
                {
                    item->SetBinding(false);
                }

                pl->SendNewItem(item, 1, false, true);
                if (pl != plTarget)
                {
                    plTarget->SendNewItem(item, 1, true, false);
                }
            }
            else
            {
                pl->SendEquipError(msg, NULL, NULL, pProto->ItemId);
                PSendSysMessage(LANG_ITEM_CANNOT_CREATE, pProto->ItemId, 1);
            }
        }
    }

    if (!found)
    {
        PSendSysMessage(LANG_NO_ITEMS_FROM_ITEMSET_FOUND, itemsetId);

        SetSentErrorMessage(true);
        return false;
    }

    return true;
}

/**
 * @brief Handler for HandleMaxSkillCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleMaxSkillCommand(char* /*args*/)
{
    Player* SelectedPlayer = getSelectedPlayer();
    if (!SelectedPlayer)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    // each skills that have max skill value dependent from level seted to current level max skill value
    SelectedPlayer->UpdateSkillsToMaxSkillsForLevel();
    return true;
}

/**
 * @brief Handler for HandleSetSkillCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleSetSkillCommand(char* args)
{
    Player* target = getSelectedPlayer();
    if (!target)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    // number or [name] Shift-click form |color|Hskill:skill_id|h[name]|h|r
    char* skill_p = ExtractKeyFromLink(&args, "Hskill");
    if (!skill_p)
    {
        return false;
    }

    int32 skill;
    if (!ExtractInt32(&skill_p, skill))
    {
        return false;
    }

    int32 level;
    if (!ExtractInt32(&args, level))
    {
        return false;
    }

    int32 maxskill;
    if (!ExtractOptInt32(&args, maxskill, target->GetPureMaxSkillValue(skill)))
    {
        return false;
    }

    if (skill <= 0)
    {
        PSendSysMessage(LANG_INVALID_SKILL_ID, skill);
        SetSentErrorMessage(true);
        return false;
    }

    SkillLineEntry const* sl = sSkillLineStore.LookupEntry(skill);
    if (!sl)
    {
        PSendSysMessage(LANG_INVALID_SKILL_ID, skill);
        SetSentErrorMessage(true);
        return false;
    }

    std::string tNameLink = GetNameLink(target);

    if (!target->GetSkillValue(skill))
    {
        PSendSysMessage(LANG_SET_SKILL_ERROR, tNameLink.c_str(), skill, sl->DisplayName_lang[GetSessionDbcLocale()]);
        SetSentErrorMessage(true);
        return false;
    }

    if (level <= 0 || level > maxskill || maxskill <= 0)
    {
        return false;
    }

    target->SetSkill(skill, level, maxskill);
    PSendSysMessage(LANG_SET_SKILL, skill, sl->DisplayName_lang[GetSessionDbcLocale()], tNameLink.c_str(), level, maxskill);

    return true;
}

/**
 * @brief Handler for HandleCombatStopCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleCombatStopCommand(char* args)
{
    Player* target;
    if (!ExtractPlayerTarget(&args, &target))
    {
        return false;
    }

    // check online security
    if (HasLowerSecurity(target))
    {
        return false;
    }

    target->CombatStop();
    target->GetHostileRefManager().deleteReferences();
    return true;
}

/**
 * @brief Handler for HandleRepairitemsCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleRepairitemsCommand(char* args)
{
    Player* target;
    if (!ExtractPlayerTarget(&args, &target))
    {
        return false;
    }

    // check online security
    if (HasLowerSecurity(target))
    {
        return false;
    }

    // Repair items
    target->DurabilityRepairAll(false, 0, false);

    PSendSysMessage(LANG_YOU_REPAIR_ITEMS, GetNameLink(target).c_str());
    if (needReportToTarget(target))
    {
        ChatHandler(target).PSendSysMessage(LANG_YOUR_ITEMS_REPAIRED, GetNameLink().c_str());
    }
    return true;
}
