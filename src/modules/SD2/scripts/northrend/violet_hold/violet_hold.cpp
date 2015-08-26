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
SDName: Violet_Hold
SD%Complete: 80
SDComment: Intro event required more research and core support.
SDCategory: Violet Hold
EndScriptData */

/* ContentData
go_activation_crystal
npc_door_seal
npc_sinclari
npc_prison_event_controller
npc_teleportation_portal
EndContentData */

#include "precompiled.h"
#include "violet_hold.h"
#include "escort_ai.h"

/*######
## go_activation_crystal
######*/

struct go_activation_crystal : public GameObjectScript
{
    go_activation_crystal() : GameObjectScript("go_activation_crystal") {}

    bool OnUse(Player* pPlayer, GameObject* pGo) override
    {
        if (InstanceData* pInstance = pGo->GetInstanceData())
            pInstance->SetData64(DATA64_CRYSTAL_ACTIVATOR, pPlayer->GetObjectGuid().GetRawValue());

        return false;
    }
};

/*######
## npc_door_seal
######*/

struct spell_destroy_door_seal : public SpellScript
{
    spell_destroy_door_seal() : SpellScript("spell_destroy_door_seal") {}

    bool EffectDummy(Unit* /*pCaster*/, uint32 uiSpellId, SpellEffectIndex uiEffIndex, Object* pTarget, ObjectGuid /*originalCasterGuid*/) override
    {
        // always check spellid and effectindex
        if (uiSpellId == SPELL_DESTROY_DOOR_SEAL && uiEffIndex == EFFECT_INDEX_0)
        {
            if (InstanceData* pInstance = pTarget->ToCreature()->GetInstanceData())
                pInstance->SetData(TYPE_SEAL, SPECIAL);

            // always return true when we are handling this spell and effect
            return true;
        }
        return false;
    }
};

/*######
## npc_sinclari
######*/

enum
{
    SAY_BEGIN                   = -1608000,
    SAY_LOCK_DOOR               = -1608001,
    SAY_VICTORY                 = -1608027,

    GOSSIP_ITEM_INTRO           = -3608000,
    GOSSIP_ITEM_START           = -3608001,
    GOSSIP_ITEM_TELEPORT        = -3608002,

    GOSSIP_TEXT_ID_INTRO        = 13853,
    GOSSIP_TEXT_ID_START        = 13854,

    SPELL_TELEPORT_INSIDE       = 62138,            // script effect - should trigger 62139
};

struct npc_sinclari : public CreatureScript
{
    npc_sinclari() : CreatureScript("npc_sinclari") {}

    struct npc_sinclariAI : public npc_escortAI
    {
        npc_sinclariAI(Creature* pCreature) : npc_escortAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        }

        ScriptedInstance* m_pInstance;

        bool m_bIsEpilogue;

        void Reset() override
        {
            m_bIsEpilogue = false;
        }

        void WaypointReached(uint32 uiPointId) override
        {
            if (!m_pInstance)
                return;

            switch (uiPointId)
            {
            case 0:
                m_pInstance->SetData64(DATA64_CRYSTAL_ACTIVATOR_INT, m_creature->GetObjectGuid().GetRawValue());
                break;
            case 1:
                DoScriptText(SAY_BEGIN, m_creature);
                m_pInstance->SetData(TYPE_DO_SINCLARI_BEGIN, 0);
                break;
            case 2:
                DoScriptText(SAY_LOCK_DOOR, m_creature);
                m_creature->SetFacingTo(0.05f);
                break;
            case 3:
                m_pInstance->SetData(TYPE_MAIN, IN_PROGRESS);
                break;
            case 4:
                m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                SetEscortPaused(true);
                break;
            case 5:
                m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                DoScriptText(SAY_VICTORY, m_creature);
                SetEscortPaused(true);
                break;
            }
        }

        void JustRespawned() override
        {
            if (m_pInstance && m_pInstance->GetData(TYPE_MAIN) != DONE)
                m_pInstance->SetData(TYPE_MAIN, NOT_STARTED);

            npc_escortAI::JustRespawned();                      // Needed, to reset escort state, waypoints, etc
        }

        void UpdateEscortAI(const uint32 /*uiDiff*/) override
        {
            // Say outro after event is finished
            if (m_pInstance && m_pInstance->GetData(TYPE_MAIN) == DONE && !m_bIsEpilogue)
            {
                SetEscortPaused(false);
                m_bIsEpilogue = true;
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_sinclariAI(pCreature);
    }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override
    {
        if (InstanceData* pInstance = pCreature->GetInstanceData())
        {
            if (pInstance->GetData(TYPE_MAIN) != IN_PROGRESS)
                pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_INTRO, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            else
                pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_TELEPORT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        }

        pPlayer->SEND_GOSSIP_MENU(GOSSIP_TEXT_ID_INTRO, pCreature->GetObjectGuid());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction) override
    {
        if (uiAction == GOSSIP_ACTION_INFO_DEF + 1)
        {
            if (InstanceData* pInstance = pCreature->GetInstanceData())
            {
                if (pInstance->GetData(TYPE_MAIN) == NOT_STARTED)
                {
                    pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_START, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
                    pPlayer->SEND_GOSSIP_MENU(GOSSIP_TEXT_ID_START, pCreature->GetObjectGuid());
                }
            }
            else
                pPlayer->CLOSE_GOSSIP_MENU();
        }

        if (uiAction == GOSSIP_ACTION_INFO_DEF + 2)
        {
            if (InstanceData* pInstance = pCreature->GetInstanceData())
            {
                pPlayer->CLOSE_GOSSIP_MENU();

                if (pInstance->GetData(TYPE_MAIN) == NOT_STARTED)
                {
                    pInstance->SetData(TYPE_MAIN, SPECIAL);

                    if (npc_sinclariAI* pEscortAI = dynamic_cast<npc_sinclariAI*>(pCreature->AI()))
                        pEscortAI->Start();
                }
            }
            else
                pPlayer->CLOSE_GOSSIP_MENU();
        }

        if (uiAction == GOSSIP_ACTION_INFO_DEF + 3)
        {
            pCreature->CastSpell(pPlayer, SPELL_TELEPORT_INSIDE, true);
            pPlayer->CLOSE_GOSSIP_MENU();
        }

        return true;
    }
};

/*######
## npc_prison_event_controller
######*/

struct npc_prison_event_controller : public CreatureScript
{
    npc_prison_event_controller() : CreatureScript("npc_prison_event_controller") {}

    struct npc_prison_event_controllerAI : public ScriptedAI
    {
        npc_prison_event_controllerAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        }

        ScriptedInstance* m_pInstance;

        GuidSet m_sTrashPackSet;

        uint32 m_uiSaboteurTimer;
        uint8 m_uiSaboteurPhase;
        uint8 m_uiCurrentTrashPortalId;

        ObjectGuid m_currentSaboteurGuid;

        void Reset() override
        {
            m_uiCurrentTrashPortalId = 0;
            m_uiSaboteurPhase = 0;
            m_uiSaboteurTimer = 0;

            m_currentSaboteurGuid.Clear();
            m_sTrashPackSet.clear();
        }

        void ReceiveAIEvent(AIEventType eventType, Creature* sender, Unit* invoker, uint32 data)
        {
            if (eventType == AI_EVENT_CUSTOM_A && sender == invoker)
                DoSetCurrentTrashPortal(data);
        }

        void DoSetCurrentTrashPortal(uint8 uiPortalId) { m_uiCurrentTrashPortalId = uiPortalId; }

        void JustSummoned(Creature* pSummoned) override
        {
            switch (pSummoned->GetEntry())
            {
            case NPC_AZURE_CAPTAIN:
                DoScriptText(EMOTE_DRAGONFLIGHT_PORTAL, pSummoned);
                // no break
            case NPC_AZURE_RAIDER:
            case NPC_AZURE_SORCEROR:
            case NPC_AZURE_STALKER:
                m_sTrashPackSet.insert(pSummoned->GetObjectGuid());
                // no break
            case NPC_AZURE_INVADER:
            case NPC_MAGE_HUNTER:
            case NPC_AZURE_SPELLBREAKER:
            case NPC_AZURE_BINDER:
            case NPC_AZURE_MAGE_SLAYER:
                pSummoned->SetWalk(false);
                pSummoned->GetMotionMaster()->MovePoint(1, fSealAttackLoc[0], fSealAttackLoc[1], fSealAttackLoc[2]);
                break;
            case NPC_AZURE_SABOTEUR:
                if (!m_pInstance)
                    return;
                m_currentSaboteurGuid = pSummoned->GetObjectGuid();
                m_pInstance->SetData64(DATA64_SABOTEUR, pSummoned->GetObjectGuid().GetRawValue());
                break;
            }
        }

        void SummonedMovementInform(Creature* pSummoned, uint32 uiMotionType, uint32 uiPointId) override
        {
            if (uiMotionType != POINT_MOTION_TYPE && !uiPointId)
                return;

            if (pSummoned->GetEntry() == NPC_AZURE_SABOTEUR)
            {
                // Prepare to release the boss
                m_uiSaboteurPhase = 0;
                m_uiSaboteurTimer = 1000;
                pSummoned->CastSpell(pSummoned, SPELL_SHIELD_DISRUPTION, false);
            }
            // For other summons, cast destroy seal when they reach the door
            else
                pSummoned->CastSpell(pSummoned, SPELL_DESTROY_DOOR_SEAL, false);
        }

        void SummonedCreatureJustDied(Creature* pSummoned) override
        {
            switch (pSummoned->GetEntry())
            {
            case NPC_AZURE_CAPTAIN:
            case NPC_AZURE_RAIDER:
            case NPC_AZURE_SORCEROR:
            case NPC_AZURE_STALKER:
                if (m_sTrashPackSet.find(pSummoned->GetObjectGuid()) != m_sTrashPackSet.end())
                    m_sTrashPackSet.erase(pSummoned->GetObjectGuid());

                if (m_sTrashPackSet.empty())
                {
                    // no need if a new portal was made while this was in progress
                    if (m_uiCurrentTrashPortalId == m_pInstance->GetData(TYPE_DATA_PORTAL_NUMBER))
                        m_pInstance->SetData(TYPE_PORTAL, DONE);
                }
                break;
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_uiSaboteurTimer)
            {
                if (m_uiSaboteurTimer <= uiDiff)
                {
                    Creature* pSaboteur = m_creature->GetMap()->GetCreature(m_currentSaboteurGuid);
                    if (!pSaboteur)
                        return;

                    switch (m_uiSaboteurPhase)
                    {
                    case 0:
                        pSaboteur->CastSpell(pSaboteur, SPELL_SHIELD_DISRUPTION, false);
                        m_uiSaboteurTimer = 1000;
                        break;
                    case 1:
                        pSaboteur->CastSpell(pSaboteur, SPELL_SHIELD_DISRUPTION, false);
                        m_uiSaboteurTimer = 1000;
                        break;
                    case 2:
                        if (m_pInstance)
                            m_pInstance->SetData(TYPE_DO_RELEASE_BOSS, 1);
                        pSaboteur->CastSpell(pSaboteur, SPELL_SIMPLE_TELEPORT, false);
                        pSaboteur->ForcedDespawn(1000);
                        m_uiSaboteurTimer = 0;
                        break;
                    }
                    ++m_uiSaboteurPhase;
                }
                else
                    m_uiSaboteurTimer -= uiDiff;
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_prison_event_controllerAI(pCreature);
    }
};

/*######
## npc_teleportation_portal
######*/

static const uint32 aTrashPortalNpcs[4] = {NPC_AZURE_CAPTAIN, NPC_AZURE_RAIDER, NPC_AZURE_SORCEROR, NPC_AZURE_STALKER};

struct npc_teleportation_portal : public CreatureScript
{
    npc_teleportation_portal() : CreatureScript("npc_teleportation_portal") {}

    struct npc_teleportation_portalAI : public ScriptedAI
    {
        npc_teleportation_portalAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
            m_uiMyPortalNumber = 0;
        }

        ScriptedInstance* m_pInstance;

        bool m_bIntro;
        uint32 m_uiMyPortalNumber;
        uint32 m_uiCyanigosaMoveTimer;

        ObjectGuid m_cyanigosaGuid;

        void Reset() override
        {
            DoCastSpellIfCan(m_creature, SPELL_PORTAL_PERIODIC);

            m_bIntro = true;
            m_uiCyanigosaMoveTimer = 0;

            if (m_pInstance)
                m_uiMyPortalNumber = m_pInstance->GetData(TYPE_DATA_PORTAL_NUMBER);
        }

        void ReceiveAIEvent(AIEventType eventType, Creature* /*sender*/, Unit* /*invoker*/, uint32 /**/)
        {
            if (!m_pInstance || eventType != AI_EVENT_CUSTOM_A)
                return;

            // Portal event used for intro
            if (m_creature->GetEntry() == NPC_PORTAL_INTRO)
            {
                // ToDo: uncomment this when the information and DB data is confirmed. Right now the mobs may overrun the guards after a few min of fightning
                // m_creature->SummonCreature(m_pInstance->GetRandomMobForIntroPortal(), 0, 0, 0, 0, TEMPSUMMON_DEAD_DESPAWN, 0);
                return;
            }

            // First summon tick
            if (m_bIntro)
            {
                if (m_creature->GetEntry() == NPC_PORTAL)
                {
                    // Summon a guardian keeper or Cyanigosa
                    if (m_uiMyPortalNumber == 18)
                        m_creature->SummonCreature(NPC_CYANIGOSA, 0.0f, 0.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 600 * IN_MILLISECONDS);
                    else
                        m_creature->SummonCreature(m_pInstance->GetData(TYPE_DATA_PORTAL_ELITE), 0.0f, 0.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 600 * IN_MILLISECONDS);
                }
                else if (m_creature->GetEntry() == NPC_PORTAL_ELITE)
                {
                    // Allow the event controller to summon the mobs, for better movement handling
                    Creature* pController = m_pInstance->GetSingleCreatureFromStorage(NPC_EVENT_CONTROLLER);
                    if (!pController)
                        return;

                    // Summon a squad or a saboteur
                    if (bool(m_pInstance->GetData(TYPE_DATA_IS_TRASH_PORTAL)))
                    {
                        float fX, fY, fZ;
                        for (uint8 i = 0; i < 4; ++i)
                        {
                            uint32 uiSummonId = aTrashPortalNpcs[i];

                            // Summon the trash pack around the portal
                            m_creature->GetNearPoint(m_creature, fX, fY, fZ, 0, 3.0f, M_PI_F / 2 * i);
                            pController->SummonCreature(uiSummonId, fX, fY, fZ, m_creature->GetOrientation(), TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 600 * IN_MILLISECONDS);
                        }

                        // If this is a trash portal, set the current number in the
                        if (CreatureAI* pControllerAI = pController->AI())
                            pControllerAI->ReceiveAIEvent(AI_EVENT_CUSTOM_A, m_creature, m_creature, m_uiMyPortalNumber);
                    }
                    else
                        pController->SummonCreature(NPC_AZURE_SABOTEUR, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), m_creature->GetOrientation(), TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 600 * IN_MILLISECONDS);

                    m_creature->ForcedDespawn(5000);
                }

                // Set special data for all the portals, except the last one
                if (m_pInstance && m_uiMyPortalNumber != 18)
                    m_pInstance->SetData(TYPE_PORTAL, SPECIAL);

                m_bIntro = false;
            }
            else
            {
                // Allow the normal mobs to be summoned by the event controller
                if (Creature* pController = m_pInstance->GetSingleCreatureFromStorage(NPC_EVENT_CONTROLLER))
                    pController->SummonCreature(m_pInstance->GetData(TYPE_DATA_GET_MOB_NORMAL), m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), m_creature->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN, 0);
            }
        }

        void JustSummoned(Creature* pSummoned) override
        {
            switch (pSummoned->GetEntry())
            {
            case NPC_CYANIGOSA:
                m_cyanigosaGuid = pSummoned->GetObjectGuid();
                m_uiCyanigosaMoveTimer = 5000;
                m_creature->ForcedDespawn(5000);
                break;
            case NPC_PORTAL_GUARDIAN:
                DoScriptText(EMOTE_GUARDIAN_PORTAL, pSummoned);
                DoCastSpellIfCan(pSummoned, SPELL_PORTAL_CHANNEL);
                break;
            case NPC_PORTAL_KEEPER:
                DoScriptText(EMOTE_KEEPER_PORTAL, pSummoned);
                DoCastSpellIfCan(pSummoned, SPELL_PORTAL_CHANNEL);
                break;
            case NPC_AZURE_BINDER_INTRO:
            case NPC_AZURE_INVADER_INTRO:
            case NPC_AZURE_SPELLBREAKER_INTRO:
            case NPC_AZURE_MAGE_SLAYER_INTRO:
                // Move them to the entrance. They will attack the guards automatically
                pSummoned->SetWalk(false);
                pSummoned->GetMotionMaster()->MovePoint(1, fSealAttackLoc[0], fSealAttackLoc[1], fSealAttackLoc[2]);
                break;
            }
        }

        void SummonedCreatureJustDied(Creature* pSummoned) override
        {
            switch (pSummoned->GetEntry())
            {
            case NPC_PORTAL_GUARDIAN:
            case NPC_PORTAL_KEEPER:
                m_creature->ForcedDespawn(3000);
                // no need if a new portal was made while this was in progress
                if (m_pInstance && m_uiMyPortalNumber == m_pInstance->GetData(TYPE_DATA_PORTAL_NUMBER))
                    m_pInstance->SetData(TYPE_PORTAL, DONE);
                break;
            }
        }

        void SummonedCreatureDespawn(Creature* pSummoned) override
        {
            switch (pSummoned->GetEntry())
            {
            case NPC_PORTAL_GUARDIAN:
            case NPC_PORTAL_KEEPER:
                // Despawn in case of event reset
                m_creature->ForcedDespawn();
                break;
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_uiCyanigosaMoveTimer)
            {
                if (m_uiCyanigosaMoveTimer <= uiDiff)
                {
                    if (Creature* pCyanigosa = m_creature->GetMap()->GetCreature(m_cyanigosaGuid))
                        pCyanigosa->GetMotionMaster()->MoveJump(afPortalLocation[8].fX, afPortalLocation[8].fY, afPortalLocation[8].fZ, pCyanigosa->GetSpeed(MOVE_RUN) * 2, 10.0f);

                    m_uiCyanigosaMoveTimer = 0;
                }
                else
                    m_uiCyanigosaMoveTimer -= uiDiff;
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_teleportation_portalAI(pCreature);
    }
};

struct spell_vh_portal_periodic : public SpellScript
{
    spell_vh_portal_periodic() : SpellScript("spell_vh_portal_periodic") {}

    bool EffectDummy(Unit* /*pCaster*/, uint32 uiSpellId, SpellEffectIndex uiEffIndex, Object* pTarget, ObjectGuid /*originalCasterGuid*/) override
    {
        // always check spellid and effectindex
        if (uiSpellId == SPELL_PORTAL_PERIODIC && uiEffIndex == EFFECT_INDEX_0)
        {
            if (CreatureAI* pPortalAI = pTarget->ToCreature()->AI())
                pPortalAI->ReceiveAIEvent(AI_EVENT_CUSTOM_A, (Creature*)NULL, (Unit*)NULL, 0);

            // always return true when we are handling this spell and effect
            return true;
        }

        return false;
    }
};

void AddSC_violet_hold()
{
    Script* s;

    s = new go_activation_crystal();
    s->RegisterSelf();
    s = new npc_sinclari();
    s->RegisterSelf();
    s = new npc_prison_event_controller();
    s->RegisterSelf();
    s = new npc_teleportation_portal();
    s->RegisterSelf();

    s = new spell_destroy_door_seal();
    s->RegisterSelf();
    s = new spell_vh_portal_periodic();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "go_activation_crystal";
    //pNewScript->pGOUse = &GOUse_go_activation_crystal;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_door_seal";
    //pNewScript->pEffectDummyNPC = &EffectDummyCreature_npc_door_seal;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_sinclari";
    //pNewScript->GetAI = &GetAI_npc_sinclari;
    //pNewScript->pGossipHello = &GossipHello_npc_sinclari;
    //pNewScript->pGossipSelect = &GossipSelect_npc_sinclari;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_prison_event_controller";
    //pNewScript->GetAI = &GetAI_npc_prison_event_controller;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_teleportation_portal";
    //pNewScript->GetAI = &GetAI_npc_teleportation_portal;
    //pNewScript->pEffectDummyNPC = &EffectDummyCreature_npc_teleportation_portal;
    //pNewScript->RegisterSelf();
}
