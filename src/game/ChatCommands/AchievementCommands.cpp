/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2023 MaNGOS <https://getmangos.eu>
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

#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "ObjectMgr.h"
#include "AccountMgr.h"
#include "PlayerDump.h"
#include "SpellMgr.h"
#include "Player.h"
#include "Opcodes.h"
#include "GameObject.h"
#include "Chat.h"
#include "Log.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "ObjectAccessor.h"
#include "MapManager.h"
#include "MassMailMgr.h"
#include "ScriptMgr.h"
#include "Language.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "Weather.h"
#include "PointMovementGenerator.h"
#include "PathFinder.h"
#include "TargetedMovementGenerator.h"
#include "SkillDiscovery.h"
#include "SkillExtraItems.h"
#include "SystemConfig.h"
#include "Config/Config.h"
#include "Mail.h"
#include "Util.h"
#include "ItemEnchantmentMgr.h"
#include "BattleGround/BattleGroundMgr.h"
#include "MapPersistentStateMgr.h"
#include "InstanceData.h"
#include "CreatureEventAIMgr.h"
#include "DBCEnums.h"
#include "AuctionHouseBot/AuctionHouseBot.h"
#include "SQLStorages.h"
#include "DisableMgr.h"

void ChatHandler::ShowAchievementCriteriaListHelper(AchievementCriteriaEntry const* criEntry, AchievementEntry const* achEntry, LocaleConstant loc, Player* target /*= NULL*/)
{
    std::ostringstream ss;
    if (m_session)
    {
        ss << criEntry->ID << " - |cffffffff|Hachievement_criteria:" << criEntry->ID << "|h[" << criEntry->name[loc] << " " << localeNames[loc] << "]|h|r";
    }
    else
    {
        ss << criEntry->ID << " - " << criEntry->name[loc] << " " << localeNames[loc];
    }

    if (target)
    {
        ss << " = " << target->GetAchievementMgr().GetCriteriaProgressCounter(criEntry);
    }

    if (achEntry->flags & ACHIEVEMENT_FLAG_COUNTER)
    {
        ss << GetMangosString(LANG_COUNTER);
    }
    else
    {
        ss << " [" << AchievementMgr::GetCriteriaProgressMaxCounter(criEntry, achEntry) << "]";

        if (target && target->GetAchievementMgr().IsCompletedCriteria(criEntry, achEntry))
        {
            ss << GetMangosString(LANG_COMPLETE);
        }
    }

    SendSysMessage(ss.str().c_str());
}

bool ChatHandler::HandleAchievementCommand(char* args)
{
    char* nameStr = ExtractOptNotLastArg(&args);

    Player* target = NULL;

    if (nameStr)
    {
        if (!ExtractPlayerTarget(&nameStr, &target))
        {
            return false;
        }
    }
    else
    {
        target = getSelectedPlayer();
    }

    uint32 achId;
    if (!ExtractUint32KeyFromLink(&args, "Hachievement", achId))
    {
        return false;
    }

    AchievementEntry const* achEntry = sAchievementStore.LookupEntry(achId);
    if (!achEntry)
    {
        PSendSysMessage(LANG_ACHIEVEMENT_NOT_EXIST, achId);
        SetSentErrorMessage(true);
        return false;
    }

    LocaleConstant loc = GetSessionDbcLocale();

    CompletedAchievementData const* completed = target ? target->GetAchievementMgr().GetCompleteData(achId) : NULL;

    ShowAchievementListHelper(achEntry, loc, completed ? &completed->date : NULL, target);

    if (AchievementCriteriaEntryList const* criteriaList = sAchievementMgr.GetAchievementCriteriaByAchievement(achEntry->ID))
    {
        SendSysMessage(LANG_COMMAND_ACHIEVEMENT_CRITERIA);
        for (AchievementCriteriaEntryList::const_iterator itr = criteriaList->begin(); itr != criteriaList->end(); ++itr)
        {
            ShowAchievementCriteriaListHelper(*itr, achEntry, loc, target);
        }
    }

    return true;
}

bool ChatHandler::HandleAchievementAddCommand(char* args)
{
    char* nameStr = ExtractOptNotLastArg(&args);

    Player* target;
    if (!ExtractPlayerTarget(&nameStr, &target))
    {
        return false;
    }

    uint32 achId;
    if (!ExtractUint32KeyFromLink(&args, "Hachievement", achId))
    {
        return false;
    }

    AchievementEntry const* achEntry = sAchievementStore.LookupEntry(achId);
    if (!achEntry || achEntry->flags & ACHIEVEMENT_FLAG_COUNTER)
    {
        PSendSysMessage(LANG_ACHIEVEMENT_NOT_EXIST, achId);
        SetSentErrorMessage(true);
        return false;
    }

    AchievementMgr& mgr = target->GetAchievementMgr();

    if (AchievementCriteriaEntryList const* criteriaList = sAchievementMgr.GetAchievementCriteriaByAchievement(achEntry->ID))
    {
        for (AchievementCriteriaEntryList::const_iterator itr = criteriaList->begin(); itr != criteriaList->end(); ++itr)
        {
            if (mgr.IsCompletedCriteria(*itr, achEntry))
            {
                continue;
            }

            uint32 maxValue = AchievementMgr::GetCriteriaProgressMaxCounter(*itr, achEntry);
            if (maxValue == std::numeric_limits<uint32>::max())
                maxValue = 1;                               // Exception for counter like achievements, set them only to 1
            mgr.SetCriteriaProgress(*itr, achEntry, maxValue, AchievementMgr::PROGRESS_SET);
        }
    }

    LocaleConstant loc = GetSessionDbcLocale();
    CompletedAchievementData const* completed = target ? target->GetAchievementMgr().GetCompleteData(achId) : NULL;
    ShowAchievementListHelper(achEntry, loc, completed ? &completed->date : NULL, target);
    return true;
}

bool ChatHandler::HandleAchievementRemoveCommand(char* args)
{
    char* nameStr = ExtractOptNotLastArg(&args);

    Player* target;
    if (!ExtractPlayerTarget(&nameStr, &target))
    {
        return false;
    }

    uint32 achId;
    if (!ExtractUint32KeyFromLink(&args, "Hachievement", achId))
    {
        return false;
    }

    AchievementEntry const* achEntry = sAchievementStore.LookupEntry(achId);
    if (!achEntry)
    {
        PSendSysMessage(LANG_ACHIEVEMENT_NOT_EXIST, achId);
        SetSentErrorMessage(true);
        return false;
    }

    AchievementMgr& mgr = target->GetAchievementMgr();

    if (AchievementCriteriaEntryList const* criteriaList = sAchievementMgr.GetAchievementCriteriaByAchievement(achEntry->ID))
        for (AchievementCriteriaEntryList::const_iterator itr = criteriaList->begin(); itr != criteriaList->end(); ++itr)
        {
            mgr.SetCriteriaProgress(*itr, achEntry, 0, AchievementMgr::PROGRESS_SET);
        }

    LocaleConstant loc = GetSessionDbcLocale();
    CompletedAchievementData const* completed = target ? target->GetAchievementMgr().GetCompleteData(achId) : NULL;
    ShowAchievementListHelper(achEntry, loc, completed ? &completed->date : NULL, target);
    return true;
}

bool ChatHandler::HandleAchievementCriteriaAddCommand(char* args)
{
    Player* target;
    uint32 criId;

    if (!ExtractUint32KeyFromLink(&args, "Hachievement_criteria", criId))
    {
        // maybe player first
        char* nameStr = ExtractArg(&args);
        if (!ExtractPlayerTarget(&nameStr, &target))
        {
            return false;
        }

        if (!ExtractUint32KeyFromLink(&args, "Hachievement_criteria", criId))
        {
            return false;
        }
    }
    else
    {
        target = getSelectedPlayer();
    }

    AchievementCriteriaEntry const* criEntry = sAchievementCriteriaStore.LookupEntry(criId);
    if (!criEntry)
    {
        PSendSysMessage(LANG_ACHIEVEMENT_CRITERIA_NOT_EXIST, criId);
        SetSentErrorMessage(true);
        return false;
    }

    AchievementEntry const* achEntry = sAchievementStore.LookupEntry(criEntry->referredAchievement);
    if (!achEntry)
    {
        return false;
    }

    LocaleConstant loc = GetSessionDbcLocale();

    uint32 maxValue = AchievementMgr::GetCriteriaProgressMaxCounter(criEntry, achEntry);
    if (maxValue == std::numeric_limits<uint32>::max())
        maxValue = 1;                                       // Exception for counter like achievements, set them only to 1

    AchievementMgr& mgr = target->GetAchievementMgr();

    // nothing do if completed
    if (mgr.IsCompletedCriteria(criEntry, achEntry))
    {
        ShowAchievementCriteriaListHelper(criEntry, achEntry, loc, target);
        return true;
    }

    uint32 progress = mgr.GetCriteriaProgressCounter(criEntry);

    uint32 val;
    if (!ExtractOptUInt32(&args, val, maxValue ? maxValue : 1))
    {
        return false;
    }

    uint32 new_val;

    if (maxValue)
    {
        new_val = progress < maxValue && maxValue - progress > val ? progress + val : maxValue;
    }
    else
    {
        uint32 max_int = std::numeric_limits<uint32>::max();
        new_val = progress < max_int && max_int - progress > val ? progress + val : max_int;
    }

    mgr.SetCriteriaProgress(criEntry, achEntry, new_val, AchievementMgr::PROGRESS_SET);

    ShowAchievementCriteriaListHelper(criEntry, achEntry, loc, target);
    return true;
}

bool ChatHandler::HandleAchievementCriteriaRemoveCommand(char* args)
{
    Player* target;
    uint32 criId;

    if (!ExtractUint32KeyFromLink(&args, "Hachievement_criteria", criId))
    {
        // maybe player first
        char* nameStr = ExtractArg(&args);
        if (!ExtractPlayerTarget(&nameStr, &target))
        {
            return false;
        }

        if (!ExtractUint32KeyFromLink(&args, "Hachievement_criteria", criId))
        {
            return false;
        }
    }
    else
    {
        target = getSelectedPlayer();
    }

    AchievementCriteriaEntry const* criEntry = sAchievementCriteriaStore.LookupEntry(criId);
    if (!criEntry)
    {
        PSendSysMessage(LANG_ACHIEVEMENT_CRITERIA_NOT_EXIST, criId);
        SetSentErrorMessage(true);
        return false;
    }

    AchievementEntry const* achEntry = sAchievementStore.LookupEntry(criEntry->referredAchievement);
    if (!achEntry)
    {
        return false;
    }

    LocaleConstant loc = GetSessionDbcLocale();

    uint32 maxValue = AchievementMgr::GetCriteriaProgressMaxCounter(criEntry, achEntry);
    if (maxValue == std::numeric_limits<uint32>::max())
        maxValue = 1;                                       // Exception for counter like achievements, set them only to 1

    AchievementMgr& mgr = target->GetAchievementMgr();

    uint32 progress = mgr.GetCriteriaProgressCounter(criEntry);

    // nothing do if not started
    if (progress == 0)
    {
        ShowAchievementCriteriaListHelper(criEntry, achEntry, loc, target);
        return true;
    }

    uint32 change;
    if (!ExtractOptUInt32(&args, change, maxValue ? maxValue : 1))
    {
        return false;
    }

    uint32 newval = change < progress ? progress - change : 0;

    mgr.SetCriteriaProgress(criEntry, achEntry, newval, AchievementMgr::PROGRESS_SET);

    ShowAchievementCriteriaListHelper(criEntry, achEntry, loc, target);
    return true;
}

bool ChatHandler::HandleResetAchievementsCommand(char* args)
{
    Player* target;
    ObjectGuid target_guid;
    if (!ExtractPlayerTarget(&args, &target, &target_guid))
    {
        return false;
    }

    if (target)
    {
        target->GetAchievementMgr().Reset();
    }
    else
    {
        AchievementMgr::DeleteFromDB(target_guid);
    }

    return true;
}
