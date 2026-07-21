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
 * @file ObjectUpdate.cpp
 * @brief Cohesion split of Object.cpp -- Object update-block builders: create/values/out-of-range/movement update assembly and bit-mask helpers.
 *        Same classes; no behaviour change.
 */

#include "Utilities/Errors.h"
#include "Object.h"
#include "SharedDefines.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Log.h"
#include "World.h"
#include "Creature.h"
#include "Player.h"
#include "Vehicle.h"
#include "ObjectMgr.h"
#include "ObjectGuid.h"
#include "UpdateData.h"
#include "UpdateMask.h"
#include "Util.h"
#include "MapManager.h"
#include "Log.h"
#include "Transports.h"
#include "TargetedMovementGenerator.h"
#include "WaypointMovementGenerator.h"
#include "VMapFactory.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "ObjectPosSelector.h"
#include "TemporarySummon.h"
#include "movement/packet_builder.h"
#include "CreatureLinkingMgr.h"
#include "Chat.h"
#include "GameTime.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#include "ElunaConfig.h"
#include "ElunaEventMgr.h"
#endif /* ENABLE_ELUNA */

/**
 * @brief Force immediate update transmission to all viewers
 *
 * Sends all pending update changes immediately rather than waiting
 * for the next update tick. This is used for urgent updates that
 * must be visible immediately (e.g., combat state changes).
 *
 * The method builds update data for all nearby players and sends
 * it immediately, then removes the object from the pending update list.
 */
void Object::SendForcedObjectUpdate()
{
    if (!m_inWorld || !m_objectUpdated)
    {
        return;
    }

    UpdateDataMapType update_players;

    BuildUpdateData(update_players);
    RemoveFromClientUpdateList();

    WorldPacket packet;                                     // here we allocate a std::vector with a size of 0x10000
    for (UpdateDataMapType::iterator iter = update_players.begin(); iter != update_players.end(); ++iter)
    {
        iter->second.BuildPacket(&packet);
        iter->first->GetSession()->SendPacket(&packet);
        packet.clear();                                     // clean the string
    }
}

/**
 * @brief Build create update block for player
 * @param data Update data buffer
 * @param target Target player
 *
 * Builds the update packet data needed to create this object
 * for the specified player. Includes movement data and
 * all update field values.
 */
void Object::BuildCreateUpdateBlockForPlayer(UpdateData* data, Player* target) const
{
    if (!target)
    {
        return;
    }

    uint8  updatetype   = UPDATETYPE_CREATE_OBJECT;
    uint16 updateFlags  = m_updateFlag;

    /** lower flag1 **/
    if (target == this)                                     // building packet for yourself
    {
        updateFlags |= UPDATEFLAG_SELF;
    }

    if (updateFlags & UPDATEFLAG_HAS_POSITION)
    {
        // UPDATETYPE_CREATE_OBJECT2 dynamic objects, corpses...
        if (isType(TYPEMASK_DYNAMICOBJECT) || isType(TYPEMASK_CORPSE) || isType(TYPEMASK_PLAYER))
        {
            updatetype = UPDATETYPE_CREATE_OBJECT2;
        }

        // UPDATETYPE_CREATE_OBJECT2 for pets...
        if (target->GetPetGuid() == GetObjectGuid())
        {
            updatetype = UPDATETYPE_CREATE_OBJECT2;
        }

        // UPDATETYPE_CREATE_OBJECT2 for some gameobject types...
        if (isType(TYPEMASK_GAMEOBJECT))
        {
            switch (((GameObject*)this)->GetGoType())
            {
                case GAMEOBJECT_TYPE_TRAP:
                case GAMEOBJECT_TYPE_DUEL_ARBITER:
                case GAMEOBJECT_TYPE_FLAGSTAND:
                case GAMEOBJECT_TYPE_FLAGDROP:
                    updatetype = UPDATETYPE_CREATE_OBJECT2;
                    break;
                case GAMEOBJECT_TYPE_TRANSPORT:
                    updateFlags |= UPDATEFLAG_TRANSPORT;
                    break;
                default:
                    break;
            }
        }

        if (isType(TYPEMASK_UNIT))
        {
            if (((Unit*)this)->getVictim())
            {
                updateFlags |= UPDATEFLAG_HAS_ATTACKING_TARGET;
            }
        }
    }

    // DEBUG_LOG("BuildCreateUpdate: update-type: %u, object-type: %u got updateFlags: %X", updatetype, m_objectTypeId, updateFlags);

    ByteBuffer& buf = data->GetBuffer();
    buf << uint8(updatetype);
    buf << GetPackGUID();
    buf << uint8(m_objectTypeId);

    BuildMovementUpdate(&buf, updateFlags);

    UpdateMask updateMask;
    updateMask.SetCount(m_valuesCount);
    _SetCreateBits(&updateMask, target);
    BuildValuesUpdate(updatetype, &buf, &updateMask, target);
    data->AddUpdateBlock();
}

/**
 * @brief Send create update to player
 * @param player Target player
 *
 * Sends the create update packet to the specified player,
 * causing the object to appear in their game world.
 */
void Object::SendCreateUpdateToPlayer(Player* player)
{
    // send create update to player
    UpdateData upd;
    WorldPacket packet;

    BuildCreateUpdateBlockForPlayer(&upd, player);
    upd.BuildPacket(&packet);
    player->GetSession()->SendPacket(&packet);
}

/**
 * @brief Build values update block for player
 * @param data Update data buffer
 * @param target Target player
 *
 * Builds the update packet data for changed field values
 * to send to the specified player.
 */
void Object::BuildValuesUpdateBlockForPlayer(UpdateData* data, Player* target) const
{
    ByteBuffer& buf = data->GetBuffer();

    buf << uint8(UPDATETYPE_VALUES);
    buf << GetPackGUID();

    UpdateMask updateMask;
    updateMask.SetCount(m_valuesCount);

    _SetUpdateBits(&updateMask, target);
    BuildValuesUpdate(UPDATETYPE_VALUES, &buf, &updateMask, target);

    data->AddUpdateBlock();
}

/**
 * @brief Build out of range update block
 * @param data Update data buffer
 *
 * Adds this object's GUID to the out-of-range list,
 * indicating it should be removed from the client's view.
 */
void Object::BuildOutOfRangeUpdateBlock(UpdateData* data) const
{
    data->AddOutOfRangeGUID(GetObjectGuid());
}

/**
 * @brief Destroy object for player
 * @param target Target player
 *
 * Sends a destroy packet to the specified player,
 * removing this object from their game world.
 */
void Object::DestroyForPlayer(Player* target, bool anim) const
{
    MANGOS_ASSERT(target);

    WorldPacket data(SMSG_DESTROY_OBJECT, 9);
    data << GetObjectGuid();
    data << uint8(anim ? 1 : 0);                            // WotLK (bool), may be despawn animation
    target->GetSession()->SendPacket(&data);
}

/**
 * @brief Build movement update block
 * @param data Byte buffer to write to
 * @param updateFlags Update flags
 *
 * Builds the movement data portion of the update packet.
 * Includes position, orientation, movement flags, and speeds
 * for living objects, or just position for static objects.
 */
void Object::BuildMovementUpdate(ByteBuffer* data, uint16 updateFlags) const
{
    // uint16 moveFlags2 = (isType(TYPEMASK_UNIT) ? ((Unit*)this)->m_movementInfo.GetMovementFlags2() : MOVEFLAG2_NONE);

    *data << uint16(updateFlags);                           // update flags

    // 0x20
    if (updateFlags & UPDATEFLAG_LIVING)
    {
        Unit* unit = ((Unit*)this);

        // ToDo: Remove this hack
        if (GetTypeId() == TYPEID_PLAYER)
        {
            Player* player = ((Player*)unit);
            if (player->GetTransport() || player->IsBoarded())
            {
                player->m_movementInfo.AddMovementFlag(MOVEFLAG_ONTRANSPORT);
            }
            else
            {
                player->m_movementInfo.RemoveMovementFlag(MOVEFLAG_ONTRANSPORT);
            }
        }

        // Update movement info time
        unit->m_movementInfo.UpdateTime(GameTime::GetGameTimeMS());
        // Write movement info
        unit->m_movementInfo.Write(*data);

        // Unit speeds
        *data << float(unit->GetSpeed(MOVE_WALK));
        *data << float(unit->GetSpeed(MOVE_RUN));
        *data << float(unit->GetSpeed(MOVE_RUN_BACK));
        *data << float(unit->GetSpeed(MOVE_SWIM));
        *data << float(unit->GetSpeed(MOVE_SWIM_BACK));
        *data << float(unit->GetSpeed(MOVE_FLIGHT));
        *data << float(unit->GetSpeed(MOVE_FLIGHT_BACK));
        *data << float(unit->GetSpeed(MOVE_TURN_RATE));
        *data << float(unit->GetSpeed(MOVE_PITCH_RATE));

        // 0x08000000
        if (unit->m_movementInfo.GetMovementFlags() & MOVEFLAG_SPLINE_ENABLED)
        {
            Movement::PacketBuilder::WriteCreate(*unit->movespline, *data);
        }
    }
    else
    {
        if (updateFlags & UPDATEFLAG_POSITION)
        {
            *data << uint8(0);                              // unk PGUID!
            *data << float(((WorldObject*)this)->GetPositionX());
            *data << float(((WorldObject*)this)->GetPositionY());
            *data << float(((WorldObject*)this)->GetPositionZ());
            *data << float(((WorldObject*)this)->GetPositionX());
            *data << float(((WorldObject*)this)->GetPositionY());
            *data << float(((WorldObject*)this)->GetPositionZ());
            *data << float(((WorldObject*)this)->GetOrientation());

            if (GetTypeId() == TYPEID_CORPSE)
            {
                *data << float(((WorldObject*)this)->GetOrientation());
            }
            else
            {
                *data << float(0);
            }
        }
        else
        {
            // 0x40
            if (updateFlags & UPDATEFLAG_HAS_POSITION)
            {
                // 0x02
                if (updateFlags & UPDATEFLAG_TRANSPORT && ((GameObject*)this)->GetGoType() == GAMEOBJECT_TYPE_MO_TRANSPORT)
                {
                    *data << float(0);
                    *data << float(0);
                    *data << float(0);
                    *data << float(((WorldObject*)this)->GetOrientation());
                }
                else
                {
                    *data << float(((WorldObject*)this)->GetPositionX());
                    *data << float(((WorldObject*)this)->GetPositionY());
                    *data << float(((WorldObject*)this)->GetPositionZ());
                    *data << float(((WorldObject*)this)->GetOrientation());
                }
            }
        }
    }

    // 0x8
    if (updateFlags & UPDATEFLAG_LOWGUID)
    {
        switch (GetTypeId())
        {
            case TYPEID_OBJECT:
            case TYPEID_ITEM:
            case TYPEID_CONTAINER:
            case TYPEID_GAMEOBJECT:
            case TYPEID_DYNAMICOBJECT:
            case TYPEID_CORPSE:
                *data << uint32(GetGUIDLow());              // GetGUIDLow()
                break;
            case TYPEID_UNIT:
                *data << uint32(0x0000000B);                // unk, can be 0xB or 0xC
                break;
            case TYPEID_PLAYER:
                if (updateFlags & UPDATEFLAG_SELF)
                {
                    *data << uint32(0x0000002F);            // unk, can be 0x15 or 0x22
                }
                else
                {
                    *data << uint32(0x00000008);            // unk, can be 0x7 or 0x8
                }
                break;
            default:
                *data << uint32(0x00000000);                // unk
                break;
        }
    }

    // 0x10
    if (updateFlags & UPDATEFLAG_HIGHGUID)
    {
        switch (GetTypeId())
        {
            case TYPEID_OBJECT:
            case TYPEID_ITEM:
            case TYPEID_CONTAINER:
            case TYPEID_GAMEOBJECT:
            case TYPEID_DYNAMICOBJECT:
            case TYPEID_CORPSE:
                *data << uint32(GetObjectGuid().GetHigh()); // GetGUIDHigh()
                break;
            case TYPEID_UNIT:
                *data << uint32(0x0000000B);                // unk, can be 0xB or 0xC
                break;
            case TYPEID_PLAYER:
                if (updateFlags & UPDATEFLAG_SELF)
                {
                    *data << uint32(0x0000002F);            // unk, can be 0x15 or 0x22
                }
                else
                {
                    *data << uint32(0x00000008);            // unk, can be 0x7 or 0x8
                }
                break;
            default:
                *data << uint32(0x00000000);                // unk
                break;
        }
    }

    // 0x4
    if (updateFlags & UPDATEFLAG_HAS_ATTACKING_TARGET)      // packed guid (current target guid)
    {
        if (((Unit*)this)->getVictim())
        {
            *data << ((Unit*)this)->getVictim()->GetPackGUID();
        }
        else
        {
            data->appendPackGUID(0);
        }
    }

    // 0x2
    if (updateFlags & UPDATEFLAG_TRANSPORT)
    {
        *data << uint32(GameTime::GetGameTimeMS());           // ms time
    }

    // 0x80
    if (updateFlags & UPDATEFLAG_VEHICLE)
    {
        *data << uint32(((Unit*)this)->GetVehicleInfo()->GetVehicleEntry()->ID); // vehicle id
        *data << float(((WorldObject*)this)->GetOrientation());
    }

    // 0x200
    if (updateFlags & UPDATEFLAG_ROTATION)
    {
        *data << int64(((GameObject*)this)->GetPackedRotation());
    }
}

/**
 * @brief Build values update data
 * @param updatetype Update type (create or values)
 * @param data Byte buffer to write to
 * @param updateMask Update mask indicating which fields changed
 * @param target Target player
 *
 * Builds the actual field value data for the update packet.
 * Handles special cases for gameobjects and units.
 */
void Object::BuildValuesUpdate(uint8 updatetype, ByteBuffer* data, UpdateMask* updateMask, Player* target) const
{
    if (!target)
    {
        return;
    }

    bool IsActivateToQuest = false;
    bool IsPerCasterAuraState = false;

    if (updatetype == UPDATETYPE_CREATE_OBJECT || updatetype == UPDATETYPE_CREATE_OBJECT2)
    {
        if (isType(TYPEMASK_GAMEOBJECT) && !((GameObject*)this)->IsTransport())
        {
            if (((GameObject*)this)->ActivateToQuest(target) || target->isGameMaster())
            {
                IsActivateToQuest = true;
            }

            updateMask->SetBit(GAMEOBJECT_DYNAMIC);
        }
        else if (isType(TYPEMASK_UNIT))
        {
            if (((Unit*)this)->HasAuraState(AURA_STATE_CONFLAGRATE))
            {
                IsPerCasterAuraState = true;
                updateMask->SetBit(UNIT_FIELD_AURASTATE);
            }
        }
    }
    else                                                    // case UPDATETYPE_VALUES
    {
        if (isType(TYPEMASK_GAMEOBJECT) && !((GameObject*)this)->IsTransport())
        {
            if (((GameObject*)this)->ActivateToQuest(target) || target->isGameMaster())
            {
                IsActivateToQuest = true;
            }

            updateMask->SetBit(GAMEOBJECT_DYNAMIC);
            updateMask->SetBit(GAMEOBJECT_BYTES_1);         // why do we need this here?
        }
        else if (isType(TYPEMASK_UNIT))
        {
            if (((Unit*)this)->HasAuraState(AURA_STATE_CONFLAGRATE))
            {
                IsPerCasterAuraState = true;
                updateMask->SetBit(UNIT_FIELD_AURASTATE);
            }
        }
    }

    MANGOS_ASSERT(updateMask && updateMask->GetCount() == m_valuesCount);

    *data << (uint8)updateMask->GetBlockCount();
    data->append(updateMask->GetMask(), updateMask->GetLength());

    // 2 specialized loops for speed optimization in non-unit case
    if (isType(TYPEMASK_UNIT))                              // unit (creature/player) case
    {
        for (uint16 index = 0; index < m_valuesCount; ++index)
        {
            if (updateMask->GetBit(index))
            {
                if (index == UNIT_NPC_FLAGS)
                {
                    uint32 appendValue = m_uint32Values[index];

                    if (GetTypeId() == TYPEID_UNIT)
                    {
                        if (!target->canSeeSpellClickOn((Creature*)this))
                        {
                            appendValue &= ~UNIT_NPC_FLAG_SPELLCLICK;
                        }

                        if (appendValue & UNIT_NPC_FLAG_TRAINER)
                        {
                            if (!((Creature*)this)->IsTrainerOf(target, false))
                            {
                                appendValue &= ~(UNIT_NPC_FLAG_TRAINER | UNIT_NPC_FLAG_TRAINER_CLASS | UNIT_NPC_FLAG_TRAINER_PROFESSION);
                            }
                        }

                        if (appendValue & UNIT_NPC_FLAG_STABLEMASTER)
                        {
                            if (target->getClass() != CLASS_HUNTER)
                            {
                                appendValue &= ~UNIT_NPC_FLAG_STABLEMASTER;
                            }
                        }
                    }

                    *data << uint32(appendValue);
                }
                else if (index == UNIT_FIELD_AURASTATE)
                {
                    if (IsPerCasterAuraState)
                    {
                        // IsPerCasterAuraState set if related pet caster aura state set already
                        if (((Unit*)this)->HasAuraStateForCaster(AURA_STATE_CONFLAGRATE, target->GetObjectGuid()))
                        {
                            *data << m_uint32Values[index];
                        }
                        else
                        {
                            *data << (m_uint32Values[index] & ~(1 << (AURA_STATE_CONFLAGRATE - 1)));
                        }
                    }
                    else
                    {
                        *data << m_uint32Values[index];
                    }
                }
                // FIXME: Some values at server stored in float format but must be sent to client in uint32 format
                else if (index >= UNIT_FIELD_BASEATTACKTIME && index <= UNIT_FIELD_RANGEDATTACKTIME)
                {
                    // convert from float to uint32 and send
                    *data << uint32(m_floatValues[index] < 0 ? 0 : m_floatValues[index]);
                }

                // there are some float values which may be negative or can't get negative due to other checks
                else if ((index >= UNIT_FIELD_NEGSTAT0 && index <= UNIT_FIELD_NEGSTAT4) ||
                         (index >= UNIT_FIELD_RESISTANCEBUFFMODSPOSITIVE  && index <= (UNIT_FIELD_RESISTANCEBUFFMODSPOSITIVE + 6)) ||
                         (index >= UNIT_FIELD_RESISTANCEBUFFMODSNEGATIVE  && index <= (UNIT_FIELD_RESISTANCEBUFFMODSNEGATIVE + 6)) ||
                         (index >= UNIT_FIELD_POSSTAT0 && index <= UNIT_FIELD_POSSTAT4))
                {
                    *data << uint32(m_floatValues[index]);
                }

                // Gamemasters should be always able to select units - remove not selectable flag
                else if (index == UNIT_FIELD_FLAGS && target->isGameMaster())
                {
                    *data << (m_uint32Values[index] & ~UNIT_FLAG_NOT_SELECTABLE);
                }
                // Hide special-info for non empathy-casters,
                // Hide lootable animation for unallowed players
                else if (index == UNIT_DYNAMIC_FLAGS)
                {
                    uint32 dynflagsValue = m_uint32Values[index];

                    // Checking SPELL_AURA_EMPATHY and caster
                    if (dynflagsValue & UNIT_DYNFLAG_SPECIALINFO && ((Unit*)this)->IsAlive())
                    {
                        bool bIsEmpathy = false;
                        bool bIsCaster = false;
                        Unit::AuraList const& mAuraEmpathy = ((Unit*)this)->GetAurasByType(SPELL_AURA_EMPATHY);
                        for (Unit::AuraList::const_iterator itr = mAuraEmpathy.begin(); !bIsCaster && itr != mAuraEmpathy.end(); ++itr)
                        {
                            bIsEmpathy = true;        // Empathy by aura set

                            if ((*itr)->GetCasterGuid() == target->GetObjectGuid())
                            {
                                bIsCaster = true;  // target is the caster of an empathy aura
                            }
                        }

                        if (bIsEmpathy && !bIsCaster) // Empathy by aura, but target is not the caster
                        {
                            dynflagsValue &= ~UNIT_DYNFLAG_SPECIALINFO;
                        }
                    }

                    // Checking lootable
                    if (dynflagsValue & UNIT_DYNFLAG_LOOTABLE && GetTypeId() == TYPEID_UNIT)
                    {
                        if (!target->isAllowedToLoot((Creature*)this))
                        {
                            dynflagsValue &= ~(UNIT_DYNFLAG_LOOTABLE | UNIT_DYNFLAG_TAPPED_BY_PLAYER);
                        }
                        else
                        {
                            // flag only for original loot recipent
                            if (target->GetObjectGuid() != ((Creature*)this)->GetLootRecipientGuid())
                            {
                                dynflagsValue &= ~(UNIT_DYNFLAG_TAPPED | UNIT_DYNFLAG_TAPPED_BY_PLAYER);
                            }
                        }
                    }
                    *data << dynflagsValue;
                }
                else                                        // Unhandled index, just send
                {
                    // send in current format (float as float, uint32 as uint32)
                    *data << m_uint32Values[index];
                }
            }
        }
    }
    else if (isType(TYPEMASK_GAMEOBJECT))                   // gameobject case
    {
        for (uint16 index = 0; index < m_valuesCount; ++index)
        {
            if (updateMask->GetBit(index))
            {
                // send in current format (float as float, uint32 as uint32)
                if (index == GAMEOBJECT_DYNAMIC)
                {
                    // GAMEOBJECT_TYPE_DUNGEON_DIFFICULTY can have lo flag = 2
                    //      most likely related to "can enter map" and then should be 0 if can not enter

                    if (IsActivateToQuest)
                    {
                        switch (((GameObject*)this)->GetGoType())
                        {
                            case GAMEOBJECT_TYPE_QUESTGIVER:
                                // GO also seen with GO_DYNFLAG_LO_SPARKLE explicit, relation/reason unclear (192861)
                                *data << uint16(GO_DYNFLAG_LO_ACTIVATE);
                                *data << uint16(-1);
                                break;
                            case GAMEOBJECT_TYPE_CHEST:
                            case GAMEOBJECT_TYPE_GENERIC:
                            case GAMEOBJECT_TYPE_SPELL_FOCUS:
                            case GAMEOBJECT_TYPE_GOOBER:
                                *data << uint16(GO_DYNFLAG_LO_ACTIVATE | GO_DYNFLAG_LO_SPARKLE);
                                *data << uint16(-1);
                                break;
                            default:
                                // unknown, not happen.
                                *data << uint16(0);
                                *data << uint16(-1);
                                break;
                        }
                    }
                    else
                    {
                        // disable quest object
                        *data << uint16(0);
                        *data << uint16(-1);
                    }
                }
                else
                {
                    *data << m_uint32Values[index];          // other cases
                }
            }
        }
    }
    else                                                    // other objects case (no special index checks)
    {
        for (uint16 index = 0; index < m_valuesCount; ++index)
        {
            if (updateMask->GetBit(index))
            {
                // send in current format (float as float, uint32 as uint32)
                *data << m_uint32Values[index];
            }
        }
    }
}

/**
 * @brief Clear update mask
 * @param remove If true, remove from client update list
 *
 * Clears all changed value flags and optionally removes
 * the object from the pending update list.
 */
void Object::ClearUpdateMask(bool remove)
{
    if (m_uint32Values)
    {
        for (uint16 index = 0; index < m_valuesCount; ++index)
        {
            m_changedValues[index] = false;
        }
    }

    if (m_objectUpdated)
    {
        if (remove)
        {
            RemoveFromClientUpdateList();
        }
        m_objectUpdated = false;
    }
}

/**
 * @brief Load values from data string
 * @param data Data string to load from
 * @return True if successful
 *
 * Loads update field values from a character data string.
 * Used when loading objects from database.
 */
bool Object::LoadValues(const char* data)
{
    if (!m_uint32Values)
    {
        _InitValues();
    }

    Tokens tokens = StrSplit(data, " ");

    if (tokens.size() != m_valuesCount)
    {
        return false;
    }

    Tokens::iterator iter;
    int index;
    for (iter = tokens.begin(), index = 0; index < m_valuesCount; ++iter, ++index)
    {
        m_uint32Values[index] = atol((*iter).c_str());
    }

    return true;
}

/**
 * @brief Set update bits in mask
 * @param updateMask Update mask to modify
 * @param target Target player (unused)
 *
 * Sets bits in the update mask for all fields that have changed.
 */
void Object::_SetUpdateBits(UpdateMask* updateMask, Player* /*target*/) const
{
    for (uint16 index = 0; index < m_valuesCount; ++index)
    {
        if (m_changedValues[index])
        {
            updateMask->SetBit(index);
        }
    }
}

/**
 * @brief Set create bits in mask
 * @param updateMask Update mask to modify
 * @param target Target player (unused)
 *
 * Sets bits in the update mask for all non-zero fields.
 * Used when creating a new object for a player.
 */
void Object::_SetCreateBits(UpdateMask* updateMask, Player* /*target*/) const
{
    for (uint16 index = 0; index < m_valuesCount; ++index)
    {
        if (GetUInt32Value(index) != 0)
        {
            updateMask->SetBit(index);
        }
    }
}
