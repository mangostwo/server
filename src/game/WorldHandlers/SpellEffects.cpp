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

#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Log.h"
#include "UpdateMask.h"
#include "World.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "SkillExtraItems.h"
#include "Unit.h"
#include "Spell.h"
#include "DynamicObject.h"
#include "SpellAuras.h"
#include "Group.h"
#include "UpdateData.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "SharedDefines.h"
#include "Pet.h"
#include "GameObject.h"
#include "GossipDef.h"
#include "Creature.h"
#include "Totem.h"
#include "CreatureAI.h"
#include "BattleGround/BattleGroundMgr.h"
#include "BattleGround/BattleGround.h"
#include "BattleGround/BattleGroundEY.h"
#include "BattleGround/BattleGroundWS.h"
#include "Language.h"
#include "SocialMgr.h"
#include "VMapFactory.h"
#include "Util.h"
#include "TemporarySummon.h"
#include "ScriptMgr.h"
#include "SkillDiscovery.h"
#include "Formulas.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "Vehicle.h"
#include "G3D/Vector3.h"
#include <random>

#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /* ENABLE_ELUNA */

pEffect SpellEffects[TOTAL_SPELL_EFFECTS] =
{
    &Spell::EffectNULL,                                     //  0
    &Spell::EffectInstaKill,                                //  1 SPELL_EFFECT_INSTAKILL
    &Spell::EffectSchoolDMG,                                //  2 SPELL_EFFECT_SCHOOL_DAMAGE
    &Spell::EffectDummy,                                    //  3 SPELL_EFFECT_DUMMY
    &Spell::EffectUnused,                                   //  4 SPELL_EFFECT_PORTAL_TELEPORT          unused from pre-1.2.1
    &Spell::EffectTeleportUnits,                            //  5 SPELL_EFFECT_TELEPORT_UNITS
    &Spell::EffectApplyAura,                                //  6 SPELL_EFFECT_APPLY_AURA
    &Spell::EffectEnvironmentalDMG,                         //  7 SPELL_EFFECT_ENVIRONMENTAL_DAMAGE
    &Spell::EffectPowerDrain,                               //  8 SPELL_EFFECT_POWER_DRAIN
    &Spell::EffectHealthLeech,                              //  9 SPELL_EFFECT_HEALTH_LEECH
    &Spell::EffectHeal,                                     // 10 SPELL_EFFECT_HEAL
    &Spell::EffectBind,                                     // 11 SPELL_EFFECT_BIND
    &Spell::EffectUnused,                                   // 12 SPELL_EFFECT_PORTAL                   unused from pre-1.2.1, exist 2 spell, but not exist any data about its real usage
    &Spell::EffectUnused,                                   // 13 SPELL_EFFECT_RITUAL_BASE              unused from pre-1.2.1
    &Spell::EffectUnused,                                   // 14 SPELL_EFFECT_RITUAL_SPECIALIZE        unused from pre-1.2.1
    &Spell::EffectUnused,                                   // 15 SPELL_EFFECT_RITUAL_ACTIVATE_PORTAL   unused from pre-1.2.1
    &Spell::EffectQuestComplete,                            // 16 SPELL_EFFECT_QUEST_COMPLETE
    &Spell::EffectWeaponDmg,                                // 17 SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL
    &Spell::EffectResurrect,                                // 18 SPELL_EFFECT_RESURRECT
    &Spell::EffectAddExtraAttacks,                          // 19 SPELL_EFFECT_ADD_EXTRA_ATTACKS
    &Spell::EffectEmpty,                                    // 20 SPELL_EFFECT_DODGE                    one spell: Dodge
    &Spell::EffectEmpty,                                    // 21 SPELL_EFFECT_EVADE                    one spell: Evade (DND)
    &Spell::EffectParry,                                    // 22 SPELL_EFFECT_PARRY
    &Spell::EffectBlock,                                    // 23 SPELL_EFFECT_BLOCK                    one spell: Block
    &Spell::EffectCreateItem,                               // 24 SPELL_EFFECT_CREATE_ITEM
    &Spell::EffectEmpty,                                    // 25 SPELL_EFFECT_WEAPON                   spell per weapon type, in ItemSubclassmask store mask that can be used for usability check at equip, but current way using skill also work.
    &Spell::EffectEmpty,                                    // 26 SPELL_EFFECT_DEFENSE                  one spell: Defense
    &Spell::EffectPersistentAA,                             // 27 SPELL_EFFECT_PERSISTENT_AREA_AURA
    &Spell::EffectSummonType,                               // 28 SPELL_EFFECT_SUMMON
    &Spell::EffectLeapForward,                              // 29 SPELL_EFFECT_LEAP
    &Spell::EffectEnergize,                                 // 30 SPELL_EFFECT_ENERGIZE
    &Spell::EffectWeaponDmg,                                // 31 SPELL_EFFECT_WEAPON_PERCENT_DAMAGE
    &Spell::EffectTriggerMissileSpell,                      // 32 SPELL_EFFECT_TRIGGER_MISSILE
    &Spell::EffectOpenLock,                                 // 33 SPELL_EFFECT_OPEN_LOCK
    &Spell::EffectSummonChangeItem,                         // 34 SPELL_EFFECT_SUMMON_CHANGE_ITEM
    &Spell::EffectApplyAreaAura,                            // 35 SPELL_EFFECT_APPLY_AREA_AURA_PARTY
    &Spell::EffectLearnSpell,                               // 36 SPELL_EFFECT_LEARN_SPELL
    &Spell::EffectEmpty,                                    // 37 SPELL_EFFECT_SPELL_DEFENSE            one spell: SPELLDEFENSE (DND)
    &Spell::EffectDispel,                                   // 38 SPELL_EFFECT_DISPEL
    &Spell::EffectEmpty,                                    // 39 SPELL_EFFECT_LANGUAGE                 misc store lang id
    &Spell::EffectDualWield,                                // 40 SPELL_EFFECT_DUAL_WIELD
    &Spell::EffectJump,                                     // 41 SPELL_EFFECT_JUMP
    &Spell::EffectJump,                                     // 42 SPELL_EFFECT_JUMP2
    &Spell::EffectTeleUnitsFaceCaster,                      // 43 SPELL_EFFECT_TELEPORT_UNITS_FACE_CASTER
    &Spell::EffectLearnSkill,                               // 44 SPELL_EFFECT_SKILL_STEP
    &Spell::EffectAddHonor,                                 // 45 SPELL_EFFECT_ADD_HONOR                honor/pvp related
    &Spell::EffectNULL,                                     // 46 SPELL_EFFECT_SPAWN                    spawn/login animation, expected by spawn unit cast, also base points store some dynflags
    &Spell::EffectTradeSkill,                               // 47 SPELL_EFFECT_TRADE_SKILL
    &Spell::EffectUnused,                                   // 48 SPELL_EFFECT_STEALTH                  one spell: Base Stealth
    &Spell::EffectUnused,                                   // 49 SPELL_EFFECT_DETECT                   one spell: Detect
    &Spell::EffectTransmitted,                              // 50 SPELL_EFFECT_TRANS_DOOR
    &Spell::EffectUnused,                                   // 51 SPELL_EFFECT_FORCE_CRITICAL_HIT       unused from pre-1.2.1
    &Spell::EffectUnused,                                   // 52 SPELL_EFFECT_GUARANTEE_HIT            unused from pre-1.2.1
    &Spell::EffectEnchantItemPerm,                          // 53 SPELL_EFFECT_ENCHANT_ITEM
    &Spell::EffectEnchantItemTmp,                           // 54 SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY
    &Spell::EffectTameCreature,                             // 55 SPELL_EFFECT_TAMECREATURE
    &Spell::EffectSummonPet,                                // 56 SPELL_EFFECT_SUMMON_PET
    &Spell::EffectLearnPetSpell,                            // 57 SPELL_EFFECT_LEARN_PET_SPELL
    &Spell::EffectWeaponDmg,                                // 58 SPELL_EFFECT_WEAPON_DAMAGE
    &Spell::EffectCreateRandomItem,                         // 59 SPELL_EFFECT_CREATE_RANDOM_ITEM       create item base at spell specific loot
    &Spell::EffectProficiency,                              // 60 SPELL_EFFECT_PROFICIENCY
    &Spell::EffectSendEvent,                                // 61 SPELL_EFFECT_SEND_EVENT
    &Spell::EffectPowerBurn,                                // 62 SPELL_EFFECT_POWER_BURN
    &Spell::EffectThreat,                                   // 63 SPELL_EFFECT_THREAT
    &Spell::EffectTriggerSpell,                             // 64 SPELL_EFFECT_TRIGGER_SPELL
    &Spell::EffectApplyAreaAura,                            // 65 SPELL_EFFECT_APPLY_AREA_AURA_RAID
    &Spell::EffectRestoreItemCharges,                       // 66 SPELL_EFFECT_RESTORE_ITEM_CHARGES     itemtype - is affected item ID
    &Spell::EffectHealMaxHealth,                            // 67 SPELL_EFFECT_HEAL_MAX_HEALTH
    &Spell::EffectInterruptCast,                            // 68 SPELL_EFFECT_INTERRUPT_CAST
    &Spell::EffectDistract,                                 // 69 SPELL_EFFECT_DISTRACT
    &Spell::EffectPull,                                     // 70 SPELL_EFFECT_PULL                     one spell: Distract Move
    &Spell::EffectPickPocket,                               // 71 SPELL_EFFECT_PICKPOCKET
    &Spell::EffectAddFarsight,                              // 72 SPELL_EFFECT_ADD_FARSIGHT
    &Spell::EffectNULL,                                     // 73 SPELL_EFFECT_UNTRAIN_TALENTS          one spell: Trainer: Untrain Talents
    &Spell::EffectApplyGlyph,                               // 74 SPELL_EFFECT_APPLY_GLYPH
    &Spell::EffectHealMechanical,                           // 75 SPELL_EFFECT_HEAL_MECHANICAL          one spell: Mechanical Patch Kit
    &Spell::EffectSummonObjectWild,                         // 76 SPELL_EFFECT_SUMMON_OBJECT_WILD
    &Spell::EffectScriptEffect,                             // 77 SPELL_EFFECT_SCRIPT_EFFECT
    &Spell::EffectUnused,                                   // 78 SPELL_EFFECT_ATTACK
    &Spell::EffectSanctuary,                                // 79 SPELL_EFFECT_SANCTUARY
    &Spell::EffectAddComboPoints,                           // 80 SPELL_EFFECT_ADD_COMBO_POINTS
    &Spell::EffectUnused,                                   // 81 SPELL_EFFECT_CREATE_HOUSE             one spell: Create House (TEST)
    &Spell::EffectNULL,                                     // 82 SPELL_EFFECT_BIND_SIGHT
    &Spell::EffectDuel,                                     // 83 SPELL_EFFECT_DUEL
    &Spell::EffectStuck,                                    // 84 SPELL_EFFECT_STUCK
    &Spell::EffectSummonPlayer,                             // 85 SPELL_EFFECT_SUMMON_PLAYER
    &Spell::EffectActivateObject,                           // 86 SPELL_EFFECT_ACTIVATE_OBJECT
    &Spell::EffectWMODamage,                                // 87 SPELL_EFFECT_WMO_DAMAGE (57 spells in 3.3.2)
    &Spell::EffectWMORepair,                                // 88 SPELL_EFFECT_WMO_REPAIR (2 spells in 3.3.2)
    &Spell::EffectWMOChange,                                // 89 SPELL_EFFECT_WMO_CHANGE (7 spells in 3.3.2)
    &Spell::EffectKillCreditPersonal,                       // 90 SPELL_EFFECT_KILL_CREDIT_PERSONAL     Kill credit but only for single person
    &Spell::EffectUnused,                                   // 91 SPELL_EFFECT_THREAT_ALL               one spell: zzOLDBrainwash
    &Spell::EffectEnchantHeldItem,                          // 92 SPELL_EFFECT_ENCHANT_HELD_ITEM
    &Spell::EffectBreakPlayerTargeting,                     // 93 SPELL_EFFECT_BREAK_PLAYER_TARGETING
    &Spell::EffectSelfResurrect,                            // 94 SPELL_EFFECT_SELF_RESURRECT
    &Spell::EffectSkinning,                                 // 95 SPELL_EFFECT_SKINNING
    &Spell::EffectCharge,                                   // 96 SPELL_EFFECT_CHARGE
    &Spell::EffectSummonAllTotems,                          // 97 SPELL_EFFECT_SUMMON_ALL_TOTEMS
    &Spell::EffectKnockBack,                                // 98 SPELL_EFFECT_KNOCK_BACK
    &Spell::EffectDisEnchant,                               // 99 SPELL_EFFECT_DISENCHANT
    &Spell::EffectInebriate,                                // 100 SPELL_EFFECT_INEBRIATE
    &Spell::EffectFeedPet,                                  // 101 SPELL_EFFECT_FEED_PET
    &Spell::EffectDismissPet,                               // 102 SPELL_EFFECT_DISMISS_PET
    &Spell::EffectReputation,                               // 103 SPELL_EFFECT_REPUTATION
    &Spell::EffectSummonObject,                             // 104 SPELL_EFFECT_SUMMON_OBJECT_SLOT1
    &Spell::EffectSummonObject,                             // 105 SPELL_EFFECT_SUMMON_OBJECT_SLOT2
    &Spell::EffectSummonObject,                             // 106 SPELL_EFFECT_SUMMON_OBJECT_SLOT3
    &Spell::EffectSummonObject,                             // 107 SPELL_EFFECT_SUMMON_OBJECT_SLOT4
    &Spell::EffectDispelMechanic,                           // 108 SPELL_EFFECT_DISPEL_MECHANIC
    &Spell::EffectSummonDeadPet,                            // 109 SPELL_EFFECT_SUMMON_DEAD_PET
    &Spell::EffectDestroyAllTotems,                         // 110 SPELL_EFFECT_DESTROY_ALL_TOTEMS
    &Spell::EffectDurabilityDamage,                         // 111 SPELL_EFFECT_DURABILITY_DAMAGE
    &Spell::EffectUnused,                                   //112 SPELL_EFFECT_112 (old SPELL_EFFECT_SUMMON_DEMON)
    &Spell::EffectResurrectNew,                             // 113 SPELL_EFFECT_RESURRECT_NEW
    &Spell::EffectTaunt,                                    // 114 SPELL_EFFECT_ATTACK_ME
    &Spell::EffectDurabilityDamagePCT,                      // 115 SPELL_EFFECT_DURABILITY_DAMAGE_PCT
    &Spell::EffectSkinPlayerCorpse,                         // 116 SPELL_EFFECT_SKIN_PLAYER_CORPSE       one spell: Remove Insignia, bg usage, required special corpse flags...
    &Spell::EffectSpiritHeal,                               // 117 SPELL_EFFECT_SPIRIT_HEAL              one spell: Spirit Heal
    &Spell::EffectSkill,                                    // 118 SPELL_EFFECT_SKILL                    professions and more
    &Spell::EffectApplyAreaAura,                            // 119 SPELL_EFFECT_APPLY_AREA_AURA_PET
    &Spell::EffectUnused,                                   // 120 SPELL_EFFECT_TELEPORT_GRAVEYARD       one spell: Graveyard Teleport Test
    &Spell::EffectWeaponDmg,                                // 121 SPELL_EFFECT_NORMALIZED_WEAPON_DMG
    &Spell::EffectUnused,                                   // 122 SPELL_EFFECT_122                      unused
    &Spell::EffectSendTaxi,                                 // 123 SPELL_EFFECT_SEND_TAXI                taxi/flight related (misc value is taxi path id)
    &Spell::EffectPlayerPull,                               // 124 SPELL_EFFECT_PLAYER_PULL              opposite of knockback effect (pulls player twoard caster)
    &Spell::EffectModifyThreatPercent,                      // 125 SPELL_EFFECT_MODIFY_THREAT_PERCENT
    &Spell::EffectStealBeneficialBuff,                      //126 SPELL_EFFECT_STEAL_BENEFICIAL_BUFF    spell steal effect?
    &Spell::EffectProspecting,                              //127 SPELL_EFFECT_PROSPECTING              Prospecting spell
    &Spell::EffectApplyAreaAura,                            //128 SPELL_EFFECT_APPLY_AREA_AURA_FRIEND
    &Spell::EffectApplyAreaAura,                            //129 SPELL_EFFECT_APPLY_AREA_AURA_ENEMY
    &Spell::EffectRedirectThreat,                           //130 SPELL_EFFECT_REDIRECT_THREAT
    &Spell::EffectPlaySound,                                //131 SPELL_EFFECT_PLAY_SOUND               sound id in misc value (SoundEntries.dbc)
    &Spell::EffectPlayMusic,                                //132 SPELL_EFFECT_PLAY_MUSIC               sound id in misc value (SoundEntries.dbc)
    &Spell::EffectUnlearnSpecialization,                    //133 SPELL_EFFECT_UNLEARN_SPECIALIZATION   unlearn profession specialization
    &Spell::EffectKillCreditGroup,                          //134 SPELL_EFFECT_KILL_CREDIT_GROUP        misc value is creature entry
    &Spell::EffectNULL,                                     //135 SPELL_EFFECT_CALL_PET
    &Spell::EffectHealPct,                                  //136 SPELL_EFFECT_HEAL_PCT
    &Spell::EffectEnergisePct,                              //137 SPELL_EFFECT_ENERGIZE_PCT
    &Spell::EffectLeapBack,                                 //138 SPELL_EFFECT_LEAP_BACK                Leap back
    &Spell::EffectClearQuest,                               //139 SPELL_EFFECT_CLEAR_QUEST              (misc - is quest ID)
    &Spell::EffectForceCast,                                //140 SPELL_EFFECT_FORCE_CAST
    &Spell::EffectForceCast,                                //141 SPELL_EFFECT_FORCE_CAST_WITH_VALUE                      damage and reduce speed?
    &Spell::EffectTriggerSpellWithValue,                    //142 SPELL_EFFECT_TRIGGER_SPELL_WITH_VALUE
    &Spell::EffectApplyAreaAura,                            //143 SPELL_EFFECT_APPLY_AREA_AURA_OWNER
    &Spell::EffectKnockBackFromPosition,                    //144 SPELL_EFFECT_KNOCKBACK_FROM_POSITION
    &Spell::EffectGravityPull,                              //145 SPELL_EFFECT_GRAVITY_PULL                     Black Hole Effect
    &Spell::EffectActivateRune,                             //146 SPELL_EFFECT_ACTIVATE_RUNE
    &Spell::EffectQuestFail,                                //147 SPELL_EFFECT_QUEST_FAIL               quest fail
    &Spell::EffectNULL,                                     //148 SPELL_EFFECT_148                      single spell: Inflicts Fire damage to an enemy.
    &Spell::EffectCharge2,                                  //149 SPELL_EFFECT_CHARGE2                  swoop
    &Spell::EffectQuestOffer,                               //150 SPELL_EFFECT_QUEST_OFFER
    &Spell::EffectTriggerRitualOfSummoning,                 //151 SPELL_EFFECT_TRIGGER_SPELL_2
    &Spell::EffectNULL,                                     //152 SPELL_EFFECT_152                      summon Refer-a-Friend
    &Spell::EffectNULL,                                     //153 SPELL_EFFECT_CREATE_PET               misc value is creature entry
    &Spell::EffectTeachTaxiNode,                            //154 SPELL_EFFECT_TEACH_TAXI_NODE          single spell: Teach River's Heart Taxi Path
    &Spell::EffectTitanGrip,                                //155 SPELL_EFFECT_TITAN_GRIP Allows you to equip two-handed axes, maces and swords in one hand, but you attack $49152s1% slower than normal.
    &Spell::EffectEnchantItemPrismatic,                     //156 SPELL_EFFECT_ENCHANT_ITEM_PRISMATIC
    &Spell::EffectCreateItem2,                              //157 SPELL_EFFECT_CREATE_ITEM_2            create item or create item template and replace by some randon spell loot item
    &Spell::EffectMilling,                                  //158 SPELL_EFFECT_MILLING                  milling
    &Spell::EffectRenamePet,                                //159 SPELL_EFFECT_ALLOW_RENAME_PET         allow rename pet once again
    &Spell::EffectNULL,                                     //160 SPELL_EFFECT_160                      single spell: Nerub'ar Web Random Unit
    &Spell::EffectSpecCount,                                //161 SPELL_EFFECT_TALENT_SPEC_COUNT        second talent spec (learn/revert)
    &Spell::EffectActivateSpec,                             //162 SPELL_EFFECT_TALENT_SPEC_SELECT       activate primary/secondary spec
    &Spell::EffectUnused,                                   //163 unused in 3.3.5a
    &Spell::EffectCancelAura,                               //164 SPELL_EFFECT_CANCEL_AURA
};

/**
 * @brief Handles a no-op spell effect used only as a marker.
 *
 * @param eff_idx Unused effect index.
 */
void Spell::EffectEmpty(SpellEffectIndex /*eff_idx*/)
{
    // NOT NEED ANY IMPLEMENTATION CODE, EFFECT POSISBLE USED AS MARKER OR CLIENT INFORM
}

/**
 * @brief Handles a legacy null spell effect placeholder.
 *
 * @param eff_idx Unused effect index.
 */
void Spell::EffectNULL(SpellEffectIndex /*eff_idx*/)
{
    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "WORLD: Spell Effect DUMMY");
}

/**
 * @brief Handles an unused spell effect slot.
 *
 * @param eff_idx Unused effect index.
 */
void Spell::EffectUnused(SpellEffectIndex /*eff_idx*/)
{
    // NOT USED BY ANY SPELL OR USELESS OR IMPLEMENTED IN DIFFERENT WAY IN MANGOS
}




















































/**
 * @brief Awards a fixed amount of honor to a player target.
 *
 * @param eff_idx Unused effect index.
 */
void Spell::EffectAddHonor(SpellEffectIndex /*eff_idx*/)
{
    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    // not scale value for item based reward (/10 value expected)
    if (m_CastItem)
    {
        ((Player*)unitTarget)->RewardHonor(NULL, 1, float(damage / 10));
        DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "SpellEffect::AddHonor (spell_id %u) rewards %d honor points (item %u) for player: %u", m_spellInfo->Id, damage / 10, m_CastItem->GetEntry(), ((Player*)unitTarget)->GetGUIDLow());
        return;
    }

    // do not allow to add too many honor for player (50 * 21) = 1040 at level 70, or (50 * 31) = 1550 at level 80
    if (damage <= 50)
    {
        float honor_reward = MaNGOS::Honor::hk_honor_at_level(unitTarget->getLevel(), damage);
        ((Player*)unitTarget)->RewardHonor(NULL, 1, honor_reward);
        DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "SpellEffect::AddHonor (spell_id %u) rewards %f honor points (scale) to player: %u", m_spellInfo->Id, honor_reward, ((Player*)unitTarget)->GetGUIDLow());
    }
    else
    {
        // maybe we have correct honor_gain in damage already
        ((Player*)unitTarget)->RewardHonor(NULL, 1, (float)damage);
        sLog.outError("SpellEffect::AddHonor (spell_id %u) rewards %u honor points (non scale) for player: %u", m_spellInfo->Id, damage, ((Player*)unitTarget)->GetGUIDLow());
    }
}















































































