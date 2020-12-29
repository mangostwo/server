/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2021 MaNGOS <https://getmangos.eu>
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
#include "GMTicketMgr.h"
#include "Mail.h"


// show ticket (helper)
void ChatHandler::ShowTicket(GMTicket const* ticket)
{
    std::string lastupdated = TimeToTimestampStr(ticket->GetLastUpdate());

    std::string name;
    if (!sObjectMgr.GetPlayerNameByGUID(ticket->GetPlayerGuid(), name))
    {
        name = GetMangosString(LANG_UNKNOWN);
    }

    std::string nameLink = playerLink(name);

    char const* response = ticket->GetResponse();

    PSendSysMessage(LANG_COMMAND_TICKETVIEW, nameLink.c_str(), lastupdated.c_str(), ticket->GetText());
    if (strlen(response))
    {
        PSendSysMessage(LANG_COMMAND_TICKETRESPONSE, ticket->GetResponse());
    }
}

// ticket commands
bool ChatHandler::HandleTicketCommand(char* args)
{
    char* px = ExtractLiteralArg(&args);

    // ticket<end>
    if (!px)
    {
        size_t count = sTicketMgr.GetTicketCount();

        if (m_session)
        {
            bool accept = m_session->GetPlayer()->isAcceptTickets();

            PSendSysMessage(LANG_COMMAND_TICKETCOUNT, count, GetOnOffStr(accept));
        }
        else
        {
            PSendSysMessage(LANG_COMMAND_TICKETCOUNT_CONSOLE, count);
        }

        return true;
    }

    // ticket on
    if (strncmp(px, "on", 3) == 0)
    {
        if (!m_session)
        {
            SendSysMessage(LANG_PLAYER_NOT_FOUND);
            SetSentErrorMessage(true);
            return false;
        }

        m_session->GetPlayer()->SetAcceptTicket(true);
        SendSysMessage(LANG_COMMAND_TICKETON);
        return true;
    }

    // ticket off
    if (strncmp(px, "off", 4) == 0)
    {
        if (!m_session)
        {
            SendSysMessage(LANG_PLAYER_NOT_FOUND);
            SetSentErrorMessage(true);
            return false;
        }

        m_session->GetPlayer()->SetAcceptTicket(false);
        SendSysMessage(LANG_COMMAND_TICKETOFF);
        return true;
    }

    // ticket respond
    if (strncmp(px, "respond", 8) == 0)
    {
        GMTicket* ticket = NULL;

        // ticket respond #num
        uint32 num;
        if (ExtractUInt32(&args, num))
        {
            if (num == 0)
            {
                return false;
            }

            // mgr numbering tickets start from 0
            ticket = sTicketMgr.GetGMTicketByOrderPos(num - 1);

            if (!ticket)
            {
                PSendSysMessage(LANG_COMMAND_TICKETNOTEXIST, num);
                SetSentErrorMessage(true);
                return false;
            }
        }
        else
        {
            ObjectGuid target_guid;
            std::string target_name;
            if (!ExtractPlayerTarget(&args, NULL, &target_guid, &target_name))
            {
                return false;
            }

            // ticket respond $char_name
            ticket = sTicketMgr.GetGMTicket(target_guid);

            if (!ticket)
            {
                PSendSysMessage(LANG_COMMAND_TICKETNOTEXIST_NAME, target_name.c_str());
                SetSentErrorMessage(true);
                return false;
            }
        }

        // no response text?
        if (!*args)
        {
            return false;
        }

        ticket->SetResponseText(args);

        if (Player* pl = sObjectMgr.GetPlayer(ticket->GetPlayerGuid()))
        {
            pl->GetSession()->SendGMResponse(ticket);
        }

        return true;
    }

    // ticket #num
    uint32 num;
    if (ExtractUInt32(&px, num))
    {
        if (num == 0)
        {
            return false;
        }

        // mgr numbering tickets start from 0
        GMTicket* ticket = sTicketMgr.GetGMTicketByOrderPos(num - 1);
        if (!ticket)
        {
            PSendSysMessage(LANG_COMMAND_TICKETNOTEXIST, num);
            SetSentErrorMessage(true);
            return false;
        }

        ShowTicket(ticket);
        return true;
    }

    ObjectGuid target_guid;
    std::string target_name;
    if (!ExtractPlayerTarget(&px, NULL, &target_guid, &target_name))
    {
        return false;
    }

    // ticket $char_name
    GMTicket* ticket = sTicketMgr.GetGMTicket(target_guid);
    if (!ticket)
    {
        PSendSysMessage(LANG_COMMAND_TICKETNOTEXIST_NAME, target_name.c_str());
        SetSentErrorMessage(true);
        return false;
    }

    ShowTicket(ticket);

    return true;
}

// dell all tickets
bool ChatHandler::HandleDelTicketCommand(char* args)
{
    char* px = ExtractLiteralArg(&args);
    if (!px)
    {
        return false;
    }

    // delticket all
    if (strncmp(px, "all", 4) == 0)
    {
        sTicketMgr.DeleteAll();
        SendSysMessage(LANG_COMMAND_ALLTICKETDELETED);
        return true;
    }

    uint32 num;

    // delticket #num
    if (ExtractUInt32(&px, num))
    {
        if (num == 0)
        {
            return false;
        }

        // mgr numbering tickets start from 0
        GMTicket* ticket = sTicketMgr.GetGMTicketByOrderPos(num - 1);

        if (!ticket)
        {
            PSendSysMessage(LANG_COMMAND_TICKETNOTEXIST, num);
            SetSentErrorMessage(true);
            return false;
        }

        ObjectGuid guid = ticket->GetPlayerGuid();

        sTicketMgr.Delete(guid);

        // notify player
        if (Player* pl = sObjectMgr.GetPlayer(guid))
        {
            pl->GetSession()->SendGMTicketGetTicket(0x0A);
            PSendSysMessage(LANG_COMMAND_TICKETPLAYERDEL, GetNameLink(pl).c_str());
        }
        else
        {
            PSendSysMessage(LANG_COMMAND_TICKETDEL);
        }

        return true;
    }

    Player* target;
    ObjectGuid target_guid;
    std::string target_name;
    if (!ExtractPlayerTarget(&px, &target, &target_guid, &target_name))
    {
        return false;
    }

    // delticket $char_name
    sTicketMgr.Delete(target_guid);

    // notify players about ticket deleting
    if (target)
    {
        target->GetSession()->SendGMTicketGetTicket(0x0A);
    }

    std::string nameLink = playerLink(target_name);

    PSendSysMessage(LANG_COMMAND_TICKETPLAYERDEL, nameLink.c_str());
    return true;
}

