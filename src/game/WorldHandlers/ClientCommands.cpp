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
#include "ObjectMgr.h"

// For generic strings: Is this really necessary?
#include "Language.h"

// Class declarations
#include "TemporarySummon.h"
#include "Totem.h"

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
				{
					target->SetSecurityGroup(securityGroup);
					if (securityGroup)
						target->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_ALLOW_CHEAT_SPELLS);
					else
						target->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_ALLOW_CHEAT_SPELLS);
				}
                else
                    CharacterDatabase.PExecute("UPDATE characters SET securitygroup='%d' WHERE name='%s'", securityGroup, playerName);
			}
		}
		else
			SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
	}
}

void WorldSession::GodmodeHandler(WorldPacket &msg)
{
	if (GetPlayer()->GetSecurityGroup() > 1)
	{
        unsigned int enable;

		msg >> enable;
		GetPlayer()->SetGodmode(enable);
	}
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}

void WorldSession::BeastmasterHandler(WorldPacket &msg)
{
	if (GetPlayer()->GetSecurityGroup() > 1)
	{
        unsigned int enable;

		msg >> enable;
		GetPlayer()->SetBeastmaster(enable);
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
		{
			if (msg.GetOpcode() == CMSG_GM_TEACH)
				player->learnSpell(spell, false);
			else if (msg.GetOpcode() == CMSG_GM_UNTEACH)
				player->removeSpell(spell);
		}
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

void WorldSession::GMResurrectHandler(WorldPacket &msg)
{
	char name[49];
	uint32 success;

	success = 0;
	if (GetPlayer()->GetSecurityGroup())
	{
		msg.GetString(name, 49);
		if (ValidateCharacterName(name))
		{
			Player *player = sObjectMgr.GetPlayer(name);
			if (!player)
				goto PLAYER_NOT_FOUND;

			if (player->IsDead())
			{
				player->ResurrectPlayer(100.0f, false);
				success = 1;
				// TODO: Send SMSG_GM_RESURRECT with result. For now we just write to console
				char buf[128];
				sprintf(buf, "Player %s resurrected", name);
				SendConsoleMessage(buf);
			}
		}
		else
		{
		PLAYER_NOT_FOUND:
			SendPlayerNotFoundFailure();
		}
	}
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}

void WorldSession::HandlePlayerLogout(WorldPacket &msg)
{
    if (GetPlayer()->GetSecurityGroup())
        LogoutPlayer(true);
    else
        SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);		
}

void WorldSession::RechargeHandler(WorldPacket &msg)
{
	Player *pPlayer;

	pPlayer = GetPlayer();
	if (pPlayer->GetSecurityGroup())
	{
		pPlayer->SetHealthPercent(100.0f);
		pPlayer->SetPowerPercent(pPlayer->GetPowerType(), 100.0f);
	}
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}

void WorldSession::DechargeHandler(WorldPacket &msg)
{
	if (GetPlayer()->GetSecurityGroup())
	{
		GetPlayer()->SetHealth(1);
		GetPlayer()->SetPower(GetPlayer()->GetPowerType(), 1);
	}
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}

void WorldSession::LevelCheatHandler(WorldPacket &msg)
{
	Player *pPlayer = GetPlayer();

	if (pPlayer->GetSecurityGroup())
	{
		uint32 level = 0;

		msg >> level;
		if (level < 1 || level > MAX_LEVEL)
		{
			SendConsoleMessage("Invalid level specified");
			return;
		}
		pPlayer->GiveLevel(level);
		pPlayer->InitTalentForLevel();
		pPlayer->SetUInt32Value(PLAYER_XP, 0);
	}
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}

void WorldSession::CreateMonsterHandler(WorldPacket &msg)
{
	Player *pPlayer = GetPlayer();

	if (pPlayer->GetSecurityGroup() > 2)
	{
		uint32 monsterId = 0;
		const char *strMsg = "Created unit";
		Map *pMap = NULL;
		uint32 monsterGuid = 0;
		Creature *pCreature = new Creature;
		CreatureCreatePos createPos(pPlayer, pPlayer->GetOrientation());

		msg >> monsterId;
		CreatureInfo const *cinfo = ObjectMgr::GetCreatureTemplate(monsterId);
		if (!cinfo)
		{
			strMsg = "Unit not found";
		}
		else
		{
			pMap = pPlayer->GetMap();
			monsterGuid = sObjectMgr.GenerateStaticCreatureLowGuid();
			if (!monsterGuid)
			{
				delete pCreature;
				SendNotification(LANG_NO_FREE_STATIC_GUID_FOR_SPAWN);
				SendConsoleMessage("No free GUID is available for this unit. Server is shuting down...");
				return;
			}

			if (!pCreature->Create(monsterGuid, createPos, cinfo))
			{
				delete pCreature;
				strMsg = "Unit not found";
			}
			else
			{
				pCreature->SaveToDB(pMap->GetId(), (1 << pMap->GetSpawnMode()), pPlayer->GetPhaseMaskForSpawn());
				monsterGuid = pCreature->GetGUIDLow(); // TODO: Is this needed? Can GetGUIDLow return a different value to what we have? Used in mangos chat commands.
				pCreature->LoadFromDB(monsterGuid, pMap); // To call _LoadGoods(); _LoadQuests(); CreateTrainerSpells();
				pMap->Add(pCreature);
				sObjectMgr.AddCreatureToGrid(monsterGuid, sObjectMgr.GetCreatureData(monsterGuid));
			}
		}
		SendConsoleMessage(strMsg);
	}
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}

void WorldSession::DestroyMonsterHandler(WorldPacket &msg)
{
	Player *pPlayer = GetPlayer();

	if (pPlayer->GetSecurityGroup() > 2)
	{
		Creature *pCreature = pPlayer->GetMap()->GetCreature(pPlayer->GetTargetGuid());
		if (!pCreature)
			return;

		switch (pCreature->GetSubtype())
		{
		case CREATURE_SUBTYPE_GENERIC:
		{
			pCreature->CombatStop();
			if (CreatureData const* data = sObjectMgr.GetCreatureData(pCreature->GetGUIDLow()))
			{
				Creature::AddToRemoveListInMaps(pCreature->GetGUIDLow(), data);
				Creature::DeleteFromDB(pCreature->GetGUIDLow(), data);
			}
			else
			{
				pCreature->AddObjectToRemoveList();
			}
			break;
		}
		case CREATURE_SUBTYPE_PET:
			((Pet *)pCreature)->Unsummon(PET_SAVE_AS_CURRENT);
			break;
		case CREATURE_SUBTYPE_TOTEM:
			((Totem *)pCreature)->UnSummon();
			break;
		case CREATURE_SUBTYPE_TEMPORARY_SUMMON:
			((TemporarySummon *)pCreature)->UnSummon();
			break;
		default:
			return;
		}
	}
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}

void WorldSession::MoveRunSpeedCheatHandler(WorldPacket &msg)
{
	Player *pPlayer = GetPlayer();

	if (pPlayer->GetSecurityGroup())
	{
		float speed;

		msg >> speed;
		if (speed < 0.1f || speed > 60.0f)
			speed = 0.0f;
		else
			speed /= 7.0f;

		pPlayer->SetSpeedRate(MOVE_RUN, speed, true);
	}
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}

void WorldSession::MoveSwimSpeedCheatHandler(WorldPacket &msg)
{
	Player *pPlayer = GetPlayer();

	if (pPlayer->GetSecurityGroup())
	{
		float speed;

		msg >> speed;
		if (speed < 0.1f || speed > 60.0f)
			speed = 0.0f;
		else
			speed /= 4.7222218513489f;

		pPlayer->SetSpeedRate(MOVE_SWIM, speed, true);
	}
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}

void WorldSession::MoveFlightSpeedCheatHandler(WorldPacket &msg)
{
	Player *pPlayer = GetPlayer();

	if (pPlayer->GetSecurityGroup())
	{
		float speed;

		msg >> speed;
		if (speed < 0.1f || speed > 100.0f)
			speed = 0.0f;
		else
			speed /= 7.0f;

		pPlayer->SetSpeedRate(MOVE_FLIGHT, speed, true);
	}
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}

void WorldSession::SetMoneyCheatHandler(WorldPacket &msg)
{
	if (GetPlayer()->GetSecurityGroup() > 1)
	{
		uint32 money = 0;

		msg >> money;
		GetPlayer()->SetMoney(money);
	}
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}

void WorldSession::CreateItemHandler(WorldPacket &msg)
{
	if (GetPlayer()->GetSecurityGroup() > 1)
	{
		uint32 id = 0;
		uint32 quantity = 1;

		msg >> id;
		msg >> quantity;
		
		// Number of items we can currently store
		uint32 quantityLimit = 0;

		// Declare our item stack size and verify if we can store it
		ItemPosCountVec stackSize;
		InventoryResult result = GetPlayer()->CanStoreNewItem(NULL_BAG, NULL_SLOT, stackSize, id, quantity, &quantityLimit);

		// Adjust quantity value to match available inventory slots/stacks
		if (result != EQUIP_ERR_OK)
			quantity -= quantityLimit;

		// No room; send result down to client
		if (quantity == 0 || stackSize.empty())
		{
			GetPlayer()->SendEquipError(result, 0, 0, id);
			return;
		}

		// Everything checks out: Create the item(s) and notify the player
		Item *item = GetPlayer()->StoreNewItem(stackSize, id, true, Item::GenerateItemRandomPropertyId(id));
		if (item)
			GetPlayer()->SendNewItem(item, quantity, false, true);
	}
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}
