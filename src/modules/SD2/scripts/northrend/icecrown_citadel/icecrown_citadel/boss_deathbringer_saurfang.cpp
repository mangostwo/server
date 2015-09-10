/* Copyright (C) 2006 - 2013 ScriptDev2 <http://www.scriptdev2.com/>
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

/* ScriptData
SDName: boss_deathbringer_saurfang
SD%Complete: 60%
SDComment: Blood Power and all the related spells require core support; Intro and Outro event NYI.
SDCategory: Icecrown Citadel
EndScriptData */

#include "precompiled.h"
#include "icecrown_citadel.h"

enum
{
    SAY_AGGRO                   = -1631028,
    SAY_FALLENCHAMPION          = -1631029,
    SAY_BLOODBEASTS             = -1631030,
    SAY_SLAY_1                  = -1631031,
    SAY_SLAY_2                  = -1631032,
    SAY_BERSERK                 = -1631033,
    SAY_DEATH                   = -1631034,
    EMOTE_FRENZY                = -1631193,
    EMOTE_SCENT                 = -1631194,

    SAY_INTRO_ALLY_0            = -1631035,
    SAY_INTRO_ALLY_1            = -1631036,
    SAY_INTRO_ALLY_2            = -1631037,
    SAY_INTRO_ALLY_3            = -1631038,
    SAY_INTRO_ALLY_4            = -1631039,
    SAY_INTRO_ALLY_5            = -1631040,
    SAY_INTRO_HORDE_1           = -1631041,
    SAY_INTRO_HORDE_2           = -1631042,
    SAY_INTRO_HORDE_3           = -1631043,
    SAY_INTRO_HORDE_4           = -1631044,
    SAY_INTRO_HORDE_5           = -1631045,
    SAY_INTRO_HORDE_6           = -1631046,
    SAY_INTRO_HORDE_7           = -1631047,
    SAY_INTRO_HORDE_8           = -1631048,
    SAY_INTRO_HORDE_9           = -1631049,
    SAY_OUTRO_ALLY_1            = -1631050,
    SAY_OUTRO_ALLY_2            = -1631051,
    SAY_OUTRO_ALLY_3            = -1631052,
    SAY_OUTRO_ALLY_4            = -1631053,
    SAY_OUTRO_ALLY_5            = -1631054,
    SAY_OUTRO_ALLY_6            = -1631055,
    SAY_OUTRO_ALLY_7            = -1631056,
    SAY_OUTRO_ALLY_8            = -1631057,
    SAY_OUTRO_ALLY_9            = -1631058,
    SAY_OUTRO_ALLY_10           = -1631059,
    SAY_OUTRO_ALLY_11           = -1631060,
    SAY_OUTRO_ALLY_12           = -1631061,
    SAY_OUTRO_ALLY_13           = -1631062,
    SAY_OUTRO_ALLY_14           = -1631063,
    SAY_OUTRO_ALLY_15           = -1631064,
    SAY_OUTRO_ALLY_16           = -1631065,
    SAY_OUTRO_HORDE_1           = -1631066,
    SAY_OUTRO_HORDE_2           = -1631067,
    SAY_OUTRO_HORDE_3           = -1631068,
    SAY_OUTRO_HORDE_4           = -1631069,

    // intro event related
    // SPELL_GRIP_OF_AGONY       = 70572,
    // SPELL_VEHICLE_HARDCODED   = 46598,

    // combat spells
    SPELL_BLOOD_POWER           = 72371,
    // SPELL_BLOOD_POWER_SCALE   = 72370,            // included in creature_template_addon
    // SPELL_ZERO_POWER          = 72242,            // included in creature_template_addon

    // SPELL_MARK_FALLEN_DAMAGE  = 72256,            // procs on Saurfang melee attack - 72255 - included in creature_template_addon
    SPELL_MARK_FALLEN_CHAMPION  = 72293,            // procs on target death - 72260
    SPELL_REMOVE_MARKS          = 72257,

    // SPELL_RUNE_OF_BLOOD_PROC  = 72408,            // procs on Saurfang mele attack - 72409 - included in creature_template_addon
    SPELL_RUNE_OF_BLOOD         = 72410,

    SPELL_BLOOD_NOVA            = 72378,
    SPELL_BOILING_BLOOD         = 72385,

    SPELL_CALL_BLOOD_BEAST_1    = 72172,            // summons 38508
    SPELL_CALL_BLOOD_BEAST_2    = 72173,
    SPELL_CALL_BLOOD_BEAST_3    = 72356,
    SPELL_CALL_BLOOD_BEAST_4    = 72357,
    SPELL_CALL_BLOOD_BEAST_5    = 72358,
    SPELL_SCENT_OF_BLOOD        = 72769,            // triggers 72771 on the blood beasts

    SPELL_BERSERK               = 26662,
    SPELL_FRENZY                = 72737,

    // Summoned spells
    SPELL_RESISTANT_SKIN        = 72723,
    SPELL_BLOOD_LINK_BEAST      = 72176,

    FACTION_ID_UNDEAD           = 974,

    POINT_ID_INTRO              = 1,
    POINT_ID_EVADE              = 2,
};

static const float fIntroPosition[4] = { -491.30f, 2211.35f, 541.11f, 3.16f};

struct boss_deathbringer_saurfang : public CreatureScript
{
    boss_deathbringer_saurfang() : CreatureScript("boss_deathbringer_saurfang") {}

    struct boss_deathbringer_saurfangAI : public ScriptedAI
    {
        boss_deathbringer_saurfangAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
            m_powerBloodPower = m_creature->GetPowerType();
            m_bIsIntroDone = false;
        }

        ScriptedInstance* m_pInstance;

        uint32 m_uiRuneOfBloodTimer;
        uint32 m_uiBoilingBloodTimer;
        uint32 m_uiBloodNovaTimer;
        uint32 m_uiBloodBeastsTimer;
        uint32 m_uiScentOfBloodTimer;
        uint32 m_uiBerserkTimer;

        bool m_bIsFrenzied;
        bool m_bIsIntroDone;

        Powers m_powerBloodPower;

        void Reset() override
        {
            m_uiRuneOfBloodTimer = 25000;
            m_uiBoilingBloodTimer = 19000;
            m_uiBloodNovaTimer = 20000;
            m_uiBloodBeastsTimer = 40000;
            m_uiScentOfBloodTimer = 47000;
            m_uiBerserkTimer = 8 * MINUTE * IN_MILLISECONDS;

            if (m_pInstance && m_pInstance->GetData(TYPE_DATA_IS_HEROIC))
                m_uiBerserkTimer = 6 * MINUTE * IN_MILLISECONDS;

            m_bIsFrenzied = false;

            m_creature->SetPower(m_powerBloodPower, 0);
        }

        void Aggro(Unit* /*pWho*/) override
        {
            DoScriptText(SAY_AGGRO, m_creature);
            DoCastSpellIfCan(m_creature, SPELL_BLOOD_POWER, CAST_TRIGGERED);

            if (m_pInstance)
                m_pInstance->SetData(TYPE_DEATHBRINGER_SAURFANG, IN_PROGRESS);
        }

        void MoveInLineOfSight(Unit* pWho) override
        {
            if (!m_bIsIntroDone && pWho->GetTypeId() == TYPEID_PLAYER && !((Player*)pWho)->isGameMaster() && m_creature->GetDistance2d(pWho) < 50.0f)
            {
                m_creature->GetMotionMaster()->MovePoint(POINT_ID_INTRO, fIntroPosition[0], fIntroPosition[1], fIntroPosition[2]);
                if (m_pInstance)
                    m_pInstance->DoUseDoorOrButton(GO_SAURFANG_DOOR);
                m_bIsIntroDone = true;
            }

            ScriptedAI::MoveInLineOfSight(pWho);
        }

        void KilledUnit(Unit* pVictim) override
        {
            if (pVictim->GetTypeId() != TYPEID_PLAYER)
                return;

            if (urand(0, 1))
                DoScriptText(urand(0, 1) ? SAY_SLAY_1 : SAY_SLAY_2, m_creature);
        }

        void JustDied(Unit* /*pKiller*/) override
        {
            DoScriptText(SAY_DEATH, m_creature);
            DoCastSpellIfCan(m_creature, SPELL_REMOVE_MARKS, CAST_TRIGGERED);

            if (m_pInstance)
                m_pInstance->SetData(TYPE_DEATHBRINGER_SAURFANG, DONE);
        }

        void JustReachedHome() override
        {
            if (m_pInstance)
                m_pInstance->SetData(TYPE_DEATHBRINGER_SAURFANG, FAIL);
        }

        void EnterEvadeMode() override
        {
            m_creature->RemoveAllAurasOnEvade();
            m_creature->DeleteThreatList();
            m_creature->CombatStop(true);

            // Boss needs to evade to the point in front of the door
            if (m_creature->IsAlive())
                m_creature->GetMotionMaster()->MovePoint(POINT_ID_EVADE, fIntroPosition[0], fIntroPosition[1], fIntroPosition[2]);

            m_creature->SetLootRecipient(NULL);

            Reset();
        }

        void MovementInform(uint32 uiMoveType, uint32 uiPointId) override
        {
            if (uiMoveType != POINT_MOTION_TYPE)
                return;

            if (uiPointId == POINT_ID_EVADE)
            {
                m_creature->SetFacingTo(fIntroPosition[3]);

                if (m_pInstance)
                    m_pInstance->SetData(TYPE_DEATHBRINGER_SAURFANG, FAIL);
            }
            else if (uiPointId == POINT_ID_INTRO)
            {
                if (m_pInstance)
                    m_pInstance->DoUseDoorOrButton(GO_SAURFANG_DOOR);

                // Note: this should be done only after the intro event is finished
                // ToDo: move this to the proper place after the intro will be implemented
                // Also the faction needs to be checked if it should be handled in database
                m_creature->setFaction(FACTION_ID_UNDEAD);
                m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_PASSIVE);
                m_creature->SetInCombatWithZone();
            }
        }

        void JustSummoned(Creature* pSummoned) override
        {
            pSummoned->CastSpell(pSummoned, SPELL_RESISTANT_SKIN, true);
            pSummoned->CastSpell(pSummoned, SPELL_BLOOD_LINK_BEAST, true);

            // Note: the summoned should be activated only after 2-3 seconds after summon - can be done in eventAI
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                pSummoned->AI()->AttackStart(pTarget);
        }

        // Wrapper to help get a random player for the Mark of the Fallen Champion
        Unit* SelectRandomPlayerForMark()
        {
            // Search only for players which are not within 18 yards of the boss
            std::vector<Unit*> suitableTargets;
            ThreatList const& threatList = m_creature->GetThreatManager().getThreatList();

            for (ThreatList::const_iterator itr = threatList.begin(); itr != threatList.end(); ++itr)
            {
                if (Unit* pTarget = m_creature->GetMap()->GetUnit((*itr)->getUnitGuid()))
                {
                    if (pTarget->GetTypeId() == TYPEID_PLAYER && pTarget != m_creature->getVictim() && !pTarget->HasAura(SPELL_MARK_FALLEN_CHAMPION))
                        suitableTargets.push_back(pTarget);
                }
            }

            if (suitableTargets.empty())
                return m_creature->getVictim();
            else
                return suitableTargets[urand(0, suitableTargets.size() - 1)];
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
                return;

            // Mark of the Fallen Champion
            // ToDo: enable this when blood power is fully supported by the core
            /*if (m_creature->GetPower(m_powerBloodPower) >= 100)
            {
            if (Unit* pTarget = SelectRandomPlayerForMark())
            {
            if (DoCastSpellIfCan(pTarget, SPELL_MARK_FALLEN_CHAMPION) == CAST_OK)
            {
            DoScriptText(SAY_FALLENCHAMPION, m_creature);
            m_creature->SetPower(m_powerBloodPower, 0);
            }
            }
            }*/

            // Frenzy (soft enrage)
            if (!m_bIsFrenzied)
            {
                if (m_creature->GetHealthPercent() <= 30.0f)
                {
                    if (DoCastSpellIfCan(m_creature, SPELL_FRENZY) == CAST_OK)
                    {
                        DoScriptText(EMOTE_FRENZY, m_creature);
                        m_bIsFrenzied = true;
                    }
                }
            }

            // Berserk (hard enrage)
            if (m_uiBerserkTimer)
            {
                if (m_uiBerserkTimer < uiDiff)
                {
                    if (DoCastSpellIfCan(m_creature, SPELL_BERSERK) == CAST_OK)
                    {
                        DoScriptText(SAY_BERSERK, m_creature);
                        m_uiBerserkTimer = 0;
                    }
                }
                else
                    m_uiBerserkTimer -= uiDiff;
            }

            // Rune of Blood
            if (m_uiRuneOfBloodTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_RUNE_OF_BLOOD) == CAST_OK)
                    m_uiRuneOfBloodTimer = 25000;
            }
            else
                m_uiRuneOfBloodTimer -= uiDiff;

            // Boiling Blood
            if (m_uiBoilingBloodTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_BOILING_BLOOD) == CAST_OK)
                    m_uiBoilingBloodTimer = 15000;
            }
            else
                m_uiBoilingBloodTimer -= uiDiff;

            // Blood Nova
            if (m_uiBloodNovaTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_BLOOD_NOVA) == CAST_OK)
                    m_uiBloodNovaTimer = 20000;
            }
            else
                m_uiBloodNovaTimer -= uiDiff;

            // Call Blood Beasts
            if (m_uiBloodBeastsTimer < uiDiff)
            {
                DoScriptText(SAY_BLOODBEASTS, m_creature);

                DoCastSpellIfCan(m_creature, SPELL_CALL_BLOOD_BEAST_1, CAST_TRIGGERED);
                DoCastSpellIfCan(m_creature, SPELL_CALL_BLOOD_BEAST_2, CAST_TRIGGERED);

                if (m_pInstance && m_pInstance->GetData(TYPE_DATA_IS_25MAN))
                {
                    DoCastSpellIfCan(m_creature, SPELL_CALL_BLOOD_BEAST_3, CAST_TRIGGERED);
                    DoCastSpellIfCan(m_creature, SPELL_CALL_BLOOD_BEAST_4, CAST_TRIGGERED);
                    DoCastSpellIfCan(m_creature, SPELL_CALL_BLOOD_BEAST_5, CAST_TRIGGERED);
                }

                m_uiBloodBeastsTimer = 40000;
                m_uiScentOfBloodTimer = 7000;
            }
            else
                m_uiBloodBeastsTimer -= uiDiff;

            // Scent of Blood
            if (m_pInstance && m_pInstance->GetData(TYPE_DATA_IS_HEROIC))
            {
                if (m_uiScentOfBloodTimer < uiDiff)
                {
                    if (DoCastSpellIfCan(m_creature, SPELL_SCENT_OF_BLOOD) == CAST_OK)
                    {
                        DoScriptText(EMOTE_SCENT, m_creature);
                        m_uiScentOfBloodTimer = 40000;
                    }
                }
                else
                    m_uiScentOfBloodTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_deathbringer_saurfangAI(pCreature);
    }
};

void AddSC_boss_deathbringer_saurfang()
{
    Script* s;

    s = new boss_deathbringer_saurfang();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "boss_deathbringer_saurfang";
    //pNewScript->GetAI = &GetAI_boss_deathbringer_saurfang;
    //pNewScript->RegisterSelf();
}
