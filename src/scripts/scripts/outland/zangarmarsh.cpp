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
SDName: Zangarmarsh
SD%Complete: 100
SDComment: Quest support: 9752, 9785, 10009.
SDCategory: Zangarmarsh
EndScriptData */

/* ContentData
npc_cooshcoosh
npc_kayra_longmane
event_stormcrow
EndContentData */

#include "precompiled.h"
#include "escort_ai.h"

/*######
## npc_cooshcoosh
######*/

enum
{
    SPELL_LIGHTNING_BOLT    = 9532,
};

struct MANGOS_DLL_DECL npc_cooshcooshAI : public ScriptedAI
{
    npc_cooshcooshAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_uiNormFaction = pCreature->getFaction();
        Reset();
    }

    uint32 m_uiNormFaction;
    uint32 m_uiLightningBolt_Timer;

    void Reset() override
    {
        m_uiLightningBolt_Timer = 2000;

        if (m_creature->getFaction() != m_uiNormFaction)
            m_creature->setFaction(m_uiNormFaction);
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiLightningBolt_Timer < uiDiff)
        {
            DoCastSpellIfCan(m_creature->getVictim(), SPELL_LIGHTNING_BOLT);
            m_uiLightningBolt_Timer = 5000;
        }
        else m_uiLightningBolt_Timer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_cooshcoosh(Creature* pCreature)
{
    return new npc_cooshcooshAI(pCreature);
}

/*#####
## npc_kayra_longmane
#####*/

enum
{
    SAY_START           = -1000343,
    SAY_AMBUSH1         = -1000344,
    SAY_PROGRESS        = -1000345,
    SAY_AMBUSH2         = -1000346,
    SAY_END             = -1000347,

    QUEST_ESCAPE_FROM   = 9752,
    NPC_SLAVEBINDER     = 18042
};

struct MANGOS_DLL_DECL npc_kayra_longmaneAI : public npc_escortAI
{
    npc_kayra_longmaneAI(Creature* pCreature) : npc_escortAI(pCreature) { Reset(); }

    void WaypointReached(uint32 i) override
    {
        Player* pPlayer = GetPlayerForEscort();

        if (!pPlayer)
            return;

        switch (i)
        {
            case 4:
                DoScriptText(SAY_AMBUSH1, m_creature, pPlayer);
                DoSpawnCreature(NPC_SLAVEBINDER, -10.0f, -5.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_OOC_DESPAWN, 30000);
                DoSpawnCreature(NPC_SLAVEBINDER, -8.0f, 5.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_OOC_DESPAWN, 30000);
                break;
            case 5:
                DoScriptText(SAY_PROGRESS, m_creature, pPlayer);
                SetRun();
                break;
            case 16:
                DoScriptText(SAY_AMBUSH2, m_creature, pPlayer);
                DoSpawnCreature(NPC_SLAVEBINDER, -10.0f, -5.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_OOC_DESPAWN, 30000);
                DoSpawnCreature(NPC_SLAVEBINDER, -8.0f, 5.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_OOC_DESPAWN, 30000);
                break;
            case 17:
                DoScriptText(SAY_END, m_creature, pPlayer);
                break;
            case 25:
                pPlayer->GroupEventHappens(QUEST_ESCAPE_FROM, m_creature);
                break;
        }
    }

    void Reset() override { }
};

bool QuestAccept_npc_kayra_longmane(Player* pPlayer, Creature* pCreature, const Quest* pQuest)
{
    if (pQuest->GetQuestId() == QUEST_ESCAPE_FROM)
    {
        DoScriptText(SAY_START, pCreature, pPlayer);

        if (npc_kayra_longmaneAI* pEscortAI = dynamic_cast<npc_kayra_longmaneAI*>(pCreature->AI()))
            pEscortAI->Start(false, pPlayer, pQuest);
    }
    return true;
}

CreatureAI* GetAI_npc_kayra_longmane(Creature* pCreature)
{
    return new npc_kayra_longmaneAI(pCreature);
}

/*######
## event_stormcrow
######*/

enum
{
    QUEST_AS_THE_CROW_FLIES = 9718,
    EVENT_ID_STORMCROW      = 11225,
};

bool ProcessEventId_event_taxi_stormcrow(uint32 uiEventId, Object* pSource, Object* /*pTarget*/, bool bIsStart)
{
    if (uiEventId == EVENT_ID_STORMCROW && !bIsStart && pSource->GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)pSource)->SetDisplayId(((Player*)pSource)->GetNativeDisplayId());
        ((Player*)pSource)->AreaExploredOrEventHappens(QUEST_AS_THE_CROW_FLIES);
        return true;
    }
    return false;
}

void AddSC_zangarmarsh()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "npc_cooshcoosh";
    pNewScript->GetAI = &GetAI_npc_cooshcoosh;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_kayra_longmane";
    pNewScript->GetAI = &GetAI_npc_kayra_longmane;
    pNewScript->pQuestAcceptNPC = &QuestAccept_npc_kayra_longmane;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "event_taxi_stormcrow";
    pNewScript->pProcessEventId = &ProcessEventId_event_taxi_stormcrow;
    pNewScript->RegisterSelf();
}
