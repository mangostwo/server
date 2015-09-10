/**
 * ScriptDev2 is an extension for mangos providing enhanced features for
 * area triggers, creatures, game objects, instances, items, and spells beyond
 * the default database scripting in mangos.
 *
 * Copyright (C) 2006-2013  ScriptDev2 <http://www.scriptdev2.com/>
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

/* ScriptData
SDName: Boss_Kelidan_The_Breaker
SD%Complete: 100
SDComment:
SDCategory: Hellfire Citadel, Blood Furnace
EndScriptData */

/* ContentData
boss_kelidan_the_breaker
mob_shadowmoon_channeler
EndContentData */

#include "precompiled.h"
#include "blood_furnace.h"

enum
{
    MAX_ADDS                    = 5,

    SAY_MAGTHERIDON_INTRO       = -1542016,                 // Yell by Magtheridon
    SAY_WAKE                    = -1542000,
    SAY_ADD_AGGRO_1             = -1542001,
    SAY_ADD_AGGRO_2             = -1542002,
    SAY_ADD_AGGRO_3             = -1542003,
    SAY_KILL_1                  = -1542004,
    SAY_KILL_2                  = -1542005,
    SAY_NOVA                    = -1542006,
    SAY_DIE                     = -1542007,

    SPELL_CORRUPTION            = 30938,
    SPELL_EVOCATION             = 30935,

    SPELL_FIRE_NOVA             = 33132,
    SPELL_FIRE_NOVA_H           = 37371,

    SPELL_SHADOW_BOLT_VOLLEY    = 28599,
    SPELL_SHADOW_BOLT_VOLLEY_H  = 40070,

    SPELL_BURNING_NOVA          = 30940,
    SPELL_VORTEX                = 37370,
};

struct boss_kelidan_the_breaker : public CreatureScript
{
    boss_kelidan_the_breaker() : CreatureScript("boss_kelidan_the_breaker") {}

    struct boss_kelidan_the_breakerAI : public ScriptedAI
    {
        boss_kelidan_the_breakerAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
            m_bIsRegularMode = pCreature->GetMap()->IsRegularDifficulty();
            m_uiSetupAddsTimer = 100;
            m_bDidMagtheridonYell = false;
            DoCastSpellIfCan(m_creature, SPELL_EVOCATION);
        }

        ScriptedInstance* m_pInstance;

        bool m_bIsRegularMode;

        uint32 m_uiShadowVolleyTimer;
        uint32 m_uiBurningNovaTimer;
        uint32 m_uiFirenovaTimer;
        uint32 m_uiCorruptionTimer;
        uint32 m_uiSetupAddsTimer;
        uint8 m_uiKilledAdds;
        bool m_bDidMagtheridonYell;

        GuidVector m_vAddGuids;

        void Reset() override
        {
            m_uiShadowVolleyTimer = 1000;
            m_uiBurningNovaTimer = 15000;
            m_uiCorruptionTimer = 5000;
            m_uiFirenovaTimer = 0;
            m_uiKilledAdds = 0;
        }

        void MoveInLineOfSight(Unit* pWho) override
        {
            if (!m_bDidMagtheridonYell && pWho->GetTypeId() == TYPEID_PLAYER && !((Player*)pWho)->isGameMaster() && m_creature->_IsWithinDist(pWho, 73.0f, false))
            {
                if (m_pInstance)
                {
                    m_pInstance->DoOrSimulateScriptTextForThisInstance(SAY_MAGTHERIDON_INTRO, NPC_MAGTHERIDON);
                }

                m_bDidMagtheridonYell = true;
            }

            ScriptedAI::MoveInLineOfSight(pWho);
        }

        void Aggro(Unit* /*pWho*/) override
        {
            DoScriptText(SAY_WAKE, m_creature);
        }

        void KilledUnit(Unit* /*pVictim*/) override
        {
            if (urand(0, 1))
            {
                return;
            }

            DoScriptText(urand(0, 1) ? SAY_KILL_1 : SAY_KILL_2, m_creature);
        }

        void JustDied(Unit* /*pKiller*/) override
        {
            DoScriptText(SAY_DIE, m_creature);

            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_KELIDAN_EVENT, DONE);
            }
        }

        void JustReachedHome() override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_KELIDAN_EVENT, FAIL);
            }

            DoCastSpellIfCan(m_creature, SPELL_EVOCATION);
            m_uiSetupAddsTimer = 2000;
        }

        void ReceiveAIEvent(AIEventType eventType, Creature *pSender, Unit *pInvoker, uint32 /*data*/) override
        {
            switch (eventType)
            {
            case AI_EVENT_CUSTOM_A:
                if (pSender->GetEntry() == NPC_SHADOWMOON_CHANNELER)
                    m_uiSetupAddsTimer = 2000;
                break;
            case AI_EVENT_CUSTOM_B:
                if (pSender->GetEntry() == NPC_SHADOWMOON_CHANNELER)
                {
                    ++m_uiKilledAdds;
                    if (m_uiKilledAdds == MAX_ADDS)
                    {
                        m_creature->InterruptNonMeleeSpells(true);
                        AttackStart(pInvoker);
                    }
                }
                break;
            default:
                break;
            }
        }

        void DoSetupAdds()
        {
            m_uiSetupAddsTimer = 0;

            if (!m_pInstance)
            {
                return;
            }
            m_pInstance->SetData(TYPE_ADD_DO_SETUP, 0);
            m_uiKilledAdds = 0;
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_uiSetupAddsTimer)
            {
                if (m_uiSetupAddsTimer <= uiDiff)
                {
                    DoSetupAdds();
                }
                else
                {
                    m_uiSetupAddsTimer -= uiDiff;
                }
            }

            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            if (m_uiFirenovaTimer)
            {
                if (m_uiFirenovaTimer <= uiDiff)
                {
                    if (DoCastSpellIfCan(m_creature, m_bIsRegularMode ? SPELL_FIRE_NOVA : SPELL_FIRE_NOVA_H) == CAST_OK)
                    {
                        m_uiFirenovaTimer = 0;
                        m_uiShadowVolleyTimer = 2000;
                    }
                }
                else
                {
                    m_uiFirenovaTimer -= uiDiff;
                }
            }

            if (m_uiShadowVolleyTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, m_bIsRegularMode ? SPELL_SHADOW_BOLT_VOLLEY : SPELL_SHADOW_BOLT_VOLLEY_H) == CAST_OK)
                {
                    m_uiShadowVolleyTimer = urand(5000, 13000);
                }
            }
            else
            {
                m_uiShadowVolleyTimer -= uiDiff;
            }

            if (m_uiCorruptionTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_CORRUPTION) == CAST_OK)
                {
                    m_uiCorruptionTimer = urand(30000, 50000);
                }
            }
            else
            {
                m_uiCorruptionTimer -= uiDiff;
            }

            if (m_uiBurningNovaTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_BURNING_NOVA, CAST_TRIGGERED) == CAST_OK)
                {
                    DoScriptText(SAY_NOVA, m_creature);

                    if (!m_bIsRegularMode)
                    {
                        DoCastSpellIfCan(m_creature, SPELL_VORTEX, CAST_TRIGGERED);
                    }

                    m_uiBurningNovaTimer = urand(20000, 28000);
                    m_uiFirenovaTimer = 5000;
                }
            }
            else
            {
                m_uiBurningNovaTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_kelidan_the_breakerAI(pCreature);
    }
};

/*######
## mob_shadowmoon_channeler
######*/

enum
{
    SPELL_SHADOW_BOLT       = 12739,
    SPELL_SHADOW_BOLT_H     = 15472,

    SPELL_MARK_OF_SHADOW    = 30937,
};

struct mob_shadowmoon_channeler : public CreatureScript
{
    mob_shadowmoon_channeler() : CreatureScript("mob_shadowmoon_channeler") {}

    struct mob_shadowmoon_channelerAI : public ScriptedAI
    {
        mob_shadowmoon_channelerAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
            m_bIsRegularMode = pCreature->GetMap()->IsRegularDifficulty();
        }

        ScriptedInstance* m_pInstance;
        bool m_bIsRegularMode;

        uint32 m_uiShadowBoltTimer;
        uint32 m_uiMarkOfShadowTimer;

        void Reset() override
        {
            m_uiShadowBoltTimer = urand(1000, 2000);
            m_uiMarkOfShadowTimer = urand(5000, 7000);
        }

        void Aggro(Unit* pWho) override
        {
            m_creature->InterruptNonMeleeSpells(false);

            switch (urand(0, 2))
            {
            case 0:
                DoScriptText(SAY_ADD_AGGRO_1, m_creature);
                break;
            case 1:
                DoScriptText(SAY_ADD_AGGRO_2, m_creature);
                break;
            case 2:
                DoScriptText(SAY_ADD_AGGRO_3, m_creature);
                break;
            }

            if (m_pInstance)
            {
                m_pInstance->SetData64(TYPE_ADD_AGGROED, pWho->GetObjectGuid().GetRawValue());
            }
        }

        void JustDied(Unit* pKiller) override
        {
            if (!m_pInstance)
            {
                return;
            }

            if (Creature* pKelidan = m_pInstance->GetSingleCreatureFromStorage(NPC_KELIDAN_THE_BREAKER))
                SendAIEvent(AI_EVENT_CUSTOM_B, pKiller, pKelidan);
        }

        void JustReachedHome() override
        {
            if (!m_pInstance)
            {
                return;
            }

            if (Creature* pKelidan = m_pInstance->GetSingleCreatureFromStorage(NPC_KELIDAN_THE_BREAKER))
                SendAIEvent(AI_EVENT_CUSTOM_A, m_creature, pKelidan);
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            if (m_uiMarkOfShadowTimer < uiDiff)
            {
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                {
                    if (DoCastSpellIfCan(pTarget, SPELL_MARK_OF_SHADOW) == CAST_OK)
                    {
                        m_uiMarkOfShadowTimer = urand(15000, 20000);
                    }
                }
            }
            else
            {
                m_uiMarkOfShadowTimer -= uiDiff;
            }

            if (m_uiShadowBoltTimer < uiDiff)
            {
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                {
                    if (DoCastSpellIfCan(pTarget, m_bIsRegularMode ? SPELL_SHADOW_BOLT : SPELL_SHADOW_BOLT_H) == CAST_OK)
                    {
                        m_uiShadowBoltTimer = urand(5000, 6000);
                    }
                }
            }
            else
            {
                m_uiShadowBoltTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new mob_shadowmoon_channelerAI(pCreature);
    }
};

void AddSC_boss_kelidan_the_breaker()
{
    Script* s;

    s = new boss_kelidan_the_breaker();
    s->RegisterSelf();
    s = new mob_shadowmoon_channeler();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "boss_kelidan_the_breaker";
    //pNewScript->GetAI = &GetAI_boss_kelidan_the_breaker;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "mob_shadowmoon_channeler";
    //pNewScript->GetAI = &GetAI_mob_shadowmoon_channeler;
    //pNewScript->RegisterSelf();
}
