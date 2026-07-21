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

/// \addtogroup u2w
/// @{
/// \file

#ifndef MANGOS_H_OPCODETABLE
#define MANGOS_H_OPCODETABLE

// Opcode dispatch: how the world routes a decoded packet to a WorldSession
// handler. The opcode numbers themselves are wire format and live in the protocol
// library (proto/Opcodes.h); everything here is game-side policy.

#include "Opcodes.h"
#include "WorldSession.h"


/**
 * Initializes opcode handler metadata tables.
 */
extern void InitializeOpcodes();

/// Player state
enum SessionStatus
{
    STATUS_AUTHED = 0,                     ///< Player authenticated (_player==NULL, m_playerRecentlyLogout = false or will be reset before handler call)
    STATUS_LOGGEDIN,                       ///< Player in game (_player!=NULL, inWorld())
    STATUS_TRANSFER,                       ///< Player transferring to another map (_player!=NULL, !inWorld())
    STATUS_LOGGEDIN_OR_RECENTLY_LOGGEDOUT, ///< _player!= NULL or _player==NULL && m_playerRecentlyLogout)
    STATUS_NEVER,                          ///< Opcode not accepted from client (deprecated or server side only)
    STATUS_UNHANDLED                       ///< We don' handle this opcode yet
};

/**
 * This determines how a \ref WorldPacket is handled by MaNGOS. This can be either in the
 * same function as we received it in, this is unusual, or it can be in:
 * - \ref World::UpdateSessions if it's not thread safe
 * - \ref Map::Update if it is thread safe
 */
enum PacketProcessing
{
    PROCESS_INPLACE = 0,   ///< process packet whenever we receive it - mostly for non-handled or non-implemented packets
    PROCESS_THREADUNSAFE,  ///< packet is not thread-safe - process it in \ref World::UpdateSessions
    PROCESS_THREADSAFE     ///< packet is thread-safe - process it in \ref Map::Update
};

class WorldPacket;

/**
 * A structure containing some of the necessary info to handle a \ref WorldPacket when it comes in.
 * The most interesting thing in here is the \ref OpcodeHandler::handler that actually does
 * something with one of the opcodes (see \ref Opcodes) that came in.
 */
struct OpcodeHandler
{
    ///A string representation of the name of this opcode (see \ref Opcodes)
    char const* name;
    ///The status for this handler, it tells whether or not we will handle the packet at all and
    ///when we will handle it.
    SessionStatus status;
    ///This tells where the packet should be processed, ie: is it thread un/safe, which in turn
    ///determines where it will be processed
    PacketProcessing packetProcessing;
    ///The callback called for this opcode which will work some magic
    void (WorldSession::*handler)(WorldPacket& recvPacket);
};

extern OpcodeHandler opcodeTable[NUM_MSG_TYPES];

/// Lookup opcode name for human understandable logging
inline const char* LookupOpcodeName(uint16 id)
{
    if (id >= NUM_MSG_TYPES)
    {
        return "Received unknown opcode, it's more than max!";
    }
    return opcodeTable[id].name;
}
#endif

/**
 * \var OpcodesList::SMSG_PERIODICAURALOG
 * This opcode is used to send data for the combat log when you receive either periodic damage or
 * buffs from a \ref Aura in some way, ie  you gain 10 life every second, you increase your regen
 * of power or something along those lines. The data that needs to be sent is a little different
 * depending on the \ref Modifier for the \ref Aura, what should always be included though is:
 * - The victims Pack GUID (see \ref Object::GetPackGUID)
 * - The casting \ref Player s Pack GUID (see \ref Object::GetPackGUID)
 * - The spellid for the \ref Aura (see \ref Aura::GetId) as a \ref uint32
 * - A 1 as a \ref uint32 this is the count of something (what)
 * - The id of the aura see \ref Modifier::m_auraname as a \ref uint32
 *
 * Now comes different parts depending on what value the \ref Modifier::m_auraname has, if it
 * is \ref AuraType::SPELL_AURA_PERIODIC_DAMAGE or
 * \ref AuraType::SPELL_AURA_PERIODIC_DAMAGE_PERCENT then this is sent:
 * - Damage done as a \ref uint32 from \ref SpellPeriodicAuraLogInfo::damage
 * - The \ref SpellSchools of the \ref SpellEntry for the \ref Aura as a \ref uint32 (see
 * \ref SpellEntry::School)
 * - How much that was absorbed as a \ref uint32
 * - How mcuh that was resisted as a \ref uint32
 *
 * If the \ref Modifier::m_auraname has one of the values of:
 * \ref AuraType::SPELL_AURA_PERIODIC_HEAL or \ref AuraType::SPELL_AURA_OBS_MOD_HEALTH then
 * this should be sent:
 * - Damage/healing (in this case) done as a \ref uint32
 *
 * If the \ref Modifier::m_auraname has one of the values of:
 * \ref AuraType::SPELL_AURA_OBS_MOD_MANA or \ref AuraType::SPELL_AURA_PERIODIC_ENERGIZE then
 * this should be sent:
 * - The \ref Modifier::m_miscvalue as a \ref uint32, in this case it's a power type from the
 * \ref Powers
 * - The damage/mana earned (in this case) as a \ref uint32
 *
 * If the \ref Modifier::m_auraname has one of the values of:
 * \ref AuraType::SPELL_AURA_PERIODIC_MANA_LEECH then this should be sent:
 * - The \ref Modifier::m_miscvalue as a \ref uint32, in this case it's a power type from the
 * \ref Powers
 * - The damage/amount of mana drained (in this case) as a \ref uint32
 * - The gain multiplier as a \ref float from the which probably increases how much power was
 * drained
 *
 * To not create this packet and send it all the time you need it you can use
 * \ref Unit::SendPeriodicAuraLog
 *
 * Also, this should be sent with \ref Object::SendMessageToSet so that all nearby (in
 * the same \ref Cell) \ref Player s get the information. To do this with an \ref Aura
 * one could use \ref Aura::GetTarget and then use the \ref Unit::SendMessageToSet
 * \todo Is it actually for the combat log?
 * \todo Is it in the same \ref Cell?
 * \todo What is the count that is sent as a uint32?
 * \todo Document the multiplier in some way?
 */

/**
 * \var OpcodesList::SMSG_SPELLNONMELEEDAMAGELOG
 * This opcode is used to send data for the combat log when you damage someone with a non melee
 * spell, ie frostbolt.
 * The data that needs to be sent is the following in the same order:
 * - The victims Pack GUID (see \ref Object::GetPackGUID)
 * - The \ref Player s Pack GUID (see \ref Object::GetPackGUID)
 * - Id of the spell that was used as a \ref uint32
 * - The amount of damage that was done (not including resisted damage etc) as a \ref uint32
 * - The \ref SpellSchoolMask of the \ref Spell as a \ref uint8, should be from the representation
 * in \ref SpellSchools though, to do this one can use \ref GetFirstSchoolInMask
 * - The amount of absorbed damage as a \ref uint32
 * - The amount of resisted damage as a \ref uint32
 * - A \ref uint8 which if it is 1 shows the spell name for the client, ie: "%s's ranged shot
 * hit %s for %d damage" (taken from source) and if it's 0 no message is shown
 * - A \ref uint8 value that seems to be unused
 * - The amount of blocked damage as a \ref uint32
 * - The \ref HitInfo as a \ref uint32 which tells what happened it would seem
 * - A \ref uint8 that's usually 0 and is used as a flag to use extended data (taken from source)
 *
 * To not create this packet and send it all the time you need it you can use
 * \ref Unit::SendSpellNonMeleeDamageLog
 *
 * Also, this should be sent with \ref Object::SendMessageToSet so that all nearby (in
 * the same \ref Cell) \ref Player s get the information.
 * \todo Is it actually for the combat log?
 * \todo Is it in the same \ref Cell?
 */

/**
 * \var OpcodesList::SMSG_SPELLENERGIZELOG
 * This opcode is used to send data for the combat log when you gain energy in some way.
 * The data that needs to be sent is the following in the same order:
 * - The victims Pack GUID (see \ref Object::GetPackGUID)
 * - The \ref Player s Pack GUID (see \ref Object::GetPackGUID)
 * - the spellid as a \ref uint32
 * - the powertype as a \ref uint32, see \ref Powers for the available power types
 * - the damage or in this case gain as a \ref uint32
 *
 * To not create this packet and send it all the time you need it you can use
 * \ref Unit::SendEnergizeSpellLog
 * Also, this should be sent with \ref Object::SendMessageToSet so that all nearby (in
 * the same \ref Cell) \ref Player s get the information.
 * \todo Is it actually for the combat log?
 * \todo Is it in the same \ref Cell?
 */

/**
 * \var OpcodesList::SMSG_SPELLHEALLOG
 * This opcode is used to send data for the combat log when healing is done. The data
 * that needs to be sent is the following in the same order:
 * - The victims Pack GUID (see \ref Object::GetPackGUID)
 * - The \ref Player s Pack GUID (see \ref Object::GetPackGUID)
 * - The spellid as a \ref uint32
 * - The damage/healing done as a \ref uint32
 * - If it was critical or not as a \ref uint8 (1 meaning critical, 0 meaning normal)
 * - And a \ref uint8 with the value 0 which doesn't seem to be used in the client
 *
 * To not create this packet and send it all the time you need it you can use
 * \ref Unit::SendHealSpellLog
 * Also, this should be sent with \ref Object::SendMessageToSet so that all nearby (in
 * the same \ref Cell) \ref Player s get the information.
 * \todo Is it actually for the combat log?
 * \todo Is it in the same \ref Cell?
 */

/**
 * \var OpcodesList::SMSG_ATTACKERSTATEUPDATE
 * This opcode is used to send information about a recent hit, who it hit, how
 * much damage it did and so forth. See the \ref CalcDamageInfo structure for more
 * info on what will be sent. The data that needs to be sent is the following in
 * the same order:
 * - The \ref CalcDamageInfo::HitInfo as a \ref uint32
 * - The \ref Unit s Pack GUID (see \ref Object::GetPackGUID)
 * - The targets Pack GUID (see \ref Object::GetPackGUID)
 * - The full damage that was done as a \ref uint32
 * - A 1 as a \ref uint8, this acts as the subdamage count (could it be higher?)
 * - A \ref uint32 of \code{.cpp} GetFirstSchoolInMask(damageInfo->damageSchoolMask) \endcode
 * Need to find out what this does
 * - A float representation of the damage (seen as sub damage from comments)
 * - A \ref uint32 representation of the same damage
 * - A \ref uint32 representation of how much was absorbed (see \ref CalcDamageInfo::absorb)
 * - A \ref uint32 representation of how much was resisted (see \ref CalcDamageInfo::resist)
 * - The targets state as a \ref uint32 (see \ref CalcDamageInfo::TargetState)
 * - If the absorbed part is zero add a 0 as an \ref uint32 otherwise add a -1 as an \ref uint32
 * - The spell id as a \ref uint32 if a spell was used, although in
 * \ref Unit::SendAttackStateUpdate it is always 0.
 * - The blocked amount as a \ref uint32 (see \ref CalcDamageInfo::blocked_amount) this is
 * normally \ref HitInfo::HITINFO_NOACTION according to comments in \ref Unit::SendAttackStateUpdate
 *
 * It appears this should also be sent with \ref Object::SendMessageToSet to that all nearby (in
 * the same \ref Cell) \ref Player s can get take part of the info
 * \see VictimState
 * \todo Is this correct? Is it really about a recent hit?
 */


/// @}
