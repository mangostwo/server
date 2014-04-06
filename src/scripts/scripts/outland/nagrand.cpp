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
SDName: Nagrand
SD%Complete: 90
SDComment: Quest support: 9868, 9918, 10085, 10646.
SDCategory: Nagrand
EndScriptData */

/* ContentData
mob_lump
npc_maghar_captive
npc_creditmarker_visit_with_ancestors
EndContentData */

#include "precompiled.h"
#include "escort_ai.h"

/*######
## mob_lump
######*/

enum
{
    SAY_LUMP_AGGRO_1            = -1000190,
    SAY_LUMP_AGGRO_2            = -1000191,
    SAY_LUMP_DEFEAT             = -1000192,

    SPELL_VISUAL_SLEEP          = 16093,
    SPELL_SPEAR_THROW           = 32248,

    FACTION_FRIENDLY            = 35
};

struct MANGOS_DLL_DECL mob_lumpAI : public ScriptedAI
{
    mob_lumpAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_bReset = false;
        Reset();
    }

    uint32 m_uiResetTimer;
    uint32 m_uiSpearThrowTimer;
    bool m_bReset;

    void Reset() override
    {
        m_uiResetTimer = MINUTE * IN_MILLISECONDS;
        m_uiSpearThrowTimer = 2000;
    }

    void AttackedBy(Unit* pAttacker) override
    {
        if (m_creature->getVictim())
            return;

        if (m_creature->IsFriendlyTo(pAttacker))
            return;

        AttackStart(pAttacker);
    }

    void DamageTaken(Unit* /*pDealer*/, uint32& uiDamage) override
    {
        if (m_creature->GetHealth() < uiDamage || (m_creature->GetHealth() - uiDamage) * 100 / m_creature->GetMaxHealth() < 30)
        {
            uiDamage = 0;                               // Take 0 damage

            m_creature->RemoveAllAuras();
            m_creature->DeleteThreatList();
            m_creature->CombatStop(true);

            // should get unit_flags UNIT_FLAG_OOC_NOT_ATTACKABLE | UNIT_FLAG_PASSIVE at faction change, but unclear why/for what reason, skipped (no flags expected as default)
            m_creature->SetFactionTemporary(FACTION_FRIENDLY, TEMPFACTION_RESTORE_REACH_HOME);

            m_creature->SetStandState(UNIT_STAND_STATE_SIT);
            DoScriptText(SAY_LUMP_DEFEAT, m_creature);

            m_bReset = true;
        }
    }

    void Aggro(Unit* pWho) override
    {
        if (m_creature->HasAura(SPELL_VISUAL_SLEEP, EFFECT_INDEX_0))
            m_creature->RemoveAurasDueToSpell(SPELL_VISUAL_SLEEP);

        if (!m_creature->IsStandState())
            m_creature->SetStandState(UNIT_STAND_STATE_STAND);

        DoScriptText(urand(0, 1) ? SAY_LUMP_AGGRO_1 : SAY_LUMP_AGGRO_2, m_creature, pWho);
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        // Check if we waiting for a reset
        if (m_bReset)
        {
            if (m_uiResetTimer < uiDiff)
            {
                EnterEvadeMode();
                m_bReset = false;
            }
            else
                m_uiResetTimer -= uiDiff;
        }

        // Return since we have no target
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        // SpearThrow Timer
        if (m_uiSpearThrowTimer < uiDiff)
        {
            DoCastSpellIfCan(m_creature->getVictim(), SPELL_SPEAR_THROW);
            m_uiSpearThrowTimer = 20000;
        }
        else
            m_uiSpearThrowTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_mob_lump(Creature* pCreature)
{
    return new mob_lumpAI(pCreature);
}

/*######
## npc_maghar_captive
#####*/

enum
{
    SAY_MAG_START               = -1000482,
    SAY_MAG_NO_ESCAPE           = -1000483,
    SAY_MAG_MORE                = -1000484,
    SAY_MAG_MORE_REPLY          = -1000485,
    SAY_MAG_LIGHTNING           = -1000486,
    SAY_MAG_SHOCK               = -1000487,
    SAY_MAG_COMPLETE            = -1000488,

    SPELL_CHAIN_LIGHTNING       = 16006,
    SPELL_EARTHBIND_TOTEM       = 15786,
    SPELL_FROST_SHOCK           = 12548,
    SPELL_HEALING_WAVE          = 12491,

    QUEST_TOTEM_KARDASH_H       = 9868,

    NPC_MURK_RAIDER             = 18203,
    NPC_MURK_BRUTE              = 18211,
    NPC_MURK_SCAVENGER          = 18207,
    NPC_MURK_PUTRIFIER          = 18202
};

static float m_afAmbushA[] = { -1568.805786f, 8533.873047f, 1.958f};
static float m_afAmbushB[] = { -1491.554321f, 8506.483398f, 1.248f};

struct MANGOS_DLL_DECL npc_maghar_captiveAI : public npc_escortAI
{
    npc_maghar_captiveAI(Creature* pCreature) : npc_escortAI(pCreature) { Reset(); }

    uint32 m_uiChainLightningTimer;
    uint32 m_uiHealTimer;
    uint32 m_uiFrostShockTimer;

    void Reset() override
    {
        m_uiChainLightningTimer = 1000;
        m_uiHealTimer = 0;
        m_uiFrostShockTimer = 6000;
    }

    void Aggro(Unit* /*pWho*/) override
    {
        m_creature->CastSpell(m_creature, SPELL_EARTHBIND_TOTEM, false);
    }

    void WaypointReached(uint32 uiPointId) override
    {
        switch (uiPointId)
        {
            case 7:
                DoScriptText(SAY_MAG_MORE, m_creature);

                if (Creature* pTemp = m_creature->SummonCreature(NPC_MURK_PUTRIFIER, m_afAmbushB[0], m_afAmbushB[1], m_afAmbushB[2], 0.0f, TEMPSUMMON_TIMED_OOC_DESPAWN, 25000))
                    DoScriptText(SAY_MAG_MORE_REPLY, pTemp);

                m_creature->SummonCreature(NPC_MURK_PUTRIFIER, m_afAmbushB[0] - 2.5f, m_afAmbushB[1] - 2.5f, m_afAmbushB[2], 0.0f, TEMPSUMMON_TIMED_OOC_DESPAWN, 25000);

                m_creature->SummonCreature(NPC_MURK_SCAVENGER, m_afAmbushB[0] + 2.5f, m_afAmbushB[1] + 2.5f, m_afAmbushB[2], 0.0f, TEMPSUMMON_TIMED_OOC_DESPAWN, 25000);
                m_creature->SummonCreature(NPC_MURK_SCAVENGER, m_afAmbushB[0] + 2.5f, m_afAmbushB[1] - 2.5f, m_afAmbushB[2], 0.0f, TEMPSUMMON_TIMED_OOC_DESPAWN, 25000);
                break;
            case 16:
                DoScriptText(SAY_MAG_COMPLETE, m_creature);

                if (Player* pPlayer = GetPlayerForEscort())
                    pPlayer->GroupEventHappens(QUEST_TOTEM_KARDASH_H, m_creature);

                SetRun();
                break;
        }
    }

    void JustSummoned(Creature* pSummoned) override
    {
        if (pSummoned->GetEntry() == NPC_MURK_BRUTE)
            DoScriptText(SAY_MAG_NO_ESCAPE, pSummoned);

        if (pSummoned->IsTotem())
            return;

        pSummoned->SetWalk(false);
        pSummoned->GetMotionMaster()->MovePoint(0, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ());
    }

    void SpellHitTarget(Unit* /*pTarget*/, const SpellEntry* pSpell) override
    {
        if (pSpell->Id == SPELL_CHAIN_LIGHTNING)
        {
            if (urand(0, 9))
                return;

            DoScriptText(SAY_MAG_LIGHTNING, m_creature);
        }
    }

    void UpdateEscortAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiChainLightningTimer < uiDiff)
        {
            DoCastSpellIfCan(m_creature->getVictim(), SPELL_CHAIN_LIGHTNING);
            m_uiChainLightningTimer = urand(7000, 14000);
        }
        else
            m_uiChainLightningTimer -= uiDiff;

        if (m_creature->GetHealthPercent() < 30.0f)
        {
            if (m_uiHealTimer < uiDiff)
            {
                DoCastSpellIfCan(m_creature, SPELL_HEALING_WAVE);
                m_uiHealTimer = 5000;
            }
            else
                m_uiHealTimer -= uiDiff;
        }

        if (m_uiFrostShockTimer < uiDiff)
        {
            DoCastSpellIfCan(m_creature->getVictim(), SPELL_FROST_SHOCK);
            m_uiFrostShockTimer = urand(7500, 15000);
        }
        else
            m_uiFrostShockTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

bool QuestAccept_npc_maghar_captive(Player* pPlayer, Creature* pCreature, const Quest* pQuest)
{
    if (pQuest->GetQuestId() == QUEST_TOTEM_KARDASH_H)
    {
        if (npc_maghar_captiveAI* pEscortAI = dynamic_cast<npc_maghar_captiveAI*>(pCreature->AI()))
        {
            pCreature->SetStandState(UNIT_STAND_STATE_STAND);
            pCreature->SetFactionTemporary(FACTION_ESCORT_H_NEUTRAL_ACTIVE, TEMPFACTION_RESTORE_RESPAWN);

            pEscortAI->Start(false, pPlayer, pQuest);

            DoScriptText(SAY_MAG_START, pCreature);

            pCreature->SummonCreature(NPC_MURK_RAIDER, m_afAmbushA[0] + 2.5f, m_afAmbushA[1] - 2.5f, m_afAmbushA[2], 0.0f, TEMPSUMMON_TIMED_OOC_DESPAWN, 25000);
            pCreature->SummonCreature(NPC_MURK_PUTRIFIER, m_afAmbushA[0] - 2.5f, m_afAmbushA[1] + 2.5f, m_afAmbushA[2], 0.0f, TEMPSUMMON_TIMED_OOC_DESPAWN, 25000);
            pCreature->SummonCreature(NPC_MURK_BRUTE, m_afAmbushA[0], m_afAmbushA[1], m_afAmbushA[2], 0.0f, TEMPSUMMON_TIMED_OOC_DESPAWN, 25000);
        }
    }
    return true;
}

CreatureAI* GetAI_npc_maghar_captive(Creature* pCreature)
{
    return new npc_maghar_captiveAI(pCreature);
}

/*######
## npc_creditmarker_visist_with_ancestors (Quest 10085)
######*/

enum
{
    QUEST_VISIT_WITH_ANCESTORS  = 10085
};

struct MANGOS_DLL_DECL npc_creditmarker_visit_with_ancestorsAI : public ScriptedAI
{
    npc_creditmarker_visit_with_ancestorsAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    void Reset() override {}

    void MoveInLineOfSight(Unit* pWho) override
    {
        if (pWho->GetTypeId() == TYPEID_PLAYER && m_creature->IsWithinDistInMap(pWho, 30.0f))
        {
            if (((Player*)pWho)->GetQuestStatus(QUEST_VISIT_WITH_ANCESTORS) == QUEST_STATUS_INCOMPLETE)
            {
                uint32 creditMarkerId = m_creature->GetEntry();
                if ((creditMarkerId >= 18840) && (creditMarkerId <= 18843))
                {
                    // 18840: Sunspring, 18841: Laughing, 18842: Garadar, 18843: Bleeding
                    if (!((Player*)pWho)->GetReqKillOrCastCurrentCount(QUEST_VISIT_WITH_ANCESTORS, creditMarkerId))
                        ((Player*)pWho)->KilledMonsterCredit(creditMarkerId, m_creature->GetObjectGuid());
                }
            }
        }
    }
};

CreatureAI* GetAI_npc_creditmarker_visit_with_ancestors(Creature* pCreature)
{
    return new npc_creditmarker_visit_with_ancestorsAI(pCreature);
}

/*######
## AddSC
######*/

void AddSC_nagrand()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "mob_lump";
    pNewScript->GetAI = &GetAI_mob_lump;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_maghar_captive";
    pNewScript->GetAI = &GetAI_npc_maghar_captive;
    pNewScript->pQuestAcceptNPC = &QuestAccept_npc_maghar_captive;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_creditmarker_visit_with_ancestors";
    pNewScript->GetAI = &GetAI_npc_creditmarker_visit_with_ancestors;
    pNewScript->RegisterSelf();
}
