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
 * @file PlayerCommands.cpp
 * @brief Implementation of player character management chat commands.
 *
 * This file contains chat command handlers for player operations including:
 * - Player property modification
 * - Character information display
 * - Player state management
 * - Character customization
 */

#include "Chat.h"
#include "ObjectMgr.h"
#include "World.h"
#include "AccountMgr.h"
#include "SQLStorages.h"

 /**********************************************************************
     CommandTable : characterCommandTable
  ***********************************************************************/
bool ChatHandler::HandleCharacterAchievementsCommand(char* args)
{
    Player* target;
    if (!ExtractPlayerTarget(&args, &target))
    {
        return false;
    }

    LocaleConstant loc = GetSessionDbcLocale();

    CompletedAchievementMap const& complitedList = target->GetAchievementMgr().GetCompletedAchievements();
    for (CompletedAchievementMap::const_iterator itr = complitedList.begin(); itr != complitedList.end(); ++itr)
    {
        AchievementEntry const* achEntry = sAchievementStore.LookupEntry(itr->first);
        ShowAchievementListHelper(achEntry, loc, &itr->second.date, target);
    }
    return true;
}

// customize characters
bool ChatHandler::HandleCharacterCustomizeCommand(char* args)
{
    Player* target;
    ObjectGuid target_guid;
    std::string target_name;
    if (!ExtractPlayerTarget(&args, &target, &target_guid, &target_name))
    {
        return false;
    }

    if (target)
    {
        PSendSysMessage(LANG_CUSTOMIZE_PLAYER, GetNameLink(target).c_str());
        target->SetAtLoginFlag(AT_LOGIN_CUSTOMIZE);
        CharacterDatabase.PExecute("UPDATE `characters` SET `at_login` = `at_login` | '8' WHERE `guid` = '%u'", target->GetGUIDLow());
    }
    else
    {
        std::string oldNameLink = playerLink(target_name);

        PSendSysMessage(LANG_CUSTOMIZE_PLAYER_GUID, oldNameLink.c_str(), target_guid.GetCounter());
        CharacterDatabase.PExecute("UPDATE `characters` SET `at_login` = `at_login` | '8' WHERE `guid` = '%u'", target_guid.GetCounter());
    }

    return true;
}

/**
 * @brief Handler for HandleCharacterEraseCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleCharacterEraseCommand(char* args)
{
    char* nameStr = ExtractLiteralArg(&args);
    if (!nameStr)
    {
        return false;
    }

    Player* target;
    ObjectGuid target_guid;
    std::string target_name;
    if (!ExtractPlayerTarget(&nameStr, &target, &target_guid, &target_name))
    {
        return false;
    }

    uint32 account_id;

    if (target)
    {
        account_id = target->GetSession()->GetAccountId();
        target->GetSession()->KickPlayer();
    }
    else
    {
        account_id = sObjectMgr.GetPlayerAccountIdByGUID(target_guid);
    }

    std::string account_name;
    sAccountMgr.GetName(account_id, account_name);

    Player::DeleteFromDB(target_guid, account_id, true, true);
    PSendSysMessage(LANG_CHARACTER_DELETED, target_name.c_str(), target_guid.GetCounter(), account_name.c_str(), account_id);
    return true;
}

/**
 * @brief Handler for HandleCharacterLevelCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleCharacterLevelCommand(char* args)
{
    char* nameStr = ExtractOptNotLastArg(&args);

    int32 newlevel;
    bool nolevel = false;
    // exception opt second arg: .character level $name
    if (!ExtractInt32(&args, newlevel))
    {
        if (!nameStr)
        {
            nameStr = ExtractArg(&args);
            if (!nameStr)
            {
                return false;
            }

            nolevel = true;
        }
        else
        {
            return false;
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
    if (nolevel)
    {
        newlevel = oldlevel;
    }

    if (newlevel < 1)
    {
        return false; // invalid level
    }

    if (newlevel > STRONG_MAX_LEVEL)                        // hardcoded maximum level
    {
        newlevel = STRONG_MAX_LEVEL;
    }

    HandleCharacterLevel(target, target_guid, oldlevel, newlevel);

    if (!m_session || m_session->GetPlayer() != target)     // including player==NULL
    {
        std::string nameLink = playerLink(target_name);
        PSendSysMessage(LANG_YOU_CHANGE_LVL, nameLink.c_str(), newlevel);
    }

    return true;
}

// set temporary phase mask for player
bool ChatHandler::HandleModifyPhaseCommand(char* args)
{
    if (!*args)
    {
        return false;
    }

    uint32 phasemask = (uint32)atoi(args);

    Unit* target = getSelectedUnit();
    if (!target)
    {
        target = m_session->GetPlayer();
    }

    // check online security
    else if (target->GetTypeId() == TYPEID_PLAYER && HasLowerSecurity((Player*)target))
    {
        return false;
    }

    target->SetPhaseMask(phasemask, true);

    return true;
}

/**
 * @brief Handler for HandleCharacterRenameCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleCharacterRenameCommand(char* args)
{
    Player* target;
    ObjectGuid target_guid;
    std::string target_name;
    if (!ExtractPlayerTarget(&args, &target, &target_guid, &target_name))
    {
        return false;
    }

    if (target)
    {
        // check online security
        if (HasLowerSecurity(target))
        {
            return false;
        }

        PSendSysMessage(LANG_RENAME_PLAYER, GetNameLink(target).c_str());
        target->SetAtLoginFlag(AT_LOGIN_RENAME);
        CharacterDatabase.PExecute("UPDATE `characters` SET `at_login` = `at_login` | '1' WHERE `guid` = '%u'", target->GetGUIDLow());
    }
    else
    {
        // check offline security
        if (HasLowerSecurity(NULL, target_guid))
        {
            return false;
        }

        std::string oldNameLink = playerLink(target_name);

        PSendSysMessage(LANG_RENAME_PLAYER_GUID, oldNameLink.c_str(), target_guid.GetCounter());
        CharacterDatabase.PExecute("UPDATE `characters` SET `at_login` = `at_login` | '1' WHERE `guid` = '%u'", target_guid.GetCounter());
    }

    return true;
}

/**
 * @brief Handler for HandleCharacterReputationCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleCharacterReputationCommand(char* args)
{
    Player* target;
    if (!ExtractPlayerTarget(&args, &target))
    {
        return false;
    }

    LocaleConstant loc = GetSessionDbcLocale();

    FactionStateList const& targetFSL = target->GetReputationMgr().GetStateList();
    for (FactionStateList::const_iterator itr = targetFSL.begin(); itr != targetFSL.end(); ++itr)
    {
        FactionEntry const* factionEntry = sFactionStore.LookupEntry(itr->second.ID);

        ShowFactionListHelper(factionEntry, loc, &itr->second, target);
    }
    return true;
}

/**********************************************************************
    CommandTable : characterDeletedCommandTable
 ***********************************************************************/

/**
 * Collects all GUIDs (and related info) from deleted characters which are still in the database.
 *
 * @param foundList    a reference to an std::list which will be filled with info data
 * @param searchString the search string which either contains a player GUID (low part) or a part of the character-name
 * @return             returns false if there was a problem while selecting the characters (e.g. player name not normalizeable)
 */

bool ChatHandler::GetDeletedCharacterInfoList(DeletedInfoList& foundList, std::string searchString)
{
    QueryResult* resultChar;
    if (!searchString.empty())
    {
        // search by GUID
        if (isNumeric(searchString))
        {
            resultChar = CharacterDatabase.PQuery("SELECT `guid`, `deleteInfos_Name`, `deleteInfos_Account`, `deleteDate` FROM `characters` WHERE `deleteDate` IS NOT NULL AND `guid` = %u", uint32(atoi(searchString.c_str())));
        }
        // search by name
        else
        {
            if (!normalizePlayerName(searchString))
            {
                return false;
            }

            resultChar = CharacterDatabase.PQuery("SELECT `guid`, `deleteInfos_Name`, `deleteInfos_Account`, `deleteDate` FROM `characters` WHERE `deleteDate` IS NOT NULL AND `deleteInfos_Name` " _LIKE_ " " _CONCAT3_("'%%'", "'%s'", "'%%'"), searchString.c_str());
        }
    }
    else
    {
        resultChar = CharacterDatabase.Query("SELECT `guid`, `deleteInfos_Name`, `deleteInfos_Account`, `deleteDate` FROM `characters` WHERE `deleteDate` IS NOT NULL");
    }

    if (resultChar)
    {
        do
        {
            Field* fields = resultChar->Fetch();

            DeletedInfo info;

            info.lowguid = fields[0].GetUInt32();
            info.name = fields[1].GetCppString();
            info.accountId = fields[2].GetUInt32();

            // account name will be empty for nonexistent account
            sAccountMgr.GetName(info.accountId, info.accountName);

            info.deleteDate = time_t(fields[3].GetUInt64());

            foundList.push_back(info);
        }
        while (resultChar->NextRow());

        delete resultChar;
    }

    return true;
}

/**
 * Generate WHERE guids list by deleted info in way preventing return too long where list for existed query string length limit.
 *
 * @param itr          a reference to an deleted info list iterator, it updated in function for possible next function call if list to long
 * @param itr_end      a reference to an deleted info list iterator end()
 * @return             returns generated where list string in form: 'guid IN (gui1, guid2, ...)'
 */
std::string ChatHandler::GenerateDeletedCharacterGUIDsWhereStr(DeletedInfoList::const_iterator& itr, DeletedInfoList::const_iterator const& itr_end)
{
    std::ostringstream wherestr;
    wherestr << "guid IN ('";
    for (; itr != itr_end; ++itr)
    {
        wherestr << itr->lowguid;

        if (wherestr.str().size() > MAX_QUERY_LEN - 50)     // near to max query
        {
            ++itr;
            break;
        }

        DeletedInfoList::const_iterator itr2 = itr;
        if (++itr2 != itr_end)
        {
            wherestr << "','";
        }
    }
    wherestr << "')";
    return wherestr.str();
}

/**
 * Shows all deleted characters which matches the given search string, expected non empty list
 *
 * @see ChatHandler::HandleCharacterDeletedListCommand
 * @see ChatHandler::HandleCharacterDeletedRestoreCommand
 * @see ChatHandler::HandleCharacterDeletedDeleteCommand
 * @see ChatHandler::DeletedInfoList
 *
 * @param foundList contains a list with all found deleted characters
 */
void ChatHandler::HandleCharacterDeletedListHelper(DeletedInfoList const& foundList)
{
    if (!m_session)
    {
        SendSysMessage(LANG_CHARACTER_DELETED_LIST_BAR);
        SendSysMessage(LANG_CHARACTER_DELETED_LIST_HEADER);
        SendSysMessage(LANG_CHARACTER_DELETED_LIST_BAR);
    }

    for (DeletedInfoList::const_iterator itr = foundList.begin(); itr != foundList.end(); ++itr)
    {
        std::string dateStr = TimeToTimestampStr(itr->deleteDate);

        if (!m_session)
            PSendSysMessage(LANG_CHARACTER_DELETED_LIST_LINE_CONSOLE,
                            itr->lowguid, itr->name.c_str(), itr->accountName.empty() ? "<nonexistent>" : itr->accountName.c_str(),
                            itr->accountId, dateStr.c_str());
        else
            PSendSysMessage(LANG_CHARACTER_DELETED_LIST_LINE_CHAT,
                            itr->lowguid, itr->name.c_str(), itr->accountName.empty() ? "<nonexistent>" : itr->accountName.c_str(),
                            itr->accountId, dateStr.c_str());
    }

    if (!m_session)
    {
        SendSysMessage(LANG_CHARACTER_DELETED_LIST_BAR);
    }
}

/**
 * Restore a previously deleted character
 *
 * @see ChatHandler::HandleCharacterDeletedListHelper
 * @see ChatHandler::HandleCharacterDeletedRestoreCommand
 * @see ChatHandler::HandleCharacterDeletedDeleteCommand
 * @see ChatHandler::DeletedInfoList
 *
 * @param delInfo the informations about the character which will be restored
 */

void ChatHandler::HandleCharacterDeletedRestoreHelper(DeletedInfo const& delInfo)
{
    if (delInfo.accountName.empty())                    // account not exist
    {
        PSendSysMessage(LANG_CHARACTER_DELETED_SKIP_ACCOUNT, delInfo.name.c_str(), delInfo.lowguid, delInfo.accountId);
        return;
    }

    // check character count
    uint32 charcount = sAccountMgr.GetCharactersCount(delInfo.accountId);
    if (charcount >= 10)
    {
        PSendSysMessage(LANG_CHARACTER_DELETED_SKIP_FULL, delInfo.name.c_str(), delInfo.lowguid, delInfo.accountId);
        return;
    }

    if (sObjectMgr.GetPlayerGuidByName(delInfo.name))
    {
        PSendSysMessage(LANG_CHARACTER_DELETED_SKIP_NAME, delInfo.name.c_str(), delInfo.lowguid, delInfo.accountId);
        return;
    }

    CharacterDatabase.PExecute("UPDATE `characters` SET `name`='%s', `account`='%u', `deleteDate`=NULL, `deleteInfos_Name`=NULL, `deleteInfos_Account`=NULL WHERE `deleteDate` IS NOT NULL AND `guid` = %u",
                               delInfo.name.c_str(), delInfo.accountId, delInfo.lowguid);
}

/**
 * Handles the '.character deleted delete' command, which completely deletes all deleted characters which matches the given search string
 *
 * @see Player::GetDeletedCharacterGUIDs
 * @see Player::DeleteFromDB
 * @see ChatHandler::HandleCharacterDeletedListCommand
 * @see ChatHandler::HandleCharacterDeletedRestoreCommand
 *
 * @param args the search string which either contains a player GUID or a part of the character-name
 */
bool ChatHandler::HandleCharacterDeletedDeleteCommand(char* args)
{
    // It is required to submit at least one argument
    if (!*args)
    {
        return false;
    }

    DeletedInfoList foundList;
    if (!GetDeletedCharacterInfoList(foundList, args))
    {
        return false;
    }

    if (foundList.empty())
    {
        SendSysMessage(LANG_CHARACTER_DELETED_LIST_EMPTY);
        return false;
    }

    SendSysMessage(LANG_CHARACTER_DELETED_DELETE);
    HandleCharacterDeletedListHelper(foundList);

    // Call the appropriate function to delete them (current account for deleted characters is 0)
    for (DeletedInfoList::const_iterator itr = foundList.begin(); itr != foundList.end(); ++itr)
    {
        Player::DeleteFromDB(ObjectGuid(HIGHGUID_PLAYER, itr->lowguid), 0, false, true);
    }

    return true;
}

/**
 * Handles the '.character deleted list' command, which shows all deleted characters which matches the given search string
 *
 * @see ChatHandler::HandleCharacterDeletedListHelper
 * @see ChatHandler::HandleCharacterDeletedRestoreCommand
 * @see ChatHandler::HandleCharacterDeletedDeleteCommand
 * @see ChatHandler::DeletedInfoList
 *
 * @param args the search string which either contains a player GUID or a part of the character-name
 */
bool ChatHandler::HandleCharacterDeletedListCommand(char* args)
{
    DeletedInfoList foundList;
    if (!GetDeletedCharacterInfoList(foundList, args))
    {
        return false;
    }

    // if no characters have been found, output a warning
    if (foundList.empty())
    {
        SendSysMessage(LANG_CHARACTER_DELETED_LIST_EMPTY);
        return false;
    }

    HandleCharacterDeletedListHelper(foundList);
    return true;
}

/**
 * Handles the '.character deleted restore' command, which restores all deleted characters which matches the given search string
 *
 * The command automatically calls '.character deleted list' command with the search string to show all restored characters.
 *
 * @see ChatHandler::HandleCharacterDeletedRestoreHelper
 * @see ChatHandler::HandleCharacterDeletedListCommand
 * @see ChatHandler::HandleCharacterDeletedDeleteCommand
 *
 * @param args the search string which either contains a player GUID or a part of the character-name
 */
bool ChatHandler::HandleCharacterDeletedRestoreCommand(char* args)
{
    // It is required to submit at least one argument
    if (!*args)
    {
        return false;
    }

    std::string searchString;
    std::string newCharName;
    uint32 newAccount = 0;

    // GCC by some strange reason fail build code without temporary variable
    std::istringstream params(args);
    params >> searchString >> newCharName >> newAccount;

    DeletedInfoList foundList;
    if (!GetDeletedCharacterInfoList(foundList, searchString))
    {
        return false;
    }

    if (foundList.empty())
    {
        SendSysMessage(LANG_CHARACTER_DELETED_LIST_EMPTY);
        return false;
    }

    SendSysMessage(LANG_CHARACTER_DELETED_RESTORE);
    HandleCharacterDeletedListHelper(foundList);

    if (newCharName.empty())
    {
        // Drop nonexistent account cases
        for (DeletedInfoList::iterator itr = foundList.begin(); itr != foundList.end(); ++itr)
        {
            HandleCharacterDeletedRestoreHelper(*itr);
        }
    }
    else if (foundList.size() == 1 && normalizePlayerName(newCharName))
    {
        DeletedInfo delInfo = foundList.front();

        // update name
        delInfo.name = newCharName;

        // if new account provided update deleted info
        if (newAccount && newAccount != delInfo.accountId)
        {
            delInfo.accountId = newAccount;
            sAccountMgr.GetName(newAccount, delInfo.accountName);
        }

        HandleCharacterDeletedRestoreHelper(delInfo);
    }
    else
    {
        SendSysMessage(LANG_CHARACTER_DELETED_ERR_RENAME);
    }

    return true;
}

/**
 * Handles the '.character deleted old' command, which completely deletes all deleted characters deleted with some days ago
 *
 * @see Player::DeleteOldCharacters
 * @see Player::DeleteFromDB
 * @see ChatHandler::HandleCharacterDeletedDeleteCommand
 * @see ChatHandler::HandleCharacterDeletedListCommand
 * @see ChatHandler::HandleCharacterDeletedRestoreCommand
 *
 * @param args the search string which either contains a player GUID or a part of the character-name
 */
bool ChatHandler::HandleCharacterDeletedOldCommand(char* args)
{
    int32 keepDays = sWorld.getConfig(CONFIG_UINT32_CHARDELETE_KEEP_DAYS);

    if (!ExtractOptInt32(&args, keepDays, sWorld.getConfig(CONFIG_UINT32_CHARDELETE_KEEP_DAYS)))
    {
        return false;
    }

    if (keepDays < 0)
    {
        return false;
    }

    Player::DeleteOldCharacters((uint32)keepDays);
    return true;
}

/**********************************************************************
    CommandTable : commandTable
 ***********************************************************************/

void ChatHandler::HandleCharacterLevel(Player* player, ObjectGuid player_guid, uint32 oldlevel, uint32 newlevel)
{
    if (player)
    {
        player->SetLevel(newlevel);
        player->InitTalentForLevel();
        player->SetUInt32Value(PLAYER_XP, 0);

        if (needReportToTarget(player))
        {
            if (oldlevel == newlevel)
            {
                ChatHandler(player).PSendSysMessage(LANG_YOURS_LEVEL_PROGRESS_RESET, GetNameLink().c_str());
            }
            else if (oldlevel < newlevel)
            {
                ChatHandler(player).PSendSysMessage(LANG_YOURS_LEVEL_UP, GetNameLink().c_str(), newlevel);
            }
            else                                            // if (oldlevel > newlevel)
            {
                ChatHandler(player).PSendSysMessage(LANG_YOURS_LEVEL_DOWN, GetNameLink().c_str(), newlevel);
            }
        }
    }
    else
    {
        // update level and XP at level, all other will be updated at loading
        CharacterDatabase.PExecute("UPDATE `characters` SET `level` = '%u', `xp` = 0 WHERE `guid` = '%u'", newlevel, player_guid.GetCounter());
    }
}





































