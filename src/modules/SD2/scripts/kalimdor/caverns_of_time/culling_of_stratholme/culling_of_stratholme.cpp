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
SDName: culling_of_stratholme
SD%Complete: 20%
SDComment: Only intro events are supported.
SDCategory: Culling of Stratholme
EndScriptData */

#include "precompiled.h"
#include "culling_of_stratholme.h"
#include "escort_ai.h"

/* *************
** npc_chromie (gossip, quest-accept)
************* */

enum
{
    QUEST_DISPELLING_ILLUSIONS  = 13149,
    QUEST_A_ROYAL_ESCORT        = 13151,

    ITEM_ARCANE_DISRUPTOR       = 37888,

    GOSSIP_ITEM_ENTRANCE_1      = -3595000,
    GOSSIP_ITEM_ENTRANCE_2      = -3595001,
    GOSSIP_ITEM_ENTRANCE_3      = -3595002,

    TEXT_ID_ENTRANCE_1          = 12992,
    TEXT_ID_ENTRANCE_2          = 12993,
    TEXT_ID_ENTRANCE_3          = 12994,
    TEXT_ID_ENTRANCE_4          = 12995,

    GOSSIP_ITEM_INN_1           = -3595003,
    GOSSIP_ITEM_INN_2           = -3595004,
    GOSSIP_ITEM_INN_3           = -3595005,
    GOSSIP_ITEM_INN_SKIP        = -3595006,                 // used to skip the intro; requires research
    GOSSIP_ITEM_INN_TELEPORT    = -3595007,                 // teleport to stratholme - used after the main event has started

    TEXT_ID_INN_1               = 12939,
    TEXT_ID_INN_2               = 12949,
    TEXT_ID_INN_3               = 12950,
    TEXT_ID_INN_4               = 12952,
    TEXT_ID_INN_TELEPORT        = 13470,
    
    SPELL_TELEPORT_COT_P4       = 53435,                    // triggers 53436
};

bool GossipHello_npc_chromie(Player* pPlayer, Creature* pCreature)
{
    if (pCreature->IsQuestGiver())
        pPlayer->PrepareQuestMenu(pCreature->GetObjectGuid());

    if (instance_culling_of_stratholme* m_pInstance = (instance_culling_of_stratholme*)pCreature->GetInstanceData())
    {
        switch (pCreature->GetEntry())
        {
            case NPC_CHROMIE_INN:
                if (m_pInstance->GetData(TYPE_GRAIN_EVENT) != DONE)
                {
                    if (pPlayer->GetQuestRewardStatus(QUEST_DISPELLING_ILLUSIONS) && !pPlayer->HasItemCount(ITEM_ARCANE_DISRUPTOR, 1))
                        pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_INN_1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
                }
                // intro skip option is available since 3.3.x
                pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_INN_SKIP, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
                pPlayer->SEND_GOSSIP_MENU(TEXT_ID_INN_1, pCreature->GetObjectGuid());
                break;
            case NPC_CHROMIE_ENTRANCE:
                if (m_pInstance->GetData(TYPE_GRAIN_EVENT) == DONE && m_pInstance->GetData(TYPE_ARTHAS_INTRO_EVENT) == NOT_STARTED && pPlayer->GetQuestRewardStatus(QUEST_A_ROYAL_ESCORT))
                    pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_ENTRANCE_1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
                pPlayer->SEND_GOSSIP_MENU(TEXT_ID_ENTRANCE_1, pCreature->GetObjectGuid());
                break;
        }
    }
    return true;
}

bool GossipSelect_npc_chromie(Player* pPlayer, Creature* pCreature, uint32 /*sender*/, uint32 uiAction)
{
    switch (pCreature->GetEntry())
    {
        case NPC_CHROMIE_INN:
            switch (uiAction)
            {
                case GOSSIP_ACTION_INFO_DEF+1:
                    pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_INN_2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
                    pPlayer->SEND_GOSSIP_MENU(TEXT_ID_INN_2, pCreature->GetObjectGuid());
                    break;
                case GOSSIP_ACTION_INFO_DEF+2:
                    pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_INN_3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
                    pPlayer->SEND_GOSSIP_MENU(TEXT_ID_INN_3, pCreature->GetObjectGuid());
                    break;
                case GOSSIP_ACTION_INFO_DEF+3:
                    pPlayer->SEND_GOSSIP_MENU(TEXT_ID_INN_4, pCreature->GetObjectGuid());
                    if (!pPlayer->HasItemCount(ITEM_ARCANE_DISRUPTOR, 1))
                    {
                        if (Item* pItem = pPlayer->StoreNewItemInInventorySlot(ITEM_ARCANE_DISRUPTOR, 1))
                        {
                            pPlayer->SendNewItem(pItem, 1, true, false);
                            if (instance_culling_of_stratholme* pInstance = (instance_culling_of_stratholme*)pCreature->GetInstanceData())
                            {
                                if (pInstance->GetData(TYPE_GRAIN_EVENT) == NOT_STARTED)
                                    pInstance->SetData(TYPE_GRAIN_EVENT, SPECIAL);
                            }
                        }
                    }
                    break;
                case GOSSIP_ACTION_INFO_DEF+4:
                    pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_INN_TELEPORT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);
                    pPlayer->SEND_GOSSIP_MENU(TEXT_ID_INN_TELEPORT, pCreature->GetObjectGuid());
                    break;
                case GOSSIP_ACTION_INFO_DEF+5:
                    pCreature->CastSpell(pPlayer, SPELL_TELEPORT_COT_P4, true);
                    if (instance_culling_of_stratholme* pInstance = (instance_culling_of_stratholme*)pCreature->GetInstanceData())
                    {
                        // only skip intro if not already started;
                        if (pInstance->GetData(TYPE_ARTHAS_INTRO_EVENT) == NOT_STARTED && pInstance->GetData(TYPE_GRAIN_EVENT) == NOT_STARTED)
                        {
                            pInstance->SetData(TYPE_ARTHAS_INTRO_EVENT, DONE);
                            pInstance->SetData(TYPE_GRAIN_EVENT, DONE);
                            
                            // spawn Arthas and Chromie
                            pInstance->DoSpawnChromieIfNeeded(pPlayer);
                            pInstance->DoSpawnArthasIfNeeded(pPlayer);
                        }
                    }
                    pPlayer->CLOSE_GOSSIP_MENU();
                    break;
            }
            break;
        case NPC_CHROMIE_ENTRANCE:
            switch (uiAction)
            {
                case GOSSIP_ACTION_INFO_DEF+1:
                    pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_ENTRANCE_2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
                    pPlayer->SEND_GOSSIP_MENU(TEXT_ID_ENTRANCE_2, pCreature->GetObjectGuid());
                    break;
                case GOSSIP_ACTION_INFO_DEF+2:
                    pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_ENTRANCE_3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
                    pPlayer->SEND_GOSSIP_MENU(TEXT_ID_ENTRANCE_3, pCreature->GetObjectGuid());
                    break;
                case GOSSIP_ACTION_INFO_DEF+3:
                    pPlayer->SEND_GOSSIP_MENU(TEXT_ID_ENTRANCE_4, pCreature->GetObjectGuid());
                    if (instance_culling_of_stratholme* pInstance = (instance_culling_of_stratholme*)pCreature->GetInstanceData())
                    {
                        if (pInstance->GetData(TYPE_ARTHAS_INTRO_EVENT) == NOT_STARTED)
                        {
                            pInstance->SetData(TYPE_ARTHAS_INTRO_EVENT, IN_PROGRESS);
                            pInstance->DoSpawnArthasIfNeeded(pPlayer);
                        }
                    }
                    break;
            }
            break;
    }
    return true;
}

bool QuestAccept_npc_chromie(Player* pPlayer, Creature* pCreature, const Quest* pQuest)
{
    switch (pQuest->GetQuestId())
    {
        case QUEST_DISPELLING_ILLUSIONS:
            if (instance_culling_of_stratholme* pInstance = (instance_culling_of_stratholme*)pCreature->GetInstanceData())
            {
                if (pInstance->GetData(TYPE_GRAIN_EVENT) == NOT_STARTED)
                    pInstance->SetData(TYPE_GRAIN_EVENT, SPECIAL);
            }
            break;
        case QUEST_A_ROYAL_ESCORT:
            if (instance_culling_of_stratholme* pInstance = (instance_culling_of_stratholme*)pCreature->GetInstanceData())
            {
                if (pInstance->GetData(TYPE_ARTHAS_INTRO_EVENT) == NOT_STARTED)
                {
                    pInstance->SetData(TYPE_ARTHAS_INTRO_EVENT, IN_PROGRESS);
                    pInstance->DoSpawnArthasIfNeeded(pPlayer);
                }
            }
            break;
    }
    return true;
}

/* *************
** npc_crates_bunny (spell aura effect dummy)
************* */

enum
{
    SAY_SOLDIERS_REPORT         = -1595000,

    SPELL_ARCANE_DISRUPTION     = 49590,
    SPELL_CRATES_KILL_CREDIT    = 58109,
};

bool EffectAuraDummy_spell_aura_dummy_npc_crates_dummy(const Aura* pAura, bool bApply)
{
    if (pAura->GetId() == SPELL_ARCANE_DISRUPTION && pAura->GetEffIndex() == EFFECT_INDEX_0 && bApply)
    {
        if (Creature* pTarget = (Creature*)pAura->GetTarget())
        {
            if (pTarget->GetEntry() != NPC_GRAIN_CRATE_HELPER)
                return true;

            std::list<Creature*> lCrateBunnyList;
            if (instance_culling_of_stratholme* pInstance = (instance_culling_of_stratholme*)pTarget->GetInstanceData())
            {
                pInstance->GetCratesBunnyOrderedList(lCrateBunnyList);
                uint8 i = 0;
                for (std::list<Creature*>::const_iterator itr = lCrateBunnyList.begin(); itr != lCrateBunnyList.end(); ++itr)
                {
                    ++i;
                    if (*itr == pTarget)
                    {
                        // check if the event can proceed
                        if (!pInstance->CanGrainEventProgress(pTarget))
                            return true;

                        break;
                    }
                }

                switch (i)
                {
                    case 1:
                        // Start NPC_ROGER_OWENS Event
                        if (Creature* pRoger = pInstance->GetSingleCreatureFromStorage(NPC_ROGER_OWENS))
                        {
                            pRoger->SetStandState(UNIT_STAND_STATE_STAND);
                            pRoger->GetMotionMaster()->MoveWaypoint();
                        }
                        break;
                    case 2:
                        // Start NPC_SERGEANT_MORIGAN  Event
                        if (Creature* pMorigan = pInstance->GetSingleCreatureFromStorage(NPC_SERGEANT_MORIGAN))
                            pMorigan->GetMotionMaster()->MoveWaypoint();
                        break;
                    case 3:
                        // Start NPC_JENA_ANDERSON Event
                        if (Creature* pJena = pInstance->GetSingleCreatureFromStorage(NPC_JENA_ANDERSON))
                            pJena->GetMotionMaster()->MoveWaypoint();
                        break;
                    case 4:
                        // Start NPC_MALCOM_MOORE Event
                        pTarget->SummonCreature(NPC_MALCOM_MOORE, 1605.452f, 804.9279f, 122.961f, 5.19f, TEMPSUMMON_DEAD_DESPAWN, 0);
                        break;
                    case 5:
                        // Start NPC_BARTLEBY_BATTSON Event
                        if (Creature* pBartleby = pInstance->GetSingleCreatureFromStorage(NPC_BARTLEBY_BATTSON))
                            pBartleby->GetMotionMaster()->MoveWaypoint();
                        break;
                }

                // Finished event, give killcredit
                if (pInstance->GetData(TYPE_GRAIN_EVENT) == DONE)
                {
                    pTarget->CastSpell(pTarget, SPELL_CRATES_KILL_CREDIT, true);
                    pInstance->DoOrSimulateScriptTextForThisInstance(SAY_SOLDIERS_REPORT, NPC_LORDAERON_CRIER);
                }

                // despawn the GO visuals and spanw the plague crate
                if (GameObject* pCrate = GetClosestGameObjectWithEntry(pTarget, GO_SUSPICIOUS_GRAIN_CRATE, 5.0f))
                    pCrate->SetLootState(GO_JUST_DEACTIVATED);
                if (GameObject* pHighlight = GetClosestGameObjectWithEntry(pTarget, GO_CRATE_HIGHLIGHT, 5.0f))
                    pHighlight->SetLootState(GO_JUST_DEACTIVATED);
                if (GameObject* pCrate = GetClosestGameObjectWithEntry(pTarget, GO_PLAGUE_GRAIN_CRATE, 5.0f))
                {
                    pCrate->SetRespawnTime(6 * HOUR * IN_MILLISECONDS);
                    pCrate->Refresh();
                }
            }
        }
    }
    return true;
}

/* *************
** npc_arthas
************* */

enum
{
    GOSSIP_ITEM_CITY_GATES      = -3595008,

    TEXT_ID_CITY_GATES          = 13076,

    SPELL_HOLY_LIGHT            = 52444,
    SPELL_EXORCISM              = 52445,
    SPELL_EXORCISM_H            = 58822,
    SPELL_DEVOTION_AURA         = 52442,
};

struct MANGOS_DLL_DECL npc_arthasAI : public npc_escortAI
{
    npc_arthasAI(Creature* pCreature) : npc_escortAI(pCreature)
    {
        m_pInstance = (instance_culling_of_stratholme*)pCreature->GetInstanceData();
        Reset();
    }

    instance_culling_of_stratholme* m_pInstance;

    void Reset() override { }

    void MovementInform(uint32 uiType, uint32 uiPointId) override
    {
        if (uiType == WAYPOINT_MOTION_TYPE)
        {
            // set the intro event as done and start the undead waves
            if (uiPointId == 31 && m_pInstance)
                m_pInstance->SetData(TYPE_ARTHAS_INTRO_EVENT, DONE);
        }
        else
            npc_escortAI::MovementInform(uiType, uiPointId);
    }

    void WaypointReached(uint32 uiPointId) override
    {
        // ToDo:
    }

    void UpdateEscortAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        // ToDo: add spells

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_arthas(Creature* pCreature)
{
    return new npc_arthasAI(pCreature);
}

bool GossipHello_npc_arthas(Player* pPlayer, Creature* pCreature)
{
    if (instance_culling_of_stratholme* pInstance = (instance_culling_of_stratholme*)pCreature->GetInstanceData())
    {
        if (pInstance->GetData(TYPE_ARTHAS_INTRO_EVENT) == IN_PROGRESS)
        {
            pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_CITY_GATES, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->SEND_GOSSIP_MENU(TEXT_ID_CITY_GATES, pCreature->GetObjectGuid());
        }
    }
    return true;
}

bool GossipSelect_npc_arthas(Player* pPlayer, Creature* pCreature, uint32 /*sender*/, uint32 uiAction)
{
    switch (uiAction)
    {
    case GOSSIP_ACTION_INFO_DEF+1:
        // resume WP movement - rest is handled by DB
        pCreature->clearUnitState(UNIT_STAT_WAYPOINT_PAUSED);
        pCreature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        pPlayer->CLOSE_GOSSIP_MENU();
        break;
    }
    return true;
}

void AddSC_culling_of_stratholme()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "npc_chromie";
    pNewScript->pGossipHello = &GossipHello_npc_chromie;
    pNewScript->pGossipSelect = &GossipSelect_npc_chromie;
    pNewScript->pQuestAcceptNPC = &QuestAccept_npc_chromie;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "spell_dummy_npc_crates_bunny";
    pNewScript->pEffectAuraDummy = &EffectAuraDummy_spell_aura_dummy_npc_crates_dummy;
    pNewScript->RegisterSelf();
    
    pNewScript = new Script;
    pNewScript->Name = "npc_arthas";
    pNewScript->GetAI = &GetAI_npc_arthas;
    pNewScript->pGossipHello = &GossipHello_npc_arthas;
    pNewScript->pGossipSelect = &GossipSelect_npc_arthas;
    pNewScript->RegisterSelf();
}
