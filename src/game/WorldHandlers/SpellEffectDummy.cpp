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
 * @file SpellEffectDummy.cpp
 * @brief Cohesion split of SpellEffects.cpp -- EffectDummy handler.
 *        Same `Spell` class; no behaviour change.
 */

#include <iterator>
#include "Platform/Define.h"
#include "Common/TimeConstants.h"
#include "Utilities/MathDefines.h"
#include <cstdlib>
#include <vector>
#include <list>
#include <algorithm>
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

/**
 * @brief Executes spell-specific dummy effect behavior.
 *
 * @param eff_idx The dummy effect index.
 */
void Spell::EffectDummy(SpellEffectIndex eff_idx)
{
    if (!unitTarget && !gameObjTarget && !itemTarget)
    {
        return;
    }

    // selection by spell family
    switch (m_spellInfo->SpellClassSet)
    {
        case SPELLFAMILY_GENERIC:
        {
            switch (m_spellInfo->ID)
            {
                case 3360:                                  // Curse of the Eye
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    uint32 spell_id = (unitTarget->getGender() == GENDER_MALE) ? 10651 : 10653;

                    m_caster->CastSpell(unitTarget, spell_id, true);
                    return;
                }
                case 7671:                                  // Transformation (human<->worgen)
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // Transform Visual
                    unitTarget->CastSpell(unitTarget, 24085, true);
                    return;
                }
                case 8063:                                  // Deviate Fish
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    uint32 spell_id = 0;
                    switch (urand(1, 5))
                    {
                        case 1: spell_id = 8064; break;     // Sleepy
                        case 2: spell_id = 8065; break;     // Invigorate
                        case 3: spell_id = 8066; break;     // Shrink
                        case 4: spell_id = 8067; break;     // Party Time!
                        case 5: spell_id = 8068; break;     // Healthy Spirit
                    }
                    m_caster->CastSpell(m_caster, spell_id, true, NULL);
                    return;
                }
                case 8213:                                  // Savory Deviate Delight
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    uint32 spell_id = 0;
                    switch (urand(1, 2))
                    {
                            // Flip Out - ninja
                        case 1: spell_id = (m_caster->getGender() == GENDER_MALE ? 8219 : 8220); break;
                            // Yaaarrrr - pirate
                        case 2: spell_id = (m_caster->getGender() == GENDER_MALE ? 8221 : 8222); break;
                    }

                    m_caster->CastSpell(m_caster, spell_id, true, NULL);
                    return;
                }
                case 9976:                                  // Polly Eats the E.C.A.C.
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    // Summon Polly Jr.
                    unitTarget->CastSpell(unitTarget, 9998, true);

                    ((Creature*)unitTarget)->ForcedDespawn(100);
                    return;
                }
                case 10254:                                 // Stone Dwarf Awaken Visual
                {
                    if (m_caster->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    // see spell 10255 (aura dummy)
                    m_caster->clearUnitState(UNIT_STAT_ROOT);
                    m_caster->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    return;
                }
                case 13120:                                 // net-o-matic
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    uint32 spell_id = 0;

                    uint32 roll = urand(0, 99);

                    if (roll < 2)                           // 2% for 30 sec self root (off-like chance unknown)
                    {
                        spell_id = 16566;
                    }
                    else if (roll < 4)                      // 2% for 20 sec root, charge to target (off-like chance unknown)
                    {
                        spell_id = 13119;
                    }
                    else                                    // normal root
                    {
                        spell_id = 13099;
                    }

                    m_caster->CastSpell(unitTarget, spell_id, true, NULL);
                    return;
                }
                case 13489:
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 14744, true);
                    return;
                }
                case 13567:                                 // Dummy Trigger
                {
                    // can be used for different aura triggering, so select by aura
                    if (!m_triggeredByAuraSpell || !unitTarget)
                    {
                        return;
                    }

                    switch (m_triggeredByAuraSpell->ID)
                    {
                        case 26467:                         // Persistent Shield
                            m_caster->CastCustomSpell(unitTarget, 26470, &damage, NULL, NULL, true);
                            break;
                        default:
                            sLog.outError("EffectDummy: Non-handled case for spell 13567 for triggered aura %u", m_triggeredByAuraSpell->ID);
                            break;
                    }
                    return;
                }
                case 14537:                                 // Six Demon Bag
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    Unit* newTarget = unitTarget;
                    uint32 spell_id = 0;
                    uint32 roll = urand(0, 99);
                    if (roll < 25)                          // Fireball (25% chance)
                    {
                        spell_id = 15662;
                    }
                    else if (roll < 50)                     // Frostbolt (25% chance)
                    {
                        spell_id = 11538;
                    }
                    else if (roll < 70)                     // Chain Lighting (20% chance)
                    {
                        spell_id = 21179;
                    }
                    else if (roll < 77)                     // Polymorph (10% chance, 7% to target)
                    {
                        spell_id = 14621;
                    }
                    else if (roll < 80)                     // Polymorph (10% chance, 3% to self, backfire)
                    {
                        spell_id = 14621;
                        newTarget = m_caster;
                    }
                    else if (roll < 95)                     // Enveloping Winds (15% chance)
                    {
                        spell_id = 25189;
                    }
                    else                                    // Summon Felhund minion (5% chance)
                    {
                        spell_id = 14642;
                        newTarget = m_caster;
                    }

                    m_caster->CastSpell(newTarget, spell_id, true, m_CastItem);
                    return;
                }
                case 15998:                                 // Capture Worg Pup
                case 29435:                                 // Capture Female Kaliri Hatchling
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    Creature* creatureTarget = (Creature*)unitTarget;

                    creatureTarget->ForcedDespawn();
                    return;
                }
                case 16589:                                 // Noggenfogger Elixir
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    uint32 spell_id = 0;
                    switch (urand(1, 3))
                    {
                        case 1: spell_id = 16595; break;
                        case 2: spell_id = 16593; break;
                        default: spell_id = 16591; break;
                    }

                    m_caster->CastSpell(m_caster, spell_id, true, NULL);
                    return;
                }
                case 17009:                                 // Voodoo
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    uint32 spell_id = 0;
                    switch (urand(0, 6))
                    {
                        case 0: spell_id = 16707; break;    // Hex
                        case 1: spell_id = 16708; break;    // Hex
                        case 2: spell_id = 16709; break;    // Hex
                        case 3: spell_id = 16711; break;    // Grow
                        case 4: spell_id = 16712; break;    // Special Brew
                        case 5: spell_id = 16713; break;    // Ghostly
                        case 6: spell_id = 16716; break;    // Launch
                    }

                    m_caster->CastSpell(unitTarget, spell_id, true, NULL, NULL, m_originalCasterGUID, m_spellInfo);
                    return;
                }
                case 17251:                                 // Spirit Healer Res
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    Unit* caster = GetAffectiveCaster();

                    if (caster && caster->GetTypeId() == TYPEID_PLAYER)
                    {
                        WorldPacket data(SMSG_SPIRIT_HEALER_CONFIRM, 8);
                        data << unitTarget->GetObjectGuid();
                        ((Player*)caster)->GetSession()->SendPacket(&data);
                    }
                    return;
                }
                case 17271:                                 // Test Fetid Skull
                {
                    if (!itemTarget && m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    uint32 spell_id = urand(0, 1)
                                      ? 17269               // Create Resonating Skull
                                      : 17270;              // Create Bone Dust

                    m_caster->CastSpell(m_caster, spell_id, true, NULL);
                    return;
                }
                case 17770:                                 // Wolfshead Helm Energy
                {
                    m_caster->CastSpell(m_caster, 29940, true, NULL);
                    return;
                }
                case 17950:                                 // Shadow Portal
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // Shadow Portal
                    const uint32 spell_list[6] = {17863, 17939, 17943, 17944, 17946, 17948};

                    m_caster->CastSpell(unitTarget, spell_list[urand(0, 5)], true);
                    return;
                }
                case 19395:                                 // Gordunni Trap
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, urand(0, 1) ? 19394 : 11756, true);
                    return;
                }
                case 19411:                                 // Lava Bomb
                case 20474:                                 // Lava Bomb
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // Hack alert!
                    // This dummy are expected to cast spell 20494 to summon GO entry 177704
                    // Spell does not exist client side, so we have to make a hack, creating the GO (SPELL_EFFECT_SUMMON_OBJECT_WILD)
                    // Spell should appear in both SMSG_SPELL_START/GO and SMSG_SPELLLOGEXECUTE

                    // For later, creating custom spell
                    // _START: packguid: target, cast flags: 0xB, TARGET_FLAG_SELF
                    // _GO: packGuid: target, cast flags: 0x4309, TARGET_FLAG_DEST_LOCATION
                    // LOG: spell: 20494, effect, pguid: goguid

                    GameObject* pGameObj = new GameObject;

                    Map* map = unitTarget->GetMap();

                    if (!pGameObj->Create(map->GenerateLocalLowGuid(HIGHGUID_GAMEOBJECT), 177704,
                                          map, m_caster->GetPhaseMask(),
                                          unitTarget->GetPositionX(), unitTarget->GetPositionY(), unitTarget->GetPositionZ(),
                                          unitTarget->GetOrientation()))
                    {
                        delete pGameObj;
                        return;
                    }

                    DEBUG_LOG("Gameobject, create custom in SpellEffects.cpp EffectDummy");

                    // Expect created without owner, but with level from _template
                    pGameObj->SetRespawnTime(MINUTE / 2);
                    pGameObj->SetUInt32Value(GAMEOBJECT_LEVEL, pGameObj->GetGOInfo()->trap.level);
                    pGameObj->SetSpellId(m_spellInfo->ID);

                    map->Add(pGameObj);
                    pGameObj->AIM_Initialize();

                    return;
                }
                case 19869:                                 // Dragon Orb
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER || unitTarget->HasAura(23958))
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 19832, true);
                    return;
                }
                case 20037:                                 // Explode Orb Effect
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 20038, true);
                    return;
                }
                case 20577:                                 // Cannibalize
                {
                    if (unitTarget)
                    {
                        m_caster->CastSpell(m_caster, 20578, false, NULL);
                    }
                    return;
                }
                case 21147:                                 // Arcane Vacuum
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // Spell used by Azuregos to teleport all the players to him
                    // This also resets the target threat
                    if (m_caster->GetThreatManager().getThreat(unitTarget))
                    {
                        m_caster->GetThreatManager().modifyThreatPercent(unitTarget, -100);
                    }

                    // cast summon player
                    m_caster->CastSpell(unitTarget, 21150, true);

                    return;
                }
                case 23019:                                 // Crystal Prison Dummy DND
                {
                    if (!unitTarget || !unitTarget->IsAlive() || unitTarget->GetTypeId() != TYPEID_UNIT || ((Creature*)unitTarget)->IsPet())
                    {
                        return;
                    }

                    Creature* creatureTarget = (Creature*)unitTarget;
                    if (creatureTarget->IsPet())
                    {
                        return;
                    }

                    GameObject* pGameObj = new GameObject;

                    Map* map = creatureTarget->GetMap();

                    // create before death for get proper coordinates
                    if (!pGameObj->Create(map->GenerateLocalLowGuid(HIGHGUID_GAMEOBJECT), 179644, map, m_caster->GetPhaseMask(),
                                          creatureTarget->GetPositionX(), creatureTarget->GetPositionY(), creatureTarget->GetPositionZ(),
                                          creatureTarget->GetOrientation()))
                    {
                        delete pGameObj;
                        return;
                    }

                    pGameObj->SetRespawnTime(creatureTarget->GetRespawnTime() - time(NULL));
                    pGameObj->SetOwnerGuid(m_caster->GetObjectGuid());
                    pGameObj->SetUInt32Value(GAMEOBJECT_LEVEL, m_caster->getLevel());
                    pGameObj->SetSpellId(m_spellInfo->ID);

                    creatureTarget->ForcedDespawn();

                    DEBUG_LOG("AddObject at SpellEfects.cpp EffectDummy");
                    map->Add(pGameObj);
                    pGameObj->AIM_Initialize();

                    return;
                }
                case 23074:                                 // Arcanite Dragonling
                {
                    if (!m_CastItem)
                    {
                        return;
                    }

                    m_caster->CastSpell(m_caster, 19804, true, m_CastItem);
                    return;
                }
                case 23075:                                 // Mithril Mechanical Dragonling
                {
                    if (!m_CastItem)
                    {
                        return;
                    }

                    m_caster->CastSpell(m_caster, 12749, true, m_CastItem);
                    return;
                }
                case 23076:                                 // Mechanical Dragonling
                {
                    if (!m_CastItem)
                    {
                        return;
                    }

                    m_caster->CastSpell(m_caster, 4073, true, m_CastItem);
                    return;
                }
                case 23133:                                 // Gnomish Battle Chicken
                {
                    if (!m_CastItem)
                    {
                        return;
                    }

                    m_caster->CastSpell(m_caster, 13166, true, m_CastItem);
                    return;
                }
                case 23138:                                 // Gate of Shazzrah
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // Effect probably include a threat change, but it is unclear if fully
                    // reset or just forced upon target for teleport (SMSG_HIGHEST_THREAT_UPDATE)

                    // Gate of Shazzrah
                    m_caster->CastSpell(unitTarget, 23139, true);
                    return;
                }
                case 23448:                                 // Transporter Arrival - Ultrasafe Transporter: Gadgetzan - backfires
                {
                    int32 r = irand(0, 119);
                    if (r < 20)                             // Transporter Malfunction - 1/6 polymorph
                    {
                        m_caster->CastSpell(m_caster, 23444, true);
                    }
                    else if (r < 100)                       // Evil Twin               - 4/6 evil twin
                    {
                        m_caster->CastSpell(m_caster, 23445, true);
                    }
                    else                                    // Transporter Malfunction - 1/6 miss the target
                    {
                        m_caster->CastSpell(m_caster, 36902, true);
                    }

                    return;
                }
                case 23453:                                 // Gnomish Transporter - Ultrasafe Transporter: Gadgetzan
                {
                    if (roll_chance_i(50))                  // Gadgetzan Transporter         - success
                    {
                        m_caster->CastSpell(m_caster, 23441, true);
                    }
                    else                                    // Gadgetzan Transporter Failure - failure
                    {
                        m_caster->CastSpell(m_caster, 23446, true);
                    }

                    return;
                }
                case 23645:                                 // Hourglass Sand
                    m_caster->RemoveAurasDueToSpell(23170); // Brood Affliction: Bronze
                    return;
                case 23725:                                 // Gift of Life (warrior bwl trinket)
                    m_caster->CastSpell(m_caster, 23782, true);
                    m_caster->CastSpell(m_caster, 23783, true);
                    return;
                case 24930:                                 // Hallow's End Treat
                {
                    uint32 spell_id = 0;

                    switch (urand(1, 4))
                    {
                        case 1: spell_id = 24924; break;    // Larger and Orange
                        case 2: spell_id = 24925; break;    // Skeleton
                        case 3: spell_id = 24926; break;    // Pirate
                        case 4: spell_id = 24927; break;    // Ghost
                    }

                    m_caster->CastSpell(m_caster, spell_id, true);
                    return;
                }
                case 25860:                                 // Reindeer Transformation
                {
                    if (!m_caster->HasAuraType(SPELL_AURA_MOUNTED))
                    {
                        return;
                    }

                    float flyspeed = m_caster->GetSpeedRate(MOVE_FLIGHT);
                    float speed = m_caster->GetSpeedRate(MOVE_RUN);

                    m_caster->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);

                    // 5 different spells used depending on mounted speed and if mount can fly or not
                    if (flyspeed >= 4.1f)
                        // Flying Reindeer
                        m_caster->CastSpell(m_caster, 44827, true); // 310% flying Reindeer
                    else if (flyspeed >= 3.8f)
                        // Flying Reindeer
                        m_caster->CastSpell(m_caster, 44825, true); // 280% flying Reindeer
                    else if (flyspeed >= 1.6f)
                        // Flying Reindeer
                        m_caster->CastSpell(m_caster, 44824, true); // 60% flying Reindeer
                    else if (speed >= 2.0f)
                        // Reindeer
                    {
                        m_caster->CastSpell(m_caster, 25859, true);  // 100% ground Reindeer
                    }
                    else
                        // Reindeer
                    {
                        m_caster->CastSpell(m_caster, 25858, true);  // 60% ground Reindeer
                    }

                    return;
                }
                case 26074:                                 // Holiday Cheer
                    // implemented at client side
                    return;
                case 28006:                                 // Arcane Cloaking
                {
                    if (unitTarget && unitTarget->GetTypeId() == TYPEID_PLAYER)
                        // Naxxramas Entry Flag Effect DND
                        m_caster->CastSpell(unitTarget, 29294, true);

                    return;
                }
                case 29126:                                 // Cleansing Flames (Darnassus)
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 29099, true);
                    return;
                }
                case 29135:                                 // Cleansing Flames (Ironforge)
                case 29136:                                 // Cleansing Flames (Orgrimmar)
                case 29137:                                 // Cleansing Flames (Stormwind)
                case 29138:                                 // Cleansing Flames (Thunder Bluff)
                case 29139:                                 // Cleansing Flames (Undercity)
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    uint32 spellIDs[] = {29102, 29130, 29101, 29132, 29133};
                    unitTarget->CastSpell(unitTarget, spellIDs[m_spellInfo->ID - 29135], true);
                    return;
                }
                case 29200:                                 // Purify Helboar Meat
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    uint32 spell_id = roll_chance_i(50)
                                      ? 29277               // Summon Purified Helboar Meat
                                      : 29278;              // Summon Toxic Helboar Meat

                    m_caster->CastSpell(m_caster, spell_id, true, NULL);
                    return;
                }
                case 29858:                                 // Soulshatter
                {
                    if (unitTarget && unitTarget->GetTypeId() == TYPEID_UNIT && unitTarget->IsHostileTo(m_caster))
                    {
                        m_caster->CastSpell(unitTarget, 32835, true);
                    }

                    return;
                }
                case 29969:                                 // Summon Blizzard
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 29952, true, NULL, NULL, m_caster->GetObjectGuid());
                    return;
                }
                case 29979:                                 // Massive Magnetic Pull
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 30010, true);
                    return;
                }
                case 30004:                                 // Flame Wreath
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 29946, true);
                    return;
                }
                case 30458:                                 // Nigh Invulnerability
                {
                    if (!m_CastItem)
                    {
                        return;
                    }

                    if (roll_chance_i(86))                  // Nigh-Invulnerability   - success
                    {
                        m_caster->CastSpell(m_caster, 30456, true, m_CastItem);
                    }
                    else                                    // Complete Vulnerability - backfire in 14% casts
                    {
                        m_caster->CastSpell(m_caster, 30457, true, m_CastItem);
                    }

                    return;
                }
                case 30507:                                 // Poultryizer
                {
                    if (!m_CastItem)
                    {
                        return;
                    }

                    if (roll_chance_i(80))                  // Poultryized! - success
                    {
                        m_caster->CastSpell(unitTarget, 30501, true, m_CastItem);
                    }
                    else                                    // Poultryized! - backfire 20%
                    {
                        m_caster->CastSpell(unitTarget, 30504, true, m_CastItem);
                    }

                    return;
                }
                case 32146:                                 // Liquid Fire
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT || m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    ((Player*)m_caster)->KilledMonsterCredit(unitTarget->GetEntry(), unitTarget->GetObjectGuid());
                    ((Creature*)unitTarget)->ForcedDespawn();
                    return;
                }
                case 32300:                                 // Focus Fire
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, unitTarget->GetMap()->IsRegularDifficulty() ? 32302 : 38382, true);
                    return;
                }
                case 32312:                                 // Move 1 (Chess event AI short distance move)
                case 37388:                                 // Move 2 (Chess event AI long distance move)
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    // cast generic move spell
                    m_caster->CastSpell(unitTarget, 30012, true);
                    return;
                }
                case 33060:                                 // Make a Wish
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    uint32 spell_id = 0;

                    switch (urand(1, 5))
                    {
                        case 1: spell_id = 33053; break;    // Mr Pinchy's Blessing
                        case 2: spell_id = 33057; break;    // Summon Mighty Mr. Pinchy
                        case 3: spell_id = 33059; break;    // Summon Furious Mr. Pinchy
                        case 4: spell_id = 33062; break;    // Tiny Magical Crawdad
                        case 5: spell_id = 33064; break;    // Mr. Pinchy's Gift
                    }

                    m_caster->CastSpell(m_caster, spell_id, true, NULL);
                    return;
                }
                case 34803:                                 // Summon Reinforcements
                {
                    m_caster->CastSpell(m_caster, 34810, true); // Summon 20083 behind of the caster
                    m_caster->CastSpell(m_caster, 34817, true); // Summon 20078 right of the caster
                    m_caster->CastSpell(m_caster, 34818, true); // Summon 20078 left of the caster
                    m_caster->CastSpell(m_caster, 34819, true); // Summon 20078 front of the caster
                    return;
                }
                case 36677:                                 // Chaos Breath
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    uint32 possibleSpells[] = {36693, 36694, 36695, 36696, 36697, 36698, 36699, 36700} ;
                    std::vector<uint32> spellPool(possibleSpells, possibleSpells + countof(possibleSpells));

                    //std::random_shuffle(spellPool.begin(), spellPool.end());
                    std::mt19937 rng(std::time(nullptr));
                    std::shuffle(spellPool.begin(), spellPool.end(), rng);

                    for (uint8 i = 0; i < (m_caster->GetMap()->IsRegularDifficulty() ? 2 : 4); ++i)
                    {
                        m_caster->CastSpell(m_caster, spellPool[i], true);
                    }

                    return;
                }
                case 33923:                                 // Sonic Boom
                case 38796:                                 // Sonic Boom (heroic)
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, m_spellInfo->ID == 33923 ? 33666 : 38795, true);
                    return;
                }
                case 35745:                                 // Socrethar's Stone
                {
                    uint32 spell_id;
                    switch (m_caster->GetAreaId())
                    {
                        case 3900: spell_id = 35743; break; // Socrethar Portal
                        case 3742: spell_id = 35744; break; // Socrethar Portal
                        default: return;
                    }

                    m_caster->CastSpell(m_caster, spell_id, true);
                    return;
                }
                case 37674:                                 // Chaos Blast
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    int32 basepoints0 = 100;
                    m_caster->CastCustomSpell(unitTarget, 37675, &basepoints0, NULL, NULL, true);
                    return;
                }
                case 39189:                                 // Sha'tari Torch
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT || m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    // Flames
                    if (unitTarget->HasAura(39199))
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 39199, true);
                    ((Player*)m_caster)->KilledMonsterCredit(unitTarget->GetEntry(), unitTarget->GetObjectGuid());
                    ((Creature*)unitTarget)->ForcedDespawn(10000);
                    return;
                }
                case 39635:                                 // Throw Glaive (first)
                case 39849:                                 // Throw Glaive (second)
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 41466, true, NULL, NULL, m_caster->GetObjectGuid());
                    return;
                }
                case 40802:                                 // Mingo's Fortune Generator (Mingo's Fortune Giblets)
                {
                    // selecting one from Bloodstained Fortune item
                    uint32 newitemid;
                    switch (urand(1, 20))
                    {
                        case 1:  newitemid = 32688; break;
                        case 2:  newitemid = 32689; break;
                        case 3:  newitemid = 32690; break;
                        case 4:  newitemid = 32691; break;
                        case 5:  newitemid = 32692; break;
                        case 6:  newitemid = 32693; break;
                        case 7:  newitemid = 32700; break;
                        case 8:  newitemid = 32701; break;
                        case 9:  newitemid = 32702; break;
                        case 10: newitemid = 32703; break;
                        case 11: newitemid = 32704; break;
                        case 12: newitemid = 32705; break;
                        case 13: newitemid = 32706; break;
                        case 14: newitemid = 32707; break;
                        case 15: newitemid = 32708; break;
                        case 16: newitemid = 32709; break;
                        case 17: newitemid = 32710; break;
                        case 18: newitemid = 32711; break;
                        case 19: newitemid = 32712; break;
                        case 20: newitemid = 32713; break;
                        default:
                            return;
                    }

                    DoCreateItem(eff_idx, newitemid);
                    return;
                }
                case 40834:                                 // Agonizing Flames
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 40932, true);
                    return;
                }
                case 40869:                                 // Fatal Attraction
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 41001, true);
                    return;
                }
                case 40962:                                 // Blade's Edge Terrace Demon Boss Summon Branch
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    uint32 spell_id = 0;
                    switch (urand(1, 4))
                    {
                        case 1: spell_id = 40957; break;    // Blade's Edge Terrace Demon Boss Summon 1
                        case 2: spell_id = 40959; break;    // Blade's Edge Terrace Demon Boss Summon 2
                        case 3: spell_id = 40960; break;    // Blade's Edge Terrace Demon Boss Summon 3
                        case 4: spell_id = 40961; break;    // Blade's Edge Terrace Demon Boss Summon 4
                    }
                    unitTarget->CastSpell(unitTarget, spell_id, true);
                    return;
                }
                case 41283:                                 // Abyssal Toss
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    m_caster->SummonCreature(23416, unitTarget->GetPositionX(), unitTarget->GetPositionY(), unitTarget->GetPositionZ(), 0, TEMPSPAWN_TIMED_OR_CORPSE_DESPAWN, 30000);
                    return;
                }
                case 41333:                                 // Empyreal Equivalency
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // Equilize the health of all targets based on the corresponding health percent
                    float health_diff = (float)unitTarget->GetMaxHealth() / (float)m_caster->GetMaxHealth();
                    unitTarget->SetHealth(m_caster->GetHealth() * health_diff);
                    return;
                }
                case 42287:                                 // Salvage Wreckage
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    if (roll_chance_i(66))
                    {
                        m_caster->CastSpell(m_caster, 42289, true, m_CastItem);
                    }
                    else
                    {
                        m_caster->CastSpell(m_caster, 42288, true);
                    }

                    return;
                }
                case 42485:                                 // End of Ooze Channel
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    unitTarget->CastSpell(m_caster, 42486, true);

                    // There is no known spell to kill the target
                    unitTarget->DealDamage(unitTarget, unitTarget->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                    return;
                }
                case 42489:                                 // Cast Ooze Zap When Energized
                {
                    // only process first effect
                    // the logic is described by spell 42488 - Caster Spell 1 Only if Aura 2 Is On Caster (not used here)
                    if (eff_idx != EFFECT_INDEX_0)
                    {
                        return;
                    }

                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT || !m_caster->HasAura(m_spellInfo->CalculateSimpleValue(EFFECT_INDEX_1)))
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    ((Creature*)unitTarget)->AI()->AttackStart(m_caster);
                    return;
                }
                 case 42628:                                 // Fire Bomb (throw)
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 42629, true);
                    return;
                }
                case 42631:                                 // Fire Bomb (explode)
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    unitTarget->RemoveAurasDueToSpell(42629);
                    unitTarget->CastSpell(unitTarget, 42630, true);

                    // despawn the bomb after exploding
                    ((Creature*)unitTarget)->ForcedDespawn(3000);
                    return;
                }
                case 42793:                                 // Burn Body
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT || m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    Creature* pCreature = (Creature*)unitTarget;

                    // Spell can be used in combat and may affect different target than the expected.
                    // If target is not the expected we need to prevent this effect.
                    if (pCreature->HasLootRecipient() || pCreature->IsInCombat())
                    {
                        return;
                    }

                    // set loot recipient, prevent re-use same target
                    pCreature->SetLootRecipient(m_caster);

                    pCreature->ForcedDespawn(m_duration);

                    // EFFECT_INDEX_2 has 0 miscvalue for effect 134, doing the killcredit here instead (only one known case exist where 0)
                    ((Player*)m_caster)->KilledMonster(pCreature->GetCreatureInfo(), pCreature->GetObjectGuid());
                    return;
                }
                case 43014:                                 // Despawn Self
                {
                    if (m_caster->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    ((Creature*)m_caster)->ForcedDespawn();
                    return;
                }
                case 43036:                                 // Dismembering Corpse
                {
                    if (!unitTarget || m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    if (unitTarget->HasAura(43059, EFFECT_INDEX_0))
                    {
                        return;
                    }

                    unitTarget->CastSpell(m_caster, 43037, true);
                    unitTarget->CastSpell(unitTarget, 43059, true);
                    return;
                }
                case 43069:                                 // Towers of Certain Doom: Skorn Cannonfire
                {
                    // Towers of Certain Doom: Tower Caster Instakill
                    m_caster->CastSpell(m_caster, 43072, true);
                    return;
                }
                case 43096:                                 // Summon All Players
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 43097, true);
                    return;
                }
                case 43144:                                 // Hatch All Eggs
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 42493, true, NULL, NULL, m_caster->GetObjectGuid());
                    return;
                }
                case 43209:                                 // Place Ram Meat
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // Self Visual - Sleep Until Cancelled (DND)
                    unitTarget->RemoveAurasDueToSpell(6606);
                    return;
                }
                case 43498:                                 // Siphon Soul
                {
                    // This spell should cast the next spell only for one (player)target, however it should hit multiple targets, hence this kind of implementation
                    if (!unitTarget || m_UniqueTargetInfo.rbegin()->targetGUID != unitTarget->GetObjectGuid())
                    {
                        return;
                    }

                    std::vector<Unit*> possibleTargets;
                    possibleTargets.reserve(m_UniqueTargetInfo.size());
                    for (TargetList::const_iterator itr = m_UniqueTargetInfo.begin(); itr != m_UniqueTargetInfo.end(); ++itr)
                    {
                        // Skip Non-Players
                        if (!itr->targetGUID.IsPlayer())
                        {
                            continue;
                        }

                        if (Unit* target = m_caster->GetMap()->GetPlayer(itr->targetGUID))
                        {
                            possibleTargets.push_back(target);
                        }
                    }

                    // Cast Siphon Soul channeling spell
                    if (!possibleTargets.empty())
                    {
                        m_caster->CastSpell(possibleTargets[urand(0, possibleTargets.size() - 1)], 43501, false);
                    }

                    return;
                }
                case 43572:                                 // Send Them Packing: On /Raise Emote Dummy to Player
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // m_caster (creature) should start walking back to it's "home" here, no clear way how to do that

                    // Send Them Packing: On Successful Dummy Spell Kill Credit
                    m_caster->CastSpell(unitTarget, 42721, true);
                    return;
                }
                // Demon Broiled Surprise
                case 43723:
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    ((Player*)m_caster)->CastSpell(unitTarget, 43753, true, m_CastItem, NULL, m_originalCasterGUID, m_spellInfo);
                    return;
                }
                case 43882:                                 // Scourging Crystal Controller Dummy
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    // see spell dummy 50133
                    unitTarget->RemoveAurasDueToSpell(43874);
                    return;
                }
                case 44454:                                 // Tasty Reef Fish
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 44455, true, m_CastItem);
                    return;
                }
                case 44869:                                 // Spectral Blast
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    // If target has spectral exhaustion or spectral realm aura return
                    if (unitTarget->HasAura(44867) || unitTarget->HasAura(46021))
                    {
                        return;
                    }

                    // Cast the spectral realm effect spell, visual spell and spectral blast rift summoning
                    unitTarget->CastSpell(unitTarget, 44866, true, NULL, NULL, m_caster->GetObjectGuid());
                    unitTarget->CastSpell(unitTarget, 46648, true, NULL, NULL, m_caster->GetObjectGuid());
                    unitTarget->CastSpell(unitTarget, 44811, true);
                    return;
                }
                case 44875:                                 // Complete Raptor Capture
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    Creature* creatureTarget = (Creature*)unitTarget;

                    creatureTarget->ForcedDespawn();

                    // cast spell Raptor Capture Credit
                    m_caster->CastSpell(m_caster, 42337, true, NULL);
                    return;
                }
                case 44997:                                 // Converting Sentry
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    Creature* creatureTarget = (Creature*)unitTarget;

                    creatureTarget->ForcedDespawn();

                    // Converted Sentry Credit
                    m_caster->CastSpell(m_caster, 45009, true);
                    return;
                }
                case 45030:                                 // Impale Emissary
                {
                    // Emissary of Hate Credit
                    m_caster->CastSpell(m_caster, 45088, true);
                    return;
                }
                case 45449:                                // Arcane Prisoner Rescue
                {
                    uint32 spellId = 0;
                    switch (rand() % 2)
                    {
                        case 0: spellId = 45446; break;    // Summon Arcane Prisoner - Male
                        case 1: spellId = 45448; break;    // Summon Arcane Prisoner - Female
                    }
                    // Spawn
                    m_caster->CastSpell(m_caster, spellId, true);

                    if (!unitTarget) return;
                    // Arcane Prisoner Kill Credit
                    unitTarget->CastSpell(m_caster, 45456, true);

                    break;
                }
                case 45583:                                 // Throw Gnomish Grenade
                {
                    if (!unitTarget || m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    ((Player*)m_caster)->KilledMonsterCredit(unitTarget->GetEntry(), unitTarget->GetObjectGuid());

                    // look for gameobject within max spell range of unitTarget, and respawn if found

                    // big fire
                    GameObject* pGo = NULL;

                    float fMaxDist = GetSpellMaxRange(sSpellRangeStore.LookupEntry(m_spellInfo->RangeIndex));

                    MaNGOS::NearestGameObjectEntryInPosRangeCheck go_check_big(*unitTarget, 187675, unitTarget->GetPositionX(), unitTarget->GetPositionY(), unitTarget->GetPositionZ(), fMaxDist);
                    MaNGOS::GameObjectSearcher<MaNGOS::NearestGameObjectEntryInPosRangeCheck> checker1(pGo, go_check_big);

                    Cell::VisitGridObjects(unitTarget, checker1, fMaxDist);

                    if (pGo && !pGo->isSpawned())
                    {
                        pGo->SetRespawnTime(MINUTE / 2);
                        pGo->Refresh();
                    }

                    // small fire
                    std::list<GameObject*> lList;

                    MaNGOS::GameObjectEntryInPosRangeCheck go_check_small(*unitTarget, 187676, unitTarget->GetPositionX(), unitTarget->GetPositionY(), unitTarget->GetPositionZ(), fMaxDist);
                    MaNGOS::GameObjectListSearcher<MaNGOS::GameObjectEntryInPosRangeCheck> checker2(lList, go_check_small);

                    Cell::VisitGridObjects(unitTarget, checker2, fMaxDist);

                    for (std::list<GameObject*>::iterator iter = lList.begin(); iter != lList.end(); ++iter)
                    {
                        if (!(*iter)->isSpawned())
                        {
                            (*iter)->SetRespawnTime(MINUTE / 2);
                            (*iter)->Refresh();
                        }
                    }

                    return;
                }
                case 45685:                                 // Magnataur On Death 2
                {
                    m_caster->RemoveAurasDueToSpell(45673);
                    m_caster->RemoveAurasDueToSpell(45672);
                    m_caster->RemoveAurasDueToSpell(45677);
                    m_caster->RemoveAurasDueToSpell(45681);
                    m_caster->RemoveAurasDueToSpell(45683);
                    return;
                }
                case 45958:                                 // Signal Alliance
                {
                    m_caster->CastSpell(m_caster, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    return;
                }
                case 45976:                                 // Open Portal
                case 46177:                                 // Open All Portals
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // portal visual
                    unitTarget->CastSpell(unitTarget, 45977, true);

                    // break in case additional procressing in scripting library required
                    break;
                }
                case 45980:                                 // Re-Cursive Transmatter Injection
                {
                    if (m_caster->GetTypeId() == TYPEID_PLAYER && unitTarget)
                    {
                        if (const SpellEntry* pSpell = sSpellStore.LookupEntry(46022))
                        {
                            m_caster->CastSpell(unitTarget, pSpell, true);
                            ((Player*)m_caster)->KilledMonsterCredit(pSpell->EffectMiscValue[EFFECT_INDEX_0]);
                        }

                        if (unitTarget->GetTypeId() == TYPEID_UNIT)
                        {
                            ((Creature*)unitTarget)->ForcedDespawn();
                        }
                    }

                    return;
                }
                case 45989:                                 // Summon Void Sentinel Summoner Visual
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // summon void sentinel
                    unitTarget->CastSpell(unitTarget, 45988, true);

                    return;
                }
                case 45990:                                 // Collect Oil
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    if (const SpellEntry* pSpell = sSpellStore.LookupEntry(45991))
                    {
                        unitTarget->CastSpell(unitTarget, pSpell, true);
                        ((Creature*)unitTarget)->ForcedDespawn(m_duration);
                    }

                    return;
                }
                case 46167:                                 // Planning for the Future: Create Snowfall Glade Pup Cover
                case 50918:                                 // Gluttonous Lurkers: Create Basilisk Crystals Cover
                case 50926:                                 // Gluttonous Lurkers: Create Zul'Drak Rat Cover
                case 51026:                                 // Create Drakkari Medallion Cover
                case 51592:                                 // Pickup Primordial Hatchling
                case 51961:                                 // Captured Chicken Cover
                case 55364:                                 // Create Ghoul Drool Cover
                case 61832:                                 // Rifle the Bodies: Create Magehunter Personal Effects Cover
                case 63125:                                 // Search Maloric
                case 74904:                                 // Pickup Sen'jin Frog
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT || m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    uint32 spellId = 0;

                    switch (m_spellInfo->ID)
                    {
                        case 46167: spellId = 46773; break;
                        case 50918: spellId = 50919; break;
                        case 50926: spellId = 50927; break;
                        case 51026: spellId = 50737; break;
                        case 51592: spellId = 51593; break;
                        case 51961: spellId = 51037; break;
                        case 55364: spellId = 55363; break;
                        case 61832: spellId = 47096; break;
                        case 63125: spellId = 63126; break;
                        case 74904: spellId = 74905; break;
                    }

                    if (const SpellEntry* pSpell = sSpellStore.LookupEntry(spellId))
                    {
                        unitTarget->CastSpell(m_caster, spellId, true);

                        Creature* creatureTarget = (Creature*)unitTarget;

                        if (const SpellCastTimesEntry* pCastTime = sSpellCastTimesStore.LookupEntry(pSpell->CastingTimeIndex))
                        {
                            creatureTarget->ForcedDespawn(pCastTime->Base + 1);
                        }
                    }
                    return;
                }
                case 46171:                                 // Scuttle Wrecked Flying Machine
                {
                    if (!unitTarget || m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    ((Player*)m_caster)->KilledMonsterCredit(unitTarget->GetEntry(), unitTarget->GetObjectGuid());

                    // look for gameobject within max spell range of unitTarget, and respawn if found
                    GameObject* pGo = NULL;

                    float fMaxDist = GetSpellMaxRange(sSpellRangeStore.LookupEntry(m_spellInfo->RangeIndex));

                    MaNGOS::NearestGameObjectEntryInPosRangeCheck go_check(*unitTarget, 187675, unitTarget->GetPositionX(), unitTarget->GetPositionY(), unitTarget->GetPositionZ(), fMaxDist);
                    MaNGOS::GameObjectSearcher<MaNGOS::NearestGameObjectEntryInPosRangeCheck> checker(pGo, go_check);

                    Cell::VisitGridObjects(unitTarget, checker, fMaxDist);

                    if (pGo && !pGo->isSpawned())
                    {
                        pGo->SetRespawnTime(MINUTE / 2);
                        pGo->Refresh();
                    }

                    return;
                }
                case 46292:                                 // Cataclysm Breath
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    uint32 spellId = 0;

                    switch (urand(0,7))
                    {
                        case 0: spellId = 46293; break; // Corrosive Poison
                        case 1: spellId = 46294; break; // Fevered Fatigue
                        case 2: spellId = 46295; break; // Hex
                        case 3: spellId = 46296; break; // Necrotic Poison
                        case 4: spellId = 46297; break; // Piercing Shadow
                        case 5: spellId = 46298; break; // Shrink
                        case 6: spellId = 46299; break; // Wavering Will
                        case 7: spellId = 46300; break; // Withered Touch
                    }

                    m_caster->CastSpell(unitTarget, spellId, true);
                    return;
                }
                case 46372:                                 // Ice Spear Target Picker
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 46359, true);
                    return;
                }
                case 46485:                                 // Greatmother's Soulcatcher
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    if (const SpellEntry* pSpell = sSpellStore.LookupEntry(46486))
                    {
                        m_caster->CastSpell(unitTarget, pSpell, true);

                        if (const SpellEntry* pSpellCredit = sSpellStore.LookupEntry(pSpell->EffectTriggerSpell[EFFECT_INDEX_0]))
                        {
                            ((Player*)m_caster)->KilledMonsterCredit(pSpellCredit->EffectMiscValue[EFFECT_INDEX_0]);
                        }

                        ((Creature*)unitTarget)->ForcedDespawn();
                    }

                    return;
                }
                case 46606:                                 // Plague Canister Dummy
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    unitTarget->CastSpell(m_caster, 43160, true);
                    unitTarget->SetDeathState(JUST_DIED);
                    unitTarget->SetHealth(0);
                    return;
                }
                case 46671:                                 // Cleansing Flames (Exodar)
                case 46672:                                 // Cleansing Flames (Silvermoon)
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, m_spellInfo->ID == 46671 ? 46690 : 46689, true);
                    return;
                }
                case 46797:                                 // Quest - Borean Tundra - Set Explosives Cart
                {
                    if (!unitTarget || m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    ((Player*)m_caster)->KilledMonsterCredit(unitTarget->GetEntry(), unitTarget->GetObjectGuid());

                    // Quest - Borean Tundra - Summon Explosives Cart
                    unitTarget->CastSpell(unitTarget, 46798, true);
                    return;
                }
                case 47110:                                 // Summon Drakuru's Image
                {
                    uint32 spellId = 0;

                    // Spell 47117,47149,47316,47405,50439 exist, are these used to check area/meet requirement
                    // and to cast correct spell in correct area?

                    switch (m_caster->GetAreaId())
                    {
                        case 4255: spellId = 47381; break;  // Reagent Check (Frozen Mojo)
                        case 4209: spellId = 47386; break;  // Reagent Check (Zim'Bo's Mojo)
                        case 4270: spellId = 47389; break;  // Reagent Check (Desperate Mojo)
                        case 4216: spellId = 47408; break;  // Reagent Check (Sacred Mojo)
                        case 4196: spellId = 50441; break;  // Reagent Check (Survival Mojo)
                    }

                    // The additional castspell arguments are needed here to remove reagents for triggered spells
                    if (spellId)
                    {
                        m_caster->CastSpell(m_caster, spellId, true, m_CastItem, NULL, m_caster->GetObjectGuid(), m_spellInfo);
                    }

                    return;
                }
                case 47170:                                 // Impale Leviroth
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    unitTarget->SetHealthPercent(8.0f);

                    // Cosmetic - Underwater Blood (no sound)
                    unitTarget->CastSpell(unitTarget, 47172, true);

                    ((Creature*)unitTarget)->AI()->AttackStart(m_caster);
                    return;
                }
                case 47176:                                 // Infect Ice Troll
                {
                    // Spell has wrong areaGroupid, so it can not be casted where expected.
                    // TODO: research if spells casted by NPC, having TARGET_SCRIPT, can have disabled area check
                    if (!unitTarget)
                    {
                        return;
                    }

                    // Plague Effect Self
                    unitTarget->CastSpell(unitTarget, 47178, true);
                    return;
                }
                case 47305:                                 // Potent Explosive Charge
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    // only if below 80% hp
                    if (unitTarget->GetHealthPercent() > 80.0f)
                    {
                        return;
                    }

                    // Issues with explosion animation (remove insta kill spell resolves the issue)

                    // Quest - Jormungar Explosion Spell Spawner
                    unitTarget->CastSpell(unitTarget, 47311, true);

                    // Potent Explosive Charge
                    unitTarget->CastSpell(unitTarget, 47306, true);

                    return;
                }
                case 47381:                                 // Reagent Check (Frozen Mojo)
                case 47386:                                 // Reagent Check (Zim'Bo's Mojo)
                case 47389:                                 // Reagent Check (Desperate Mojo)
                case 47408:                                 // Reagent Check (Sacred Mojo)
                case 50441:                                 // Reagent Check (Survival Mojo)
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    switch (m_spellInfo->ID)
                    {
                        case 47381:
                            // Envision Drakuru
                            m_caster->CastSpell(m_caster, 47118, true);
                            break;
                        case 47386:
                            m_caster->CastSpell(m_caster, 47150, true);
                            break;
                        case 47389:
                            m_caster->CastSpell(m_caster, 47317, true);
                            break;
                        case 47408:
                            m_caster->CastSpell(m_caster, 47406, true);
                            break;
                        case 50441:
                            m_caster->CastSpell(m_caster, 50440, true);
                            break;
                    }

                    return;
                }
                case 48046:                                 // Use Camera
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // No despawn expected, nor any change in dynamic flags/other flags.
                    // Need internal way to track if credit has been given for this object.

                    // Iron Dwarf Snapshot Credit
                    m_caster->CastSpell(m_caster, 48047, true, m_CastItem, NULL, unitTarget->GetObjectGuid());
                    return;
                }
                case 48790:                                 // Neltharion's Flame
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // Neltharion's Flame Fire Bunny: Periodic Fire Aura
                    unitTarget->CastSpell(unitTarget, 48786, false);
                    return;
                }
                case 49357:                                 // Brewfest Mount Transformation
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    if (!m_caster->HasAuraType(SPELL_AURA_MOUNTED))
                    {
                        return;
                    }

                    m_caster->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);

                    // Ram for Alliance, Kodo for Horde
                    if (((Player*)m_caster)->GetTeam() == ALLIANCE)
                    {
                        if (m_caster->GetSpeedRate(MOVE_RUN) >= 2.0f)
                            // 100% Ram
                            m_caster->CastSpell(m_caster, 43900, true);
                        else
                            // 60% Ram
                            m_caster->CastSpell(m_caster, 43899, true);
                    }
                    else
                    {
                        if (((Player*)m_caster)->GetSpeedRate(MOVE_RUN) >= 2.0f)
                            // 100% Kodo
                            m_caster->CastSpell(m_caster, 49379, true);
                        else
                            // 60% Kodo
                            m_caster->CastSpell(m_caster, 49378, true);
                    }
                    return;
                }
                case 49634:                                 // Sergeant's Flare
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    // Towers of Certain Doom: Tower Bunny Smoke Flare Effect
                    // TODO: MaNGOS::DynamicObjectUpdater::VisitHelper prevent aura to be applied to dummy creature (see HandleAuraDummy for effect of aura)
                    m_caster->CastSpell(unitTarget, 56511, true);

                    static uint32 const spellCredit[4] =
                    {
                        43077,                              // E Kill Credit
                        43067,                              // NW Kill Credit
                        43087,                              // SE Kill Credit
                        43086,                              // SW Kill Credit
                    };

                    // for sizeof(spellCredit)
                    for (int i = 0; i < 4; ++i)
                    {
                        const SpellEntry* pSpell = sSpellStore.LookupEntry(spellCredit[i]);

                        if (pSpell->EffectMiscValue[EFFECT_INDEX_0] == unitTarget->GetEntry())
                        {
                            m_caster->CastSpell(m_caster, spellCredit[i], true);
                            break;
                        }
                    }

                    return;
                }
                case 49859:                                 // Rune of Command
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    // Captive Stone Giant Kill Credit
                    unitTarget->CastSpell(m_caster, 43564, true);
                    // Is it supposed to despawn?
                    ((Creature*)unitTarget)->ForcedDespawn();
                    return;
                }
                case 50133:                                 // Scourging Crystal Controller
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    // Scourge Mur'gul Camp: Force Shield Arcane Purple x3
                    if (unitTarget->HasAura(43874))
                    {
                        // someone else is already channeling target
                        if (unitTarget->HasAura(43878))
                        {
                            return;
                        }

                        // Scourging Crystal Controller
                        m_caster->CastSpell(unitTarget, 43878, true, m_CastItem);
                    }

                    return;
                }
                case 50243:                                 // Teach Language
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    // spell has a 1/3 chance to trigger one of the below
                    if (roll_chance_i(66))
                    {
                        return;
                    }

                    if (((Player*)m_caster)->GetTeam() == ALLIANCE)
                    {
                        // 1000001 - gnomish binary
                        m_caster->CastSpell(m_caster, 50242, true);
                    }
                    else
                    {
                        // 01001000 - goblin binary
                        m_caster->CastSpell(m_caster, 50246, true);
                    }

                    return;
                }
                case 50440:                                 // Envision Drakuru
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // Script Cast Summon Image of Drakuru 05
                    unitTarget->CastSpell(unitTarget, 50439, true);
                    return;
                }
                case 50546:                                 // Ley Line Focus Control Ring Effect
                case 50547:                                 // Ley Line Focus Control Amulet Effect
                case 50548:                                 // Ley Line Focus Control Talisman Effect
                {
                    if (!m_originalCaster || !unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    switch (m_spellInfo->ID)
                    {
                     case 50546: m_originalCaster->CastSpell(m_originalCaster, 47390, false); break;
                     case 50547: m_originalCaster->CastSpell(m_originalCaster, 47472, false); break;
                     case 50548: m_originalCaster->CastSpell(m_originalCaster, 47635, false); break;
                    }

                    return;
                }
                case 51276:                                 // Incinerate Corpse
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 51278, true);
                    unitTarget->CastSpell(m_caster, 51279, true);

                    unitTarget->SetDeathState(JUST_DIED);
                    return;
                }
                case 51330:                                 // Shoot RJR
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // guessed chances
                    if (roll_chance_i(75))
                    {
                        m_caster->CastSpell(unitTarget, roll_chance_i(50) ? 51332 : 51366, true, m_CastItem);
                    }
                    else
                    {
                        m_caster->CastSpell(unitTarget, 51331, true, m_CastItem);
                    }

                    return;
                }
                case 51333:                                 // Dig For Treasure
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    if (roll_chance_i(75))
                    {
                        m_caster->CastSpell(unitTarget, 51370, true, m_CastItem);
                    }
                    else
                    {
                        m_caster->CastSpell(m_caster, 51345, true);
                    }

                    return;
                }
                case 51336:                                 // Magic Pull
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 50770, true);
                    return;
                }
                case 51420:                                 // Digging for Treasure Ping
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    // only spell related protector pets exist currently
                    Pet* pPet = m_caster->GetProtectorPet();
                    if (!pPet)
                    {
                        return;
                    }

                    pPet->SetFacingToObject(unitTarget);

                    // Digging for Treasure
                    pPet->CastSpell(unitTarget, 51405, true);

                    ((Creature*)unitTarget)->ForcedDespawn(1);
                    return;
                }
                case 51582:                                 // Rocket Boots Engaged (Rocket Boots Xtreme and Rocket Boots Xtreme Lite)
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    if (BattleGround* bg = ((Player*)m_caster)->GetBattleGround())
                    {
                        bg->EventPlayerDroppedFlag((Player*)m_caster);
                    }

                    m_caster->CastSpell(m_caster, 30452, true, NULL);
                    return;
                }
                case 51840:                                 // Despawn Fruit Tosser
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    if (roll_chance_i(20))
                    {
                        // summon NPC, or...
                        unitTarget->CastSpell(m_caster, 52070, true);
                    }
                    else
                    {
                        // ...drop banana, orange or papaya
                        switch (urand(0, 2))
                        {
                            case 0: unitTarget->CastSpell(m_caster, 51836, true); break;
                            case 1: unitTarget->CastSpell(m_caster, 51837, true); break;
                            case 2: unitTarget->CastSpell(m_caster, 51839, true); break;
                        }
                    }

                    ((Creature*)unitTarget)->ForcedDespawn(5000);
                    return;
                }
                case 51866:                                 // Kick Nass
                {
                    // It is possible that Nass Heartbeat (spell id 61438) is involved in this
                    // If so, unclear how it should work and using the below instead (even though it could be a bit hack-ish)

                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    // Only own guardian pet
                    if (m_caster != unitTarget->GetOwner())
                    {
                        return;
                    }

                    // This means we already set state (see below) and need to wait.
                    if (unitTarget->hasUnitState(UNIT_STAT_ROOT))
                    {
                        return;
                    }

                    // Expecting pTargetDummy to be summoned by AI at death of target creatures.

                    Creature* pTargetDummy = NULL;
                    float fRange = GetSpellMaxRange(sSpellRangeStore.LookupEntry(m_spellInfo->RangeIndex));

                    MaNGOS::NearestCreatureEntryWithLiveStateInObjectRangeCheck u_check(*m_caster, 28523, true, false, fRange * 2);
                    MaNGOS::CreatureLastSearcher<MaNGOS::NearestCreatureEntryWithLiveStateInObjectRangeCheck> searcher(pTargetDummy, u_check);

                    Cell::VisitGridObjects(m_caster, searcher, fRange * 2);

                    if (pTargetDummy)
                    {
                        if (unitTarget->hasUnitState(UNIT_STAT_FOLLOW | UNIT_STAT_FOLLOW_MOVE))
                        {
                            unitTarget->GetMotionMaster()->MovementExpired();
                        }

                        unitTarget->MonsterMoveWithSpeed(pTargetDummy->GetPositionX(), pTargetDummy->GetPositionY(), pTargetDummy->GetPositionZ(), 24.f);

                        // Add state to temporarily prevent follow
                        unitTarget->addUnitState(UNIT_STAT_ROOT);

                        // Collect Hair Sample
                        unitTarget->CastSpell(pTargetDummy, 51870, true);
                    }

                    return;
                }
                case 51872:                                 // Hair Sample Collected
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    // clear state to allow follow again
                    m_caster->clearUnitState(UNIT_STAT_ROOT);

                    // Nass Kill Credit
                    m_caster->CastSpell(m_caster, 51871, true);

                    // Despawn dummy creature
                    ((Creature*)unitTarget)->ForcedDespawn();

                    return;
                }
                case 51964:                                 // Tormentor's Incense
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    // This might not be the best way, and effect may need some adjustment. Removal of any aura from surrounding dummy creatures?
                    if (((Creature*)unitTarget)->AI())
                    {
                        ((Creature*)unitTarget)->AI()->AttackStart(m_caster);
                    }

                    return;
                }
                case 51858:                                 // Siphon of Acherus
                {
                    if (!m_originalCaster || !unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    if (Player* pPlayer = m_originalCaster->GetCharmerOrOwnerPlayerOrPlayerItself())
                    {
                        pPlayer->KilledMonsterCredit(unitTarget->GetEntry(), unitTarget->GetObjectGuid());
                    }

                    return;
                }
                case 52308:                                 // Take Sputum Sample
                {
                    switch (eff_idx)
                    {
                        case EFFECT_INDEX_0:
                        {
                            uint32 spellID = m_spellInfo->CalculateSimpleValue(EFFECT_INDEX_0);
                            uint32 reqAuraID = m_spellInfo->CalculateSimpleValue(EFFECT_INDEX_1);

                            if (m_caster->HasAura(reqAuraID, EFFECT_INDEX_0))
                            {
                                m_caster->CastSpell(m_caster, spellID, true, NULL);
                            }
                            return;
                        }
                        case EFFECT_INDEX_1:                // additional data for dummy[0]
                        case EFFECT_INDEX_2:
                            return;
                    }
                    return;
                }
                case 52369:                                 // Detonate Explosives
                case 52371:                                 // Detonate Explosives
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // Cosmetic - Explosion
                    unitTarget->CastSpell(unitTarget, 46419, true);

                    // look for gameobjects within max spell range of unitTarget, and respawn if found
                    std::list<GameObject*> lList;

                    float fMaxDist = GetSpellMaxRange(sSpellRangeStore.LookupEntry(m_spellInfo->RangeIndex));

                    MaNGOS::GameObjectEntryInPosRangeCheck go_check(*unitTarget, 182071, unitTarget->GetPositionX(), unitTarget->GetPositionY(), unitTarget->GetPositionZ(), fMaxDist);
                    MaNGOS::GameObjectListSearcher<MaNGOS::GameObjectEntryInPosRangeCheck> checker(lList, go_check);

                    Cell::VisitGridObjects(unitTarget, checker, fMaxDist);

                    for (std::list<GameObject*>::iterator iter = lList.begin(); iter != lList.end(); ++iter)
                    {
                        if (!(*iter)->isSpawned())
                        {
                            (*iter)->SetRespawnTime(MINUTE / 2);
                            (*iter)->Refresh();
                        }
                    }

                    return;
                }
                case 52759:                                 // Ancestral Awakening
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    m_caster->CastCustomSpell(unitTarget, 52752, &damage, NULL, NULL, true);
                    return;
                }
                case 52845:                                 // Brewfest Mount Transformation (Faction Swap)
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    if (!m_caster->HasAuraType(SPELL_AURA_MOUNTED))
                    {
                        return;
                    }

                    m_caster->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);

                    // Ram for Horde, Kodo for Alliance
                    if (((Player*)m_caster)->GetTeam() == HORDE)
                    {
                        if (m_caster->GetSpeedRate(MOVE_RUN) >= 2.0f)
                            // Swift Brewfest Ram, 100% Ram
                            m_caster->CastSpell(m_caster, 43900, true);
                        else
                            // Brewfest Ram, 60% Ram
                            m_caster->CastSpell(m_caster, 43899, true);
                    }
                    else
                    {
                        if (((Player*)m_caster)->GetSpeedRate(MOVE_RUN) >= 2.0f)
                            // Great Brewfest Kodo, 100% Kodo
                            m_caster->CastSpell(m_caster, 49379, true);
                        else
                            // Brewfest Riding Kodo, 60% Kodo
                            m_caster->CastSpell(m_caster, 49378, true);
                    }
                    return;
                }
                case 53341:                                 // Rune of Cinderglacier
                case 53343:                                 // Rune of Razorice
                {
                    // Runeforging Credit
                    m_caster->CastSpell(m_caster, 54586, true);
                    return;
                }
                case 53475:                                 // Set Oracle Faction Friendly
                case 53487:                                 // Set Wolvar Faction Honored
                case 54015:                                 // Set Oracle Faction Honored
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    if (eff_idx == EFFECT_INDEX_0)
                    {
                        Player* pPlayer = (Player*)m_caster;

                        uint32 faction_id = m_currentBasePoints[eff_idx];
                        int32  rep_change = m_currentBasePoints[EFFECT_INDEX_1];

                        FactionEntry const* factionEntry = sFactionStore.LookupEntry(faction_id);

                        if (!factionEntry)
                        {
                            return;
                        }

                        // Set rep to baserep + basepoints (expecting spillover for oposite faction -> become hated)
                        // Not when player already has equal or higher rep with this faction
                        if (pPlayer->GetReputationMgr().GetBaseReputation(factionEntry) < rep_change)
                        {
                            pPlayer->GetReputationMgr().SetReputation(factionEntry, rep_change);
                        }

                        // EFFECT_INDEX_2 most likely update at war state, we already handle this in SetReputation
                    }

                    return;
                }
                case 53808:                                 // Pygmy Oil
                {
                    const uint32 spellShrink = 53805;
                    const uint32 spellTransf = 53806;

                    if (SpellAuraHolder* holder = m_caster->GetSpellAuraHolder(spellShrink))
                    {
                        // chance to become pygmified (5, 10, 15 etc)
                        if (roll_chance_i(holder->GetStackAmount() * 5))
                        {
                            m_caster->RemoveAurasDueToSpell(spellShrink);
                            m_caster->CastSpell(m_caster, spellTransf, true);
                            return;
                        }
                    }

                    if (m_caster->HasAura(spellTransf))
                    {
                        return;
                    }

                    m_caster->CastSpell(m_caster, spellShrink, true);
                    return;
                }
                case 54577:                                 // Throw U.D.E.D.
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    // Sometimes issues with explosion animation. Unclear why
                    // but possibly caused by the order of spells.

                    // Permanent Feign Death
                    unitTarget->CastSpell(unitTarget, 29266, true);

                    // need to despawn later
                    ((Creature*)unitTarget)->ForcedDespawn(2000);

                    // Mammoth Explosion Spell Spawner
                    unitTarget->CastSpell(unitTarget, 54581, true, m_CastItem);
                    return;
                }
                case 54850:                                 // Emerge
                {
                    // Cast Emerge summon
                    m_caster->CastSpell(m_caster, 54851, true);
                    return;
                }
                case 54092:                                 // Monster Slayer's Kit
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    uint32 spellIds[] = {51853, 54063, 54071, 54086};
                    m_caster->CastSpell(unitTarget, spellIds[urand(0, 3)], true);
                    return;
                }
                case 55004:                                 // Nitro Boosts
                {
                    if (!m_CastItem)
                    {
                        return;
                    }

                    if (roll_chance_i(95))                  // Nitro Boosts - success
                    {
                        m_caster->CastSpell(m_caster, 54861, true, m_CastItem);
                    }
                    else                                    // Knocked Up   - backfire 5%
                    {
                        m_caster->CastSpell(m_caster, 46014, true, m_CastItem);
                    }

                    return;
                }
                case 55818:                                 // Hurl Boulder
                {
                    // unclear how many summon min/max random, best guess below
                    uint32 random = urand(3, 5);

                    for (uint32 i = 0; i < random; ++i)
                    {
                        m_caster->CastSpell(m_caster, 55528, true);
                    }

                    return;
                }
                case 56430:                                 // Arcane Bomb
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 56431, true, NULL, NULL, m_caster->GetObjectGuid());
                    unitTarget->CastSpell(unitTarget, 56432, true, NULL, NULL, m_caster->GetObjectGuid());
                    return;
                }
                case 57578:                                 // Lava Strike
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    return;
                }
                case 57908:                                 // Stain Cloth
                {
                    // nothing do more
                    finish();

                    m_caster->CastSpell(m_caster, 57915, false, m_CastItem);

                    // cast item deleted
                    ClearCastItem();
                    break;
                }
                case 58418:                                 // Portal to Orgrimmar
                case 58420:                                 // Portal to Stormwind
                    return;                                 // implemented in EffectScript[0]
                case 58601:                                 // Remove Flight Auras
                {
                    m_caster->RemoveSpellsCausingAura(SPELL_AURA_FLY);
                    m_caster->RemoveSpellsCausingAura(SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED);
                    return;
                }
                case 59640:                                 // Underbelly Elixir
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    uint32 spell_id = 0;
                    switch (urand(1, 3))
                    {
                        case 1: spell_id = 59645; break;
                        case 2: spell_id = 59831; break;
                        case 3: spell_id = 59843; break;
                    }

                    m_caster->CastSpell(m_caster, spell_id, true, NULL);
                    return;
                }
                case 60932:                                 // Disengage (one from creature versions)
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 60934, true, NULL);
                    return;
                }
                case 62105:                                 // To'kini's Blowgun
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    // Sleeping Sleep
                    unitTarget->CastSpell(unitTarget, 62248, true);

                    // Although not really correct, it's needed to have access to m_caster later,
                    // to properly process spell 62110 (cast from gossip).
                    // Can possibly be replaced with a similar function that doesn't set any dynamic flags.
                    ((Creature*)unitTarget)->SetLootRecipient(m_caster);

                    unitTarget->setFaction(190);            // Ambient (neutral)
                    unitTarget->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);
                    return;
                }
                case 62278:                                 // Lightning Orb Charger
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(m_caster, 62466, true);
                    unitTarget->CastSpell(unitTarget, 62279, true);
                    return;
                }
                case 62652:                                 // Tidal Wave
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, m_caster->GetMap()->IsRegularDifficulty() ? 62653 : 62935, true);
                    return;
                }
                case 62797:                                 // Storm Cloud
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, m_caster->GetMap()->IsRegularDifficulty() ? 65123 : 65133, true);
                    return;
                }
                case 62907:                                 // Freya's Ward
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    for (uint8 i = 0; i < 5; ++i)
                    {
                        m_caster->CastSpell(unitTarget, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    }
                    return;
                }
                case 63030:                                 // Boil Ominously
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 63031, true);
                    return;
                }
                case 63499:                                 // Dispel Magic
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    uint32 triggerSpell = 0;
                    switch (m_spellInfo->ID)
                    {
                        case 63820: triggerSpell = 64398; break;
                        case 64425: triggerSpell = 64426; break;
                        case 64620: triggerSpell = 64621; break;
                    }
                    unitTarget->CastSpell(unitTarget, triggerSpell, false);
                    return;
                }
                case 63545:                                 // Icicle
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    return;
                }
                case 63744:                                 // Sara's Anger
                case 63745:                                 // Sara's Blessing
                case 63747:                                 // Sara's Fervor
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    return;
                }
                case 64172:                                 // Titanic Storm
                {
                    if (!unitTarget || !unitTarget->HasAura(m_spellInfo->CalculateSimpleValue(eff_idx)))
                    {
                        return;
                    }

                    // There is no known spell to kill the target
                    m_caster->DealDamage(unitTarget, unitTarget->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                    return;
                }
                case 64385:                                 // Spinning (from Unusual Compass)
                {
                    m_caster->SetFacingTo(frand(0, M_PI_F * 2));
                    return;
                }
                case 64402:                                 // Rocket Strike
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 63681, true);
                    return;
                }
                case 64489:                                 // Feral Rush
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 64496, true);
                    return;
                }
                case 64543:                                 // Melt Ice
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    m_caster->CastSpell(m_caster, 64540, true);
                    return;
                }
                case 64555:                                 // Insane Periodic
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER || unitTarget->HasAura(63050) || unitTarget->HasAura(m_spellInfo->CalculateSimpleValue(eff_idx)))
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 64464, true);
                    m_caster->CastSpell(unitTarget, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    return;
                }
                case 64673:                                 // Feral Rush (h)
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 64674, true);
                    return;
                }
                case 64981:                                 // Summon Random Vanquished Tentacle
                {
                    uint32 spell_id = 0;

                    switch (urand(0, 2))
                    {
                        case 0: spell_id = 64982; break;    // Summon Vanquished Crusher Tentacle
                        case 1: spell_id = 64983; break;    // Summon Vanquished Constrictor Tentacle
                        case 2: spell_id = 64984; break;    // Summon Vanquished Corruptor Tentacle
                    }

                    m_caster->CastSpell(m_caster, spell_id, true);
                    return;
                }
                case 65206:                                 // Destabilization Matrix
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    return;
                }
                case 65346:                                 // Proximity Mine
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    m_caster->CastSpell(m_caster, m_caster->GetMap()->IsRegularDifficulty() ? 66351 : 63009, true);
                    m_caster->RemoveAurasDueToSpell(65345);
                    m_caster->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    return;
                }
                case 66312:                                 // Light Ball Passive
                {
                    if (!unitTarget || m_caster->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    if (unitTarget->GetTypeId() == TYPEID_PLAYER)
                    {
                        if (unitTarget->HasAuraOfDifficulty(65686))
                        {
                            unitTarget->CastSpell(unitTarget, 67590, true);
                        }
                        else
                        {
                            m_caster->CastSpell(m_caster, 65795, true);
                        }

                        ((Creature*)m_caster)->ForcedDespawn();
                    }
                    return;
                }
                case 66314:                                 // Dark Ball Passive
                {
                    if (!unitTarget || m_caster->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    if (unitTarget->GetTypeId() == TYPEID_PLAYER)
                    {
                        if (unitTarget->HasAuraOfDifficulty(65684))
                        {
                            unitTarget->CastSpell(unitTarget, 67590, true);
                        }
                        else
                        {
                            m_caster->CastSpell(m_caster, 65808, true);
                        }

                        ((Creature*)m_caster)->ForcedDespawn();
                    }
                    return;
                }
                case 66390:                                 // Read Last Rites
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT || m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    // Summon Tualiq Proxy
                    // Not known what purpose this has
                    unitTarget->CastSpell(unitTarget, 66411, true);

                    // Summon Tualiq Spirit
                    // Offtopic note: the summoned has aura from spell 37119 and 66419. One of them should
                    // most likely make summoned "rise", hover up/sideways in the air (MOVEFLAG_LEVITATING + MOVEFLAG_HOVER)
                    unitTarget->CastSpell(unitTarget, 66412, true);

                    ((Player*)m_caster)->KilledMonsterCredit(unitTarget->GetEntry(), unitTarget->GetObjectGuid());

                    // Must have a delay for proper spell animation
                    ((Creature*)unitTarget)->ForcedDespawn(1000);
                    return;
                }
                case 67019:                                 // Flask of the North
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    uint32 spell_id = 0;
                    switch (m_caster->getClass())
                    {
                        case CLASS_WARRIOR:
                        case CLASS_DEATH_KNIGHT:
                            spell_id = 67018;               // STR for Warriors, Death Knights
                            break;
                        case CLASS_ROGUE:
                        case CLASS_HUNTER:
                            spell_id = 67017;               // AP for Rogues, Hunters
                            break;
                        case CLASS_PRIEST:
                        case CLASS_MAGE:
                        case CLASS_WARLOCK:
                            spell_id = 67016;               // SPD for Priests, Mages, Warlocks
                            break;
                        case CLASS_SHAMAN:
                            // random (SPD, AP)
                            spell_id = roll_chance_i(50) ? 67016 : 67017;
                            break;
                        case CLASS_PALADIN:
                        case CLASS_DRUID:
                        default:
                            // random (SPD, STR)
                            spell_id = roll_chance_i(50) ? 67016 : 67018;
                            break;
                    }
                    m_caster->CastSpell(m_caster, spell_id, true);
                    return;
                }
                case 69922:                                 // Temper Quel'Delar
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // Return Tempered Quel'Delar
                    unitTarget->CastSpell(m_caster, 69956, true);
                    return;
                }
                case 71445:                                 // Twilight Bloodbolt
                case 71471:                                 // Twilight Bloodbolt
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 71818, true);
                    return;
                }
                case 71718:                                 // Conjure Flame
                case 72040:                                 // Conjure Empowered Flame
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    return;
                }
                case 71837:                                 // Vampiric Bite
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 71726, true);
                    return;
                }
                case 71861:                                 // Swarming Shadows
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 71264, true);
                    return;
                }
                case 72261:                                 // Delirious Slash
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, m_caster->CanReachWithMeleeAttack(unitTarget) ? 71623 : 72264, true);
                    return;
                }
                case 74452:                                 // Conflagration
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 74453, true);
                    m_caster->CastSpell(unitTarget, 74454, true, NULL, NULL, m_caster->GetObjectGuid(), m_spellInfo);
                    return;
                }
            }
            break;
        }
        case SPELLFAMILY_MAGE:
        {
            switch (m_spellInfo->ID)
            {
                case 11958:                                 // Cold Snap
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    // immediately finishes the cooldown on Frost spells
                    const SpellCooldowns& cm = ((Player*)m_caster)->GetSpellCooldownMap();
                    for (SpellCooldowns::const_iterator itr = cm.begin(); itr != cm.end();)
                    {
                        SpellEntry const* spellInfo = sSpellStore.LookupEntry(itr->first);

                        if (spellInfo->SpellClassSet == SPELLFAMILY_MAGE &&
                                (GetSpellSchoolMask(spellInfo) & SPELL_SCHOOL_MASK_FROST) &&
                                spellInfo->ID != 11958 && GetSpellRecoveryTime(spellInfo) > 0)
                        {
                            ((Player*)m_caster)->RemoveSpellCooldown((itr++)->first, true);
                        }
                        else
                        {
                            ++itr;
                        }
                    }
                    return;
                }
                case 31687:                                 // Summon Water Elemental
                {
                    if (m_caster->HasAura(70937))           // Glyph of Eternal Water (permanent limited by known spells version)
                    {
                        m_caster->CastSpell(m_caster, 70908, true);
                    }
                    else                                    // temporary version
                    {
                        m_caster->CastSpell(m_caster, 70907, true);
                    }

                    return;
                }
                case 32826:                                 // Polymorph Cast Visual
                {
                    if (unitTarget && unitTarget->GetTypeId() == TYPEID_UNIT)
                    {
                        // Polymorph Cast Visual Rank 1
                        const uint32 spell_list[6] =
                        {
                            32813,                          // Squirrel Form
                            32816,                          // Giraffe Form
                            32817,                          // Serpent Form
                            32818,                          // Dragonhawk Form
                            32819,                          // Worgen Form
                            32820                           // Sheep Form
                        };
                        unitTarget->CastSpell(unitTarget, spell_list[urand(0, 5)], true);
                    }
                    return;
                }
                case 38194:                                 // Blink
                {
                    // Blink
                    if (unitTarget)
                    {
                        m_caster->CastSpell(unitTarget, 38203, true);
                    }

                    return;
                }
            }

            // Conjure Mana Gem
            if (eff_idx == EFFECT_INDEX_1 && m_spellInfo->Effect[EFFECT_INDEX_0] == SPELL_EFFECT_CREATE_ITEM)
            {
                if (m_caster->GetTypeId() != TYPEID_PLAYER)
                {
                    return;
                }

                // checked in create item check, avoid unexpected
                if (Item* item = ((Player*)m_caster)->GetItemByLimitedCategory(ITEM_LIMIT_CATEGORY_MANA_GEM))
                    if (item->HasMaxCharges())
                    {
                        return;
                    }

                unitTarget->CastSpell(unitTarget, m_spellInfo->CalculateSimpleValue(eff_idx), true, m_CastItem);
                return;
            }
            break;
        }
        case SPELLFAMILY_WARRIOR:
        {
            // Charge
            if ((m_spellInfo->SpellClassMask & UI64LIT(0x1)) && m_spellInfo->SpellVisualID[0] == 867)
            {
                int32 chargeBasePoints0 = damage;
                m_caster->CastCustomSpell(m_caster, 34846, &chargeBasePoints0, NULL, NULL, true);
                return;
            }
            // Execute
            if (m_spellInfo->SpellClassMask & UI64LIT(0x20000000))
            {
                if (!unitTarget)
                {
                    return;
                }

                uint32 rage = m_caster->GetPower(POWER_RAGE);

                // up to max 30 rage cost
                if (rage > 300)
                {
                    rage = 300;
                }

                // Glyph of Execution bonus
                uint32 rage_modified = rage;

                if (Aura* aura = m_caster->GetDummyAura(58367))
                {
                    rage_modified +=  aura->GetModifier()->m_amount * 10;
                }

                int32 basePoints0 = damage + int32(rage_modified * m_spellInfo->EffectChainAmplitude[eff_idx] +
                                                   m_caster->GetTotalAttackPowerValue(BASE_ATTACK) * 0.2f);

                m_caster->CastCustomSpell(unitTarget, 20647, &basePoints0, NULL, NULL, true, 0);

                // Sudden Death
                if (m_caster->HasAura(52437))
                {
                    Unit::AuraList const& auras = m_caster->GetAurasByType(SPELL_AURA_PROC_TRIGGER_SPELL);
                    for (Unit::AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                    {
                        // Only Sudden Death have this SpellIconID with SPELL_AURA_PROC_TRIGGER_SPELL
                        if ((*itr)->GetSpellProto()->SpellIconID == 1989)
                        {
                            // saved rage top stored in next affect
                            uint32 lastrage = (*itr)->GetSpellProto()->CalculateSimpleValue(EFFECT_INDEX_1) * 10;
                            if (lastrage < rage)
                            {
                                rage -= lastrage;
                            }
                            break;
                        }
                    }
                }

                m_caster->SetPower(POWER_RAGE, m_caster->GetPower(POWER_RAGE) - rage);
                return;
            }
            // Slam
            if (m_spellInfo->SpellClassMask & UI64LIT(0x0000000000200000))
            {
                if (!unitTarget)
                {
                    return;
                }

                // dummy cast itself ignored by client in logs
                m_caster->CastCustomSpell(unitTarget, 50782, &damage, NULL, NULL, true);
                return;
            }
            // Concussion Blow
            if (m_spellInfo->SpellClassMask & UI64LIT(0x0000000004000000))
            {
                m_damage += uint32(damage * m_caster->GetTotalAttackPowerValue(BASE_ATTACK) / 100);
                return;
            }

            switch (m_spellInfo->ID)
            {
                    // Warrior's Wrath
                case 21977:
                {
                    if (!unitTarget)
                    {
                        return;
                    }
                    m_caster->CastSpell(unitTarget, 21887, true); // spell mod
                    return;
                }
                // Last Stand
                case 12975:
                {
                    int32 healthModSpellBasePoints0 = int32(m_caster->GetMaxHealth() * 0.3);
                    m_caster->CastCustomSpell(m_caster, 12976, &healthModSpellBasePoints0, NULL, NULL, true, NULL);
                    return;
                }
                // Bloodthirst
                case 23881:
                {
                    m_caster->CastCustomSpell(unitTarget, 23885, &damage, NULL, NULL, true, NULL);
                    return;
                }
                case 30012:                                 // Move
                {
                    if (!unitTarget || unitTarget->HasAura(39400))
                    {
                        return;
                    }

                    unitTarget->CastSpell(m_caster, 30253, true);
                }
                case 30284:                                 // Change Facing
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(m_caster, 30270, true);
                    return;
                }
                case 37144:                                 // Move (Chess event player knight move)
                case 37146:                                 // Move (Chess event player pawn move)
                case 37148:                                 // Move (Chess event player queen move)
                case 37151:                                 // Move (Chess event player rook move)
                case 37152:                                 // Move (Chess event player bishop move)
                case 37153:                                 // Move (Chess event player king move)
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    // cast generic move spell
                    m_caster->CastSpell(unitTarget, 30012, true);
                    return;
                }
            }
            break;
        }
        case SPELLFAMILY_WARLOCK:
        {
            // Life Tap
            if (m_spellInfo->SpellClassMask & UI64LIT(0x0000000000040000))
            {
                if (unitTarget && (int32(unitTarget->GetHealth()) > damage))
                {
                    // Shouldn't Appear in Combat Log
                    unitTarget->ModifyHealth(-damage);

                    int32 spell_power = m_caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(m_spellInfo));
                    int32 mana = damage + spell_power / 2;

                    // Improved Life Tap mod
                    Unit::AuraList const& auraDummy = m_caster->GetAurasByType(SPELL_AURA_DUMMY);
                    for (Unit::AuraList::const_iterator itr = auraDummy.begin(); itr != auraDummy.end(); ++itr)
                    {
                        if ((*itr)->GetSpellProto()->SpellClassSet == SPELLFAMILY_WARLOCK && (*itr)->GetSpellProto()->SpellIconID == 208)
                        {
                            mana = ((*itr)->GetModifier()->m_amount + 100) * mana / 100;
                        }
                    }

                    m_caster->CastCustomSpell(unitTarget, 31818, &mana, NULL, NULL, true);

                    // Mana Feed
                    int32 manaFeedVal = 0;
                    Unit::AuraList const& mod = m_caster->GetAurasByType(SPELL_AURA_ADD_FLAT_MODIFIER);
                    for (Unit::AuraList::const_iterator itr = mod.begin(); itr != mod.end(); ++itr)
                    {
                        if ((*itr)->GetSpellProto()->SpellClassSet == SPELLFAMILY_WARLOCK && (*itr)->GetSpellProto()->SpellIconID == 1982)
                        {
                            manaFeedVal += (*itr)->GetModifier()->m_amount;
                        }
                    }
                    if (manaFeedVal > 0)
                    {
                        manaFeedVal = manaFeedVal * mana / 100;
                        m_caster->CastCustomSpell(m_caster, 32553, &manaFeedVal, NULL, NULL, true, NULL);
                    }
                }
                else
                {
                    SendCastResult(SPELL_FAILED_FIZZLE);
                }

                return;
            }
            break;
        }
        case SPELLFAMILY_PRIEST:
        {
            // Penance
            if (m_spellInfo->SpellClassMask & UI64LIT(0x0080000000000000))
            {
                if (!unitTarget)
                {
                    return;
                }

                int hurt = 0;
                int heal = 0;
                switch (m_spellInfo->ID)
                {
                    case 47540: hurt = 47758; heal = 47757; break;
                    case 53005: hurt = 53001; heal = 52986; break;
                    case 53006: hurt = 53002; heal = 52987; break;
                    case 53007: hurt = 53003; heal = 52988; break;
                    default:
                        sLog.outError("Spell::EffectDummy: Spell %u Penance need set correct heal/damage spell", m_spellInfo->ID);
                        return;
                }

                // prevent interrupted message for main spell
                finish(true);

                // replace cast by selected spell, this also make it interruptible including target death case
                if (m_caster->IsFriendlyTo(unitTarget))
                {
                    m_caster->CastSpell(unitTarget, heal, false);
                }
                else
                {
                    m_caster->CastSpell(unitTarget, hurt, false);
                }

                return;
            }
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            // Starfall
            if (m_spellInfo->IsFitToFamilyMask(UI64LIT(0x0000000000000000), 0x00000100))
            {
                // Shapeshifting into an animal form or mounting cancels the effect.
                if (m_caster->GetCreatureType() == CREATURE_TYPE_BEAST || m_caster->IsMounted())
                {
                    if (m_triggeredByAuraSpell)
                    {
                        m_caster->RemoveAurasDueToSpell(m_triggeredByAuraSpell->ID);
                    }
                    return;
                }

                // Any effect which causes you to lose control of your character will supress the starfall effect.
                if (m_caster->hasUnitState(UNIT_STAT_NO_FREE_MOVE))
                {
                    return;
                }

                switch (m_spellInfo->ID)
                {
                    case 50286: m_caster->CastSpell(unitTarget, 50288, true); return;
                    case 53196: m_caster->CastSpell(unitTarget, 53191, true); return;
                    case 53197: m_caster->CastSpell(unitTarget, 53194, true); return;
                    case 53198: m_caster->CastSpell(unitTarget, 53195, true); return;
                    default:
                        sLog.outError("Spell::EffectDummy: Unhandeled Starfall spell rank %u", m_spellInfo->ID);
                        return;
                }
            }
            break;
        }
        case SPELLFAMILY_ROGUE:
        {
            switch (m_spellInfo->ID)
            {
                case 5938:                                  // Shiv
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    Player* pCaster = ((Player*)m_caster);

                    Item* item = pCaster->GetWeaponForAttack(OFF_ATTACK);
                    if (!item)
                    {
                        return;
                    }

                    // all poison enchantments is temporary
                    uint32 enchant_id = item->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT);
                    if (!enchant_id)
                    {
                        return;
                    }

                    SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
                    if (!pEnchant)
                    {
                        return;
                    }

                    for (int s = 0; s < 3; ++s)
                    {
                        if (pEnchant->Effect[s] != ITEM_ENCHANTMENT_TYPE_COMBAT_SPELL)
                        {
                            continue;
                        }

                        SpellEntry const* combatEntry = sSpellStore.LookupEntry(pEnchant->EffectArg[s]);
                        if (!combatEntry || combatEntry->DispelType != DISPEL_POISON)
                        {
                            continue;
                        }

                        m_caster->CastSpell(unitTarget, combatEntry, true, item);
                    }

                    m_caster->CastSpell(unitTarget, 5940, true);
                    return;
                }
                case 14185:                                 // Preparation
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    // immediately finishes the cooldown on certain Rogue abilities
                    const SpellCooldowns& cm = ((Player*)m_caster)->GetSpellCooldownMap();
                    for (SpellCooldowns::const_iterator itr = cm.begin(); itr != cm.end();)
                    {
                        SpellEntry const* spellInfo = sSpellStore.LookupEntry(itr->first);

                        if (spellInfo->SpellClassSet == SPELLFAMILY_ROGUE && (spellInfo->SpellClassMask & UI64LIT(0x0000024000000860)))
                        {
                            ((Player*)m_caster)->RemoveSpellCooldown((itr++)->first, true);
                        }
                        else
                        {
                            ++itr;
                        }
                    }
                    return;
                }
                case 31231:                                 // Cheat Death
                {
                    // Cheating Death
                    m_caster->CastSpell(m_caster, 45182, true);
                    return;
                }
                case 51662:                                 // Hunger for Blood
                {
                    m_caster->CastSpell(m_caster, 63848, true);
                    return;
                }
            }
            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            // Steady Shot
            if (m_spellInfo->SpellClassMask & UI64LIT(0x100000000))
            {
                if (!unitTarget || !unitTarget->IsAlive())
                {
                    return;
                }

                bool found = false;

                // check dazed affect
                Unit::AuraList const& decSpeedList = unitTarget->GetAurasByType(SPELL_AURA_MOD_DECREASE_SPEED);
                for (Unit::AuraList::const_iterator iter = decSpeedList.begin(); iter != decSpeedList.end(); ++iter)
                {
                    if ((*iter)->GetSpellProto()->SpellIconID == 15 && (*iter)->GetSpellProto()->DispelType == 0)
                    {
                        found = true;
                        break;
                    }
                }

                if (found)
                {
                    m_damage += damage;
                }
                return;
            }

            // Disengage
            if (m_spellInfo->SpellClassMask & UI64LIT(0x0000400000000000))
            {
                Unit* target = unitTarget;
                uint32 spellid;
                switch (m_spellInfo->ID)
                {
                    case 57635: spellid = 57636; break;     // one from creature cases
                    case 61507: spellid = 61508; break;     // one from creature cases
                    default:
                        sLog.outError("Spell %u not handled propertly in EffectDummy(Disengage)", m_spellInfo->ID);
                        return;
                }
                if (!target || !target->IsAlive())
                {
                    return;
                }
                m_caster->CastSpell(target, spellid, true, NULL);
            }

            switch (m_spellInfo->ID)
            {
                case 23989:                                 // Readiness talent
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    // immediately finishes the cooldown for hunter abilities
                    const SpellCooldowns& cm = ((Player*)m_caster)->GetSpellCooldownMap();
                    for (SpellCooldowns::const_iterator itr = cm.begin(); itr != cm.end();)
                    {
                        SpellEntry const* spellInfo = sSpellStore.LookupEntry(itr->first);

                        if (spellInfo->SpellClassSet == SPELLFAMILY_HUNTER && spellInfo->ID != 23989 && GetSpellRecoveryTime(spellInfo) > 0)
                        {
                            ((Player*)m_caster)->RemoveSpellCooldown((itr++)->first, true);
                        }
                        else
                        {
                            ++itr;
                        }
                    }
                    return;
                }
                case 37506:                                 // Scatter Shot
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    // break Auto Shot and autohit
                    m_caster->InterruptSpell(CURRENT_AUTOREPEAT_SPELL);
                    m_caster->AttackStop();
                    ((Player*)m_caster)->SendAttackSwingCancelAttack();
                    return;
                }
                // Last Stand
                case 53478:
                {
                    if (!unitTarget)
                    {
                        return;
                    }
                    int32 healthModSpellBasePoints0 = int32(unitTarget->GetMaxHealth() * 0.3);
                    unitTarget->CastCustomSpell(unitTarget, 53479, &healthModSpellBasePoints0, NULL, NULL, true, NULL);
                    return;
                }
                // Master's Call
                case 53271:
                {
                    Pet* pet = m_caster->GetPet();
                    if (!pet || !unitTarget)
                    {
                        return;
                    }

                    pet->CastSpell(unitTarget, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    return;
                }
            }
            break;
        }
        case SPELLFAMILY_PALADIN:
        {
            switch (m_spellInfo->SpellIconID)
            {
                case 156:                                   // Holy Shock
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    int hurt = 0;
                    int heal = 0;

                    switch (m_spellInfo->ID)
                    {
                        case 20473: hurt = 25912; heal = 25914; break;
                        case 20929: hurt = 25911; heal = 25913; break;
                        case 20930: hurt = 25902; heal = 25903; break;
                        case 27174: hurt = 27176; heal = 27175; break;
                        case 33072: hurt = 33073; heal = 33074; break;
                        case 48824: hurt = 48822; heal = 48820; break;
                        case 48825: hurt = 48823; heal = 48821; break;
                        default:
                            sLog.outError("Spell::EffectDummy: Spell %u not handled in HS", m_spellInfo->ID);
                            return;
                    }

                    if (m_caster->IsFriendlyTo(unitTarget))
                    {
                        m_caster->CastSpell(unitTarget, heal, true);
                    }
                    else
                    {
                        m_caster->CastSpell(unitTarget, hurt, true);
                    }

                    return;
                }
                case 561:                                   // Judgement of command
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    uint32 spell_id = m_currentBasePoints[eff_idx];
                    SpellEntry const* spell_proto = sSpellStore.LookupEntry(spell_id);
                    if (!spell_proto)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, spell_proto, true, NULL);
                    return;
                }
            }

            switch (m_spellInfo->ID)
            {
                case 31789:                                 // Righteous Defense (step 1)
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        SendCastResult(SPELL_FAILED_TARGET_AFFECTING_COMBAT);
                        return;
                    }

                    // 31989 -> dummy effect (step 1) + dummy effect (step 2) -> 31709 (taunt like spell for each target)
                    Unit* friendTarget = !unitTarget || unitTarget->IsFriendlyTo(m_caster) ? unitTarget : unitTarget->getVictim();
                    if (friendTarget)
                    {
                        Player* player = friendTarget->GetCharmerOrOwnerPlayerOrPlayerItself();
                        if (!player || !player->IsInSameRaidWith((Player*)m_caster))
                        {
                            friendTarget = NULL;
                        }
                    }

                    // non-standard cast requirement check
                    if (!friendTarget || friendTarget->getAttackers().empty())
                    {
                        ((Player*)m_caster)->RemoveSpellCooldown(m_spellInfo->ID, true);
                        SendCastResult(SPELL_FAILED_TARGET_AFFECTING_COMBAT);
                        return;
                    }

                    // Righteous Defense (step 2) (in old version 31980 dummy effect)
                    // Clear targets for eff 1
                    for (TargetList::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
                    {
                        ihit->effectMask &= ~(1 << 1);
                    }

                    // not empty (checked), copy
                    Unit::AttackerSet attackers = friendTarget->getAttackers();

                    // selected from list 3
                    for (uint32 i = 0; i < std::min(size_t(3), attackers.size()); ++i)
                    {
                        Unit::AttackerSet::iterator aItr = attackers.begin();
                        std::advance(aItr, rand() % attackers.size());
                        AddUnitTarget((*aItr), EFFECT_INDEX_1);
                        attackers.erase(aItr);
                    }

                    // now let next effect cast spell at each target.
                    return;
                }
                case 37877:                                 // Blessing of Faith
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    uint32 spell_id = 0;
                    switch (unitTarget->getClass())
                    {
                        case CLASS_DRUID:   spell_id = 37878; break;
                        case CLASS_PALADIN: spell_id = 37879; break;
                        case CLASS_PRIEST:  spell_id = 37880; break;
                        case CLASS_SHAMAN:  spell_id = 37881; break;
                        default: return;                    // ignore for not healing classes
                    }

                    m_caster->CastSpell(m_caster, spell_id, true);
                    return;
                }
            }
            break;
        }
        case SPELLFAMILY_SHAMAN:
        {
            // Cleansing Totem
            if ((m_spellInfo->SpellClassMask & UI64LIT(0x0000000004000000)) && m_spellInfo->SpellIconID == 1673)
            {
                if (unitTarget)
                {
                    m_caster->CastSpell(unitTarget, 52025, true);
                }
                return;
            }
            // Healing Stream Totem
            if (m_spellInfo->SpellClassMask & UI64LIT(0x0000000000002000))
            {
                if (unitTarget)
                {
                    if (Unit* owner = m_caster->GetOwner())
                    {
                        // spell have SPELL_DAMAGE_CLASS_NONE and not get bonuses from owner, use main spell for bonuses
                        if (m_triggeredBySpellInfo)
                        {
                            damage = int32(owner->SpellHealingBonusDone(unitTarget, m_triggeredBySpellInfo, damage, HEAL));
                            damage = int32(unitTarget->SpellHealingBonusTaken(owner, m_triggeredBySpellInfo, damage, HEAL));
                        }

                        // Restorative Totems
                        Unit::AuraList const& mDummyAuras = owner->GetAurasByType(SPELL_AURA_DUMMY);
                        for (Unit::AuraList::const_iterator i = mDummyAuras.begin(); i != mDummyAuras.end(); ++i)
                        {
                            // only its have dummy with specific icon
                            if ((*i)->GetSpellProto()->SpellClassSet == SPELLFAMILY_SHAMAN && (*i)->GetSpellProto()->SpellIconID == 338)
                            {
                                damage += (*i)->GetModifier()->m_amount * damage / 100;
                            }
                        }

                        // Glyph of Healing Stream Totem
                        if (Aura* dummy = owner->GetDummyAura(55456))
                        {
                            damage += dummy->GetModifier()->m_amount * damage / 100;
                        }
                    }
                    m_caster->CastCustomSpell(unitTarget, 52042, &damage, NULL, NULL, true, 0, 0, m_originalCasterGUID);
                }
                return;
            }
            // Mana Spring Totem
            if (m_spellInfo->SpellClassMask & UI64LIT(0x0000000000004000))
            {
                if (!unitTarget || unitTarget->GetPowerType() != POWER_MANA)
                {
                    return;
                }
                m_caster->CastCustomSpell(unitTarget, 52032, &damage, 0, 0, true, 0, 0, m_originalCasterGUID);
                return;
            }
            // Flametongue Weapon Proc, Ranks
            if (m_spellInfo->SpellClassMask & UI64LIT(0x0000000000200000))
            {
                if (!m_CastItem)
                {
                    sLog.outError("Spell::EffectDummy: spell %i requires cast Item", m_spellInfo->ID);
                    return;
                }
                // found spelldamage coefficients of 0.381% per 0.1 speed and 15.244 per 4.0 speed
                // but own calculation say 0.385 gives at most one point difference to published values
                int32 spellDamage = m_caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(m_spellInfo));
                float weaponSpeed = (1.0f / IN_MILLISECONDS) * m_CastItem->GetProto()->Delay;
                int32 totalDamage = int32((damage + 3.85f * spellDamage) * 0.01 * weaponSpeed);

                m_caster->CastCustomSpell(unitTarget, 10444, &totalDamage, NULL, NULL, true, m_CastItem);
                return;
            }
            if (m_spellInfo->ID == 39610)                   // Mana Tide Totem effect
            {
                if (!unitTarget || unitTarget->GetPowerType() != POWER_MANA)
                {
                    return;
                }

                // Glyph of Mana Tide
                if (Unit* owner = m_caster->GetOwner())
                    if (Aura* dummy = owner->GetDummyAura(55441))
                    {
                        damage += dummy->GetModifier()->m_amount;
                    }
                // Regenerate 6% of Total Mana Every 3 secs
                int32 EffectBasePoints0 = unitTarget->GetMaxPower(POWER_MANA)  * damage / 100;
                m_caster->CastCustomSpell(unitTarget, 39609, &EffectBasePoints0, NULL, NULL, true, NULL, NULL, m_originalCasterGUID);
                return;
            }
            // Lava Lash
            if (m_spellInfo->IsFitToFamilyMask(UI64LIT(0x0000000000000000), 0x00000004))
            {
                if (m_caster->GetTypeId() != TYPEID_PLAYER)
                {
                    return;
                }
                Item* item = ((Player*)m_caster)->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
                if (item)
                {
                    // Damage is increased if your off-hand weapon is enchanted with Flametongue.
                    Unit::AuraList const& auraDummy = m_caster->GetAurasByType(SPELL_AURA_DUMMY);
                    for (Unit::AuraList::const_iterator itr = auraDummy.begin(); itr != auraDummy.end(); ++itr)
                    {
                        if ((*itr)->GetSpellProto()->SpellClassSet == SPELLFAMILY_SHAMAN &&
                                ((*itr)->GetSpellProto()->SpellClassMask & UI64LIT(0x0000000000200000)) &&
                                (*itr)->GetCastItemGuid() == item->GetObjectGuid())
                        {
                            m_damage += m_damage * damage / 100;
                            return;
                        }
                    }
                }
                return;
            }
            // Fire Nova
            if (m_spellInfo->SpellIconID == 33)
            {
                // fire totems slot
                Totem* totem = m_caster->GetTotem(TOTEM_SLOT_FIRE);
                if (!totem)
                {
                    return;
                }

                uint32 triggered_spell_id;
                switch (m_spellInfo->ID)
                {
                    case 1535:  triggered_spell_id = 8349;  break;
                    case 8498:  triggered_spell_id = 8502;  break;
                    case 8499:  triggered_spell_id = 8503;  break;
                    case 11314: triggered_spell_id = 11306; break;
                    case 11315: triggered_spell_id = 11307; break;
                    case 25546: triggered_spell_id = 25535; break;
                    case 25547: triggered_spell_id = 25537; break;
                    case 61649: triggered_spell_id = 61650; break;
                    case 61657: triggered_spell_id = 61654; break;
                    default: return;
                }

                totem->CastSpell(totem, triggered_spell_id, true, NULL, NULL, m_caster->GetObjectGuid());

                // Fire Nova Visual
                totem->CastSpell(totem, 19823, true, NULL, NULL, m_caster->GetObjectGuid());
                return;
            }
            break;
        }
        case SPELLFAMILY_DEATHKNIGHT:
        {
            // Death Coil
            if (m_spellInfo->SpellClassMask & UI64LIT(0x002000))
            {
                if (m_caster->IsFriendlyTo(unitTarget))
                {
                    if (!unitTarget || unitTarget->GetCreatureType() != CREATURE_TYPE_UNDEAD)
                    {
                        return;
                    }

                    int32 bp = int32(damage * 1.5f);
                    m_caster->CastCustomSpell(unitTarget, 47633, &bp, NULL, NULL, true);
                }
                else
                {
                    int32 bp = damage;
                    m_caster->CastCustomSpell(unitTarget, 47632, &bp, NULL, NULL, true);
                }
                return;
            }
            // Hungering Cold
            else if (m_spellInfo->SpellClassMask & UI64LIT(0x0000100000000000))
            {
                m_caster->CastSpell(m_caster, 51209, true);
                return;
            }
            // Death Strike
            else if (m_spellInfo->SpellClassMask & UI64LIT(0x0000000000000010))
            {

                // Death Rune Mastery talents
                if ((Player*)m_caster->HasSpell(49467))
                {
                    // 33% chance
                    if (roll_chance_i(33))
                    {
                        for(uint8 i = 2; i <= 5; i++)
                        {
                            ((Player*)m_caster)->ConvertRune(i, RUNE_DEATH);
                        }
                    }
                }
                else if ((Player*)m_caster->HasSpell(50033))
                {
                    // 66% chance
                    if (roll_chance_i(66))
                    {
                        for(uint8 i = 2; i <= 5; i++)
                        {
                            ((Player*)m_caster)->ConvertRune(i, RUNE_DEATH);
                        }
                    }
                }
                else if ((Player*)m_caster->HasSpell(50034))
                {
                    for(uint8 i = 2; i <= 5; i++)
                    {
                        ((Player*)m_caster)->ConvertRune(i, RUNE_DEATH);
                    }
                }

                uint32 count = 0;
                Unit::SpellAuraHolderMap const& auras = unitTarget->GetSpellAuraHolderMap();
                for (Unit::SpellAuraHolderMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                {
                    if (itr->second->GetSpellProto()->DispelType == DISPEL_DISEASE && itr->second->GetCasterGuid() == m_caster->GetObjectGuid())
                    {
                        ++count;
                        // max. 15%
                        if (count == 3)
                        {
                            break;
                        }
                    }
                }

                int32 bp = int32(count * m_caster->GetMaxHealth() * m_spellInfo->EffectChainAmplitude[EFFECT_INDEX_0] / 100);

                // Improved Death Strike (percent stored in nonexistent EFFECT_INDEX_2 effect base points)
                Unit::AuraList const& auraMod = m_caster->GetAurasByType(SPELL_AURA_ADD_FLAT_MODIFIER);
                for (Unit::AuraList::const_iterator iter = auraMod.begin(); iter != auraMod.end(); ++iter)
                {
                    // only required spell have spellicon for SPELL_AURA_ADD_FLAT_MODIFIER
                    if ((*iter)->GetSpellProto()->SpellIconID == 2751 && (*iter)->GetSpellProto()->SpellClassSet == SPELLFAMILY_DEATHKNIGHT)
                    {
                        bp += (*iter)->GetSpellProto()->CalculateSimpleValue(EFFECT_INDEX_2) * bp / 100;
                        break;
                    }
                }

                m_caster->CastCustomSpell(m_caster, 45470, &bp, NULL, NULL, true);
                return;
            }
            // Death Grip
            else if (m_spellInfo->ID == 49576)
            {
                if (!unitTarget)
                {
                    return;
                }

                m_caster->CastSpell(unitTarget, 49560, true);
                return;
            }
            else if (m_spellInfo->ID == 49560)
            {
                if (!unitTarget || unitTarget == m_caster)
                {
                    return;
                }

                uint32 spellId = m_spellInfo->CalculateSimpleValue(EFFECT_INDEX_0);
                float dest_x, dest_y;
                m_caster->GetNearPoint2D(dest_x, dest_y, m_caster->GetObjectBoundingRadius() + unitTarget->GetObjectBoundingRadius(), m_caster->GetOrientation());
                unitTarget->CastSpell(dest_x, dest_y, m_caster->GetPositionZ() + 0.5f, spellId, true, NULL, NULL, m_caster->GetObjectGuid(), m_spellInfo);
                return;
            }
            // Obliterate
            else if (m_spellInfo->SpellClassMask & UI64LIT(0x0002000000000000))
            {

                // Death Rune Mastery talents
                if ((Player*)m_caster->HasSpell(49467))
                {
                    // 33% chance
                    if (roll_chance_i(33))
                    {
                        for(uint8 i = 2; i <= 5; i++)
                        {
                            ((Player*)m_caster)->ConvertRune(i, RUNE_DEATH);
                        }
                    }
                }
                else if ((Player*)m_caster->HasSpell(50033))
                {
                    // 66% chance
                    if (roll_chance_i(66))
                    {
                        for(uint8 i = 2; i <= 5; i++)
                        {
                            ((Player*)m_caster)->ConvertRune(i, RUNE_DEATH);
                        }
                    }
                }
                else if ((Player*)m_caster->HasSpell(50034))
                {
                    for(uint8 i = 2; i <= 5; i++)
                    {
                        ((Player*)m_caster)->ConvertRune(i, RUNE_DEATH);
                    }
                }

                // search for Annihilation
                Unit::AuraList const& dummyList = m_caster->GetAurasByType(SPELL_AURA_DUMMY);
                for (Unit::AuraList::const_iterator itr = dummyList.begin(); itr != dummyList.end(); ++itr)
                {
                    if ((*itr)->GetSpellProto()->SpellClassSet == SPELLFAMILY_DEATHKNIGHT && (*itr)->GetSpellProto()->SpellIconID == 2710)
                    {
                        if (roll_chance_i((*itr)->GetModifier()->m_amount)) // don't consume if found
                        {
                            return;
                        }
                        else
                        {
                            break;
                        }
                    }
                }

                // consume diseases
                unitTarget->RemoveAurasWithDispelType(DISPEL_DISEASE, m_caster->GetObjectGuid());
            }
            break;
        }
    }

    // pet auras
    if (PetAura const* petSpell = sSpellMgr.GetPetAura(m_spellInfo->ID, eff_idx))
    {
        m_caster->AddPetAura(petSpell);
        return;
    }

    // Script based implementation. Must be used only for not good for implementation in core spell effects
    // So called only for not processed cases
    bool libraryResult = false;
    if (gameObjTarget)
    {
        libraryResult = sScriptMgr.OnEffectDummy(m_caster, m_spellInfo->ID, eff_idx, gameObjTarget, m_originalCasterGUID);
    }
    else if (unitTarget && (unitTarget->GetTypeId() == TYPEID_UNIT || unitTarget->GetTypeId() == TYPEID_PLAYER))
    {
        libraryResult = sScriptMgr.OnEffectDummy(m_caster, m_spellInfo->ID, eff_idx, unitTarget, m_originalCasterGUID);
    }
    else if (itemTarget)
    {
        libraryResult = sScriptMgr.OnEffectDummy(m_caster, m_spellInfo->ID, eff_idx, itemTarget, m_originalCasterGUID);
    }

    if (libraryResult || !unitTarget)
    {
        return;
    }

    // Previous effect might have started script
    if (!ScriptMgr::CanSpellEffectStartDBScript(m_spellInfo, eff_idx))
    {
        return;
    }

    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Spell ScriptStart spellid %u in EffectDummy", m_spellInfo->ID);
    m_caster->GetMap()->ScriptsStart(DBS_ON_SPELL, m_spellInfo->ID, m_caster, unitTarget);
}
