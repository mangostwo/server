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
 */

/* ScriptData
SDName: boss_general_vezax
SD%Complete: 90%
SDComment: Some details may need some small adjustments
SDCategory: Ulduar
EndScriptData */

#include "precompiled.h"
#include "ulduar.h"

enum
{
    SAY_AGGRO                           = -1603096,
    SAY_SLAY_1                          = -1603097,
    SAY_SLAY_2                          = -1603098,
    SAY_SURGE                           = -1603099,
    SAY_DEATH                           = -1603100,
    SAY_ENRAGE                          = -1603101,
    SAY_HARD_MODE                       = -1603102,

    EMOTE_VAPOR                         = -1603103,
    EMOTE_SURGE                         = -1603104,
    EMOTE_ANIMUS                        = -1603105,

    SPELL_AURA_OF_DESPAIR               = 62692,
    SPELL_SHADOW_CRASH                  = 62660,
    SPELL_MARK_OF_FACELESS              = 63276,            // triggers 63278
    SPELL_SEARING_FLAMES                = 62661,
    SPELL_SURGE_OF_DARKNESS             = 62662,
    SPELL_BERSERK                       = 26662,
    SPELL_SARONITE_BARRIER              = 63364,            // also sends event 9735
    SPELL_SUMMON_ANIMUS                 = 63145,            // the animus should spam 63420 on target - to be done in acid
    SPELL_SUMMON_VAPORS                 = 63081,
    SPELL_ANIMUS_FORMATION              = 63319,            // visual aura on the vapors
    SPELL_SARONITE_VAPORS               = 63323,            // cast by vapor on death
    SPELL_CORRUPTED_RAGE                = 68415,            // Unused
    SPELL_CORRUPTED_WISDOM              = 64646,

    MAX_HARD_MODE_VAPORS                = 6,

    NPC_SARONITE_VAPOR                  = 33488,
};

/*######
## boss_general_vezax
######*/

struct MANGOS_DLL_DECL boss_general_vezaxAI : public ScriptedAI
{
    boss_general_vezaxAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (instance_ulduar*)pCreature->GetInstanceData();
        Reset();
    }

    instance_ulduar* m_pInstance;

    uint32 m_uiEnrageTimer;
    uint32 m_uiCrashTimer;
    uint32 m_uiMarkTimer;
    uint32 m_uiFlamesTimer;
    uint32 m_uiSurgeTimer;
    uint32 m_uiSaroniteVaporTimer;
    uint32 m_uiSaroniteBarrierTimer;

    uint8 m_uiVaporsGathered;

    GuidList m_lVaporsGuids;

    void Reset() override
    {
        m_uiEnrageTimer          = 10 * MINUTE * IN_MILLISECONDS;
        m_uiFlamesTimer          = 8000;
        m_uiSaroniteVaporTimer   = 30000;
        m_uiSurgeTimer           = 60000;
        m_uiMarkTimer            = 20000;
        m_uiCrashTimer           = 15000;
        m_uiSaroniteBarrierTimer = 0;
        m_uiVaporsGathered       = 0;
    }

    void Aggro(Unit* /*pWho*/) override
    {
        if (m_pInstance)
        {
            m_pInstance->SetData(TYPE_VEZAX, IN_PROGRESS);
            m_pInstance->SetData(TYPE_VEZAX_HARD, NOT_STARTED);
        }

        DoCastSpellIfCan(m_creature, SPELL_AURA_OF_DESPAIR);

        DoScriptText(SAY_AGGRO, m_creature);
    }

    void JustReachedHome() override
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_VEZAX, FAIL);
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_VEZAX, DONE);

        DoScriptText(SAY_DEATH, m_creature);
    }

    void KilledUnit(Unit* /*pVictim*/) override
    {
        DoScriptText(urand(0, 1) ? SAY_SLAY_1 : SAY_SLAY_2, m_creature);
    }

    void JustSummoned(Creature* pSummoned) override
    {
        if (pSummoned->GetEntry() == NPC_SARONITE_VAPOR)
        {
            m_lVaporsGuids.push_back(pSummoned->GetObjectGuid());

            // if vapors have reached the max number for hard mode then summon animus
            if (m_lVaporsGuids.size() == MAX_HARD_MODE_VAPORS)
                DoPrepareAnimusIfCan();
        }
        else if (pSummoned->GetEntry() == NPC_SARONITE_ANIMUS)
            pSummoned->SetInCombatWithZone();
    }

    void SummonedCreatureJustDied(Creature* pSummoned) override
    {
        // decrease the number of vapors when they die
        if (pSummoned->GetEntry() == NPC_SARONITE_VAPOR)
        {
            pSummoned->CastSpell(pSummoned, SPELL_SARONITE_VAPORS, true);
            m_lVaporsGuids.remove(pSummoned->GetObjectGuid());
        }
        // remove saronite barrier when animus dies
        else if (pSummoned->GetEntry() == NPC_SARONITE_ANIMUS)
        {
            m_creature->RemoveAurasDueToSpell(SPELL_SARONITE_BARRIER);

            if (m_creature->isAlive() && m_pInstance)
                m_pInstance->SetData(TYPE_VEZAX_HARD, DONE);
        }
    }

    void SummonedMovementInform(Creature* pSummoned, uint32 uiMoveType, uint32 uiPointId) override
    {
        if (uiMoveType != POINT_MOTION_TYPE || uiPointId != 0 || pSummoned->GetEntry() != NPC_SARONITE_VAPOR)
            return;

        ++m_uiVaporsGathered;

        // Cast the saronite formation aura when all vapors arive
        if (m_uiVaporsGathered == MAX_HARD_MODE_VAPORS)
        {
            pSummoned->CastSpell(pSummoned, SPELL_ANIMUS_FORMATION, true);

            // Despawn the vapors
            for (GuidList::const_iterator itr = m_lVaporsGuids.begin(); itr != m_lVaporsGuids.end(); ++itr)
            {
                if (Creature* pVapor = m_creature->GetMap()->GetCreature(*itr))
                    pVapor->ForcedDespawn(4000);
            }

            DoScriptText(EMOTE_ANIMUS, m_creature);
            m_uiSaroniteBarrierTimer = 4000;
        }
    }

    // Merge vapors
    void DoPrepareAnimusIfCan()
    {
        // Gather the vapors to the boss - NOTE: not sure if position is ok
        for (GuidList::const_iterator itr = m_lVaporsGuids.begin(); itr != m_lVaporsGuids.end(); ++itr)
        {
            // Better place: near him or some fixed position?
            if (Creature* pVapor = m_creature->GetMap()->GetCreature(*itr))
                pVapor->GetMotionMaster()->MovePoint(0, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ());
        }
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiSaroniteBarrierTimer)
        {
            if (m_uiSaroniteBarrierTimer <= uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_SARONITE_BARRIER) == CAST_OK)
                    m_uiSaroniteBarrierTimer = 0;
            }
            else
                m_uiSaroniteBarrierTimer -= uiDiff;
        }

        // summon saronite vapors before the hard mode
        if (m_pInstance && m_pInstance->GetData(TYPE_VEZAX_HARD) == NOT_STARTED)
        {
            if (m_uiSaroniteVaporTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_SUMMON_VAPORS) == CAST_OK)
                {
                    DoScriptText(EMOTE_VAPOR, m_creature);
                    m_uiSaroniteVaporTimer = 30000;
                }
            }
            else
                m_uiSaroniteVaporTimer -= uiDiff;
        }

        // Searing flames only while animus is not around
        if (m_pInstance && m_pInstance->GetData(TYPE_VEZAX_HARD) != IN_PROGRESS)
        {
            if (m_uiFlamesTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_SEARING_FLAMES) == CAST_OK)
                    m_uiFlamesTimer = urand(9000, 16000);
            }
            else
                m_uiFlamesTimer -= uiDiff;
        }

        if (m_uiSurgeTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_SURGE_OF_DARKNESS) == CAST_OK)
            {
                DoScriptText(SAY_SURGE, m_creature);
                DoScriptText(EMOTE_SURGE, m_creature);
                m_uiSurgeTimer = 62000;
            }
        }
        else
            m_uiSurgeTimer -= uiDiff;

        if (m_uiMarkTimer < uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 1))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_MARK_OF_FACELESS) == CAST_OK)
                    m_uiMarkTimer = urand(25000, 30000);
            }
        }
        else
            m_uiMarkTimer -= uiDiff;

        if (m_uiCrashTimer < uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 1))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_SHADOW_CRASH) == CAST_OK)
                    m_uiCrashTimer = 15000;
            }
        }
        else
            m_uiCrashTimer -= uiDiff;

        if (m_uiEnrageTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_BERSERK) == CAST_OK)
            {
                DoScriptText(SAY_ENRAGE, m_creature);
                m_uiEnrageTimer = 30000;
            }
        }
        else
            m_uiEnrageTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_general_vezax(Creature* pCreature)
{
    return new boss_general_vezaxAI(pCreature);
}

bool ProcessEventId_event_spell_saronite_barrier(uint32 /*uiEventId*/, Object* pSource, Object* /*pTarget*/, bool /*bIsStart*/)
{
    if (pSource->GetTypeId() == TYPEID_UNIT && ((Creature*)pSource)->GetEntry() == NPC_VEZAX)
    {
        if (instance_ulduar* pInstance = (instance_ulduar*)((Creature*)pSource)->GetInstanceData())
        {
            // Start hard mode for Vezax and summon the Animus
            pInstance->SetData(TYPE_VEZAX_HARD, IN_PROGRESS);

            ((Creature*)pSource)->CastSpell((Creature*)pSource, SPELL_SUMMON_ANIMUS, true);
            DoScriptText(SAY_HARD_MODE, (Creature*)pSource);

            return true;
        }
    }
    return false;
}

void AddSC_boss_general_vezax()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "boss_general_vezax";
    pNewScript->GetAI = &GetAI_boss_general_vezax;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "event_spell_saronite_barrier";
    pNewScript->pProcessEventId = &ProcessEventId_event_spell_saronite_barrier;
    pNewScript->RegisterSelf();
}
