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
SDName:
SD%Complete: 0
SDComment:
SDCategory:
EndScriptData */

#include "precompiled.h"
#include "trial_of_the_crusader.h"

/*######
## npc_beast_combat_stalker
######*/

enum
{
    SAY_TIRION_BEAST_2                  = -1649005,
    SAY_TIRION_BEAST_3                  = -1649006,

    SPELL_BERSERK                       = 26662,

    PHASE_GORMOK                        = 0,
    PHASE_WORMS                         = 1,
    PHASE_ICEHOWL                       = 2,
};

struct npc_beast_combat_stalker : public CreatureScript
{
    npc_beast_combat_stalker() : CreatureScript("npc_beast_combat_stalker") {}

    struct npc_beast_combat_stalkerAI : public Scripted_NoMovementAI
    {
        npc_beast_combat_stalkerAI(Creature* pCreature) : Scripted_NoMovementAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
            m_creature->SetInCombatWithZone();
        }

        ScriptedInstance* m_pInstance;
        ObjectGuid m_aSummonedBossGuid[4];
        bool m_bFirstWormDied;
        uint32 m_uiBerserkTimer;
        uint32 m_uiAttackDelayTimer;
        uint32 m_uiNextBeastTimer;
        uint32 m_uiPhase;

        void Reset() override
        {
            m_uiAttackDelayTimer = 0;
            m_uiNextBeastTimer = 0;
            m_bFirstWormDied = false;
            m_uiPhase = PHASE_GORMOK;

            if (m_creature->GetMap()->GetDifficulty() == RAID_DIFFICULTY_10MAN_NORMAL || m_creature->GetMap()->GetDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL)
                m_uiBerserkTimer = 15 * MINUTE * IN_MILLISECONDS;
            else
                m_uiBerserkTimer = 9 * MINUTE * IN_MILLISECONDS;
        }

        void MoveInLineOfSight(Unit* /*pWho*/) override {}

        void DamageTaken(Unit* /*pDealer*/, uint32& uiDamage) override
        {
            uiDamage = 0;
        }

        void EnterEvadeMode() override
        {
            if (m_pInstance)
                m_pInstance->SetData(TYPE_NORTHREND_BEASTS, FAIL);

            for (uint8 i = 0; i < 4; ++i)
            {
                if (Creature* pBoss = m_creature->GetMap()->GetCreature(m_aSummonedBossGuid[i]))
                    pBoss->ForcedDespawn();
            }

            m_creature->ForcedDespawn();
        }

        void Aggro(Unit* /*pWho*/) override
        {
            if (m_pInstance)
                m_pInstance->SetData(TYPE_NORTHREND_BEASTS, IN_PROGRESS);
        }

        void JustSummoned(Creature* pSummoned) override
        {
            switch (pSummoned->GetEntry())
            {
            case NPC_GORMOK:     m_uiPhase = PHASE_GORMOK;  break;
            case NPC_DREADSCALE: m_uiPhase = PHASE_WORMS;   break;
            case NPC_ICEHOWL:    m_uiPhase = PHASE_ICEHOWL; break;
            case NPC_ACIDMAW:
                // Cast emerge and delayed set in combat?
                pSummoned->SetInCombatWithZone();
                m_aSummonedBossGuid[3] = pSummoned->GetObjectGuid();
                return;
            }

            m_aSummonedBossGuid[m_uiPhase] = pSummoned->GetObjectGuid();

            pSummoned->GetMotionMaster()->MovePoint(m_uiPhase, aMovePositions[m_uiPhase][0], aMovePositions[m_uiPhase][1], aMovePositions[m_uiPhase][2]);

            // Next beasts are summoned only for heroic modes
            if (m_creature->GetMap()->GetDifficulty() == RAID_DIFFICULTY_10MAN_HEROIC || m_creature->GetMap()->GetDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC)
                m_uiNextBeastTimer = 150 * IN_MILLISECONDS;     // 2 min 30

            m_uiAttackDelayTimer = 15000;                       // TODO, must be checked.
        }

        // Only for Dreadscale and Icehowl
        void DoSummonNextBeast(uint32 uiBeastEntry)
        {
            if (uiBeastEntry == NPC_DREADSCALE)
            {
                if (Creature* pTirion = m_pInstance->GetSingleCreatureFromStorage(NPC_TIRION_A))
                    DoScriptText(SAY_TIRION_BEAST_2, pTirion);

                m_creature->SummonCreature(NPC_DREADSCALE, aSpawnPositions[2][0], aSpawnPositions[2][1], aSpawnPositions[2][2], aSpawnPositions[2][3], TEMPSUMMON_DEAD_DESPAWN, 0);
            }
            else
            {
                if (Creature* pTirion = m_pInstance->GetSingleCreatureFromStorage(NPC_TIRION_A))
                    DoScriptText(SAY_TIRION_BEAST_3, pTirion);

                m_creature->SummonCreature(NPC_ICEHOWL, aSpawnPositions[4][0], aSpawnPositions[4][1], aSpawnPositions[4][2], aSpawnPositions[4][3], TEMPSUMMON_DEAD_DESPAWN, 0);
            }
        }

        void SummonedCreatureJustDied(Creature* pSummoned) override
        {
            if (!m_pInstance)
                return;

            switch (pSummoned->GetEntry())
            {
            case NPC_GORMOK:
                if (m_uiPhase == PHASE_GORMOK)
                    DoSummonNextBeast(NPC_DREADSCALE);
                break;

            case NPC_DREADSCALE:
            case NPC_ACIDMAW:
                if (m_bFirstWormDied && m_uiPhase == PHASE_WORMS)
                    DoSummonNextBeast(NPC_ICEHOWL);
                else
                    m_bFirstWormDied = true;
                break;

            case NPC_ICEHOWL:
                m_pInstance->SetData(TYPE_NORTHREND_BEASTS, DONE);
                m_creature->ForcedDespawn();
                break;
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_uiNextBeastTimer)
            {
                if (m_uiNextBeastTimer <= uiDiff)
                {
                    if (m_uiPhase == PHASE_GORMOK)
                        DoSummonNextBeast(NPC_DREADSCALE);
                    else if (m_uiPhase == PHASE_WORMS)
                        DoSummonNextBeast(NPC_ICEHOWL);

                    m_uiNextBeastTimer = 0;
                }
                else
                    m_uiNextBeastTimer -= uiDiff;
            }

            if (m_uiAttackDelayTimer)
            {
                if (m_uiAttackDelayTimer <= uiDiff)
                {
                    if (m_uiPhase == PHASE_WORMS)
                        m_creature->SummonCreature(NPC_ACIDMAW, aSpawnPositions[3][0], aSpawnPositions[3][1], aSpawnPositions[3][2], aSpawnPositions[3][3], TEMPSUMMON_DEAD_DESPAWN, 0);

                    if (Creature* pBeast = m_creature->GetMap()->GetCreature(m_aSummonedBossGuid[m_uiPhase]))
                        pBeast->SetInCombatWithZone();

                    m_uiAttackDelayTimer = 0;
                }
                else
                    m_uiAttackDelayTimer -= uiDiff;
            }

            if (m_uiBerserkTimer)
            {
                if (m_uiBerserkTimer < uiDiff)
                {
                    for (uint8 i = 0; i < 4; ++i)
                    {
                        Creature* pBoss = m_creature->GetMap()->GetCreature(m_aSummonedBossGuid[i]);
                        if (pBoss && pBoss->IsAlive())
                            pBoss->CastSpell(pBoss, SPELL_BERSERK, true);
                    }
                }
                else
                    m_uiBerserkTimer -= uiDiff;
            }

            m_creature->SelectHostileTarget();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_beast_combat_stalkerAI(pCreature);
    }
};

/*######
## boss_gormok, vehicle driven by 34800 (multiple times)
######*/

struct boss_gormok : public CreatureScript
{
    boss_gormok() : CreatureScript("boss_gormok") {}

    struct boss_gormokAI : public ScriptedAI
    {
        boss_gormokAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        }

        ScriptedInstance* m_pInstance;

        void Reset() override {}

        void JustReachedHome() override
        {
        }

        void Aggro(Unit* /*pWho*/) override
        {
        }

        void UpdateAI(const uint32 /*uiDiff*/) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_gormokAI(pCreature);
    }
};

/*######
## boss_acidmaw
######*/

struct boss_acidmaw : public CreatureScript
{
    boss_acidmaw() : CreatureScript("boss_acidmaw") {}

    struct boss_acidmawAI : public ScriptedAI
    {
        boss_acidmawAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        }

        ScriptedInstance* m_pInstance;

        void Reset() override {}

        void JustReachedHome() override
        {
        }

        void Aggro(Unit* /*pWho*/) override
        {
        }

        void UpdateAI(const uint32 /*uiDiff*/) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_acidmawAI(pCreature);
    }
};

/*######
## boss_dreadscale
######*/

struct boss_dreadscale : public CreatureScript
{
    boss_dreadscale() : CreatureScript("boss_dreadscale") {}

    struct boss_dreadscaleAI : public ScriptedAI
    {
        boss_dreadscaleAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        }

        ScriptedInstance* m_pInstance;

        void Reset() override {}

        void JustReachedHome() override
        {
        }

        void Aggro(Unit* /*pWho*/) override
        {
        }

        void UpdateAI(const uint32 /*uiDiff*/) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_dreadscaleAI(pCreature);
    }
};

/*######
## boss_icehowl
######*/

enum
{
    EMOTE_MASSIVE_CRASH                 = -1649039,
};

struct boss_icehowl : public CreatureScript
{
    boss_icehowl() : CreatureScript("boss_icehowl") {}

    struct boss_icehowlAI : public ScriptedAI
    {
        boss_icehowlAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        }

        ScriptedInstance* m_pInstance;

        void Reset() override {}

        void JustReachedHome() override
        {
        }

        void Aggro(Unit* /*pWho*/) override
        {
        }

        void UpdateAI(const uint32 /*uiDiff*/) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_icehowlAI(pCreature);
    }
};

void AddSC_northrend_beasts()
{
    Script* s;

    s = new npc_beast_combat_stalker();
    s->RegisterSelf();
    s = new boss_gormok();
    s->RegisterSelf();
    s = new boss_acidmaw();
    s->RegisterSelf();
    s = new boss_dreadscale();
    s->RegisterSelf();
    s = new boss_icehowl();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_beast_combat_stalker";
    //pNewScript->GetAI = &GetAI_npc_beast_combat_stalker;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "boss_gormok";
    //pNewScript->GetAI = &GetAI_boss_gormok;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "boss_acidmaw";
    //pNewScript->GetAI = &GetAI_boss_acidmaw;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "boss_dreadscale";
    //pNewScript->GetAI = &GetAI_boss_dreadscale;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "boss_icehowl";
    //pNewScript->GetAI = &GetAI_boss_icehowl;
    //pNewScript->RegisterSelf();
}
