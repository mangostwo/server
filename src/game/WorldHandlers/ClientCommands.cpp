/**
 *	The Open WoW project
 *	https://github.com/openwow-org
 *
 *	This file hosts all the client command handlers
 */


 // For the external references to our handler methods
#include "WorldSession.h"

// For Player class getter/setter methods
#include "Player.h"

// For accessing world objects
#include "ObjectAccessor.h"

// For generic strings: Is this really necessary?
#include "Language.h"

inline int ValidateCharacterName(const char *name)
{
	int result;

	result = 0;
	if (name && *name)
	{
		std::string strName = name;
		wchar_t wstr_buf[16];
		size_t wstr_len = 15;

		if (!Utf8toWStr(strName, &wstr_buf[0], wstr_len))
			return result;

		wstr_buf[0] = wcharToUpper(wstr_buf[0]);
		for (size_t i = 1; i < wstr_len; ++i)
			wstr_buf[i] = wcharToLower(wstr_buf[i]);

		if (!WStrToUtf8(wstr_buf, wstr_len, strName))
			return result;

		strcpy((char *)name, strName.c_str());
		result = 1;
	}
	return result;
}

void WorldSession::BootMeHandler(WorldPacket &msg)
{
	if (Player *plyr = GetPlayer())
	{
		if (plyr->GetSecurityGroup())
		{
			KickPlayer();
		}
		else
			SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
	}
}

void WorldSession::GmSetSecurityGroupHandler(WorldPacket &msg)
{
	if (Player *plyr = GetPlayer())
	{
		if (plyr->GetSecurityGroup() > SEC_GAMEMASTER)
		{
			char playerName[49];
			uint32 securityGroup;

			msg.GetString(playerName, 49);
			msg >> securityGroup;
			if (ValidateCharacterName(playerName))
			{
				if (Player *target = sObjectAccessor.FindPlayerByName(playerName))
					target->SetSecurityGroup(securityGroup);
			}
		}
		else
			SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
	}
}

void WorldSession::GodmodeHandler(WorldPacket &msg)
{
	unsigned int enable;

	if (GetPlayer()->GetSecurityGroup() > 1)
	{
		WorldPacket outbound(SMSG_GODMODE, 4);

		msg >> enable;
		GetPlayer()->SetGodmode(enable);
		outbound << enable;
		SendPacket(&outbound);
	}
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}

void WorldSession::BeastmasterHandler(WorldPacket &msg)
{
	unsigned int enable;

	if (GetPlayer()->GetSecurityGroup() > 1)
	{
		msg >> enable;
		GetPlayer()->SetBeastmaster(enable);
		SendConsoleMessage(enable ? "Beastmaster enabled" : "Beastmaster disabled");
	}
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}

void WorldSession::WorldportHandler(WorldPacket &msg)
{
	if (GetPlayer()->GetSecurityGroup())
	{
		if (GetPlayer()->IsTaxiFlying())
		{
			SendConsoleMessage("Cannot teleport while flying");
			return;
		}
		Position position;
		uint32 time;
		uint32 worldID;
		uint64 mapDBPtr;

		msg >> time;
		msg >> worldID;
		msg >> mapDBPtr;
		msg >> position.x;
		msg >> position.y;
		msg >> position.z;
		msg >> position.o;
		GetPlayer()->TeleportTo(worldID, position.x, position.y, position.z, position.o, TELE_TO_GM_MODE, 0);
	}
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}

void WorldSession::GMTeachHandler(WorldPacket &msg)
{
	Player *player;
	ObjectGuid guid;
	int32 spell;

	msg >> guid;
	msg >> spell;
	if (GetPlayer()->GetSecurityGroup() > 2)
	{
		if ((player = sObjectAccessor.FindPlayer(guid, true)))
			player->learnSpell(spell, false);
		else
			SendPlayerNotFoundFailure();
	}
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}

void WorldSession::LearnSpellHandler(WorldPacket &msg)
{
	int32 spell;

	msg >> spell;
	if (GetPlayer()->GetSecurityGroup() > 2)
		GetPlayer()->learnSpell(spell, false);
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}
