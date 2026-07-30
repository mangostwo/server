/**
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2026 MaNGOS <https://www.getmangos.eu>
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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

/**
 * @file MovementHandler.cpp
 * @brief Movement opcode handlers
 *
 * This file handles movement-related opcodes including:
 * - MSG_MOVE_WORLDPORT_ACK: Acknowledge map teleport
 * - MSG_MOVE_TELEPORT_ACK: Acknowledge teleport
 * - MSG_MOVE_HEARTBEAT: Movement heartbeat
 * - MSG_MOVE_SET_FACING: Set facing direction
 * - MSG_MOVE_JUMP: Jump
 * - MSG_MOVE_START_FORWARD: Start moving forward
 * - MSG_MOVE_START_BACKWARD: Start moving backward
 * - MSG_MOVE_STOP: Stop movement
 * - MSG_MOVE_START_STRAFE_LEFT: Start strafing left
 * - MSG_MOVE_START_STRAFE_RIGHT: Start strafing right
 * - MSG_MOVE_START_PITCH_UP: Start pitching up
 * - MSG_MOVE_START_PITCH_DOWN: Start pitching down
 * - MSG_MOVE_SET_RUN_MODE: Set run mode
 * - MSG_MOVE_SET_WALK_MODE: Set walk mode
 * - MSG_MOVE_FALL_LAND: Land after fall
 * - MSG_MOVE_START_SWIM: Start swimming
 * - MSG_MOVE_STOP_SWIM: Stop swimming
 * - MSG_MOVE_SPLASH: Water splash
 * - MSG_MOVE_ASCEND: Ascend (flying)
 * - MSG_MOVE_DESCEND: Descend (flying)
 *
 * Movement packets are validated and synchronized with the server's
 * authoritative position to prevent cheating.
 */

#include "OpcodeTable.h"
#include "Platform/Define.h"
#include "Common/TimeConstants.h"
#include <ctime>
#include <cmath>
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "Corpse.h"
#include "Player.h"
#include "Vehicle.h"
#include "SpellAuras.h"
#include "MapManager.h"
#include "Transports.h"
#include "TransportMap.h"
#include "BattleGround/BattleGround.h"
#include "WaypointMovementGenerator.h"
#include "MapPersistentStateMgr.h"
#include "ObjectMgr.h"

/**
 * @brief Handles the packet-based worldport acknowledgement.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleMoveWorldportAckOpcode(WorldPacket& /*recv_data*/)
{
    DEBUG_LOG("WORLD: got MSG_MOVE_WORLDPORT_ACK.");
    HandleMoveWorldportAckOpcode();
}

/**
 * @brief Finalizes a far teleport after the client acknowledges worldport.
 */
void WorldSession::HandleMoveWorldportAckOpcode()
{
    // ignore unexpected far teleports
    if (!GetPlayer()->IsBeingTeleportedFar())
    {
        return;
    }

    // get start teleport coordinates (will used later in fail case)
    WorldLocation old_loc;
    old_loc = WorldLocation(GetPlayer()->GetMapId(), GetPlayer()->Where().X(), GetPlayer()->Where().Y(), GetPlayer()->Where().Z(), GetPlayer()->Where().Facing());

    // get the teleport destination
    WorldLocation& loc = GetPlayer()->GetTeleportDest();

    // possible errors in the coordinate validity check (only cheating case possible)
    if (!MapManager::IsValidMapCoord(loc.mapid, loc.coord_x, loc.coord_y, loc.coord_z, loc.orientation))
    {
        sLog.outError("WorldSession::HandleMoveWorldportAckOpcode: %s was teleported far to a not valid location "
                      "(map:%u, x:%f, y:%f, z:%f) We port him to his homebind instead..",
                      GetPlayer()->GetGuidStr().c_str(), loc.mapid, loc.coord_x, loc.coord_y, loc.coord_z);
        // stop teleportation else we would try this again and again in LogoutPlayer...
        GetPlayer()->SetSemaphoreTeleportFar(false);
        // and teleport the player to a valid place
        GetPlayer()->TeleportToHomebind();
        return;
    }

    // get the destination map entry, not the current one, this will fix homebind and reset greeting
    MapEntry const* mEntry = sMapStore.LookupEntry(loc.mapid);

    Map* map = NULL;

    // prevent crash at attempt landing to not existed battleground instance
    if (mEntry->IsBattleGroundOrArena())
    {
        if (GetPlayer()->GetBattleGroundId())
        {
            map = sMapMgr.FindMap(loc.mapid, GetPlayer()->GetBattleGroundId());
        }

        if (!map)
        {
            DETAIL_LOG("WorldSession::HandleMoveWorldportAckOpcode: %s was teleported far to nonexisten battleground instance "
                       " (map:%u, x:%f, y:%f, z:%f) Trying to port him to his previous place..",
                       GetPlayer()->GetGuidStr().c_str(), loc.mapid, loc.coord_x, loc.coord_y, loc.coord_z);

            GetPlayer()->SetSemaphoreTeleportFar(false);

            // Teleport to previous place, if can not be ported back TP to homebind place
            if (!GetPlayer()->TeleportTo(old_loc))
            {
                DETAIL_LOG("WorldSession::HandleMoveWorldportAckOpcode: %s can not be ported to his previous place, teleporting him to his homebind place...",
                           GetPlayer()->GetGuidStr().c_str());
                GetPlayer()->TeleportToHomebind();
            }
            return;
        }
    }

    InstanceTemplate const* mInstance = ObjectMgr::GetInstanceTemplate(loc.mapid);

    // reset instance validity, except if going to an instance inside an instance
    if (GetPlayer()->m_InstanceValid == false && !mInstance)
    {
        GetPlayer()->m_InstanceValid = true;
    }

    GetPlayer()->SetSemaphoreTeleportFar(false);

    // relocate the player to the teleport destination
    if (!map)
    {
        map = sMapMgr.CreateMap(loc.mapid, GetPlayer());
    }

    GetPlayer()->SetMap(map);
    GetPlayer()->Place().MoveTo(loc.coord_x, loc.coord_y, loc.coord_z, loc.orientation);

    // And the movement state, which is what the packets on the far side are written from.
    // Without it the client arrives on the new map holding the pose it had on the old one:
    // `.tele menethil` from a deck put a player on Eastern Kingdoms still carrying the
    // ship's Icecrown position, down to the hull's own facing.
    GetPlayer()->m_movementInfo.Report(loc.coord_x, loc.coord_y, loc.coord_z,
                                       loc.orientation);

    // The client threw away every object it had when it left the old map, so the set of
    // "things he already has" is now a lie in the one direction that hurts: anything still
    // listed here will be skipped by UpdateVisibilityOf and never sent again. That
    // includes the vessel he is standing on, which exists on both sides of the seam and so
    // keeps its guid across it.
    GetPlayer()->m_clientGUIDs.clear();

    GetPlayer()->SendInitialPacketsBeforeAddToMap();
    // the CanEnter checks are done in TeleporTo but conditions may change
    // while the player is in transit, for example the map may get full
    // Aboard, this is HER map, not the one just named in SMSG_NEW_WORLD -- see
    // Player::BoardingMap. The client is loading the world map she sails and never learns
    // the other one exists.
    if (!GetPlayer()->BoardingMap()->Add(GetPlayer()))
    {
        // if player wasn't added to map, reset his map pointer!
        GetPlayer()->ResetMap();

        DETAIL_LOG("WorldSession::HandleMoveWorldportAckOpcode: %s was teleported far but couldn't be added to map "
                   " (map:%u, x:%f, y:%f, z:%f) Trying to port him to his previous place..",
                   GetPlayer()->GetGuidStr().c_str(), loc.mapid, loc.coord_x, loc.coord_y, loc.coord_z);

        // Teleport to previous place, if can not be ported back TP to homebind place
        if (!GetPlayer()->TeleportTo(old_loc))
        {
            DETAIL_LOG("WorldSession::HandleMoveWorldportAckOpcode: %s can not be ported to his previous place, teleporting him to his homebind place...",
                       GetPlayer()->GetGuidStr().c_str());
            GetPlayer()->TeleportToHomebind();
        }
        return;
    }

    // battleground state prepare (in case join to BG), at relogin/tele player not invited
    // only add to bg group and object, if the player was invited (else he entered through command)
    if (_player->InBattleGround())
    {
        // cleanup setting if outdated
        if (!mEntry->IsBattleGroundOrArena())
        {
            // We're not in BG
            _player->SetBattleGroundId(0, BATTLEGROUND_TYPE_NONE);
            // reset destination bg team
            _player->SetBGTeam(TEAM_NONE);
        }
        // join to bg case
        else if (BattleGround* bg = _player->GetBattleGround())
        {
            if (_player->IsInvitedForBattleGroundInstance(_player->GetBattleGroundId()))
            {
                bg->AddPlayer(_player);
            }
        }
    }

    GetPlayer()->SendInitialPacketsAfterAddToMap();

    // flight fast teleport case
    if (GetPlayer()->GetMotionMaster()->GetCurrentMovementGeneratorType() == FLIGHT_MOTION_TYPE)
    {
        if (!_player->InBattleGround())
        {
            // short preparations to continue flight
            FlightPathMovementGenerator* flight = (FlightPathMovementGenerator*)(GetPlayer()->GetMotionMaster()->top());
            flight->Reset(*GetPlayer());
            return;
        }

        // battleground state prepare, stop flight
        GetPlayer()->GetMotionMaster()->MovementExpired();
        GetPlayer()->m_taxi.ClearTaxiDestinations();
    }

    if (mInstance)
    {
        Difficulty diff = GetPlayer()->GetDifficulty(mEntry->IsRaid());
        if (MapDifficultyEntry const* mapDiff = GetMapDifficultyData(mEntry->ID, diff))
        {
            if (mapDiff->RaidDuration)
            {
                if (time_t timeReset = sMapPersistentStateMgr.GetScheduler().GetResetTimeFor(mEntry->ID, diff))
                {
                    uint32 timeleft = uint32(timeReset - time(NULL));
                    GetPlayer()->SendInstanceResetWarning(mEntry->ID, diff, timeleft);
                }
            }
        }
    }

    // mount allow check
    if (!mEntry->IsMountAllowed())
    {
        _player->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);
    }

    // honorless target
    if (GetPlayer()->pvpInfo.inHostileArea)
    {
        GetPlayer()->CastSpell(GetPlayer(), 2479, true);
    }

    // resummon pet
    GetPlayer()->ResummonPetTemporaryUnSummonedIfAny();

    // lets process all delayed operations on successful teleport
    GetPlayer()->ProcessDelayedOperations();

    // notify group after successful teleport
    if (_player->GetGroup())
    {
        _player->SetGroupUpdateFlag(GROUP_UPDATE_FULL);
    }
}

/**
 * @brief Finalizes a near teleport after the client acknowledges it.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleMoveTeleportAckOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("MSG_MOVE_TELEPORT_ACK");

    ObjectGuid guid;

    recv_data >> guid.ReadAsPacked();

    uint32 counter, time;
    recv_data >> counter >> time;
    DEBUG_LOG("Guid: %s", guid.GetString().c_str());
    DEBUG_LOG("Counter %u, time %u", counter, time / IN_MILLISECONDS);

    Unit* mover = _player->GetMover();
    Player* plMover = mover->GetTypeId() == TYPEID_PLAYER ? (Player*)mover : NULL;

    if (!plMover || !plMover->IsBeingTeleportedNear())
    {
        return;
    }

    if (guid != plMover->GetObjectGuid())
    {
        return;
    }

    plMover->SetSemaphoreTeleportNear(false);

    uint32 old_zone = plMover->GetTerrain()->GetZoneId(plMover->Where().X(), plMover->Where().Y(), plMover->Where().Z());

    WorldLocation const& dest = plMover->GetTeleportDest();

    plMover->SetPosition(dest.coord_x, dest.coord_y, dest.coord_z, dest.orientation, true);

    uint32 newzone, newarea;
    plMover->GetTerrain()->GetZoneAndAreaId(newzone, newarea, plMover->Where().X(), plMover->Where().Y(), plMover->Where().Z());
    plMover->UpdateZone(newzone, newarea);

    // new zone
    if (old_zone != newzone)
    {
        // honorless target
        if (plMover->pvpInfo.inHostileArea)
        {
            plMover->CastSpell(plMover, 2479, true);
        }
    }

    // resummon pet
    GetPlayer()->ResummonPetTemporaryUnSummonedIfAny();

    // lets process all delayed operations on successful teleport
    GetPlayer()->ProcessDelayedOperations();
}

/**
 * @brief Processes standard client movement updates.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleMovementOpcodes(WorldPacket& recv_data)
{
    uint16 opcode = recv_data.GetOpcode();
    if (!sLog.HasLogFilter(LOG_FILTER_PLAYER_MOVES))
    {
        DEBUG_LOG("WORLD: Received opcode %s (%u, 0x%X)", LookupOpcodeName(opcode), opcode, opcode);
        recv_data.hexlike();
    }

    Unit* mover = _player->GetMover();
    Player* plMover = mover->GetTypeId() == TYPEID_PLAYER ? (Player*)mover : NULL;

    // ignore, waiting processing in WorldSession::HandleMoveWorldportAckOpcode and WorldSession::HandleMoveTeleportAck
    if (plMover && plMover->IsBeingTeleported())
    {
        recv_data.rpos(recv_data.wpos());                   // prevent warnings spam
        return;
    }

    /* extract packet */
    ObjectGuid guid;
    MovementInfo movementInfo;

    recv_data >> guid.ReadAsPacked();
    recv_data >> movementInfo;
    /*----------------*/

    if (!VerifyMovementInfo(movementInfo))
    {
        return;
    }

    // fall damage generation (ignore in flight case that can be triggered also at lags in moment teleportation to another map).
    if (opcode == MSG_MOVE_FALL_LAND && plMover && !plMover->IsTaxiFlying())
    {
        plMover->HandleFall(movementInfo);
    }

    /* process position-change */
    HandleMoverRelocation(movementInfo);

    if (plMover)
    {
        plMover->UpdateFallInformationIfNeed(movementInfo, opcode);
    }

    WorldPacket data(opcode, recv_data.size());
    data << mover->GetPackGUID();             // write guid
    movementInfo.Write(data);                               // write data
    mover->SendMessageToSetExcept(&data, _player);
}

/**
 * @brief Verifies client acknowledgement packets for forced speed changes.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleForceSpeedChangeAckOpcodes(WorldPacket& recv_data)
{
    uint16 opcode = recv_data.GetOpcode();
    DEBUG_LOG("WORLD: Received %s (%u, 0x%X) opcode", LookupOpcodeName(recv_data.GetOpcode()), opcode, opcode);

    /* extract packet */
    ObjectGuid guid;
    MovementInfo movementInfo;
    float  newspeed;

    recv_data >> guid.ReadAsPacked();
    recv_data >> Unused<uint32>();                          // counter or moveEvent
    recv_data >> movementInfo;
    recv_data >> newspeed;

    // now can skip not our packet
    if (_player->GetObjectGuid() != guid)
    {
        recv_data.rpos(recv_data.wpos());                   // prevent warnings spam
        return;
    }
    /*----------------*/

    // client ACK send one packet for mounted/run case and need skip all except last from its
    // in other cases anti-cheat check can be fail in false case
    UnitMoveType move_type;
    UnitMoveType force_move_type;

    static char const* move_type_name[MAX_MOVE_TYPE] = {  "Walk", "Run", "RunBack", "Swim", "SwimBack", "TurnRate", "Flight", "FlightBack", "PitchRate" };

    switch (opcode)
    {
        case CMSG_FORCE_WALK_SPEED_CHANGE_ACK:          move_type = MOVE_WALK;          force_move_type = MOVE_WALK;        break;
        case CMSG_FORCE_RUN_SPEED_CHANGE_ACK:           move_type = MOVE_RUN;           force_move_type = MOVE_RUN;         break;
        case CMSG_FORCE_RUN_BACK_SPEED_CHANGE_ACK:      move_type = MOVE_RUN_BACK;      force_move_type = MOVE_RUN_BACK;    break;
        case CMSG_FORCE_SWIM_SPEED_CHANGE_ACK:          move_type = MOVE_SWIM;          force_move_type = MOVE_SWIM;        break;
        case CMSG_FORCE_SWIM_BACK_SPEED_CHANGE_ACK:     move_type = MOVE_SWIM_BACK;     force_move_type = MOVE_SWIM_BACK;   break;
        case CMSG_FORCE_TURN_RATE_CHANGE_ACK:           move_type = MOVE_TURN_RATE;     force_move_type = MOVE_TURN_RATE;   break;
        case CMSG_FORCE_FLIGHT_SPEED_CHANGE_ACK:        move_type = MOVE_FLIGHT;        force_move_type = MOVE_FLIGHT;      break;
        case CMSG_FORCE_FLIGHT_BACK_SPEED_CHANGE_ACK:   move_type = MOVE_FLIGHT_BACK;   force_move_type = MOVE_FLIGHT_BACK; break;
        case CMSG_FORCE_PITCH_RATE_CHANGE_ACK:          move_type = MOVE_PITCH_RATE;    force_move_type = MOVE_PITCH_RATE;  break;
        default:
            sLog.outError("WorldSession::HandleForceSpeedChangeAck: Unknown move type opcode: %u", opcode);
            return;
    }

    // skip all forced speed changes except last and unexpected
    // in run/mounted case used one ACK and it must be skipped.m_forced_speed_changes[MOVE_RUN} store both.
    if (_player->m_forced_speed_changes[force_move_type] > 0)
    {
        --_player->m_forced_speed_changes[force_move_type];
        if (_player->m_forced_speed_changes[force_move_type] > 0)
        {
            return;
        }
    }

    if (!_player->GetTransport() && fabs(_player->GetSpeed(move_type) - newspeed) > 0.01f)
    {
        if (_player->GetSpeed(move_type) > newspeed)        // must be greater - just correct
        {
            sLog.outError("%sSpeedChange player %s is NOT correct (must be %f instead %f), force set to correct value",
                          move_type_name[move_type], _player->GetName(), _player->GetSpeed(move_type), newspeed);
            _player->SetSpeedRate(move_type, _player->GetSpeedRate(move_type), true);
        }
        else                                                // must be lesser - cheating
        {
            BASIC_LOG("Player %s from account id %u kicked for incorrect speed (must be %f instead %f)",
                      _player->GetName(), _player->GetSession()->GetAccountId(), _player->GetSpeed(move_type), newspeed);
            _player->GetSession()->KickPlayer();
        }
    }
}

/**
 * @brief Validates the active mover guid reported by the client.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleSetActiveMoverOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Received opcode CMSG_SET_ACTIVE_MOVER");
    recv_data.hexlike();

    ObjectGuid guid;
    recv_data >> guid;

    if (_player->GetMover()->GetObjectGuid() != guid)
    {
        sLog.outError("HandleSetActiveMoverOpcode: incorrect mover guid: mover is %s and should be %s",
                      _player->GetMover()->GetGuidStr().c_str(), guid.GetString().c_str());
        return;
    }
}

/**
 * @brief Stores movement info sent for a non-active mover.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleMoveNotActiveMoverOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Received opcode CMSG_MOVE_NOT_ACTIVE_MOVER");
    recv_data.hexlike();

    ObjectGuid old_mover_guid;
    MovementInfo mi;

    recv_data >> old_mover_guid.ReadAsPacked();
    recv_data >> mi;

    if (_player->GetMover()->GetObjectGuid() == old_mover_guid)
    {
        sLog.outError("HandleMoveNotActiveMover: incorrect mover guid: mover is %s and should be %s instead of %s",
                      _player->GetMover()->GetGuidStr().c_str(),
                      _player->GetGuidStr().c_str(),
                      old_mover_guid.GetString().c_str());
        recv_data.rpos(recv_data.wpos());                   // prevent warnings spam
        return;
    }

    _player->m_movementInfo = mi;
}

/**
 * @brief Broadcasts the player's mount special animation.
 *
 * @param recvdata The received opcode packet.
 */
void WorldSession::HandleMountSpecialAnimOpcode(WorldPacket& /*recvdata*/)
{
    // DEBUG_LOG("WORLD: Received opcode CMSG_MOUNTSPECIAL_ANIM");

    WorldPacket data(SMSG_MOUNTSPECIAL_ANIM, 8);
    data << GetPlayer()->GetObjectGuid();

    GetPlayer()->SendMessageToSet(&data, false);
}

/**
 * @brief Handles knockback acknowledgement movement updates.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleMoveKnockBackAck(WorldPacket& recv_data)
{
    DEBUG_LOG("CMSG_MOVE_KNOCK_BACK_ACK");

    Unit* mover = _player->GetMover();
    Player* plMover = mover->GetTypeId() == TYPEID_PLAYER ? (Player*)mover : NULL;

    // ignore, waiting processing in WorldSession::HandleMoveWorldportAckOpcode and WorldSession::HandleMoveTeleportAck
    if (plMover && plMover->IsBeingTeleported())
    {
        recv_data.rpos(recv_data.wpos());                   // prevent warnings spam
        return;
    }

    ObjectGuid guid;
    MovementInfo movementInfo;                              // Sent in addition to knockback data

    recv_data >> guid.ReadAsPacked();
    recv_data >> Unused<uint32>();                          // knockback packets counter
    recv_data >> movementInfo;

    if (!VerifyMovementInfo(movementInfo, guid))
    {
        return;
    }

    HandleMoverRelocation(movementInfo);

    /* Weird size, maybe needs correcting */
    WorldPacket data(MSG_MOVE_KNOCK_BACK, recv_data.size() + 15);
    data << mover->GetPackGUID();
    /* Includes data shown below (but in different order) */
    data << movementInfo;

    /* This is sent in addition to the rest of the movement data (yes, angle+velocity are sent twice) */
    data << movementInfo.GetJumpInfo().sinAngle;
    data << movementInfo.GetJumpInfo().cosAngle;
    data << movementInfo.GetJumpInfo().xyspeed;
    data << movementInfo.GetJumpInfo().velocity;

    /* Do we really need to send the data to everyone? Seemed to work better */
    mover->SendMessageToSetExcept(&data, _player);
}

/**
 * @brief Sends a knockback packet to the client.
 *
 * @param angle The horizontal knockback angle.
 * @param horizontalSpeed The horizontal speed component.
 * @param verticalSpeed The vertical speed component.
 */
void WorldSession::SendKnockBack(float angle, float horizontalSpeed, float verticalSpeed)
{
    float vsin = sin(angle);
    float vcos = cos(angle);

    WorldPacket data(SMSG_MOVE_KNOCK_BACK, 9 + 4 + 4 + 4 + 4 + 4);
    data << GetPlayer()->GetPackGUID();
    data << uint32(0);                                  // Sequence
    data << float(vcos);                                // x direction
    data << float(vsin);                                // y direction
    data << float(horizontalSpeed);                     // Horizontal speed
    data << float(-verticalSpeed);                      // Z Movement speed (vertical)
    SendPacket(&data);
}

/**
 * @brief Handles hover movement acknowledgement packets.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleMoveHoverAck(WorldPacket& recv_data)
{
    DEBUG_LOG("CMSG_MOVE_HOVER_ACK");

    ObjectGuid guid;                                        // guid - unused
    MovementInfo movementInfo;

    recv_data >> guid.ReadAsPacked();
    recv_data >> Unused<uint32>();                          // unk
    recv_data >> movementInfo;
    recv_data >> Unused<uint32>();                          // unk2
}

/**
 * @brief Handles water-walk acknowledgement packets.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleMoveWaterWalkAck(WorldPacket& recv_data)
{
    DEBUG_LOG("CMSG_MOVE_WATER_WALK_ACK");

    ObjectGuid guid;                                        // guid - unused
    MovementInfo movementInfo;

    recv_data >> guid.ReadAsPacked();
    recv_data >> Unused<uint32>();                          // unk1
    recv_data >> movementInfo;
    recv_data >> Unused<uint32>();                          // unk2
}

/**
 * @brief Handles the client's response to a summon request.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleSummonResponseOpcode(WorldPacket& recv_data)
{
    if (!_player->IsAlive() || _player->IsInCombat())
    {
        return;
    }

    ObjectGuid summonerGuid;
    bool agree;
    recv_data >> summonerGuid;
    recv_data >> agree;

    _player->SummonIfPossible(agree);
}

/**
 * @brief Verifies movement data for a specific mover guid.
 *
 * @param movementInfo The movement state to validate.
 * @param guid The expected mover guid.
 * @return true if the movement data is valid; otherwise false.
 */
bool WorldSession::VerifyMovementInfo(MovementInfo const& movementInfo, ObjectGuid const& guid) const
{
    // ignore wrong guid (player attempt cheating own session for not own guid possible...)
    if (guid != _player->GetMover()->GetObjectGuid())
    {
        return false;
    }

    return VerifyMovementInfo(movementInfo);
}

/**
 * @brief Verifies movement coordinates and transport offsets.
 *
 * @param movementInfo The movement state to validate.
 * @return true if the movement data is valid; otherwise false.
 */
bool WorldSession::VerifyMovementInfo(MovementInfo const& movementInfo) const
{
    if (!MaNGOS::IsValidMapCoord(movementInfo.Reported().X(), movementInfo.Reported().Y(), movementInfo.Reported().Z(), movementInfo.Reported().Facing()))
    {
        return false;
    }

    if (movementInfo.HasMovementFlag(MOVEFLAG_ONTRANSPORT))
    {
        // WHERE HE STANDS ON THE DECK MAP. The wire spells this field t_x/t_y/t_z and the
        // protocol calls it an offset, but the moment it is ours it is a position on the
        // vessel's own map -- nothing is composed with it, ever.
        //
        // So the test is whether it names a place on that map, and a map's bounds are the
        // hull's. The old one compared 50 yards against the POSITIVE side alone and threw
        // the whole packet away otherwise, which froze the stored position at the last one
        // accepted: Orgrim's Hammer runs x[-99, +83] and even a common transport ship
        // reaches x[-59, +45], so anyone forward of the mast stopped moving at 50. That is
        // how `.trans npc add` came to plant crew there.
        //
        // The guard it replaces is still needed: a leaving zeppelin sometimes reports these
        // as absolute continent coordinates, and those are thousands.
        const Position* onDeck = movementInfo.GetTransportPos();

        float extent = MAX_DECK_EXTENT;
        if (Transport* vessel = _player
                                    ? Transport::GetTransport(_player->GetMap(),
                                                              movementInfo.GetTransportGuid())
                                    : NULL)
        {
            extent = vessel->AsMap() ? vessel->AsMap()->HullRadius() + DECK_EDGE_MARGIN
                                     : MAX_DECK_EXTENT;
        }

        if (std::fabs(onDeck->x) > extent || std::fabs(onDeck->y) > extent ||
            std::fabs(onDeck->z) > extent)
        {
            return false;
        }

        if (!MaNGOS::IsValidMapCoord(movementInfo.Reported().X() + movementInfo.GetTransportPos()->x, movementInfo.Reported().Y() + movementInfo.GetTransportPos()->y,
                                     movementInfo.Reported().Z() + movementInfo.GetTransportPos()->z, movementInfo.Reported().Facing() + movementInfo.GetTransportPos()->o))
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Applies validated movement info to the current mover.
 *
 * @param movementInfo The movement state to apply.
 */
void WorldSession::HandleMoverRelocation(MovementInfo& movementInfo)
{
    //uint32 mstime = GameTime::GetGameTimeMS();
    //if (m_clientTimeDelay == 0)
    //    m_clientTimeDelay = mstime - movementInfo.GetTime();

    //movementInfo.UpdateTime(movementInfo.GetTime() + m_clientTimeDelay + MOVEMENT_PACKET_TIME_DELAY);
    movementInfo.UpdateTime(movementInfo.GetTime() + GetLatency());

    Unit* mover = _player->GetMover();

    if (Player* plMover = mover->GetTypeId() == TYPEID_PLAYER ? (Player*)mover : NULL)
    {
        if (movementInfo.HasMovementFlag(MOVEFLAG_ONTRANSPORT))
        {
            if (!plMover->m_transport)
            {
                // elevators also cause the client to send MOVEFLAG_ONTRANSPORT - just unmount if the guid can be found in the transport list
                for (MapManager::TransportSet::const_iterator iter = sMapMgr.m_Transports.begin(); iter != sMapMgr.m_Transports.end(); ++iter)
                {
                    if ((*iter)->GetObjectGuid() == movementInfo.GetTransportGuid())
                    {
                        plMover->m_transport = (*iter);

                        // He walked aboard, so his client already has the vessel and is
                        // rendering the map she sails; moving him onto her own map is safe
                        // at once. Nothing tells the client -- it never learns that id.
                        if (TransportMap* hull = (*iter)->AsMap())
                        {
                            hull->Embark(plMover);
                        }
                        break;
                    }
                }
            }

        }
        else if (plMover->m_transport)               // if we were on a transport, leave
        {
            // Unboard the instant the flag drops. There used to be a debounce here, against
            // the client omitting the flag for a packet: back when a vessel was shown only
            // to the players aboard, a one-packet blink destroyed it at their client. It is
            // now shown to everyone on its map regardless, so it cannot blink -- and the
            // delay only kept the master's pet chasing him around a deck he had already left
            // instead of teleporting to his heel ashore.
            // He walked ashore, and his own client just told us where: that world point is
            // better than anything we could derive from a hull whose pose we only estimate.
            //
            // BUT ONLY IF IT IS NEXT TO THE SHIP. Nobody steps off a vessel onto another
            // continent, and the number in this field is not always his: while he is aboard
            // we tell him his world position is (0, 0, 0), and he echoes it straight back.
            // Drop the flag for one packet -- the client does, on arrival, before it has
            // resolved the hull -- and that zero is read as a destination. (0, 0, 0) on map
            // 0 is the middle of Lordamere Lake, which is exactly where people landed.
            if (TransportMap* hull = plMover->m_transport->AsMap())
            {
                Transport* vessel = plMover->m_transport;

                const float reach = hull->HullRadius() + vessel->NodeSlack() +
                                    DECK_EDGE_MARGIN;

                const bool ashore = vessel->Where().WithinDist(
                    Geometry::Vector3(movementInfo.Reported().X(),
                                      movementInfo.Reported().Y(),
                                      movementInfo.Reported().Z()), reach);

                if (ashore)
                {
                    hull->Disembark(plMover,
                                    movementInfo.Reported().X(),
                                    movementInfo.Reported().Y(),
                                    movementInfo.Reported().Z(),
                                    movementInfo.Reported().Facing());
                }
                else
                {
                    // Not a step ashore at all. Put him down on the ship's own coarse pose:
                    // wrong by a hull's length at worst, instead of by a continent.
                    hull->Disembark(plMover, vessel->Where().X(), vessel->Where().Y(),
                                    vessel->Where().Z(), vessel->Where().Facing());
                }
            }
            plMover->m_transport = NULL;
            movementInfo.ClearTransportData();
        }

        if (movementInfo.HasMovementFlag(MOVEFLAG_SWIMMING) != plMover->IsInWater())
        {
            // now client not include swimming flag in case jumping under water
            plMover->SetInWater(!plMover->IsInWater() || plMover->GetTerrain()->IsUnderWater(movementInfo.Reported().X(), movementInfo.Reported().Y(), movementInfo.Reported().Z()));
        }

        // Aboard, the deck offset IS his position: it is what the client computed against
        // the hull it is drawing, and the world pair in the same packet describes a place
        // on a map he is no longer filed under. Ashore, the two are the same packet field.
        if (plMover->m_transport && plMover->GetMap()->AsTransport())
        {
            const Position* offset = movementInfo.GetTransportPos();
            plMover->SetPosition(offset->x, offset->y, offset->z, offset->o);
        }
        else
        {
            plMover->SetPosition(movementInfo.Reported().X(), movementInfo.Reported().Y(), movementInfo.Reported().Z(), movementInfo.Reported().Facing());
        }
        plMover->m_movementInfo = movementInfo;

        // Event-driven, straight off this packet: it carries the whole transport state, and
        // the pet is linked to the player, so mirror it onto his minions -- put them on a
        // type-11 lift he has stepped onto, keep them at his heel as it rises, take them off
        // when he leaves. No polling; the pet moves exactly when its master reports moving.
        plMover->UpdateLiftMinions();

        /* Movement should cancel looting */
        if (ObjectGuid lootGUID = plMover->GetLootGuid())
        {
            plMover->SendLootRelease(lootGUID);
        }

        if (movementInfo.Reported().Z() < -500.0f)
        {
            if (plMover->GetBattleGround()
                && plMover->GetBattleGround()->HandlePlayerUnderMap(_player))
            {
                // do nothing, the handle already did if returned true
            }
            else
            {
                // NOTE: this is actually called many times while falling
                // even after the player has been teleported away
                // TODO: discard movement packets after the player is rooted
                if (plMover->IsAlive())
                {
                    plMover->EnvironmentalDamage(DAMAGE_FALL_TO_VOID, plMover->GetMaxHealth());
                    // pl can be alive if GM/etc
                    if (!plMover->IsAlive())
                    {
                        // change the death state to CORPSE to prevent the death timer from
                        // starting in the next player update
                        plMover->KillPlayer();
                        plMover->BuildPlayerRepop();
                    }
                }

                // cancel the death timer here if started
                plMover->RepopAtGraveyard();
            }
        }
    }
    else                                                    // creature charmed
    {
        if (mover->IsInWorld())
        {
            mover->GetMap()->CreatureRelocation((Creature*)mover, movementInfo.Reported().X(), movementInfo.Reported().Y(), movementInfo.Reported().Z(), movementInfo.Reported().Facing());
        }
    }
}
