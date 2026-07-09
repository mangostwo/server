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
 * @file PlayerStatsMods.cpp
 * @brief Cohesion split of PlayerCommands.cpp -- .modify GM commands: power
 *        and resource pools (hp/mana/rage/energy/runic), speeds (run/swim/fly/
 *        backwalk/all), scale, mount, gender, drunk, reputation, arena points
 *        and talent points. Same `ChatHandler` commands; no behaviour change.
 */

#include "Chat.h"
#include "ObjectMgr.h"
#include "World.h"
#include "AccountMgr.h"
#include "SQLStorages.h"

/**
 * @brief Handler for HandleModifyHPCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleModifyHPCommand(char* args)
{
    if (!*args)
    {
        return false;
    }

    int32 hp = atoi(args);
    int32 hpm = atoi(args);

    if (hp <= 0 || hpm <= 0 || hpm < hp)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    Player* chr = getSelectedPlayer();
    if (chr == NULL)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    // check online security
    if (HasLowerSecurity(chr))
    {
        return false;
    }

    PSendSysMessage(LANG_YOU_CHANGE_HP, GetNameLink(chr).c_str(), hp, hpm);
    if (needReportToTarget(chr))
    {
        ChatHandler(chr).PSendSysMessage(LANG_YOURS_HP_CHANGED, GetNameLink().c_str(), hp, hpm);
    }

    chr->SetMaxHealth(hpm);
    chr->SetHealth(hp);

    return true;
}

/**
 * @brief Handler for HandleModifyManaCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleModifyManaCommand(char* args)
{
    if (!*args)
    {
        return false;
    }

    int32 mana = atoi(args);
    int32 manam = atoi(args);

    if (mana <= 0 || manam <= 0 || manam < mana)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    Player* chr = getSelectedPlayer();
    if (chr == NULL)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    // check online security
    if (HasLowerSecurity(chr))
    {
        return false;
    }

    PSendSysMessage(LANG_YOU_CHANGE_MANA, GetNameLink(chr).c_str(), mana, manam);
    if (needReportToTarget(chr))
    {
        ChatHandler(chr).PSendSysMessage(LANG_YOURS_MANA_CHANGED, GetNameLink().c_str(), mana, manam);
    }

    chr->SetMaxPower(POWER_MANA, manam);
    chr->SetPower(POWER_MANA, mana);

    return true;
}

/**
 * @brief Handler for HandleModifyEnergyCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleModifyEnergyCommand(char* args)
{
    if (!*args)
    {
        return false;
    }

    int32 energy = atoi(args) * 10;
    int32 energym = atoi(args) * 10;

    if (energy <= 0 || energym <= 0 || energym < energy)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    Player* chr = getSelectedPlayer();
    if (!chr)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    // check online security
    if (HasLowerSecurity(chr))
    {
        return false;
    }

    PSendSysMessage(LANG_YOU_CHANGE_ENERGY, GetNameLink(chr).c_str(), energy / 10, energym / 10);
    if (needReportToTarget(chr))
    {
        ChatHandler(chr).PSendSysMessage(LANG_YOURS_ENERGY_CHANGED, GetNameLink().c_str(), energy / 10, energym / 10);
    }

    chr->SetMaxPower(POWER_ENERGY, energym);
    chr->SetPower(POWER_ENERGY, energy);

    DETAIL_LOG(GetMangosString(LANG_CURRENT_ENERGY), chr->GetMaxPower(POWER_ENERGY));

    return true;
}

/**
 * @brief Handler for HandleModifyRageCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleModifyRageCommand(char* args)
{
    if (!*args)
    {
        return false;
    }

    int32 rage = atoi(args) * 10;
    int32 ragem = atoi(args) * 10;

    if (rage <= 0 || ragem <= 0 || ragem < rage)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    Player* chr = getSelectedPlayer();
    if (chr == NULL)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    // check online security
    if (HasLowerSecurity(chr))
    {
        return false;
    }

    PSendSysMessage(LANG_YOU_CHANGE_RAGE, GetNameLink(chr).c_str(), rage / 10, ragem / 10);
    if (needReportToTarget(chr))
    {
        ChatHandler(chr).PSendSysMessage(LANG_YOURS_RAGE_CHANGED, GetNameLink().c_str(), rage / 10, ragem / 10);
    }

    chr->SetMaxPower(POWER_RAGE, ragem);
    chr->SetPower(POWER_RAGE, rage);

    return true;
}

// Edit Player Runic Power
bool ChatHandler::HandleModifyRunicPowerCommand(char* args)
{
    if (!*args)
    {
        return false;
    }

    int32 rune = atoi(args) * 10;
    int32 runem = atoi(args) * 10;

    if (rune <= 0 || runem <= 0 || runem < rune)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    Player* chr = getSelectedPlayer();
    if (chr == NULL)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    PSendSysMessage(LANG_YOU_CHANGE_RUNIC_POWER, GetNameLink(chr).c_str(), rune / 10, runem / 10);
    if (needReportToTarget(chr))
    {
        ChatHandler(chr).PSendSysMessage(LANG_YOURS_RUNIC_POWER_CHANGED, GetNameLink().c_str(), rune / 10, runem / 10);
    }

    chr->SetMaxPower(POWER_RUNIC_POWER, runem);
    chr->SetPower(POWER_RUNIC_POWER, rune);

    return true;
}

/**
 * @brief Handler for HandleModifyTalentCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleModifyTalentCommand(char* args)
{
    if (!*args)
    {
        return false;
    }

    int tp = atoi(args);
    if (tp < 0)
    {
        return false;
    }

    Unit* target = getSelectedUnit();
    if (!target)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    if (target->GetTypeId() == TYPEID_PLAYER)
    {
        // check online security
        if (HasLowerSecurity((Player*)target))
        {
            return false;
        }

        ((Player*)target)->SetFreeTalentPoints(tp);
        ((Player*)target)->SendTalentsInfoData(false);
        return true;
    }
    else if (((Creature*)target)->IsPet())
    {
        Unit* owner = target->GetOwner();
        if (owner && owner->GetTypeId() == TYPEID_PLAYER && ((Pet*)target)->IsPermanentPetFor((Player*)owner))
        {
            // check online security
            if (HasLowerSecurity((Player*)owner))
            {
                return false;
            }

            ((Pet*)target)->SetFreeTalentPoints(tp);
            ((Player*)owner)->SendTalentsInfoData(true);
            return true;
        }
    }

    SendSysMessage(LANG_NO_CHAR_SELECTED);
    SetSentErrorMessage(true);
    return false;
}

/**
 * @brief Handler for HandleModifyASpeedCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleModifyASpeedCommand(char* args)
{
    if (!*args)
    {
        return false;
    }

    float modSpeed = (float)atof(args);

    if (modSpeed > sWorld.getConfig(CONFIG_UINT32_GM_MAX_SPEED_FACTOR) || modSpeed < 0.1)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    Player* chr = getSelectedPlayer();
    if (chr == NULL)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    // check online security
    if (HasLowerSecurity(chr))
    {
        return false;
    }

    std::string chrNameLink = GetNameLink(chr);

    if (chr->IsTaxiFlying())
    {
        PSendSysMessage(LANG_CHAR_IN_FLIGHT, chrNameLink.c_str());
        SetSentErrorMessage(true);
        return false;
    }

    PSendSysMessage(LANG_YOU_CHANGE_ASPEED, modSpeed, chrNameLink.c_str());
    if (needReportToTarget(chr))
    {
        ChatHandler(chr).PSendSysMessage(LANG_YOURS_ASPEED_CHANGED, GetNameLink().c_str(), modSpeed);
    }

    chr->UpdateSpeed(MOVE_WALK, true, modSpeed);
    chr->UpdateSpeed(MOVE_RUN, true, modSpeed);
    chr->UpdateSpeed(MOVE_SWIM, true, modSpeed);
    // chr->UpdateSpeed(MOVE_TURN,   true, modSpeed);
    chr->UpdateSpeed(MOVE_FLIGHT, true, modSpeed);
    return true;
}

/**
 * @brief Handler for HandleModifySpeedCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleModifySpeedCommand(char* args)
{
    if (!*args)
    {
        return false;
    }

    float modSpeed = (float)atof(args);

    if (modSpeed > sWorld.getConfig(CONFIG_UINT32_GM_MAX_SPEED_FACTOR) || modSpeed < 0.1)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    Player* chr = getSelectedPlayer();
    if (chr == NULL)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    // check online security
    if (HasLowerSecurity(chr))
    {
        return false;
    }

    std::string chrNameLink = GetNameLink(chr);

    if (chr->IsTaxiFlying())
    {
        PSendSysMessage(LANG_CHAR_IN_FLIGHT, chrNameLink.c_str());
        SetSentErrorMessage(true);
        return false;
    }

    PSendSysMessage(LANG_YOU_CHANGE_SPEED, modSpeed, chrNameLink.c_str());
    if (needReportToTarget(chr))
    {
        ChatHandler(chr).PSendSysMessage(LANG_YOURS_SPEED_CHANGED, GetNameLink().c_str(), modSpeed);
    }

    chr->UpdateSpeed(MOVE_RUN, true, modSpeed);

    return true;
}

/**
 * @brief Handler for HandleModifySwimCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleModifySwimCommand(char* args)
{
    if (!*args)
    {
        return false;
    }

    float modSpeed = (float)atof(args);

    if (modSpeed > sWorld.getConfig(CONFIG_UINT32_GM_MAX_SPEED_FACTOR) || modSpeed < 0.01f)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    Player* chr = getSelectedPlayer();
    if (chr == NULL)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    // check online security
    if (HasLowerSecurity(chr))
    {
        return false;
    }

    std::string chrNameLink = GetNameLink(chr);

    if (chr->IsTaxiFlying())
    {
        PSendSysMessage(LANG_CHAR_IN_FLIGHT, chrNameLink.c_str());
        SetSentErrorMessage(true);
        return false;
    }

    PSendSysMessage(LANG_YOU_CHANGE_SWIM_SPEED, modSpeed, chrNameLink.c_str());
    if (needReportToTarget(chr))
    {
        ChatHandler(chr).PSendSysMessage(LANG_YOURS_SWIM_SPEED_CHANGED, GetNameLink().c_str(), modSpeed);
    }

    chr->UpdateSpeed(MOVE_SWIM, true, modSpeed);

    return true;
}

//Edit Player Fly command
bool ChatHandler::HandleModifyFlyCommand(char* args)
{
    if (!*args)
    {
        return false;
    }

    float modSpeed = (float)atof(args);

    if (modSpeed > 10.0f || modSpeed < 0.1f)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    Player* chr = getSelectedPlayer();
    if (chr == NULL)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    // check online security
    if (HasLowerSecurity(chr))
    {
        return false;
    }

    PSendSysMessage(LANG_YOU_CHANGE_FLY_SPEED, modSpeed, GetNameLink(chr).c_str());
    if (needReportToTarget(chr))
    {
        ChatHandler(chr).PSendSysMessage(LANG_YOURS_FLY_SPEED_CHANGED, GetNameLink().c_str(), modSpeed);
    }

    chr->UpdateSpeed(MOVE_FLIGHT, true, modSpeed);

    return true;
}

/**
 * @brief Handler for HandleModifyBWalkCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleModifyBWalkCommand(char* args)
{
    if (!*args)
    {
        return false;
    }

    float modSpeed = (float)atof(args);

    if (modSpeed > sWorld.getConfig(CONFIG_UINT32_GM_MAX_SPEED_FACTOR) || modSpeed < 0.1f)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    Player* chr = getSelectedPlayer();
    if (chr == NULL)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    // check online security
    if (HasLowerSecurity(chr))
    {
        return false;
    }

    std::string chrNameLink = GetNameLink(chr);

    if (chr->IsTaxiFlying())
    {
        PSendSysMessage(LANG_CHAR_IN_FLIGHT, chrNameLink.c_str());
        SetSentErrorMessage(true);
        return false;
    }

    PSendSysMessage(LANG_YOU_CHANGE_BACK_SPEED, modSpeed, chrNameLink.c_str());
    if (needReportToTarget(chr))
    {
        ChatHandler(chr).PSendSysMessage(LANG_YOURS_BACK_SPEED_CHANGED, GetNameLink().c_str(), modSpeed);
    }

    chr->UpdateSpeed(MOVE_RUN_BACK, true, modSpeed);

    return true;
}

/**
 * @brief Handler for HandleModifyScaleCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleModifyScaleCommand(char* args)
{
    if (!*args)
    {
        return false;
    }

    float Scale = (float)atof(args);
    if (Scale > 10.0f || Scale <= 0.0f)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    Unit* target = getSelectedUnit();
    if (target == NULL)
    {
        SendSysMessage(LANG_SELECT_CHAR_OR_CREATURE);
        SetSentErrorMessage(true);
        return false;
    }

    if (target->GetTypeId() == TYPEID_PLAYER)
    {
        // check online security
        if (HasLowerSecurity((Player*)target))
        {
            return false;
        }

        PSendSysMessage(LANG_YOU_CHANGE_SIZE, Scale, GetNameLink((Player*)target).c_str());
        if (needReportToTarget((Player*)target))
        {
            ChatHandler((Player*)target).PSendSysMessage(LANG_YOURS_SIZE_CHANGED, GetNameLink().c_str(), Scale);
        }
    }

    target->SetObjectScale(Scale);
    target->UpdateModelData();

    return true;
}

/**
 * @brief Handler for HandleModifyMountCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleModifyMountCommand(char* args)
{
    if (!*args)
    {
        return false;
    }

    uint16 mId = 1147;
    float speed = (float)15;
    uint32 num = atoi(args);
    switch (num)
    {
        case 1:
            mId = 14340;
            break;
        case 2:
            mId = 4806;
            break;
        case 3:
            mId = 6471;
            break;
        case 4:
            mId = 12345;
            break;
        case 5:
            mId = 6472;
            break;
        case 6:
            mId = 6473;
            break;
        case 7:
            mId = 10670;
            break;
        case 8:
            mId = 10719;
            break;
        case 9:
            mId = 10671;
            break;
        case 10:
            mId = 10672;
            break;
        case 11:
            mId = 10720;
            break;
        case 12:
            mId = 14349;
            break;
        case 13:
            mId = 11641;
            break;
        case 14:
            mId = 12244;
            break;
        case 15:
            mId = 12242;
            break;
        case 16:
            mId = 14578;
            break;
        case 17:
            mId = 14579;
            break;
        case 18:
            mId = 14349;
            break;
        case 19:
            mId = 12245;
            break;
        case 20:
            mId = 14335;
            break;
        case 21:
            mId = 207;
            break;
        case 22:
            mId = 2328;
            break;
        case 23:
            mId = 2327;
            break;
        case 24:
            mId = 2326;
            break;
        case 25:
            mId = 14573;
            break;
        case 26:
            mId = 14574;
            break;
        case 27:
            mId = 14575;
            break;
        case 28:
            mId = 604;
            break;
        case 29:
            mId = 1166;
            break;
        case 30:
            mId = 2402;
            break;
        case 31:
            mId = 2410;
            break;
        case 32:
            mId = 2409;
            break;
        case 33:
            mId = 2408;
            break;
        case 34:
            mId = 2405;
            break;
        case 35:
            mId = 14337;
            break;
        case 36:
            mId = 6569;
            break;
        case 37:
            mId = 10661;
            break;
        case 38:
            mId = 10666;
            break;
        case 39:
            mId = 9473;
            break;
        case 40:
            mId = 9476;
            break;
        case 41:
            mId = 9474;
            break;
        case 42:
            mId = 14374;
            break;
        case 43:
            mId = 14376;
            break;
        case 44:
            mId = 14377;
            break;
        case 45:
            mId = 2404;
            break;
        case 46:
            mId = 2784;
            break;
        case 47:
            mId = 2787;
            break;
        case 48:
            mId = 2785;
            break;
        case 49:
            mId = 2736;
            break;
        case 50:
            mId = 2786;
            break;
        case 51:
            mId = 14347;
            break;
        case 52:
            mId = 14346;
            break;
        case 53:
            mId = 14576;
            break;
        case 54:
            mId = 9695;
            break;
        case 55:
            mId = 9991;
            break;
        case 56:
            mId = 6448;
            break;
        case 57:
            mId = 6444;
            break;
        case 58:
            mId = 6080;
            break;
        case 59:
            mId = 6447;
            break;
        case 60:
            mId = 4805;
            break;
        case 61:
            mId = 9714;
            break;
        case 62:
            mId = 6448;
            break;
        case 63:
            mId = 6442;
            break;
        case 64:
            mId = 14632;
            break;
        case 65:
            mId = 14332;
            break;
        case 66:
            mId = 14331;
            break;
        case 67:
            mId = 8469;
            break;
        case 68:
            mId = 2830;
            break;
        case 69:
            mId = 2346;
            break;
        default:
            SendSysMessage(LANG_NO_MOUNT);
            SetSentErrorMessage(true);
            return false;
    }

    Player* chr = getSelectedPlayer();
    if (!chr)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    // check online security
    if (HasLowerSecurity(chr))
    {
        return false;
    }

    PSendSysMessage(LANG_YOU_GIVE_MOUNT, GetNameLink(chr).c_str());
    if (needReportToTarget(chr))
    {
        ChatHandler(chr).PSendSysMessage(LANG_MOUNT_GIVED, GetNameLink().c_str());
    }

    chr->SetUInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP);
    chr->Mount(mId);

    WorldPacket data(SMSG_FORCE_RUN_SPEED_CHANGE, (8 + 4 + 1 + 4));
    data << chr->GetPackGUID();
    data << (uint32)0;
    data << (uint8)0;                                       // new 2.1.0
    data << float(speed);
    chr->SendMessageToSet(&data, true);

    data.Initialize(SMSG_FORCE_SWIM_SPEED_CHANGE, (8 + 4 + 4));
    data << chr->GetPackGUID();
    data << (uint32)0;
    data << float(speed);
    chr->SendMessageToSet(&data, true);

    return true;
}

/**
 * @brief Handler for HandleModifyMoneyCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleModifyMoneyCommand(char* args)
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

    // check online security
    if (HasLowerSecurity(chr))
    {
        return false;
    }

    int32 addmoney = atoi(args);

    uint32 moneyuser = chr->GetMoney();

    if (addmoney < 0)
    {
        int32 newmoney = int32(moneyuser) + addmoney;

        DETAIL_LOG(GetMangosString(LANG_CURRENT_MONEY), moneyuser, addmoney, newmoney);
        if (newmoney <= 0)
        {
            PSendSysMessage(LANG_YOU_TAKE_ALL_MONEY, GetNameLink(chr).c_str());
            if (needReportToTarget(chr))
            {
                ChatHandler(chr).PSendSysMessage(LANG_YOURS_ALL_MONEY_GONE, GetNameLink().c_str());
            }

            chr->SetMoney(0);
        }
        else
        {
            if (newmoney > MAX_MONEY_AMOUNT)
            {
                newmoney = MAX_MONEY_AMOUNT;
            }

            PSendSysMessage(LANG_YOU_TAKE_MONEY, abs(addmoney), GetNameLink(chr).c_str());
            if (needReportToTarget(chr))
            {
                ChatHandler(chr).PSendSysMessage(LANG_YOURS_MONEY_TAKEN, GetNameLink().c_str(), abs(addmoney));
            }
            chr->SetMoney(newmoney);
        }
    }
    else
    {
        PSendSysMessage(LANG_YOU_GIVE_MONEY, addmoney, GetNameLink(chr).c_str());
        if (needReportToTarget(chr))
        {
            ChatHandler(chr).PSendSysMessage(LANG_YOURS_MONEY_GIVEN, GetNameLink().c_str(), addmoney);
        }

        if (addmoney >= MAX_MONEY_AMOUNT)
        {
            chr->SetMoney(MAX_MONEY_AMOUNT);
        }
        else
        {
            chr->ModifyMoney(addmoney);
        }
    }

    DETAIL_LOG(GetMangosString(LANG_NEW_MONEY), moneyuser, addmoney, chr->GetMoney());

    return true;
}

/**
 * @brief Handler for HandleModifyDrunkCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleModifyDrunkCommand(char* args)
{
    if (!*args)
    {
        return false;
    }

    uint8 drunkValue = (uint8)atoi(args);
    if (drunkValue > 100)
    {
        drunkValue = 100;
    }

    if (Player* target = getSelectedPlayer())
    {
        target->SetDrunkValue(drunkValue);
    }

    return true;
}

/**
 * @brief Handler for HandleModifyRepCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleModifyRepCommand(char* args)
{
    if (!*args)
    {
        return false;
    }

    Player* target = getSelectedPlayer();

    if (!target)
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    // check online security
    if (HasLowerSecurity(target))
    {
        return false;
    }

    uint32 factionId;
    if (!ExtractUint32KeyFromLink(&args, "Hfaction", factionId))
    {
        return false;
    }

    if (!factionId)
    {
        return false;
    }

    int32 amount = 0;
    if (!ExtractInt32(&args, amount))
    {
        char* rankTxt = ExtractLiteralArg(&args);
        if (!rankTxt)
        {
            return false;
        }

        std::string rankStr = rankTxt;
        std::wstring wrankStr;
        if (!Utf8toWStr(rankStr, wrankStr))
        {
            return false;
        }
        wstrToLower(wrankStr);

        int r = 0;
        amount = -42000;
        for (; r < MAX_REPUTATION_RANK; ++r)
        {
            std::string rank = GetMangosString(ReputationRankStrIndex[r]);
            if (rank.empty())
            {
                continue;
            }

            std::wstring wrank;
            if (!Utf8toWStr(rank, wrank))
            {
                continue;
            }

            wstrToLower(wrank);

            if (wrank.substr(0, wrankStr.size()) == wrankStr)
            {
                int32 delta;
                if (!ExtractOptInt32(&args, delta, 0) || (delta < 0) || (delta > ReputationMgr::PointsInRank[r] - 1))
                {
                    PSendSysMessage(LANG_COMMAND_FACTION_DELTA, (ReputationMgr::PointsInRank[r] - 1));
                    SetSentErrorMessage(true);
                    return false;
                }
                amount += delta;
                break;
            }
            amount += ReputationMgr::PointsInRank[r];
        }
        if (r >= MAX_REPUTATION_RANK)
        {
            PSendSysMessage(LANG_COMMAND_FACTION_INVPARAM, rankTxt);
            SetSentErrorMessage(true);
            return false;
        }
    }

    FactionEntry const* factionEntry = sFactionStore.LookupEntry(factionId);

    if (!factionEntry)
    {
        PSendSysMessage(LANG_COMMAND_FACTION_UNKNOWN, factionId);
        SetSentErrorMessage(true);
        return false;
    }

    if (factionEntry->ReputationIndex < 0)
    {
        PSendSysMessage(LANG_COMMAND_FACTION_NOREP_ERROR, factionEntry->Name_lang[GetSessionDbcLocale()], factionId);
        SetSentErrorMessage(true);
        return false;
    }

    target->GetReputationMgr().SetReputation(factionEntry, amount);
    PSendSysMessage(LANG_COMMAND_MODIFY_REP, factionEntry->Name_lang[GetSessionDbcLocale()], factionId,
                    GetNameLink(target).c_str(), target->GetReputationMgr().GetReputation(factionEntry));
    return true;
}

/**
 * @brief Handler for HandleModifyGenderCommand command.
 *
 * @param args Command arguments.
 * @returns True if the command executed successfully, false otherwise.
 */
bool ChatHandler::HandleModifyGenderCommand(char* args)
{
    if (!*args)
    {
        return false;
    }

    Player* player = getSelectedPlayer();

    if (!player)
    {
        PSendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    PlayerInfo const* info = sObjectMgr.GetPlayerInfo(player->getRace(), player->getClass());
    if (!info)
    {
        return false;
    }

    char* gender_str = args;
    int gender_len = strlen(gender_str);

    Gender gender;

    if (!strncmp(gender_str, "male", gender_len))           // MALE
    {
        if (player->getGender() == GENDER_MALE)
        {
            return true;
        }

        gender = GENDER_MALE;
    }
    else if (!strncmp(gender_str, "female", gender_len))    // FEMALE
    {
        if (player->getGender() == GENDER_FEMALE)
        {
            return true;
        }

        gender = GENDER_FEMALE;
    }
    else
    {
        SendSysMessage(LANG_MUST_MALE_OR_FEMALE);
        SetSentErrorMessage(true);
        return false;
    }

    // Set gender
    player->SetByteValue(UNIT_FIELD_BYTES_0, 2, gender);
    player->SetUInt16Value(PLAYER_BYTES_3, 0, uint16(gender) | (player->GetDrunkValue() & 0xFFFE));

    // Change display ID
    player->InitDisplayIds();

    char const* gender_full = gender ? "female" : "male";

    PSendSysMessage(LANG_YOU_CHANGE_GENDER, player->GetName(), gender_full);

    if (needReportToTarget(player))
    {
        ChatHandler(player).PSendSysMessage(LANG_YOUR_GENDER_CHANGED, gender_full, GetNameLink().c_str());
    }

    return true;
}

bool ChatHandler::HandleModifyArenaCommand(char* args)
{
    if (!*args)
    {
        return false;
    }

    Player* target = getSelectedPlayer();
    if (!target)
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    int32 amount = (int32)atoi(args);

    target->ModifyArenaPoints(amount);

    PSendSysMessage(LANG_COMMAND_MODIFY_ARENA, GetNameLink(target).c_str(), target->GetArenaPoints());

    return true;
}
