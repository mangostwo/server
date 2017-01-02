/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2017  MaNGOS project <https://getmangos.eu>
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

#ifndef MANGOS_H_GMTICKETMGR
#define MANGOS_H_GMTICKETMGR

#include "Policies/Singleton.h"
#include "Database/DatabaseEnv.h"
#include "Util.h"
#include "ObjectGuid.h"
#include <map>

/**
 * \addtogroup game
 * @{
 * \file
 */

/**
 * This is the class that takes care of representing a ticket made to the GMs on the server
 * with a question of some sort.
 *
 * The code responsible for taking care of the opcodes coming
 * in can be found in:
 * - \ref WorldSession::SendGMTicketStatusUpdate
 * - \ref WorldSession::SendGMTicketGetTicket
 * - \ref WorldSession::HandleGMTicketGetTicketOpcode
 * - \ref WorldSession::HandleGMTicketUpdateTextOpcode
 * - \ref WorldSession::HandleGMTicketDeleteTicketOpcode
 * - \ref WorldSession::HandleGMTicketCreateOpcode
 * - \ref WorldSession::HandleGMTicketSystemStatusOpcode
 * - \ref WorldSession::HandleGMTicketSurveySubmitOpcode
 * These in their turn will make calls to the \ref GMTicketMgr which will take
 * care of what needs to be done by giving back a \ref GMTicket. The database table interesting
 * in this case is character_ticket in the characaters database.
 *
 * Theres also some handling of tickets in \ref ChatHandler::HandleTicketCommand where
 * you can turn on/off accepting tickets with your current GM char. You can also turn
 * off tickets globally, this will show the client a message about tickets not being
 * available at the moment. The commands that can be used are:
 * <dl>
 * <dt>.ticket on/off</dt>
 * <dd>Turns on/off showing new incoming tickets for you character</dd>
 * <dt>.ticket system_on/off</dt>
 * <dd>Will turn the whole ticket reporting system on/off, ie: if it's off the clients
 * will get a message that the system is unavailable when trying to submit a ticket</dd>
 * <dt>.ticket close $character_name/.ticket close #num_of_ticket</dt>
 * <dd>Will close a ticket for the given character name or the given number of the ticket,
 * this will make the little icon in the top right go away for the player</dd>
 * <dt>.ticket close_survey $character_name/.ticket close_survey #num_of_ticket</dt>
 * <dd>Does the same as .ticket close but instead of just closing it it also asks the \ref Player
 * to answer a survey about how please they were with the experience</dd>
 * <dt>.ticket respond $character_name/.ticket respond #num_of_ticket</dt>
 * <dd>Will respond to a ticket, this will whisper the \ref Player who asked the question and from
 * there on you will have to explain the solution etc. and then close the ticket again.</dd>
 * <dt>.ticket</dt>
 * <dd>Shows the number of currently active tickets</dd>
 * <dt>.ticket $character_name/.ticket #num_of_ticket</dt>
 * <dd>Will show the question and name of the character for the given ticket</dd>
 *
 * \todo Do not remove tickets from db when closing but mark them as solved instead.
 * \todo Log conversations between GM and the player receiving help.
 */
class GMTicket
{
    public:
        explicit GMTicket() : m_lastUpdate(0)
        {}


        void Init(ObjectGuid guid, const std::string& text, const std::string& responsetext, time_t update)
        {
            m_guid = guid;
            m_text = text;
            m_responseText = responsetext;
            m_lastUpdate = update;
        }

        /** 
         * Gets the \ref Player s \ref ObjectGuid which asked the question and created the ticket
         * @return the \ref ObjectGuid for the \ref Player that asked the question
         */
        ObjectGuid const& GetPlayerGuid() const
        {
            return m_guid;
        }

        const char* GetText() const
        {
            return m_text.c_str();
        }

        const char* GetResponse() const
        {
            return m_responseText.c_str();
        }

        uint64 GetLastUpdate() const
        {
            return m_lastUpdate;
        }

        /** 
         * Changes the tickets question text.
         * @param text the text to change the question to
         */
        void SetText(const char* text)
        {
            m_text = text ? text : "";
            m_lastUpdate = time(NULL);

            std::string escapedString = m_text;
            CharacterDatabase.escape_string(escapedString);
            CharacterDatabase.PExecute("UPDATE character_ticket SET ticket_text = '%s' WHERE guid = '%u'", escapedString.c_str(), m_guid.GetCounter());
        }

        void SetResponseText(const char* text)
        {
            m_responseText = text ? text : "";
            m_lastUpdate = time(NULL);

            std::string escapedString = m_responseText;
            CharacterDatabase.escape_string(escapedString);
            CharacterDatabase.PExecute("UPDATE character_ticket SET response_text = '%s' WHERE guid = '%u'", escapedString.c_str(), m_guid.GetCounter());
        }

        bool HasResponse() { return !m_responseText.empty(); }

        void DeleteFromDB() const
        {
            CharacterDatabase.PExecute("DELETE FROM character_ticket WHERE guid = '%u' LIMIT 1", m_guid.GetCounter());
        }

        void SaveToDB() const
        {
            CharacterDatabase.BeginTransaction();
            DeleteFromDB();

            std::string escapedString = m_text;
            CharacterDatabase.escape_string(escapedString);

            std::string escapedString2 = m_responseText;
            CharacterDatabase.escape_string(escapedString2);

            CharacterDatabase.PExecute("INSERT INTO character_ticket (guid, ticket_text, response_text) VALUES ('%u', '%s', '%s')", m_guid.GetCounter(), escapedString.c_str(), escapedString2.c_str());
            CharacterDatabase.CommitTransaction();
        }
    private:
        ObjectGuid m_guid;
        std::string m_text;
        std::string m_responseText;
        time_t m_lastUpdate;
};
typedef std::map<ObjectGuid, GMTicket> GMTicketMap;
typedef std::list<GMTicket*> GMTicketList;                  // for creating order access

class GMTicketMgr
{
    public:
        GMTicketMgr() {  }
        ~GMTicketMgr() {  }

        void LoadGMTickets();

        GMTicket* GetGMTicket(ObjectGuid guid)
        {
            GMTicketMap::iterator itr = m_GMTicketMap.find(guid);
            if (itr == m_GMTicketMap.end())
                { return NULL; }
            return &(itr->second);
        }
        
        size_t GetTicketCount() const
        {
            return m_GMTicketMap.size();
        }

        GMTicket* GetGMTicketByOrderPos(uint32 pos)
        {
            if (pos >= GetTicketCount())
                { return NULL; }

            GMTicketList::iterator itr = m_GMTicketListByCreatingOrder.begin();
            std::advance(itr, pos);
            if (itr == m_GMTicketListByCreatingOrder.end())
                { return NULL; }
            return *itr;
        }

        /** 
         * This will delete a \ref GMTicket from this manager of tickets so that we don't
         * need to handle it anymore, this should be used in conjunction with setting
         * resolved = 1 in the character_ticket table.
         *
         * Note: This will _not_ remove anything from the DB
         * @param guid guid of the \ref Player who created the ticket that we want to delete
         */
        void Delete(ObjectGuid guid)
        {
            GMTicketMap::iterator itr = m_GMTicketMap.find(guid);
            if (itr == m_GMTicketMap.end())
                { return; }
            itr->second.DeleteFromDB();
            m_GMTicketListByCreatingOrder.remove(&itr->second);
            m_GMTicketMap.erase(itr);
        }

        void DeleteAll();

        /** 
         * This will create a new \ref GMTicket and fill it with the given question so that
         * a GM can find it and answer it. Should only be called if we've already checked
         * that there are no open tickets already, as this function will close any other
         * currently open tickets for the given \ref Player and open a new one with the given
         * text.
         *
         * Tables of interest here are characters.character_ticket and possibly characaters.
         * character_whispers
         * @param guid \ref ObjectGuid of the creator of the \ref GMTicket
         * @param text the question text sent
         */
        void Create(ObjectGuid guid, const char* text)
        {
            GMTicket& ticket = m_GMTicketMap[guid];
            if (ticket.GetPlayerGuid())                     // overwrite ticket
            {
                ticket.DeleteFromDB();
                m_GMTicketListByCreatingOrder.remove(&ticket);
            }

            ticket.Init(guid, text, "", time(NULL));
            ticket.SaveToDB();
            m_GMTicketListByCreatingOrder.push_back(&ticket);
        }
    private:
        GMTicketMap m_GMTicketMap;
        GMTicketList m_GMTicketListByCreatingOrder;
};

#define sTicketMgr MaNGOS::Singleton<GMTicketMgr>::Instance()

/** @} */
#endif
