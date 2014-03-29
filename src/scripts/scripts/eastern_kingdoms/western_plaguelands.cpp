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
SDName: Western_Plaguelands
SD%Complete: 90
SDComment: Quest support: 5216, 5219, 5222, 5225, 5229, 5231, 5233, 5235, 9446.
SDCategory: Western Plaguelands
EndScriptData */

/* ContentData
npc_the_scourge_cauldron
npc_anchorite_truuen
EndContentData */

#include "precompiled.h"
#include "escort_ai.h"

/*######
## npc_the_scourge_cauldron
######*/

struct MANGOS_DLL_DECL npc_the_scourge_cauldronAI : public ScriptedAI
{
    npc_the_scourge_cauldronAI(Creature* pCreature) : ScriptedAI(pCreature) {Reset();}

    void Reset() override {}

    void DoDie()
    {
        // summoner dies here
        m_creature->DealDamage(m_creature, m_creature->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
        // override any database `spawntimesecs` to prevent duplicated summons
        uint32 rTime = m_creature->GetRespawnDelay();
        if (rTime < 600)
            m_creature->SetRespawnDelay(600);
    }

    void MoveInLineOfSight(Unit* who) override
    {
        if (!who || who->GetTypeId() != TYPEID_PLAYER)
            return;

        if (who->GetTypeId() == TYPEID_PLAYER)
        {
            switch (m_creature->GetAreaId())
            {
                case 199:                                   // felstone
                    if (((Player*)who)->GetQuestStatus(5216) == QUEST_STATUS_INCOMPLETE ||
                            ((Player*)who)->GetQuestStatus(5229) == QUEST_STATUS_INCOMPLETE)
                    {
                        m_creature->SummonCreature(11075, 0.0f, 0.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 600000);
                        DoDie();
                    }
                    break;
                case 200:                                   // dalson
                    if (((Player*)who)->GetQuestStatus(5219) == QUEST_STATUS_INCOMPLETE ||
                            ((Player*)who)->GetQuestStatus(5231) == QUEST_STATUS_INCOMPLETE)
                    {
                        m_creature->SummonCreature(11077, 0.0f, 0.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 600000);
                        DoDie();
                    }
                    break;
                case 201:                                   // gahrron
                    if (((Player*)who)->GetQuestStatus(5225) == QUEST_STATUS_INCOMPLETE ||
                            ((Player*)who)->GetQuestStatus(5235) == QUEST_STATUS_INCOMPLETE)
                    {
                        m_creature->SummonCreature(11078, 0.0f, 0.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 600000);
                        DoDie();
                    }
                    break;
                case 202:                                   // writhing
                    if (((Player*)who)->GetQuestStatus(5222) == QUEST_STATUS_INCOMPLETE ||
                            ((Player*)who)->GetQuestStatus(5233) == QUEST_STATUS_INCOMPLETE)
                    {
                        m_creature->SummonCreature(11076, 0.0f, 0.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 600000);
                        DoDie();
                    }
                    break;
            }
        }
    }
};

CreatureAI* GetAI_npc_the_scourge_cauldron(Creature* pCreature)
{
    return new npc_the_scourge_cauldronAI(pCreature);
}

/*######
## npc_anchorite_truuen
######*/

enum
{
    SAY_BEGIN                   = -1000910,
    SAY_FIRST_STOP              = -1000911,
    SAY_CONTINUE                = -1000912,
    SAY_FIRST_ATTACK            = -1000913,
    SAY_PURITY                  = -1000914,
    SAY_SECOND_ATTACK           = -1000915,
    SAY_CLEANSE                 = -1000916,
    SAY_WELCOME                 = -1000917,
    SAY_EPILOGUE_1              = -1000918,
    SAY_EPILOGUE_2              = -1000919,

    NPC_PRIEST_THELDANIS        = 1854,
    NPC_HUNGERING_WRAITH        = 1802,
    NPC_HAUNDING_VISION         = 4472,
    NPC_BLIGHTED_ZOMBIE         = 4475,
    NPC_GHOST_OF_UTHER          = 17233,

    QUEST_ID_TOMB_LIGHTBRINGER  = 9446,
};

struct MANGOS_DLL_DECL npc_anchorite_truuenAI: public npc_escortAI
{
    npc_anchorite_truuenAI(Creature* pCreature): npc_escortAI(pCreature) { Reset(); }

    ObjectGuid m_utherGhostGuid;

    void Reset() override { }

    void ReceiveAIEvent(AIEventType eventType, Creature* /*pSender*/, Unit* pInvoker, uint32 uiMiscValue) override
    {
        if (eventType == AI_EVENT_START_ESCORT && pInvoker->GetTypeId() == TYPEID_PLAYER)
        {
            DoScriptText(SAY_BEGIN, m_creature);
            Start(false, (Player*)pInvoker, GetQuestTemplateStore(uiMiscValue));
        }
    }

    void WaypointReached(uint32 uiPointId) override
    {
        switch (uiPointId)
        {
            case 4:
                DoScriptText(SAY_FIRST_STOP, m_creature);
                break;
            case 5:
                DoScriptText(SAY_CONTINUE, m_creature);
                break;
            case 10:
                DoScriptText(SAY_FIRST_ATTACK, m_creature);
                // spawn first attacker wave
                m_creature->SummonCreature(NPC_HAUNDING_VISION, 1045.26f, -1576.50f, 62.42f, 2.82f, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                m_creature->SummonCreature(NPC_HUNGERING_WRAITH, 1021.74f, -1547.49f, 63.44f, 5.24f, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                break;
            case 11:
                DoScriptText(SAY_PURITY, m_creature);
                break;
            case 21:
                DoScriptText(SAY_SECOND_ATTACK, m_creature);
                // spawn second attacker wave
                m_creature->SummonCreature(NPC_BLIGHTED_ZOMBIE, 1123.08f, -1738.70f, 61.65f, 3.63f, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                m_creature->SummonCreature(NPC_BLIGHTED_ZOMBIE, 1117.07f, -1763.47f, 62.72f, 1.83f, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                m_creature->SummonCreature(NPC_BLIGHTED_ZOMBIE, 1096.79f, -1719.14f, 62.69f, 4.88f, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                m_creature->SummonCreature(NPC_BLIGHTED_ZOMBIE, 1068.92f, -1739.68f, 62.23f, 6.21f, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                break;
            case 22:
                DoScriptText(SAY_CLEANSE, m_creature);
                break;
            case 35:
                if (Creature* pPriest = GetClosestCreatureWithEntry(m_creature, NPC_PRIEST_THELDANIS, 60.0f))
                    DoScriptText(SAY_WELCOME, pPriest);
                break;
            case 38:
                m_creature->SetStandState(UNIT_STAND_STATE_KNEEL);
                m_creature->SummonCreature(NPC_GHOST_OF_UTHER, 972.96f, -1824.82f, 82.54f, 0.27f, TEMPSUMMON_TIMED_DESPAWN, 45000);
                // complete the quest - the event continues with the dialogue
                if (Player* pPlayer = GetPlayerForEscort())
                    pPlayer->GroupEventHappens(QUEST_ID_TOMB_LIGHTBRINGER, m_creature);
                break;
            case 39:
                if (Creature* pUther = m_creature->GetMap()->GetCreature(m_utherGhostGuid))
                {
                    pUther->SetFacingToObject(m_creature);
                    DoScriptText(SAY_EPILOGUE_1, pUther);
                }
                break;
            case 40:
                if (Creature* pUther = m_creature->GetMap()->GetCreature(m_utherGhostGuid))
                    DoScriptText(SAY_EPILOGUE_2, pUther);
                break;
            case 41:
                m_creature->SetStandState(UNIT_STAND_STATE_STAND);
                break;
        }
    }

    void JustSummoned(Creature* pSummoned) override
    {
        if (pSummoned->GetEntry() != NPC_GHOST_OF_UTHER)
            pSummoned->AI()->AttackStart(m_creature);
        else
            m_utherGhostGuid = pSummoned->GetObjectGuid();
    }
};

CreatureAI* GetAI_npc_anchorite_truuen(Creature* pCreature)
{
    return new npc_anchorite_truuenAI(pCreature);
}

bool QuestAccept_npc_anchorite_truuen(Player* pPlayer, Creature* pCreature, const Quest* pQuest)
{
    if (pQuest->GetQuestId() == QUEST_ID_TOMB_LIGHTBRINGER)
        pCreature->AI()->SendAIEvent(AI_EVENT_START_ESCORT, pPlayer, pCreature, pQuest->GetQuestId());

    return true;
}

void AddSC_western_plaguelands()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "npc_the_scourge_cauldron";
    pNewScript->GetAI = &GetAI_npc_the_scourge_cauldron;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_anchorite_truuen";
    pNewScript->GetAI = &GetAI_npc_anchorite_truuen;
    pNewScript->pQuestAcceptNPC = &QuestAccept_npc_anchorite_truuen;
    pNewScript->RegisterSelf();
}
