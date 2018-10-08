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