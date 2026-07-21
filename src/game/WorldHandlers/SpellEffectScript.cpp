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
 * @file SpellEffectScript.cpp
 * @brief Cohesion split of SpellEffects.cpp -- EffectScriptEffect handler.
 *        Same `Spell` class; no behaviour change.
 */

#include <iterator>
#include "Platform/Define.h"
#include "Common/TimeConstants.h"
#include "Utilities/MathDefines.h"
#include <cstdlib>
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
 * @brief Executes script-driven spell effect behavior for special cases.
 *
 * @param eff_idx The script effect index.
 */
void Spell::EffectScriptEffect(SpellEffectIndex eff_idx)
{
    // TODO: we must implement hunter pet summon at login there (spell 6962)

    switch (m_spellInfo->SpellClassSet)
    {
        case SPELLFAMILY_GENERIC:
        {
            switch (m_spellInfo->ID)
            {
                case 1509:                                  // GM Mode OFF
                {
                    if (unitTarget->GetTypeId() == TYPEID_PLAYER)
                    {
                        ((Player*)unitTarget)->SetGameMaster(false);
                    }
                    break;
                }
                case 18139:                                 // GM Mode ON
                {
                    if (unitTarget->GetTypeId() == TYPEID_PLAYER)
                    {
                        ((Player*)unitTarget)->SetGameMaster(true);
                    }
                    break;
                }

                case 5249:                                  // Ice Lock
                {
                    if (unitTarget)
                    {
                        m_caster->CastSpell(unitTarget, 22856, true);
                        sLog.outString("EffectScriptEffect : %s target of spell 5249", unitTarget->GetName());
                    }
                    break;
                }
                case 8856:                                  // Bending Shinbone
                {
                    if (!itemTarget && m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    uint32 spell_id = 0;
                    switch (urand(1, 5))
                    {
                        case 1:  spell_id = 8854; break;
                        default: spell_id = 8855; break;
                    }

                    m_caster->CastSpell(m_caster, spell_id, true, NULL);
                    return;
                }
                case 17512:                                 // Piccolo of the Flaming Fire
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    unitTarget->HandleEmoteCommand(EMOTE_STATE_DANCE);
                    return;
                }
                case 20589:                                 // Escape artist
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->RemoveSpellsCausingAura(SPELL_AURA_MOD_ROOT);
                    unitTarget->RemoveSpellsCausingAura(SPELL_AURA_MOD_DECREASE_SPEED);
                    return;
                }
                case 22539:                                 // Shadow Flame (All script effects, not just end ones to
                case 22972:                                 // prevent player from dodging the last triggered spell)
                case 22975:
                case 22976:
                case 22977:
                case 22978:
                case 22979:
                case 22980:
                case 22981:
                case 22982:
                case 22983:
                case 22984:
                case 22985:
                {
                    if (!unitTarget || !unitTarget->IsAlive())
                    {
                        return;
                    }

                    // Onyxia Scale Cloak
                    if (unitTarget->GetDummyAura(22683))
                    {
                        return;
                    }

                    // Shadow Flame
                    m_caster->CastSpell(unitTarget, 22682, true);
                    return;
                }
                case 24194:                                 // Uther's Tribute
                case 24195:                                 // Grom's Tribute
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    uint8 race = m_caster->getRace();
                    uint32 spellId = 0;

                    switch (m_spellInfo->ID)
                    {
                        case 24194:
                            switch (race)
                            {
                                case RACE_HUMAN:            spellId = 24105; break;
                                case RACE_DWARF:            spellId = 24107; break;
                                case RACE_NIGHTELF:         spellId = 24108; break;
                                case RACE_GNOME:            spellId = 24106; break;
                                case RACE_DRAENEI:          spellId = 69533; break;
                            }
                            break;
                        case 24195:
                            switch (race)
                            {
                                case RACE_ORC:              spellId = 24104; break;
                                case RACE_UNDEAD:           spellId = 24103; break;
                                case RACE_TAUREN:           spellId = 24102; break;
                                case RACE_TROLL:            spellId = 24101; break;
                                case RACE_BLOODELF:         spellId = 69530; break;
                            }
                            break;
                    }

                    if (spellId)
                    {
                        m_caster->CastSpell(m_caster, spellId, true);
                    }

                    return;
                }
                case 24320:                                 // Poisonous Blood
                {
                    unitTarget->CastSpell(unitTarget, 24321, true, NULL, NULL, m_caster->GetObjectGuid());
                    return;
                }
                case 24324:                                 // Blood Siphon
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    unitTarget->CastSpell(m_caster, unitTarget->HasAura(24321) ? 24323 : 24322, true);
                    return;
                }
                case 24590:                                 // Brittle Armor - need remove one 24575 Brittle Armor aura
                    unitTarget->RemoveAuraHolderFromStack(24575);
                    return;
                case 24714:                                 // Trick
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    if (roll_chance_i(14))                  // Trick (can be different critter models). 14% since below can have 1 of 6
                    {
                        m_caster->CastSpell(m_caster, 24753, true);
                    }
                    else                                    // Random Costume, 6 different (plus add. for gender)
                    {
                        m_caster->CastSpell(m_caster, 24720, true);
                    }

                    return;
                }
                case 24717:                                 // Pirate Costume
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    // Pirate Costume (male or female)
                    m_caster->CastSpell(unitTarget, unitTarget->getGender() == GENDER_MALE ? 24708 : 24709, true);
                    return;
                }
                case 24718:                                 // Ninja Costume
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    // Ninja Costume (male or female)
                    m_caster->CastSpell(unitTarget, unitTarget->getGender() == GENDER_MALE ? 24711 : 24710, true);
                    return;
                }
                case 24719:                                 // Leper Gnome Costume
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    // Leper Gnome Costume (male or female)
                    m_caster->CastSpell(unitTarget, unitTarget->getGender() == GENDER_MALE ? 24712 : 24713, true);
                    return;
                }
                case 24720:                                 // Random Costume
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    uint32 spellId = 0;

                    switch (urand(0, 6))
                    {
                        case 0:
                            spellId = unitTarget->getGender() == GENDER_MALE ? 24708 : 24709;
                            break;
                        case 1:
                            spellId = unitTarget->getGender() == GENDER_MALE ? 24711 : 24710;
                            break;
                        case 2:
                            spellId = unitTarget->getGender() == GENDER_MALE ? 24712 : 24713;
                            break;
                        case 3:
                            spellId = 24723;
                            break;
                        case 4:
                            spellId = 24732;
                            break;
                        case 5:
                            spellId = unitTarget->getGender() == GENDER_MALE ? 24735 : 24736;
                            break;
                        case 6:
                            spellId = 24740;
                            break;
                    }

                    m_caster->CastSpell(unitTarget, spellId, true);
                    return;
                }
                case 24737:                                 // Ghost Costume
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    // Ghost Costume (male or female)
                    m_caster->CastSpell(unitTarget, unitTarget->getGender() == GENDER_MALE ? 24735 : 24736, true);
                    return;
                }
                case 24751:                                 // Trick or Treat
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    // Tricked or Treated
                    unitTarget->CastSpell(unitTarget, 24755, true);

                    // Treat / Trick
                    unitTarget->CastSpell(unitTarget, roll_chance_i(50) ? 24714 : 24715, true);
                    return;
                }
                case 25140:                                 // Orb teleport spells
                case 25143:
                case 25650:
                case 25652:
                case 29128:
                case 29129:
                case 35376:
                case 35727:
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    uint32 spellid;
                    switch (m_spellInfo->ID)
                    {
                        case 25140: spellid =  32568; break;
                        case 25143: spellid =  32572; break;
                        case 25650: spellid =  30140; break;
                        case 25652: spellid =  30141; break;
                        case 29128: spellid =  32571; break;
                        case 29129: spellid =  32569; break;
                        case 35376: spellid =  25649; break;
                        case 35727: spellid =  35730; break;
                        default:
                            return;
                    }

                    unitTarget->CastSpell(unitTarget, spellid, false);
                    return;
                }
                case 26004:                                 // Mistletoe
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->HandleEmote(EMOTE_ONESHOT_CHEER);
                    return;
                }
                case 26137:                                 // Rotate Trigger
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, urand(0, 1) ? 26009 : 26136, true);
                    return;
                }
                case 26218:                                 // Mistletoe
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    uint32 spells[3] = {26206, 26207, 45036};

                    m_caster->CastSpell(unitTarget, spells[urand(0, 2)], true);
                    return;
                }
                case 26275:                                 // PX-238 Winter Wondervolt TRAP
                {
                    uint32 spells[4] = {26272, 26157, 26273, 26274};

                    // check presence
                    for (int j = 0; j < 4; ++j)
                    {
                        if (unitTarget->HasAura(spells[j], EFFECT_INDEX_0))
                        {
                            return;
                        }
                    }

                    // cast
                    unitTarget->CastSpell(unitTarget, spells[urand(0, 3)], true);
                    return;
                }
                case 26465:                                 // Mercurial Shield - need remove one 26464 Mercurial Shield aura
                    unitTarget->RemoveAuraHolderFromStack(26464);
                    return;
                case 26656:                                 // Summon Black Qiraji Battle Tank
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // Prevent stacking of mounts
                    unitTarget->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);

                    // Two separate mounts depending on area id (allows use both in and out of specific instance)
                    if (unitTarget->GetAreaId() == 3428)
                    {
                        unitTarget->CastSpell(unitTarget, 25863, false);
                    }
                    else
                    {
                        unitTarget->CastSpell(unitTarget, 26655, false);
                    }

                    return;
                }
                case 27687:                                 // Summon Bone Minions
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // Spells 27690, 27691, 27692, 27693 are missing from DBC
                    // So we need to summon creature 16119 manually
                    float x, y, z;
                    float angle = unitTarget->GetOrientation();
                    for (uint8 i = 0; i < 4; ++i)
                    {
                        unitTarget->GetNearPoint(unitTarget, x, y, z, unitTarget->GetObjectBoundingRadius(), INTERACTION_DISTANCE, angle + i * M_PI_F / 2);
                        unitTarget->SummonCreature(16119, x, y, z, angle, TEMPSPAWN_TIMED_OOC_OR_DEAD_DESPAWN, 10 * MINUTE * IN_MILLISECONDS);
                    }
                    return;
                }
                case 27695:                                 // Summon Bone Mages
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 27696, true);
                    unitTarget->CastSpell(unitTarget, 27697, true);
                    unitTarget->CastSpell(unitTarget, 27698, true);
                    unitTarget->CastSpell(unitTarget, 27699, true);
                    return;
                }
                case 28352:                                 // Breath of Sargeras
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 28342, true);
                    return;
                }
                case 28374:                                 // Decimate (Naxxramas: Gluth)
                case 54426:                                 // Decimate (Naxxramas: Gluth (spells are identical))
                case 71123:                                 // Decimate (ICC: Precious / Stinky)
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    float downToHealthPercent = (m_spellInfo->ID != 71123 ? 5 : m_spellInfo->CalculateSimpleValue(eff_idx)) * 0.01f;

                    int32 damage = unitTarget->GetHealth() - unitTarget->GetMaxHealth() * downToHealthPercent;
                    if (damage > 0)
                    {
                        m_caster->CastCustomSpell(unitTarget, 28375, &damage, NULL, NULL, true);
                    }
                    return;
                }
                case 28560:                                 // Summon Blizzard
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    m_caster->SummonCreature(16474, unitTarget->GetPositionX(), unitTarget->GetPositionY(), unitTarget->GetPositionZ(), 0.0f, TEMPSPAWN_TIMED_DESPAWN, 30000);
                    return;
                }
                case 28859:                                 // Cleansing Flames
                case 29126:                                 // Cleansing Flames (Darnassus)
                case 29135:                                 // Cleansing Flames (Ironforge)
                case 29136:                                 // Cleansing Flames (Orgrimmar)
                case 29137:                                 // Cleansing Flames (Stormwind)
                case 29138:                                 // Cleansing Flames (Thunder Bluff)
                case 29139:                                 // Cleansing Flames (Undercity)
                case 46671:                                 // Cleansing Flames (Exodar)
                case 46672:                                 // Cleansing Flames (Silvermoon)
                {
                    // Cleanse all magic, curse, disease and poison
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->RemoveAurasWithDispelType(DISPEL_MAGIC);
                    unitTarget->RemoveAurasWithDispelType(DISPEL_CURSE);
                    unitTarget->RemoveAurasWithDispelType(DISPEL_DISEASE);
                    unitTarget->RemoveAurasWithDispelType(DISPEL_POISON);

                    return;
                }
                case 29395:                                 // Break Kaliri Egg
                {
                    uint32 creature_id = 0;
                    uint32 rand = urand(0, 99);

                    if (rand < 10)
                    {
                        creature_id = 17034;
                    }
                    else if (rand < 60)
                    {
                        creature_id = 17035;
                    }
                    else
                    {
                        creature_id = 17039;
                    }

                    if (WorldObject* pSource = GetAffectiveCasterObject())
                    {
                        pSource->SummonCreature(creature_id, 0.0f, 0.0f, 0.0f, 0.0f, TEMPSPAWN_TIMED_OOC_OR_DEAD_DESPAWN, 120 * IN_MILLISECONDS);
                    }
                    return;
                }
                case 29830:                                 // Mirren's Drinking Hat
                {
                    uint32 item = 0;
                    switch (urand(1, 6))
                    {
                        case 1:
                        case 2:
                        case 3:
                            item = 23584; break;            // Loch Modan Lager
                        case 4:
                        case 5:
                            item = 23585; break;            // Stouthammer Lite
                        case 6:
                            item = 23586; break;            // Aerie Peak Pale Ale
                    }

                    if (item)
                    {
                        DoCreateItem(eff_idx, item);
                    }

                    break;
                }
                case 30541:                                 // Blaze
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 30542, true);
                    break;
                }
                case 30769:                                 // Pick Red Riding Hood
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    // cast Little Red Riding Hood
                    m_caster->CastSpell(unitTarget, 30768, true);
                    break;
                }
                case 30835:                                 // Infernal Relay
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 30836, true, NULL, NULL, m_caster->GetObjectGuid());
                    break;
                }
                case 30918:                                 // Improved Sprint
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // Removes snares and roots.
                    unitTarget->RemoveAurasAtMechanicImmunity(IMMUNE_TO_ROOT_AND_SNARE_MASK, 30918, true);
                    break;
                }
                case 37142:                                 // Karazhan - Chess NPC Action: Melee Attack: Conjured Water Elemental
                case 37143:                                 // Karazhan - Chess NPC Action: Melee Attack: Charger
                case 37147:                                 // Karazhan - Chess NPC Action: Melee Attack: Human Cleric
                case 37149:                                 // Karazhan - Chess NPC Action: Melee Attack: Human Conjurer
                case 37150:                                 // Karazhan - Chess NPC Action: Melee Attack: King Llane
                case 37220:                                 // Karazhan - Chess NPC Action: Melee Attack: Summoned Daemon
                case 32227:                                 // Karazhan - Chess NPC Action: Melee Attack: Footman
                case 32228:                                 // Karazhan - Chess NPC Action: Melee Attack: Grunt
                case 37337:                                 // Karazhan - Chess NPC Action: Melee Attack: Orc Necrolyte
                case 37339:                                 // Karazhan - Chess NPC Action: Melee Attack: Orc Wolf
                case 37345:                                 // Karazhan - Chess NPC Action: Melee Attack: Orc Warlock
                case 37348:                                 // Karazhan - Chess NPC Action: Melee Attack: Warchief Blackhand
                {
                        if (!unitTarget)
                        {
                            return;
                        }

                        m_caster->CastSpell(unitTarget, 32247, true);
                        return;
                }
                case 32301:                                 // Ping Shirrak
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // Cast Focus fire on caster
                    unitTarget->CastSpell(m_caster, 32300, true);
                    return;
                }
                case 33676:                                 // Incite Chaos
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 33684, true);
                    return;
                }
                case 34653:                                 // Fireball
                case 36920:                                 // Fireball (h)
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, unitTarget->GetMap()->IsRegularDifficulty() ? 23971 : 30928, true, NULL, NULL, m_caster->GetObjectGuid());
                    return;
                }
                case 35865:                                 // Summon Nether Vapor
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    float x, y, z;
                    for (uint8 i = 0; i < 4; ++i)
                    {
                        m_caster->GetNearPoint(m_caster, x, y, z, 0.0f, INTERACTION_DISTANCE, M_PI_F * .5f * i + M_PI_F * .25f);
                        m_caster->SummonCreature(21002, x, y, z, 0, TEMPSPAWN_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                    }
                    return;
                }
                case 37431:                                 // Spout
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, urand(0, 1) ? 37429 : 37430, true);
                    return;
                }
                case 37775:                                 // Karazhan - Chess NPC Action - Poison Cloud
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 37469, true);
                    return;
                }
                case 37824:                                 // Karazhan - Chess NPC Action - Shadow Mend
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 37456, true);
                    return;
                }
                case 38358:                                 // Tidal Surge
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 38353, true, NULL, NULL, m_caster->GetObjectGuid());
                    return;
                }
                case 39338:                                 // Karazhan - Chess, Medivh CHEAT: Hand of Medivh, Target Horde
                case 39342:                                 // Karazhan - Chess, Medivh CHEAT: Hand of Medivh, Target Alliance
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 39339, true);
                    return;
                }
                case 39341:                                 // Karazhan - Chess, Medivh CHEAT: Fury of Medivh, Target Horde
                case 39344:                                 // Karazhan - Chess, Medivh CHEAT: Fury of Medivh, Target Alliance
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    return;
                }
                case 41055:                                 // Copy Weapon
                case 63416:                                 // Copy Weapon
                case 69891:                                 // Copy Weapon (No Threat)
                {
                    if (m_caster->GetTypeId() != TYPEID_UNIT || !unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    if (Item* pItem = ((Player*)unitTarget)->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND))
                    {
                        ((Creature*)m_caster)->SetVirtualItem(VIRTUAL_ITEM_SLOT_0, pItem->GetEntry());

                        // Unclear what this spell should do
                        unitTarget->CastSpell(m_caster, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    }

                    return;
                }
                case 41072:                                 // Bloodbolt
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    return;
                }
                case 41126:                                 // Flame Crash
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 41131, true);
                    break;
                }
                case 42281:                                 // Sprouting
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->RemoveAurasDueToSpell(42280);
                    unitTarget->RemoveAurasDueToSpell(42294);
                    unitTarget->CastSpell(unitTarget, 42285, true);
                    unitTarget->CastSpell(unitTarget, 42291, true);
                    return;
                }
                case 42492:                                 // Cast Energized
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    uint32 questId = m_spellInfo->CalculateSimpleValue(EFFECT_INDEX_1);
                    if (!questId || !GetQuestTemplateStore(questId) || !((Player*)unitTarget)->IsCurrentQuest(questId))
                    {
                        return;
                    }

                    m_caster->CastSpell(m_caster, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    return;
                }
                case 42578:                                 // Cannon Blast
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    int32 basePoints = m_spellInfo->CalculateSimpleValue(eff_idx);
                    unitTarget->CastCustomSpell(unitTarget, 42576, &basePoints, NULL, NULL, true);
                    return;
                }
                case 43365:                                 // The Cleansing: Shrine Cast
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    // Script Effect Player Cast Mirror Image
                    m_caster->CastSpell(m_caster, 50217, true);
                    return;
                }
                case 43375:                                 // Mixing Vrykul Blood
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    uint32 triggeredSpell[] = {43376, 43378, 43970, 43377};

                    unitTarget->CastSpell(unitTarget, triggeredSpell[urand(0, 3)], true);
                    return;
                }
                case 44323:                                 // Hawk Hunting
                case 44407:                                 // Hawk Hunting
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    // check target entry specific to each spell
                    if (m_spellInfo->ID == 44323 && unitTarget->GetEntry() != 24746)
                    {
                        return;
                    }

                    if (m_spellInfo->ID == 44407 && unitTarget->GetEntry() != 24747)
                    {
                        return;
                    }

                    unitTarget->CastSpell(m_caster, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    // despawn delay depends on the distance between caster and target
                    ((Creature*)unitTarget)->ForcedDespawn(100 * unitTarget->GetDistance2d(m_caster));
                    return;
                }
                case 44364:                                 // Rock Falcon Primer
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    // Are there anything special with this, a random chance or condition?
                    // Feeding Rock Falcon
                    unitTarget->CastSpell(unitTarget, m_spellInfo->CalculateSimpleValue(eff_idx), true, NULL, NULL, unitTarget->GetObjectGuid(), m_spellInfo);
                    return;
                }
                case 44455:                                 // Character Script Effect Reverse Cast
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    Creature* pTarget = (Creature*)unitTarget;

                    if (const SpellEntry* pSpell = sSpellStore.LookupEntry(m_spellInfo->CalculateSimpleValue(eff_idx)))
                    {
                        // if we used item at least once...
                        if (pTarget->IsTemporarySummon() && int32(pTarget->GetEntry()) == pSpell->EffectMiscValue[eff_idx])
                        {
                            TemporarySummon* pSummon = (TemporarySummon*)pTarget;

                            // can only affect "own" summoned
                            if (pSummon->GetSummonerGuid() == m_caster->GetObjectGuid())
                            {
                                if (pTarget->hasUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE))
                                {
                                    pTarget->GetMotionMaster()->MovementExpired();
                                }

                                // trigger cast of quest complete script (see code for this spell below)
                                pTarget->CastSpell(pTarget, 44462, true);

                                pTarget->GetMotionMaster()->MovePoint(0, m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ());
                            }

                            return;
                        }

                        // or if it is first time used item, cast summon and despawn the target
                        m_caster->CastSpell(pTarget, pSpell, true);
                        pTarget->ForcedDespawn();

                        // TODO: here we should get pointer to the just summoned and make it move.
                        // without, it will be one extra use of quest item
                    }

                    return;
                }
                case 44462:                                 // Cast Quest Complete on Master
                {
                    if (m_caster->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    Creature* pQuestCow = NULL;

                    float range = 20.0f;

                    // search for a reef cow nearby
                    MaNGOS::NearestCreatureEntryWithLiveStateInObjectRangeCheck u_check(*m_caster, 24797, true, false, range);
                    MaNGOS::CreatureLastSearcher<MaNGOS::NearestCreatureEntryWithLiveStateInObjectRangeCheck> searcher(pQuestCow, u_check);

                    Cell::VisitGridObjects(m_caster, searcher, range);

                    // no cows found, so return
                    if (!pQuestCow)
                    {
                        return;
                    }

                    if (!((Creature*)m_caster)->IsTemporarySummon())
                    {
                        return;
                    }

                    if (const SpellEntry* pSpell = sSpellStore.LookupEntry(m_spellInfo->CalculateSimpleValue(eff_idx)))
                    {
                        TemporarySummon* pSummon = (TemporarySummon*)m_caster;

                        // all ok, so make summoner cast the quest complete
                        if (Unit* pSummoner = pSummon->GetSummoner())
                        {
                            pSummoner->CastSpell(pSummoner, pSpell, true);
                        }
                    }

                    return;
                }
                case 44876:                                 // Force Cast - Portal Effect: Sunwell Isle
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 44870, true);
                    break;
                }
                case 44811:                                 // Spectral Realm
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    // If the player can't be teleported, send him a notification
                    if (unitTarget->HasAura(44867))
                    {
                        ((Player*)unitTarget)->GetSession()->SendNotification(LANG_FAIL_ENTER_SPECTRAL_REALM);
                        return;
                    }

                    // Teleport target to the spectral realm, add debuff and force faction
                    unitTarget->CastSpell(unitTarget, 46019, true);
                    unitTarget->CastSpell(unitTarget, 46021, true);
                    unitTarget->CastSpell(unitTarget, 44845, true);
                    unitTarget->CastSpell(unitTarget, 44852, true);
                    return;
                }
                case 45141:                                 // Burn
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 46394, true, NULL, NULL, m_caster->GetObjectGuid());
                    return;
                }
                case 45151:                                 // Burn
                {
                    if (!unitTarget || unitTarget->HasAura(46394))
                    {
                        return;
                    }

                    // Make the burn effect jump to another friendly target
                    unitTarget->CastSpell(unitTarget, 46394, true);
                    return;
                }
                case 45185:                                 // Stomp
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // Remove the burn effect
                    unitTarget->RemoveAurasDueToSpell(46394);
                    return;
                }
                case 45204:                                 // Clone Me!
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(m_caster, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    return;
                }
                case 45206:                                 // Copy Off-hand Weapon
                case 69892:                                 // Copy Off-hand Weapon (No Threat)
                {
                    if (m_caster->GetTypeId() != TYPEID_UNIT || !unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    if (Item* pItem = ((Player*)unitTarget)->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND))
                    {
                        ((Creature*)m_caster)->SetVirtualItem(VIRTUAL_ITEM_SLOT_1, pItem->GetEntry());

                        // Unclear what this spell should do
                        unitTarget->CastSpell(m_caster, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    }

                    return;
                }
                case 45235:                                 // Blaze
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 45236, true, NULL, NULL, m_caster->GetObjectGuid());
                    return;
                }
                case 45260:                                 // Karazhan - Chess - Force Player to Kill Bunny
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 45259, true);
                    return;
                }
                case 45313:                                 // Anchor Here
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    ((Creature*)unitTarget)->SetRespawnCoord(unitTarget->GetPositionX(), unitTarget->GetPositionY(), unitTarget->GetPositionZ(), unitTarget->GetOrientation());
                    return;
                }
                case 45625:                                 // Arcane Chains: Character Force Cast
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    return;
                }
                case 45668:                                 // Ultra-Advanced Proto-Typical Shortening Blaster
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    if (roll_chance_i(25))                  // chance unknown, using 25
                    {
                        return;
                    }

                    static uint32 const spellPlayer[5] =
                    {
                        45674,                              // Bigger!
                        45675,                              // Shrunk
                        45678,                              // Yellow
                        45682,                              // Ghost
                        45684                               // Polymorph
                    };

                    static uint32 const spellTarget[5] =
                    {
                        45673,                              // Bigger!
                        45672,                              // Shrunk
                        45677,                              // Yellow
                        45681,                              // Ghost
                        45683                               // Polymorph
                    };

                    m_caster->CastSpell(m_caster, spellPlayer[urand(0, 4)], true);
                    unitTarget->CastSpell(unitTarget, spellTarget[urand(0, 4)], true);

                    return;
                }
                case 45691:                                 // Magnataur On Death 1
                {
                    // assuming caster is creature, if not, then return
                    if (m_caster->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    Player* pPlayer = ((Creature*)m_caster)->GetOriginalLootRecipient();

                    if (!pPlayer)
                    {
                        return;
                    }

                    if (pPlayer->HasAura(45674) || pPlayer->HasAura(45675) || pPlayer->HasAura(45678) || pPlayer->HasAura(45682) || pPlayer->HasAura(45684))
                    {
                        pPlayer->CastSpell(pPlayer, 45686, true);
                    }

                    m_caster->CastSpell(m_caster, 45685, true);

                    return;
                }
                case 45713:                                 // Naked Caravan Guard - Master Transform
                {
                    if (m_caster->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    const CreatureInfo* cTemplate = NULL;

                    switch (m_caster->GetEntry())
                    {
                        case 25342: cTemplate = ObjectMgr::GetCreatureTemplate(25340); break;
                        case 25343: cTemplate = ObjectMgr::GetCreatureTemplate(25341); break;
                    }

                    if (!cTemplate)
                    {
                        return;
                    }

                    uint32 display_id = 0;

                    // Spell is designed to be used in creature addon.
                    // This makes it possible to set proper model before adding to map.
                    // For later, spell is used in gossip (with following despawn,
                    // so addon can reload the default model and data again).

                    // It should be noted that additional spell id's have been seen in relation to this spell, but
                    // those does not exist in client (45701 (regular spell), 45705-45712 (auras), 45459-45460 (auras)).
                    // We can assume 45705-45712 are transform auras, used instead of hard coded models in the below code.

                    // not in map yet OR no npc flags yet (restored after LoadCreatureAddon for respawn cases)
                    if (!m_caster->IsInMap(m_caster) || m_caster->GetUInt32Value(UNIT_NPC_FLAGS) == UNIT_NPC_FLAG_NONE)
                    {
                        display_id = Creature::ChooseDisplayId(cTemplate);
                        ((Creature*)m_caster)->LoadEquipment(((Creature*)m_caster)->GetEquipmentId());
                    }
                    else
                    {
                        m_caster->SetUInt32Value(UNIT_NPC_FLAGS, cTemplate->NpcFlags);
                        ((Creature*)m_caster)->SetVirtualItem(VIRTUAL_ITEM_SLOT_0, 0);
                        ((Creature*)m_caster)->SetVirtualItem(VIRTUAL_ITEM_SLOT_1, 0);

                        switch (m_caster->GetDisplayId())
                        {
                            case 23246: display_id = 23245; break;
                            case 23247: display_id = 23250; break;
                            case 23248: display_id = 23251; break;
                            case 23249: display_id = 23252; break;
                            case 23124: display_id = 23253; break;
                            case 23125: display_id = 23254; break;
                            case 23126: display_id = 23255; break;
                            case 23127: display_id = 23256; break;
                        }
                    }

                    m_caster->SetDisplayId(display_id);
                    return;
                }
                case 45714:                                 // Fog of Corruption (caster inform)
                {
                    if (!unitTarget || m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    unitTarget->CastSpell(m_caster, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    return;
                }
                case 45717:                                 // Fog of Corruption (player buff)
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 45726, true);
                    return;
                }
                case 45785:                                 // Sinister Reflection Clone
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    return;
                }
                case 45833:                                 // Power of the Blue Flight
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 45836, true);
                    return;
                }
                case 45892:                                 // Sinister Reflection
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    // Summon 4 clones of the same player
                    for (uint8 i = 0; i < 4; ++i)
                    {
                        unitTarget->CastSpell(unitTarget, 45891, true, NULL, NULL, m_caster->GetObjectGuid());
                    }
                    return;
                }
                case 45918:                                 // Soul Sever
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER || !unitTarget->HasAura(45717))
                    {
                        return;
                    }

                    // kill all charmed targets
                    unitTarget->CastSpell(unitTarget, 45917, true);
                    return;
                }
                case 45958:                                 // Signal Alliance
                {
                    // "escort" aura not present, so let nothing happen
                    if (!m_caster->HasAura(m_spellInfo->CalculateSimpleValue(eff_idx)))
                    {
                        return;
                    }
                    // "escort" aura is present so break; and let DB table dbscripts_on_spell be used and process further.
                    else
                    {
                        break;
                    }
                }
                case 46203:                                 // Goblin Weather Machine
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    uint32 spellId = 0;
                    switch (rand() % 4)
                    {
                        case 0: spellId = 46740; break;
                        case 1: spellId = 46739; break;
                        case 2: spellId = 46738; break;
                        case 3: spellId = 46736; break;
                    }
                    unitTarget->CastSpell(unitTarget, spellId, true);
                    break;
                }
                case 46289:                                 // Negative Energy
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 46285, true);
                    return;
                }
                case 46430:                                 // Synch Health
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->SetHealth(m_caster->GetHealth());
                    return;
                }
                case 46642:                                 //5,000 Gold
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    ((Player*)unitTarget)->ModifyMoney(50000000);
                    break;
                }
                case 47097:                                 // Surge Needle Teleporter
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    if (unitTarget->GetAreaId() == 4157)
                    {
                        unitTarget->CastSpell(unitTarget, 47324, true);
                    }
                    else if (unitTarget->GetAreaId() == 4156)
                    {
                        unitTarget->CastSpell(unitTarget, 47325, true);
                    }

                    break;
                }
                case 47311:                                 // Quest - Jormungar Explosion Spell Spawner
                {
                    // Summons npc's. They are expected to summon GO from 47315
                    // but there is no way to get the summoned, to trigger a spell
                    // cast (workaround can be done with ai script).

                    // Quest - Jormungar Explosion Summon Object
                    for (int i = 0; i < 2; ++i)
                    {
                        m_caster->CastSpell(m_caster, 47309, true);
                    }

                    for (int i = 0; i < 2; ++i)
                    {
                        m_caster->CastSpell(m_caster, 47924, true);
                    }

                    for (int i = 0; i < 2; ++i)
                    {
                        m_caster->CastSpell(m_caster, 47925, true);
                    }

                    return;
                }
                case 47393:                                 // The Focus on the Beach: Quest Completion Script
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // Ley Line Information
                    unitTarget->RemoveAurasDueToSpell(47391);
                    return;
                }
                case 47615:                                 // Atop the Woodlands: Quest Completion Script
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // Ley Line Information
                    unitTarget->RemoveAurasDueToSpell(47473);
                    return;
                }
                case 47638:                                 // The End of the Line: Quest Completion Script
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // Ley Line Information
                    unitTarget->RemoveAurasDueToSpell(47636);
                    return;
                }
                case 47703:                                 // Unholy Union
                {
                    m_caster->CastSpell(m_caster, 50254, true);
                    return;
                }
                case 47724:                                 // Frost Draw
                {
                    m_caster->CastSpell(m_caster, 50239, true);
                    return;
                }
                case 47958:                                 // Crystal Spikes
                case 57083:                                 // Crystal Spikes (h2)
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 47954, true);
                    unitTarget->CastSpell(unitTarget, 47955, true);
                    unitTarget->CastSpell(unitTarget, 47956, true);
                    unitTarget->CastSpell(unitTarget, 47957, true);
                    return;
                }
                case 48590:                                 // Avenging Spirits
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // Summon 4 spirits summoners
                    unitTarget->CastSpell(unitTarget, 48586, true);
                    unitTarget->CastSpell(unitTarget, 48587, true);
                    unitTarget->CastSpell(unitTarget, 48588, true);
                    unitTarget->CastSpell(unitTarget, 48589, true);
                    return;
                }
                case 48603:                                 // High Executor's Branding Iron
                    // Torture the Torturer: High Executor's Branding Iron Impact
                    unitTarget->CastSpell(unitTarget, 48614, true);
                    return;
                case 48724:                                 // The Denouncement: Commander Jordan On Death
                case 48726:                                 // The Denouncement: Lead Cannoneer Zierhut On Death
                case 48728:                                 // The Denouncement: Blacksmith Goodman On Death
                case 48730:                                 // The Denouncement: Stable Master Mercer On Death
                {
                    // Compelled
                    if (!unitTarget || !m_caster->HasAura(48714))
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    return;
                }
                // Gender spells
                case 48762:                                 // A Fall from Grace: Scarlet Raven Priest Image - Master
                case 45759:                                 // Warsong Orc Disguise
                case 69672:                                 // Sunreaver Disguise
                case 69673:                                 // Silver Covenant Disguise
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    uint8 gender = unitTarget->getGender();
                    uint32 spellId;
                    switch (m_spellInfo->ID)
                    {
                        case 48762: spellId = (gender == GENDER_MALE ? 48763 : 48761); break;
                        case 45759: spellId = (gender == GENDER_MALE ? 45760 : 45762); break;
                        case 69672: spellId = (gender == GENDER_MALE ? 70974 : 70973); break;
                        case 69673: spellId = (gender == GENDER_MALE ? 70972 : 70971); break;
                        default: return;
                    }
                    unitTarget->CastSpell(unitTarget, spellId, true);
                    return;
                }
                case 48810:                                 // Death's Door
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    // Spell effect order will summon creature first and then apply invisibility to caster.
                    // This result in that summoner/summoned can not see each other and that is not expected.
                    // Aura from 48814 can be used as a hack from creature_addon, but we can not get the
                    // summoned to cast this from this spell effect since we have no way to get pointer to creature.
                    // Most proper would be to summon to same visibility mask as summoner, and not use spell at all.

                    // Binding Life
                    m_caster->CastSpell(m_caster, 48809, true);

                    // After (after: meaning creature does not have auras at creation)
                    // creature is summoned and visible for player in map, it is expected to
                    // gain two auras. First from 29266(aura slot0) and then from 48808(aura slot1).
                    // We have no pointer to summoned, so only 48808 is possible from this spell effect.

                    // Binding Death
                    m_caster->CastSpell(m_caster, 48808, true);
                    return;
                }
                case 48811:                                 // Despawn Forgotten Soul
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    if (!((Creature*)unitTarget)->IsTemporarySummon())
                    {
                        return;
                    }

                    TemporarySummon* pSummon = (TemporarySummon*)unitTarget;

                    Unit::AuraList const& images = unitTarget->GetAurasByType(SPELL_AURA_MIRROR_IMAGE);

                    if (images.empty())
                    {
                        return;
                    }

                    Unit* pCaster = images.front()->GetCaster();
                    Unit* pSummoner = unitTarget->GetMap()->GetUnit(pSummon->GetSummonerGuid());

                    if (pSummoner && pSummoner == pCaster)
                    {
                        pSummon->UnSummon();
                    }

                    return;
                }
                case 48917:                                 // Who Are They: Cast from Questgiver
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    // Male Shadowy Disguise / Female Shadowy Disguise
                    unitTarget->CastSpell(unitTarget, unitTarget->getGender() == GENDER_MALE ? 38080 : 38081, true);
                    // Shadowy Disguise
                    unitTarget->CastSpell(unitTarget, 32756, true);
                    return;
                }
                case 49380:                                 // Consume
                case 59803:                                 // Consume (heroic)
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // Each target hit buffs the caster
                    unitTarget->CastSpell(m_caster, m_spellInfo->ID == 49380 ? 49381 : 59805, true, NULL, NULL, m_caster->GetObjectGuid());
                    return;
                }
                case 49405:                                 // Invader Taunt Trigger
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(m_caster, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    return;
                }
                case 50217:                                 // The Cleansing: Script Effect Player Cast Mirror Image
                {
                    // Summon Your Inner Turmoil
                    m_caster->CastSpell(m_caster, 50167, true);

                    // Spell 50218 has TARGET_SCRIPT, but other wild summons near may exist, and then target can become wrong
                    // Only way to make this safe is to get the actual summoned by m_caster

                    // Your Inner Turmoil's Mirror Image Aura
                    m_caster->CastSpell(m_caster, 50218, true);

                    return;
                }
                case 50218:                                 // The Cleansing: Your Inner Turmoil's Mirror Image Aura
                {
                    if (!m_originalCaster || m_originalCaster->GetTypeId() != TYPEID_PLAYER || !unitTarget)
                    {
                        return;
                    }

                    // determine if and what weapons can be copied
                    switch (eff_idx)
                    {
                        case EFFECT_INDEX_1:
                            if (((Player*)m_originalCaster)->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND))
                            {
                                unitTarget->CastSpell(m_originalCaster, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                            }

                            return;
                        case EFFECT_INDEX_2:
                            if (((Player*)m_originalCaster)->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND))
                            {
                                unitTarget->CastSpell(m_originalCaster, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                            }

                            return;
                        default:
                            return;
                    }
                    return;
                }
                case 50238:                                 // The Cleansing: Your Inner Turmoil's On Death Cast on Master
                {
                    if (m_caster->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    if (((Creature*)m_caster)->IsTemporarySummon())
                    {
                        TemporarySummon* pSummon = (TemporarySummon*)m_caster;

                        if (pSummon->GetSummonerGuid().IsPlayer())
                        {
                            if (Player* pSummoner = sObjectMgr.GetPlayer(pSummon->GetSummonerGuid()))
                            {
                                pSummoner->CastSpell(pSummoner, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                            }
                        }
                    }

                    return;
                }
                case 50252:                                 // Blood Draw
                {
                    m_caster->CastSpell(m_caster, 50250, true);
                    return;
                }
                case 50255:                                 // Poisoned Spear
                case 59331:                                 // Poisoned Spear (heroic)
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, m_spellInfo->CalculateSimpleValue(eff_idx), true, NULL, NULL, m_originalCasterGUID);
                    return;
                }
                case 50439:                                 // Script Cast Summon Image of Drakuru 05
                {
                    // TODO: check if summon already exist, if it does in this instance, return.

                    // Summon Drakuru
                    m_caster->CastSpell(m_caster, 50446, true);
                    return;
                }
                case 50630:                                 // Eject All Passengers
                {
                    m_caster->RemoveSpellsCausingAura(SPELL_AURA_CONTROL_VEHICLE);
                    return;
                }
                case 50725:                                 // Vigilance - remove cooldown on Taunt
                {
                    Unit* caster = GetAffectiveCaster();
                    if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    ((Player*)caster)->RemoveSpellCategoryCooldown(82, true);
                    return;
                }
                case 50742:                                 // Ooze Combine
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 50747, true);
                    ((Creature*)m_caster)->ForcedDespawn();
                    return;
                }
                case 50810:                                 // Shatter
                case 61546:                                 // Shatter (h)
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    if (!unitTarget->HasAura(50812))
                    {
                        return;
                    }

                    unitTarget->RemoveAurasDueToSpell(50812);
                    unitTarget->CastSpell(unitTarget, m_spellInfo->ID == 50810 ? 50811 : 61547 , true, NULL, NULL, m_caster->GetObjectGuid());
                    return;
                }
                case 50894:                                 // Zul'Drak Rat
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    if (SpellAuraHolder* pHolder = unitTarget->GetSpellAuraHolder(m_spellInfo->ID))
                    {
                        if (pHolder->GetStackAmount() + 1 >= m_spellInfo->CumulativeAura)
                        {
                            // Gluttonous Lurkers: Summon Gorged Lurking Basilisk
                            unitTarget->CastSpell(m_caster, 50928, true);
                            ((Creature*)unitTarget)->ForcedDespawn(1);
                        }
                    }

                    return;
                }
                case 51519:                                 // Death Knight Initiate Visual
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    uint32 spellId = 0;

                    bool isMale = unitTarget->getGender() == GENDER_MALE;
                    switch (unitTarget->getRace())
                    {
                        case RACE_HUMAN:    spellId = isMale ? 51520 : 51534; break;
                        case RACE_DWARF:    spellId = isMale ? 51538 : 51537; break;
                        case RACE_NIGHTELF: spellId = isMale ? 51535 : 51536; break;
                        case RACE_GNOME:    spellId = isMale ? 51539 : 51540; break;
                        case RACE_DRAENEI:  spellId = isMale ? 51541 : 51542; break;
                        case RACE_ORC:      spellId = isMale ? 51543 : 51544; break;
                        case RACE_UNDEAD:   spellId = isMale ? 51549 : 51550; break;
                        case RACE_TAUREN:   spellId = isMale ? 51547 : 51548; break;
                        case RACE_TROLL:    spellId = isMale ? 51546 : 51545; break;
                        case RACE_BLOODELF: spellId = isMale ? 51551 : 51552; break;
                        default:
                            return;
                    }

                    unitTarget->CastSpell(unitTarget, spellId, true);
                    return;
                }
                case 51770:                                 // Emblazon Runeblade
                {
                    Unit* caster = GetAffectiveCaster();
                    if (!caster)
                    {
                        return;
                    }

                    caster->CastSpell(caster, damage, false);
                    break;
                }
                case 51864:                                 // Player Summon Nass
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    // Summon Nass
                    if (const SpellEntry* pSpell = sSpellStore.LookupEntry(51865))
                    {
                        // Only if he is not already there
                        if (!m_caster->FindGuardianWithEntry(pSpell->EffectMiscValue[EFFECT_INDEX_0]))
                        {
                            m_caster->CastSpell(m_caster, pSpell, true);

                            if (Pet* pPet = m_caster->FindGuardianWithEntry(pSpell->EffectMiscValue[EFFECT_INDEX_0]))
                            {
                                // Nass Periodic Say aura
                                pPet->CastSpell(pPet, 51868, true);
                            }
                        }
                    }
                    return;
                }
                case 51889:                                 // Quest Accept Summon Nass
                {
                    // This is clearly for quest accept, is spell 51864 then for gossip and does pretty much the same thing?
                    // Just "jumping" to what may be the "gossip-spell" for now, doing the same thing
                    m_caster->CastSpell(m_caster, 51864, true);
                    return;
                }
                case 51904:                                 // Summon Ghouls On Scarlet Crusade
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // cast Summon Ghouls On Scarlet Crusade
                    float x, y, z;
                    m_targets.getDestination(x, y, z);
                    unitTarget->CastSpell(x, y, z, 54522, true, NULL, NULL, m_originalCasterGUID);
                    return;
                }
                case 51910:                                 // Kickin' Nass: Quest Completion
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    if (const SpellEntry* pSpell = sSpellStore.LookupEntry(51865))
                    {
                        // Is this all to be done at completion?
                        if (Pet* pPet = m_caster->FindGuardianWithEntry(pSpell->EffectMiscValue[EFFECT_INDEX_0]))
                        {
                            pPet->Unsummon(PET_SAVE_AS_DELETED, m_caster);
                        }
                    }
                    return;
                }
                case 52479:                                 // Gift of the Harvester
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER || !unitTarget)
                    {
                        return;
                    }

                    // Each ghoul casts 52500 onto player, so use number of auras as check
                    Unit::SpellAuraHolderConstBounds bounds = m_caster->GetSpellAuraHolderBounds(52500);
                    uint32 summonedGhouls = std::distance(bounds.first, bounds.second);

                    m_caster->CastSpell(unitTarget->GetPositionX(), unitTarget->GetPositionY(), unitTarget->GetPositionZ(), urand(0, 2) || summonedGhouls >= 5 ? 52505 : 52490, true);
                    return;
                }
                case 52555:                                 // Dispel Scarlet Ghoul Credit Counter
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->RemoveAurasByCasterSpell(m_spellInfo->CalculateSimpleValue(eff_idx), m_caster->GetObjectGuid());
                    return;
                }
                case 52694:                                 // Recall Eye of Acherus
                {
                    if (!m_caster || m_caster->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    Unit* charmer = m_caster->GetCharmer();
                    if (!charmer || charmer->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    charmer->RemoveAurasDueToSpell(51923);

                    // HACK ALERT
                    // Replace with Spell Interrupting, when casting spells properly is possible in mangos
                    //charmer->InterruptNonMeleeSpells(true);

                    Player* player = (Player*)charmer;
                    Creature* possessed = (Creature*)m_caster;
                    player->RemoveAurasDueToSpell(51852);

                    player->SetCharm(NULL);
                    player->SetClientControl(possessed, 0);
                    player->SetMover(NULL);
                    player->GetCamera().ResetView();
                    player->RemovePetActionBar();

                    possessed->clearUnitState(UNIT_STAT_CONTROLLED);
                    possessed->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);
                    possessed->SetCharmerGuid(ObjectGuid());
                    possessed->ForcedDespawn();

                    return;
                }
                case 52751:                                 // Death Gate
                {
                    if (!unitTarget || unitTarget->getClass() != CLASS_DEATH_KNIGHT)
                    {
                        return;
                    }

                    // triggered spell is stored in m_spellInfo->EffectBasePoints[0]
                    unitTarget->CastSpell(unitTarget, damage, false);
                    break;
                }
                case 52941:                                 // Song of Cleansing
                {
                    uint32 spellId = 0;

                    switch (m_caster->GetAreaId())
                    {
                        case 4385: spellId = 52954; break;  // Bittertide Lake
                        case 4290: spellId = 52958; break;  // River's Heart
                        case 4388: spellId = 52959; break;  // Wintergrasp River
                    }

                    if (spellId)
                    {
                        m_caster->CastSpell(m_caster, spellId, true);
                    }

                    break;
                }
                case 53110:                                 // Devour Humanoid
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    unitTarget->CastSpell(m_caster, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    ((Creature*)unitTarget)->ForcedDespawn(8000);
                    return;
                }
                case 54182:                                 // An End to the Suffering: Quest Completion Script
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // Remove aura (Mojo of Rhunok) given at quest accept / gossip
                    unitTarget->RemoveAurasDueToSpell(51967);
                    return;
                }
                case 54581:                                 // Mammoth Explosion Spell Spawner
                {
                    if (m_caster->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    // Summons misc npc's. They are expected to summon GO from 54625
                    // but there is no way to get the summoned, to trigger a spell
                    // cast (workaround can be done with ai script).

                    // Quest - Mammoth Explosion Summon Object
                    for (int i = 0; i < 2; ++i)
                    {
                        m_caster->CastSpell(m_caster, 54623, true);
                    }

                    for (int i = 0; i < 2; ++i)
                    {
                        m_caster->CastSpell(m_caster, 54627, true);
                    }

                    for (int i = 0; i < 2; ++i)
                    {
                        m_caster->CastSpell(m_caster, 54628, true);
                    }

                    // Summon Main Mammoth Meat
                    m_caster->CastSpell(m_caster, 57444, true);
                    return;
                }
                case 54436:                                 // Demonic Empowerment (succubus Vanish effect)
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->RemoveSpellsCausingAura(SPELL_AURA_MOD_ROOT);
                    unitTarget->RemoveSpellsCausingAura(SPELL_AURA_MOD_DECREASE_SPEED);
                    unitTarget->RemoveSpellsCausingAura(SPELL_AURA_MOD_STALKED);
                    unitTarget->RemoveSpellsCausingAura(SPELL_AURA_MOD_STUN);
                    return;
                }
                case 55693:                                 // Remove Collapsing Cave Aura
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->RemoveAurasDueToSpell(m_spellInfo->CalculateSimpleValue(eff_idx));
                    break;
                }
                case 56072:                                 // Ride Red Dragon Buddy
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    break;
                }
                case 57082:                                 // Crystal Spikes (h1)
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 57077, true);
                    unitTarget->CastSpell(unitTarget, 57078, true);
                    unitTarget->CastSpell(unitTarget, 57080, true);
                    unitTarget->CastSpell(unitTarget, 57081, true);
                    return;
                }
                case 57337:                                 // Great Feast
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 58067, true);
                    break;
                }
                case 57397:                                 // Fish Feast
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 45548, true);
                    unitTarget->CastSpell(unitTarget, 57073, true);
                    unitTarget->CastSpell(unitTarget, 57398, true);
                    break;
                }
                case 58466:                                 // Gigantic Feast
                case 58475:                                 // Small Feast
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 57085, true);
                    break;
                }
                case 58418:                                 // Portal to Orgrimmar
                case 58420:                                 // Portal to Stormwind
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER || eff_idx != EFFECT_INDEX_0)
                    {
                        return;
                    }

                    uint32 spellID = m_spellInfo->CalculateSimpleValue(EFFECT_INDEX_0);
                    uint32 questID = m_spellInfo->CalculateSimpleValue(EFFECT_INDEX_1);

                    if (((Player*)unitTarget)->GetQuestStatus(questID) == QUEST_STATUS_COMPLETE && !((Player*)unitTarget)->GetQuestRewardStatus(questID))
                    {
                        unitTarget->CastSpell(unitTarget, spellID, true);
                    }

                    return;
                }
                case 59317:                                 // Teleporting
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    // return from top
                    if (((Player*)unitTarget)->GetAreaId() == 4637)
                    {
                        unitTarget->CastSpell(unitTarget, 59316, true);
                    }
                    // teleport atop
                    else
                    {
                        unitTarget->CastSpell(unitTarget, 59314, true);
                    }

                    return;
                }                                           // random spell learn instead placeholder
                case 59789:                                 // Oracle Ablutions
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    switch (unitTarget->GetPowerType())
                    {
                        case POWER_RUNIC_POWER:
                        {
                            unitTarget->CastSpell(unitTarget, 59812, true);
                            break;
                        }
                        case POWER_MANA:
                        {
                            int32 manapool = unitTarget->GetMaxPower(POWER_MANA) * 0.05;
                            unitTarget->CastCustomSpell(unitTarget, 59813, &manapool, NULL, NULL, true);
                            break;
                        }
                        case POWER_RAGE:
                        {
                            unitTarget->CastSpell(unitTarget, 59814, true);
                            break;
                        }
                        case POWER_ENERGY:
                        {
                            unitTarget->CastSpell(unitTarget, 59815, true);
                            break;
                        }
                        // These are not restored
                        case POWER_FOCUS:
                        case POWER_HAPPINESS:
                        case POWER_RUNE:
                        case POWER_HEALTH:
                            break;
                    }
                    return;
                }
                case 61122:                                 // Contact Brann
                {
                    m_caster->CastSpell(m_caster, 55038, true);
                    return;
                }
                case 60893:                                 // Northrend Alchemy Research
                case 61177:                                 // Northrend Inscription Research
                case 61288:                                 // Minor Inscription Research
                case 61756:                                 // Northrend Inscription Research (FAST QA VERSION)
                case 64323:                                 // Book of Glyph Mastery
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    // learn random explicit discovery recipe (if any)
                    if (uint32 discoveredSpell = GetExplicitDiscoverySpell(m_spellInfo->ID, (Player*)m_caster))
                    {
                        ((Player*)m_caster)->learnSpell(discoveredSpell, false);
                    }

                    return;
                }
                case 62042:                                 // Stormhammer
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 62470, true);
                    unitTarget->CastSpell(m_caster, 64909, true);
                    return;
                }
                case 62217:                                 // Unstable Energy
                case 62922:                                 // Unstable Energy (h)
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->RemoveAurasDueToSpell(m_spellInfo->CalculateSimpleValue(eff_idx));
                    return;
                }
                case 62262:                                 // Brightleaf Flux
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    if (unitTarget->HasAura(62239))
                    {
                        unitTarget->RemoveAurasDueToSpell(62239);
                    }
                    else
                    {
                        uint32 stackAmount = urand(1, GetSpellStore()->LookupEntry(62239)->CumulativeAura);

                        for (uint8 i = 0; i < stackAmount; ++i)
                        {
                            unitTarget->CastSpell(unitTarget, 62239, true);
                        }
                    }
                    return;
                }
                case 62282:                                 // Iron Roots
                case 62440:                                 // Strengthened Iron Roots
                case 63598:                                 // Iron Roots (h)
                case 63601:                                 // Strengthened Iron Roots (h)
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT || !((Creature*)unitTarget)->IsTemporarySummon())
                    {
                        return;
                    }

                    uint32 ownerAura = 0;

                    switch (m_spellInfo->ID)
                    {
                        case 62282: ownerAura = 62283; break;
                        case 62440: ownerAura = 62438; break;
                        case 63598: ownerAura = 62930; break;
                        case 63601: ownerAura = 62861; break;
                    };

                    if (Unit* summoner = unitTarget->GetMap()->GetUnit(((TemporarySummon*)unitTarget)->GetSummonerGuid()))
                    {
                        summoner->RemoveAurasDueToSpell(ownerAura);
                    }
                    return;
                }
                case 62381:                                 // Chill
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->RemoveAurasDueToSpell(62373);
                    unitTarget->CastSpell(unitTarget, 62382, true);
                    return;
                }
                case 62488:                                 // Activate Construct
                {
                    if (!unitTarget || !unitTarget->HasAura(62468))
                    {
                        return;
                    }

                    unitTarget->RemoveAurasDueToSpell(62468);
                    unitTarget->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    unitTarget->CastSpell(unitTarget, 64474, true);

                    if (m_caster->getVictim())
                    {
                        ((Creature*)unitTarget)->AI()->AttackStart(m_caster->getVictim());
                    }
                    return;
                }
                case 62524:                                 // Attuned to Nature 2 Dose Reduction
                case 62525:                                 // Attuned to Nature 10 Dose Reduction
                case 62521:                                 // Attuned to Nature 25 Dose Reduction
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    uint32 numStacks = 0;

                    switch (m_spellInfo->ID)
                    {
                        case 62524: numStacks = 2;  break;
                        case 62525: numStacks = 10; break;
                        case 62521: numStacks = 25; break;
                    };

                    uint32 spellId = m_spellInfo->CalculateSimpleValue(eff_idx);
                    unitTarget->RemoveAuraHolderFromStack(spellId, numStacks);
                    return;
                }
                case 62552:                                 // Defend
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 63119, true);
                    return;
                }
                case 62575:                                 // Shield-Breaker (player)
                case 68282:                                 // Charge (player)
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->RemoveAuraHolderFromStack(62719);
                    unitTarget->RemoveAuraHolderFromStack(64100);
                    unitTarget->RemoveAuraHolderFromStack(64192);
                    return;
                }
                case 62688:                                 // Summon Wave - 10 Mob
                {
                    uint32 spellId = m_spellInfo->CalculateSimpleValue(eff_idx);

                    for (uint32 i = 0; i < 10; ++i)
                    {
                        m_caster->CastSpell(m_caster, spellId, true);
                    }

                    return;
                }
                case 62707:                                 // Grab
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 62708, true);
                    return;
                }
                case 63010:                                 // Charge
                case 68307:                                 // Charge
                case 68504:                                 // Shield-Breaker
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    Unit* owner = unitTarget->GetCharmerOrOwnerPlayerOrPlayerItself();
                    if (!owner)
                    {
                        return;
                    }

                    owner->RemoveAuraHolderFromStack(62552);
                    owner->RemoveAuraHolderFromStack(63119);

                    if (owner->HasAura(63132))
                    {
                        owner->RemoveAurasDueToSpell(63132);
                        owner->CastSpell(unitTarget, 63131, true);
                    }
                    else if (owner->HasAura(63131))
                    {
                        owner->RemoveAurasDueToSpell(63131);
                        owner->CastSpell(unitTarget, 63130, true);
                    }
                    else if (owner->HasAura(63130))
                    {
                        owner->RemoveAurasDueToSpell(63130);
                    }

                    return;
                }
                case 63027:                                 // Proximity Mines
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    for (uint8 i = 0; i < 15; ++i)
                    {
                        unitTarget->CastSpell(unitTarget, 65347, true);
                    }
                    return;
                }
                case 63119:                                 // Block!
                case 64192:                                 // Block!
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    if (unitTarget->HasAura(63132))
                    {
                        return;
                    }
                    else if (unitTarget->HasAura(63131))
                    {
                        unitTarget->RemoveAurasDueToSpell(63131);
                        unitTarget->CastSpell(unitTarget, 63132, true);         // Shield Level 3
                    }
                    else if (unitTarget->HasAura(63130))
                    {
                        unitTarget->RemoveAurasDueToSpell(63130);
                        unitTarget->CastSpell(unitTarget, 63131, true);         // Shield Level 2
                    }
                    else
                    {
                        unitTarget->CastSpell(unitTarget, 63130, true);         // Shield Level 1
                    }
                    return;
                }
                case 63122:                                 // Clear Insane
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    unitTarget->RemoveAurasDueToSpell(m_spellInfo->CalculateSimpleValue(eff_idx));
                    return;
                }
                case 63633:                                 // Summon Rubble
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    for (uint8 i = 0; i < 5; ++i)
                    {
                        unitTarget->CastSpell(unitTarget, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    }
                    return;
                }
                case 63667:                                 // Napalm Shell
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, m_caster->GetMap()->IsRegularDifficulty() ? 63666 : 65026, true);
                    return;
                }
                case 63681:                                 // Rocket Strike
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 63036, true);
                    return;
                }
                case 63795:                                 // Psychosis
                case 65301:                                 // Psychosis (h)
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER || unitTarget->HasAura(m_spellInfo->CalculateSimpleValue(eff_idx)))
                    {
                        return;
                    }

                    unitTarget->RemoveAuraHolderFromStack(63050, 12);
                    return;
                }
                case 63803:                                 // Brain Link
                case 64164:                                 // Lunatic Gaze (Yogg)
                case 64168:                                 // Lunatic Gaze (Skull)
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    uint8 removedAmount = 0;
                    switch (m_spellInfo->ID)
                    {
                        case 63803: removedAmount = 2; break;
                        case 64164: removedAmount = 4; break;
                        case 64168: removedAmount = 2; break;
                    }

                    unitTarget->RemoveAuraHolderFromStack(63050, removedAmount);
                    return;
                }
                case 63993:                                 // Cancel Illusion Room Aura
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 63992, true);
                    unitTarget->RemoveAurasDueToSpell(m_spellInfo->CalculateSimpleValue(eff_idx));
                    return;
                }
                case 64059:                                // Induce Madness
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER || !unitTarget->HasAura(m_spellInfo->CalculateSimpleValue(eff_idx)))
                    {
                        return;
                    }

                    unitTarget->RemoveAurasDueToSpell(63050);
                    return;
                }
                case 64069:                                 // Match Health (Rank 1)
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->SetHealthPercent(m_caster->GetHealthPercent());
                    return;
                }
                case 64123:                                 // Lunge
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, unitTarget->GetMap()->IsRegularDifficulty() ? 64125 : 64126, true);
                    return;
                }
                case 64131:                                 // Lunge
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    unitTarget->CastSpell(m_caster, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    return;
                }
                case 64456:                                 // Feral Essence Application Removal
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    uint32 spellId = m_spellInfo->CalculateSimpleValue(eff_idx);
                    unitTarget->RemoveAuraHolderFromStack(spellId);
                    return;
                }
                case 64466:                                 // Empowering Shadows
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(m_caster, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    return;
                }
                case 64467:                                 // Empowering Shadows
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, m_caster->GetMap()->IsRegularDifficulty() ? 64468 : 64486, true);
                    return;
                }
                case 64475:                                 // Strength of the Creator
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->RemoveAuraHolderFromStack(64473);
                    return;
                }
                case 64623:                                 // Frost Bomb
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 64627, true);
                    return;
                }
                case 64767:                                 // Stormhammer
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    if (Creature* target = (Creature*)unitTarget)
                    {
                        target->AI()->EnterEvadeMode();
                        target->CastSpell(target, 62470, true);
                        target->CastSpell(m_caster, 64909, true);
                        target->CastSpell(target, 64778, true);
                        target->ForcedDespawn(10000);
                    }
                    return;
                }
                case 64841:                                 // Rapid Burst
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(m_caster, 63382, false);
                    return;
                }
                case 65238:                                 // Shattered Illusion
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->RemoveAurasDueToSpell(m_spellInfo->CalculateSimpleValue(eff_idx));
                    return;
                }
                case 66477:                                 // Bountiful Feast
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 65422, true);
                    unitTarget->CastSpell(unitTarget, 66622, true);
                    break;
                }
                case 66741:                                 // Chum the Water
                {
                    // maybe this check should be done sooner?
                    if (!m_caster->IsInWater())
                    {
                        return;
                    }

                    uint32 spellId = 0;

                    // too low/high?
                    if (roll_chance_i(33))
                    {
                        spellId = 66737;                    // angry
                    }
                    else
                    {
                        switch (rand() % 3)
                        {
                            case 0: spellId = 66740; break; // blue
                            case 1: spellId = 66739; break; // tresher
                            case 2: spellId = 66738; break; // mako
                        }
                    }

                    if (spellId)
                    {
                        m_caster->CastSpell(m_caster, spellId, true);
                    }

                    return;
                }
                case 66744:                                 // Make Player Destroy Totems
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    // Totem of the Earthen Ring does not really require or take reagents.
                    // Expecting RewardQuest() to already destroy them or we need additional code here to destroy.
                    unitTarget->CastSpell(unitTarget, 66747, true);
                    return;
                }
                case 67009:                                 // Nether Power (ToC25: Lord Jaraxxus)
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    for (uint8 i = 0; i < 11; ++i)
                    {
                        unitTarget->CastSpell(unitTarget, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    }

                    return;
                }
                case 68084:                                 // Clear Val'kyr Touch of Light/Dark
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->RemoveAurasDueToSpell(66001);
                    unitTarget->RemoveAurasDueToSpell(m_spellInfo->CalculateSimpleValue(eff_idx));
                    return;
                }
                case 68861:                                 // Consume Soul (ICC FoS: Bronjahm)
                    if (unitTarget)
                    {
                        unitTarget->CastSpell(unitTarget, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    }
                    return;
                case 68871:                                 // Wailing Souls
                    // Left or Right direction?
                    m_caster->CastSpell(m_caster, urand(0, 1) ? 68875 : 68876, false);
                    // Clear TargetGuid for sweeping
                    m_caster->SetTargetGuid(ObjectGuid());
                    return;
                case 69048:                                 // Mirrored Soul
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // This is extremely strange!
                    // The spell should send MSG_CHANNEL_START, SMSG_SPELL_START
                    // However it has cast time 2s, but should send SMSG_SPELL_GO instantly.
                    m_caster->CastSpell(unitTarget, 69051, true);
                    return;
                }
                case 69051:                                 // Mirrored Soul
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // Actually this spell should be sent with SMSG_SPELL_START
                    unitTarget->CastSpell(m_caster, 69023, true);
                    return;
                }
                case 69140:                                 // Coldflame (random target selection)
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    return;
                }
                case 69147:                                 // Coldflame
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    return;
                }
                case 69377:                                 // Fortitude
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 72590, true);
                    return;
                }
                case 69378:                                 // Blessing of Forgotten Kings
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 72586, true);
                    return;
                }
                case 69381:                                 // Gift of the Wild
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, 72588, true);
                    return;
                }
                case 71806:                                 // Glittering Sparks
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    m_caster->CastSpell(unitTarget, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    return;
                }
                case 72034:                                 // Whiteout
                case 72096:                                 // Whiteout (heroic)
                {
                    // cast Whiteout visual
                    m_caster->CastSpell(unitTarget, 72036, true);
                    return;
                }
                case 72705:                                 // Coldflame (summon around the caster)
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // Cast summon spells 72701, 72702, 72703, 72704
                    for (uint32 triggeredSpell = m_spellInfo->CalculateSimpleValue(eff_idx); triggeredSpell < m_spellInfo->ID; ++triggeredSpell)
                    {
                        unitTarget->CastSpell(unitTarget, triggeredSpell, true);
                    }

                    return;
                }
                case 74455:                                 // Conflagration
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(m_caster, m_spellInfo->CalculateSimpleValue(eff_idx), true);
                    return;
                }
            }
            break;
        }
        case SPELLFAMILY_WARLOCK:
        {
            switch (m_spellInfo->ID)
            {
                case  6201:                                 // Healthstone creating spells
                case  6202:
                case  5699:
                case 11729:
                case 11730:
                case 27230:
                case 47871:
                case 47878:
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    uint32 itemtype;
                    uint32 rank = 0;
                    Unit::AuraList const& mDummyAuras = unitTarget->GetAurasByType(SPELL_AURA_DUMMY);
                    for (Unit::AuraList::const_iterator i = mDummyAuras.begin(); i != mDummyAuras.end(); ++i)
                    {
                        if ((*i)->GetId() == 18692)
                        {
                            rank = 1;
                            break;
                        }
                        else if ((*i)->GetId() == 18693)
                        {
                            rank = 2;
                            break;
                        }
                    }

                    static uint32 const itypes[8][3] =
                    {
                        { 5512, 19004, 19005},              // Minor Healthstone
                        { 5511, 19006, 19007},              // Lesser Healthstone
                        { 5509, 19008, 19009},              // Healthstone
                        { 5510, 19010, 19011},              // Greater Healthstone
                        { 9421, 19012, 19013},              // Major Healthstone
                        {22103, 22104, 22105},              // Master Healthstone
                        {36889, 36890, 36891},              // Demonic Healthstone
                        {36892, 36893, 36894}               // Fel Healthstone
                    };

                    switch (m_spellInfo->ID)
                    {
                        case  6201:
                            itemtype = itypes[0][rank]; break; // Minor Healthstone
                        case  6202:
                            itemtype = itypes[1][rank]; break; // Lesser Healthstone
                        case  5699:
                            itemtype = itypes[2][rank]; break; // Healthstone
                        case 11729:
                            itemtype = itypes[3][rank]; break; // Greater Healthstone
                        case 11730:
                            itemtype = itypes[4][rank]; break; // Major Healthstone
                        case 27230:
                            itemtype = itypes[5][rank]; break; // Master Healthstone
                        case 47871:
                            itemtype = itypes[6][rank]; break; // Demonic Healthstone
                        case 47878:
                            itemtype = itypes[7][rank]; break; // Fel Healthstone
                        default:
                            return;
                    }
                    DoCreateItem(eff_idx, itemtype);
                    return;
                }
                case 47193:                                 // Demonic Empowerment
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    uint32 entry = unitTarget->GetEntry();
                    uint32 spellID;
                    switch (entry)
                    {
                        case   416: spellID = 54444; break; // imp
                        case   417: spellID = 54509; break; // fellhunter
                        case  1860: spellID = 54443; break; // void
                        case  1863: spellID = 54435; break; // succubus
                        case 17252: spellID = 54508; break; // fellguard
                        default:
                            return;
                    }
                    unitTarget->CastSpell(unitTarget, spellID, true);
                    return;
                }
                case 47422:                                 // Everlasting Affliction
                {
                    // Need refresh caster corruption auras on target
                    Unit::SpellAuraHolderMap& suAuras = unitTarget->GetSpellAuraHolderMap();
                    for (Unit::SpellAuraHolderMap::iterator itr = suAuras.begin(); itr != suAuras.end(); ++itr)
                    {
                        SpellEntry const* spellInfo = (*itr).second->GetSpellProto();
                        if (spellInfo->SpellClassSet == SPELLFAMILY_WARLOCK &&
                                (spellInfo->SpellClassMask & UI64LIT(0x0000000000000002)) &&
                                (*itr).second->GetCasterGuid() == m_caster->GetObjectGuid())
                            (*itr).second->RefreshHolder();
                    }
                    return;
                }
                case 63521:                                 // Guarded by The Light (Paladin spell with SPELLFAMILY_WARLOCK)
                {
                    // Divine Plea, refresh on target (3 aura slots)
                    if (SpellAuraHolder* holder = unitTarget->GetSpellAuraHolder(54428))
                    {
                        holder->RefreshHolder();
                    }

                    return;
                }
            }
            break;
        }
        case SPELLFAMILY_PRIEST:
        {
            switch (m_spellInfo->ID)
            {
                case 47948:                                 // Pain and Suffering
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // Refresh Shadow Word: Pain on target
                    Unit::SpellAuraHolderMap& auras = unitTarget->GetSpellAuraHolderMap();
                    for (Unit::SpellAuraHolderMap::iterator itr = auras.begin(); itr != auras.end(); ++itr)
                    {
                        SpellEntry const* spellInfo = (*itr).second->GetSpellProto();
                        if (spellInfo->SpellClassSet == SPELLFAMILY_PRIEST &&
                                (spellInfo->SpellClassMask & UI64LIT(0x0000000000008000)) &&
                                (*itr).second->GetCasterGuid() == m_caster->GetObjectGuid())
                        {
                            (*itr).second->RefreshHolder();
                            return;
                        }
                    }
                    return;
                }
                default:
                    break;
            }
            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            switch (m_spellInfo->ID)
            {
                case 53209:                                 // Chimera Shot
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    uint32 spellId = 0;
                    int32 basePoint = 0;
                    Unit* target = unitTarget;
                    Unit::SpellAuraHolderMap& Auras = unitTarget->GetSpellAuraHolderMap();
                    for (Unit::SpellAuraHolderMap::iterator i = Auras.begin(); i != Auras.end(); ++i)
                    {
                        SpellAuraHolder* holder = i->second;
                        if (holder->GetCasterGuid() != m_caster->GetObjectGuid())
                        {
                            continue;
                        }

                        // Search only Serpent Sting, Viper Sting, Scorpid Sting auras
                        ClassFamilyMask const& familyFlag = holder->GetSpellProto()->SpellClassMask;
                        if (!familyFlag.IsFitToFamilyMask(UI64LIT(0x000000800000C000)))
                        {
                            continue;
                        }

                        // Refresh aura duration
                        holder->RefreshHolder();

                        Aura* aura = holder->GetAuraByEffectIndex(EFFECT_INDEX_0);

                        if (!aura)
                        {
                            continue;
                        }

                        // Serpent Sting - Instantly deals 40% of the damage done by your Serpent Sting.
                        if (familyFlag.IsFitToFamilyMask(UI64LIT(0x0000000000004000)))
                        {
                            // m_amount already include RAP bonus
                            basePoint = aura->GetModifier()->m_amount * aura->GetAuraMaxTicks() * 40 / 100;
                            spellId = 53353;                // Chimera Shot - Serpent
                        }

                        // Viper Sting - Instantly restores mana to you equal to 60% of the total amount drained by your Viper Sting.
                        if (familyFlag.IsFitToFamilyMask(UI64LIT(0x0000008000000000)))
                        {
                            uint32 target_max_mana = unitTarget->GetMaxPower(POWER_MANA);
                            if (!target_max_mana)
                            {
                                continue;
                            }

                            // ignore non positive values (can be result apply spellmods to aura damage
                            uint32 pdamage = aura->GetModifier()->m_amount > 0 ? aura->GetModifier()->m_amount : 0;

                            // Special case: draining x% of mana (up to a maximum of 2*x% of the caster's maximum mana)
                            uint32 maxmana = m_caster->GetMaxPower(POWER_MANA)  * pdamage * 2 / 100;

                            pdamage = target_max_mana * pdamage / 100;
                            if (pdamage > maxmana)
                            {
                                pdamage = maxmana;
                            }

                            pdamage *= 4;                   // total aura damage
                            basePoint = pdamage * 60 / 100;
                            spellId = 53358;                // Chimera Shot - Viper
                            target = m_caster;
                        }

                        // Scorpid Sting - Attempts to Disarm the target for 10 sec. This effect cannot occur more than once per 1 minute.
                        if (familyFlag.IsFitToFamilyMask(UI64LIT(0x0000000000008000)))
                        {
                            spellId = 53359;                // Chimera Shot - Scorpid
                        }
                        // ?? nothing say in spell desc (possibly need addition check)
                        // if ((familyFlag & UI64LIT(0x0000010000000000)) || // dot
                        //    (familyFlag & UI64LIT(0x0000100000000000)))   // stun
                        //{
                        //    spellId = 53366; // 53366 Chimera Shot - Wyvern
                        //}
                    }

                    if (spellId)
                    {
                        m_caster->CastCustomSpell(target, spellId, &basePoint, 0, 0, false);
                    }

                    return;
                }
                case 53412:                                 // Invigoration (pet triggered script, master targeted)
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    Unit::AuraList const& auras = unitTarget->GetAurasByType(SPELL_AURA_DUMMY);
                    for (Unit::AuraList::const_iterator i = auras.begin(); i != auras.end(); ++i)
                    {
                        // Invigoration (master talent)
                        if ((*i)->GetModifier()->m_miscvalue == 8 && (*i)->GetSpellProto()->SpellIconID == 3487)
                        {
                            if (roll_chance_i((*i)->GetModifier()->m_amount))
                            {
                                unitTarget->CastSpell(unitTarget, 53398, true, NULL, (*i), m_caster->GetObjectGuid());
                                break;
                            }
                        }
                    }
                    return;
                }
                case 53271:                                 // Master's Call
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // script effect have in value, but this outdated removed part
                    unitTarget->CastSpell(unitTarget, 62305, true);
                    return;
                }
                default:
                    break;
            }
            break;
        }
        case SPELLFAMILY_PALADIN:
        {
            // Judgement (seal trigger)
            if (m_spellInfo->Category == SPELLCATEGORY_JUDGEMENT)
            {
                if (!unitTarget || !unitTarget->IsAlive())
                {
                    return;
                }

                uint32 spellId1 = 0;
                uint32 spellId2 = 0;

                // Judgement self add switch
                switch (m_spellInfo->ID)
                {
                    case 53407: spellId1 = 20184; break;    // Judgement of Justice
                    case 20271:                             // Judgement of Light
                    case 57774: spellId1 = 20185; break;    // Judgement of Light
                    case 53408: spellId1 = 20186; break;    // Judgement of Wisdom
                    default:
                        sLog.outError("Unsupported Judgement (seal trigger) spell (Id: %u) in Spell::EffectScriptEffect", m_spellInfo->ID);
                        return;
                }

                // offensive seals have aura dummy in 2 effect
                Unit::AuraList const& m_dummyAuras = m_caster->GetAurasByType(SPELL_AURA_DUMMY);
                for (Unit::AuraList::const_iterator itr = m_dummyAuras.begin(); itr != m_dummyAuras.end(); ++itr)
                {
                    // search seal (offensive seals have judgement's aura dummy spell id in 2 effect
                    if ((*itr)->GetEffIndex() != EFFECT_INDEX_2 || !IsSealSpell((*itr)->GetSpellProto()))
                    {
                        continue;
                    }
                    spellId2 = (*itr)->GetModifier()->m_amount;
                    SpellEntry const* judge = sSpellStore.LookupEntry(spellId2);
                    if (!judge)
                    {
                        continue;
                    }
                    break;
                }

                // if there were no offensive seals than there is seal with proc trigger aura
                if (!spellId2)
                {
                    Unit::AuraList const& procTriggerAuras = m_caster->GetAurasByType(SPELL_AURA_PROC_TRIGGER_SPELL);
                    for (Unit::AuraList::const_iterator itr = procTriggerAuras.begin(); itr != procTriggerAuras.end(); ++itr)
                    {
                        if ((*itr)->GetEffIndex() != EFFECT_INDEX_0 || !IsSealSpell((*itr)->GetSpellProto()))
                        {
                            continue;
                        }
                        spellId2 = 54158;
                        break;
                    }
                }

                if (spellId1)
                {
                    m_caster->CastSpell(unitTarget, spellId1, true);
                }

                if (spellId2)
                {
                    m_caster->CastSpell(unitTarget, spellId2, true);
                }

                return;
            }
            break;
        }
        case SPELLFAMILY_POTION:
        {
            switch (m_spellInfo->ID)
            {
                case 28698:                                 // Dreaming Glory
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 28694, true);
                    break;
                }
                case 28702:                                 // Netherbloom
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // 25% chance of casting a random buff
                    if (roll_chance_i(75))
                    {
                        return;
                    }

                    // triggered spells are 28703 to 28707
                    // Note: some sources say, that there was the possibility of
                    //       receiving a debuff. However, this seems to be removed by a patch.
                    const uint32 spellid = 28703;

                    // don't overwrite an existing aura
                    for (uint8 i = 0; i < 5; ++i)
                    {
                        if (unitTarget->HasAura(spellid + i, EFFECT_INDEX_0))
                        {
                            return;
                        }
                    }

                    unitTarget->CastSpell(unitTarget, spellid + urand(0, 4), true);
                    break;
                }
                case 28720:                                 // Nightmare Vine
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    // 25% chance of casting Nightmare Pollen
                    if (roll_chance_i(75))
                    {
                        return;
                    }

                    unitTarget->CastSpell(unitTarget, 28721, true);
                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_DEATHKNIGHT:
        {
            switch (m_spellInfo->ID)
            {
                case 50842:                                 // Pestilence
                {
                    if (!unitTarget)
                    {
                        return;
                    }

                    Unit* mainTarget = m_targets.getUnitTarget();
                    if (!mainTarget)
                    {
                        return;
                    }

                    // do only refresh diseases on main target if caster has Glyph of Disease
                    if (mainTarget == unitTarget && !m_caster->HasAura(63334))
                    {
                        return;
                    }

                    // Blood Plague
                    if (mainTarget->HasAura(55078))
                    {
                        m_caster->CastSpell(unitTarget, 55078, true);
                    }

                    // Frost Fever
                    if (mainTarget->HasAura(55095))
                    {
                        m_caster->CastSpell(unitTarget, 55095, true);
                    }

                    break;
                }
                case 46584:                                   // Raise dead
                {
                    if (!unitTarget || m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    // If DK has spell 52143, summon pet from dummy effect
                    // otherwise summon guardian
                    uint32 triggered_spell_id = m_spellInfo->CalculateSimpleValue(SpellEffectIndex(m_caster->HasSpell(52143) ? EFFECT_INDEX_2 : EFFECT_INDEX_1));

                    float x,y,z;
                    m_caster->GetClosePoint(x, y, z, m_caster->GetObjectBoundingRadius(), PET_FOLLOW_DIST);

                    if (unitTarget != (Unit*)m_caster)
                    {
                        m_caster->CastSpell(unitTarget->GetPositionX(),unitTarget->GetPositionY(),unitTarget->GetPositionZ(),triggered_spell_id, true, NULL, NULL, m_caster->GetObjectGuid(), m_spellInfo);
                    }
                    else if (m_caster->HasAura(60200))
                    {
                        m_caster->CastSpell(x,y,z,triggered_spell_id, true, NULL, NULL, m_caster->GetObjectGuid(), m_spellInfo);
                    }
                    else if (((Player*)m_caster)->HasItemCount(37201,1))
                    {
                        ((Player*)m_caster)->DestroyItemCount(37201, 1, true);
                        m_caster->CastSpell(m_caster,48289,true);
                        m_caster->CastSpell(x,y,z,triggered_spell_id, true, NULL, NULL, m_caster->GetObjectGuid(), m_spellInfo);
                    }
                    else
                    {
                        SendCastResult(SPELL_FAILED_REAGENTS);
                        finish(true);
                        CancelGlobalCooldown();
                        return;
                    }
                    finish(true);
                    ((Player*)m_caster)->RemoveSpellCooldown(m_spellInfo->CalculateSimpleValue(EFFECT_INDEX_2),true);
                    ((Player*)m_caster)->RemoveSpellCooldown(m_spellInfo->CalculateSimpleValue(EFFECT_INDEX_1),true);
                    //CancelGlobalCooldown();
                    return;
                }
                case 61999:                                                 // Raise ally
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER || unitTarget->IsAlive())
                    {
                        SendCastResult(SPELL_FAILED_TARGET_NOT_DEAD);
                        finish(true);
                        CancelGlobalCooldown();
                        return;
                    }

                    // hacky remove death
                    unitTarget->CastSpell(unitTarget, m_spellInfo->CalculateSimpleValue(EFFECT_INDEX_0), true);
                    CancelGlobalCooldown();
                    return;
                }
            }
            break;
        }
    }

    // normal DB scripted effect
    if (!unitTarget)
    {
        return;
    }

    // Script based implementation. Must be used only for not good for implementation in core spell effects
    // So called only for not processed cases
    if (unitTarget->GetTypeId() == TYPEID_UNIT || unitTarget->GetTypeId() == TYPEID_PLAYER)
    {
        if (sScriptMgr.OnEffectScriptEffect(m_caster, m_spellInfo->ID, eff_idx, unitTarget, m_originalCasterGUID))
        {
            return;
        }
    }

    // Previous effect might have started script
    if (!ScriptMgr::CanSpellEffectStartDBScript(m_spellInfo, eff_idx))
    {
        return;
    }

    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Spell ScriptStart spellid %u in EffectScriptEffect", m_spellInfo->ID);
    m_caster->GetMap()->ScriptsStart(DBS_ON_SPELL, m_spellInfo->ID, m_caster, unitTarget);
}
