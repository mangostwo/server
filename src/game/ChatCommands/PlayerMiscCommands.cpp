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

#include "Chat.h"
#include "Language.h"
#include "World.h"
#include "ObjectMgr.h"

 /**********************************************************************
     CommandTable : commandTable
 /***********************************************************************/

bool ChatHandler::HandleBankCommand(char* /*args*/)
{
    m_session->SendShowBank(m_session->GetPlayer()->GetObjectGuid());

    return true;
}

bool ChatHandler::HandleStableCommand(char* /*args*/)
{
    m_session->SendStablePet(m_session->GetPlayer()->GetObjectGuid());

    return true;
}

bool ChatHandler::HandleShowGearScoreCommand(char* args)
{
    Player* player = getSelectedPlayer();

    if (!player)
    {
        PSendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    uint32 withBags, withBank;
    if (!ExtractOptUInt32(&args, withBags, 1))
    {
        return false;
    }

    if (!ExtractOptUInt32(&args, withBank, 0))
    {
        return false;
    }

    // always recalculate gear score for display
    player->ResetCachedGearScore();

    uint32 gearScore = player->GetEquipGearScore(withBags != 0, withBank != 0);

    PSendSysMessage(LANG_GEARSCORE, GetNameLink(player).c_str(), gearScore);

    return true;
}

/**********************************************************************
    CommandTable : resetCommandTable
/***********************************************************************/

bool ChatHandler::HandleResetSpecsCommand(char* args)
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
        target->resetTalents(true, true);
        target->SendTalentsInfoData(false);

        ChatHandler(target).SendSysMessage(LANG_RESET_TALENTS);
        if (!m_session || m_session->GetPlayer() != target)
        {
            PSendSysMessage(LANG_RESET_TALENTS_ONLINE, GetNameLink(target).c_str());
        }

        Pet* pet = target->GetPet();
        Pet::resetTalentsForAllPetsOf(target, pet);
        if (pet)
        {
            target->SendTalentsInfoData(true);
        }
        return true;
    }
    else if (target_guid)
    {
        uint32 at_flags = AT_LOGIN_RESET_TALENTS | AT_LOGIN_RESET_PET_TALENTS;
        CharacterDatabase.PExecute("UPDATE `characters` SET `at_login` = `at_login` | '%u' WHERE `guid` = '%u'", at_flags, target_guid.GetCounter());
        std::string nameLink = playerLink(target_name);
        PSendSysMessage(LANG_RESET_TALENTS_OFFLINE, nameLink.c_str());
        return true;
    }

    SendSysMessage(LANG_NO_CHAR_SELECTED);
    SetSentErrorMessage(true);
    return false;
}

static bool HandleResetStatsOrLevelHelper(Player* player)
{
    ChrClassesEntry const* cEntry = sChrClassesStore.LookupEntry(player->getClass());
    if (!cEntry)
    {
        sLog.outError("Class %u not found in DBC (Wrong DBC files?)", player->getClass());
        return false;
    }

    uint8 powertype = cEntry->powerType;

    // reset m_form if no aura
    if (!player->HasAuraType(SPELL_AURA_MOD_SHAPESHIFT))
    {
        player->SetShapeshiftForm(FORM_NONE);
    }

    player->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, DEFAULT_WORLD_OBJECT_SIZE);
    player->SetFloatValue(UNIT_FIELD_COMBATREACH, 1.5f);

    player->setFactionForRace(player->getRace());

    player->SetByteValue(UNIT_FIELD_BYTES_0, 3, powertype);

    // reset only if player not in some form;
    if (player->GetShapeshiftForm() == FORM_NONE)
    {
        player->InitDisplayIds();
    }

    player->SetByteValue(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_PVP);

    player->SetUInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);

    //-1 is default value
    player->SetInt32Value(PLAYER_FIELD_WATCHED_FACTION_INDEX, -1);

    // player->SetUInt32Value(PLAYER_FIELD_BYTES, 0xEEE00000 );
    return true;
}

bool ChatHandler::HandleResetLevelCommand(char* args)
{
    Player* target;
    if (!ExtractPlayerTarget(&args, &target))
    {
        return false;
    }

    if (!HandleResetStatsOrLevelHelper(target))
    {
        return false;
    }

    // set starting level
    uint32 start_level = target->getClass() != CLASS_DEATH_KNIGHT
                         ? sWorld.getConfig(CONFIG_UINT32_START_PLAYER_LEVEL)
                         : sWorld.getConfig(CONFIG_UINT32_START_HEROIC_PLAYER_LEVEL);

    target->_ApplyAllLevelScaleItemMods(false);

    target->SetLevel(start_level);
    target->InitRunes();
    target->InitStatsForLevel(true);
    target->InitTaxiNodesForLevel();
    target->InitGlyphsForLevel();
    target->InitTalentForLevel();
    target->SetUInt32Value(PLAYER_XP, 0);

    target->_ApplyAllLevelScaleItemMods(true);

    // reset level for pet
    if (Pet* pet = target->GetPet())
    {
        pet->SynchronizeLevelWithOwner();
    }

    return true;
}

bool ChatHandler::HandleResetStatsCommand(char* args)
{
    Player* target;
    if (!ExtractPlayerTarget(&args, &target))
    {
        return false;
    }

    if (!HandleResetStatsOrLevelHelper(target))
    {
        return false;
    }

    target->InitRunes();
    target->InitStatsForLevel(true);
    target->InitTaxiNodesForLevel();
    target->InitGlyphsForLevel();
    target->InitTalentForLevel();

    return true;
}

bool ChatHandler::HandleResetSpellsCommand(char* args)
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
        target->resetSpells();

        ChatHandler(target).SendSysMessage(LANG_RESET_SPELLS);
        if (!m_session || m_session->GetPlayer() != target)
        {
            PSendSysMessage(LANG_RESET_SPELLS_ONLINE, GetNameLink(target).c_str());
        }
    }
    else
    {
        CharacterDatabase.PExecute("UPDATE `characters` SET `at_login` = `at_login` | '%u' WHERE `guid` = '%u'", uint32(AT_LOGIN_RESET_SPELLS), target_guid.GetCounter());
        PSendSysMessage(LANG_RESET_SPELLS_OFFLINE, target_name.c_str());
    }

    return true;
}

bool ChatHandler::HandleResetTalentsCommand(char* args)
{
    Player* target;
    std::string target_name;
    if (!ExtractPlayerTarget(&args, &target, NULL, &target_name))
    {
        // Try reset talents as Hunter Pet
        Creature* creature = getSelectedCreature();
        if (!*args && creature && creature->IsPet())
        {
            Unit* owner = creature->GetOwner();
            if (owner && owner->GetTypeId() == TYPEID_PLAYER && ((Pet*)creature)->IsPermanentPetFor((Player*)owner))
            {
                ((Pet*)creature)->resetTalents(true);
                ((Player*)owner)->SendTalentsInfoData(true);

                ChatHandler((Player*)owner).SendSysMessage(LANG_RESET_PET_TALENTS);
                if (!m_session || m_session->GetPlayer() != ((Player*)owner))
                {
                    PSendSysMessage(LANG_RESET_PET_TALENTS_ONLINE, GetNameLink((Player*)owner).c_str());
                }
            }
            return true;
        }

        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    if (target)
    {
        target->resetTalents(true);
        target->SendTalentsInfoData(false);
        ChatHandler(target).SendSysMessage(LANG_RESET_TALENTS);
        if (!m_session || m_session->GetPlayer() != target)
        {
            PSendSysMessage(LANG_RESET_TALENTS_ONLINE, GetNameLink(target).c_str());
        }

        Pet* pet = target->GetPet();
        Pet::resetTalentsForAllPetsOf(target, pet);
        if (pet)
        {
            target->SendTalentsInfoData(true);
        }
        return true;
    }

    SendSysMessage(LANG_NO_CHAR_SELECTED);
    SetSentErrorMessage(true);
    return false;
}

bool ChatHandler::HandleResetAllCommand(char* args)
{
    if (!*args)
    {
        return false;
    }

    std::string casename = args;

    AtLoginFlags atLogin;

    // Command specially created as single command to prevent using short case names
    if (casename == "spells")
    {
        atLogin = AT_LOGIN_RESET_SPELLS;
        sWorld.SendWorldText(LANG_RESETALL_SPELLS);
        if (!m_session)
        {
            SendSysMessage(LANG_RESETALL_SPELLS);
        }
    }
    else if (casename == "talents")
    {
        atLogin = AtLoginFlags(AT_LOGIN_RESET_TALENTS | AT_LOGIN_RESET_PET_TALENTS);
        sWorld.SendWorldText(LANG_RESETALL_TALENTS);
        if (!m_session)
        {
            SendSysMessage(LANG_RESETALL_TALENTS);
        }
    }
    else
    {
        PSendSysMessage(LANG_RESETALL_UNKNOWN_CASE, args);
        SetSentErrorMessage(true);
        return false;
    }

    CharacterDatabase.PExecute("UPDATE `characters` SET `at_login` = `at_login` | '%u' WHERE (`at_login` & '%u') = '0'", atLogin, atLogin);
    sObjectAccessor.DoForAllPlayers([atLogin](Player* plr)
    {
        plr->SetAtLoginFlag(atLogin);
    });
    return true;
}
