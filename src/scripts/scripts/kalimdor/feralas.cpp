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
SDName: Feralas
SD%Complete: 100
SDComment: Quest support: 2767.
SDCategory: Feralas
EndScriptData */

#include "precompiled.h"
#include "escort_ai.h"

/*######
## npc_oox22fe
######*/

enum
{
    SAY_OOX_START           = -1000287,
    SAY_OOX_AGGRO1          = -1000288,
    SAY_OOX_AGGRO2          = -1000289,
    SAY_OOX_AMBUSH          = -1000290,
    SAY_OOX_END             = -1000292,

    NPC_YETI                = 7848,
    NPC_GORILLA             = 5260,
    NPC_WOODPAW_REAVER      = 5255,
    NPC_WOODPAW_BRUTE       = 5253,
    NPC_WOODPAW_ALPHA       = 5258,
    NPC_WOODPAW_MYSTIC      = 5254,

    QUEST_RESCUE_OOX22FE    = 2767
};

struct MANGOS_DLL_DECL npc_oox22feAI : public npc_escortAI
{
    npc_oox22feAI(Creature* pCreature) : npc_escortAI(pCreature) { Reset(); }

    void WaypointReached(uint32 i) override
    {
        switch (i)
        {
                // First Ambush(3 Yetis)
            case 11:
                DoScriptText(SAY_OOX_AMBUSH, m_creature);
                m_creature->SummonCreature(NPC_YETI, -4841.01f, 1593.91f, 73.42f, 3.98f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000);
                m_creature->SummonCreature(NPC_YETI, -4837.61f, 1568.58f, 78.21f, 3.13f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000);
                m_creature->SummonCreature(NPC_YETI, -4841.89f, 1569.95f, 76.53f, 0.68f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000);
                break;
                // Second Ambush(3 Gorillas)
            case 21:
                DoScriptText(SAY_OOX_AMBUSH, m_creature);
                m_creature->SummonCreature(NPC_GORILLA, -4595.81f, 2005.99f, 53.08f, 3.74f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000);
                m_creature->SummonCreature(NPC_GORILLA, -4597.53f, 2008.31f, 52.70f, 3.78f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000);
                m_creature->SummonCreature(NPC_GORILLA, -4599.37f, 2010.59f, 52.77f, 3.84f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000);
                break;
                // Third Ambush(4 Gnolls)
            case 30:
                DoScriptText(SAY_OOX_AMBUSH, m_creature);
                m_creature->SummonCreature(NPC_WOODPAW_REAVER, -4425.14f, 2075.87f, 47.77f, 3.77f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000);
                m_creature->SummonCreature(NPC_WOODPAW_BRUTE , -4426.68f, 2077.98f, 47.57f, 3.77f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000);
                m_creature->SummonCreature(NPC_WOODPAW_MYSTIC, -4428.33f, 2080.24f, 47.43f, 3.87f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000);
                m_creature->SummonCreature(NPC_WOODPAW_ALPHA , -4430.04f, 2075.54f, 46.83f, 3.81f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000);
                break;
            case 37:
                DoScriptText(SAY_OOX_END, m_creature);
                // Award quest credit
                if (Player* pPlayer = GetPlayerForEscort())
                    pPlayer->GroupEventHappens(QUEST_RESCUE_OOX22FE, m_creature);
                break;
        }
    }

    void Reset() override
    {
        if (!HasEscortState(STATE_ESCORT_ESCORTING))
            m_creature->SetStandState(UNIT_STAND_STATE_DEAD);
    }

    void Aggro(Unit* /*who*/) override
    {
        // For an small probability the npc says something when he get aggro
        switch (urand(0, 9))
        {
            case 0: DoScriptText(SAY_OOX_AGGRO1, m_creature); break;
            case 1: DoScriptText(SAY_OOX_AGGRO2, m_creature); break;
        }
    }

    void JustSummoned(Creature* summoned) override
    {
        summoned->AI()->AttackStart(m_creature);
    }
};

CreatureAI* GetAI_npc_oox22fe(Creature* pCreature)
{
    return new npc_oox22feAI(pCreature);
}

bool QuestAccept_npc_oox22fe(Player* pPlayer, Creature* pCreature, const Quest* pQuest)
{
    if (pQuest->GetQuestId() == QUEST_RESCUE_OOX22FE)
    {
        DoScriptText(SAY_OOX_START, pCreature);
        // change that the npc is not lying dead on the ground
        pCreature->SetStandState(UNIT_STAND_STATE_STAND);

        if (pPlayer->GetTeam() == ALLIANCE)
            pCreature->SetFactionTemporary(FACTION_ESCORT_A_PASSIVE, TEMPFACTION_RESTORE_RESPAWN);

        if (pPlayer->GetTeam() == HORDE)
            pCreature->SetFactionTemporary(FACTION_ESCORT_H_PASSIVE, TEMPFACTION_RESTORE_RESPAWN);

        if (npc_oox22feAI* pEscortAI = dynamic_cast<npc_oox22feAI*>(pCreature->AI()))
            pEscortAI->Start(false, pPlayer, pQuest);
    }
    return true;
}

/*######
## AddSC
######*/

void AddSC_feralas()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "npc_oox22fe";
    pNewScript->GetAI = &GetAI_npc_oox22fe;
    pNewScript->pQuestAcceptNPC = &QuestAccept_npc_oox22fe;
    pNewScript->RegisterSelf();
}
