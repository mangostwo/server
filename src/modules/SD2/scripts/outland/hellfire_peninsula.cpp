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
SDName: Hellfire_Peninsula
SD%Complete: 100
SDComment: Quest support: 9375, 9410, 9418, 10629, 10838, 10935.
SDCategory: Hellfire Peninsula
EndScriptData */

/* ContentData
npc_aeranas
npc_ancestral_wolf
npc_demoniac_scryer
npc_wounded_blood_elf
npc_fel_guard_hound
npc_anchorite_barada
npc_colonel_jules
EndContentData */

#include "precompiled.h"
#include "escort_ai.h"
#include "pet_ai.h"

/*######
## npc_aeranas
######*/

enum
{
    SAY_SUMMON                      = -1000138,
    SAY_FREE                        = -1000139,

    FACTION_HOSTILE                 = 16,
    FACTION_FRIENDLY                = 35,

    SPELL_ENVELOPING_WINDS          = 15535,
    SPELL_SHOCK                     = 12553,
};

struct npc_aeranas : public CreatureScript
{
    npc_aeranas() : CreatureScript("npc_aeranas") {}

    struct npc_aeranasAI : public ScriptedAI
    {
        npc_aeranasAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        uint32 m_uiFactionTimer;
        uint32 m_uiEnvelopingWindsTimer;
        uint32 m_uiShockTimer;

        void Reset() override
        {
            m_uiFactionTimer = 8000;
            m_uiEnvelopingWindsTimer = 9000;
            m_uiShockTimer = 5000;

            m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);

            DoScriptText(SAY_SUMMON, m_creature);
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_uiFactionTimer)
            {
                if (m_uiFactionTimer <= uiDiff)
                {
                    m_creature->SetFactionTemporary(FACTION_HOSTILE, TEMPFACTION_RESTORE_RESPAWN | TEMPFACTION_RESTORE_COMBAT_STOP);
                    m_uiFactionTimer = 0;
                }
                else
                {
                    m_uiFactionTimer -= uiDiff;
                }
            }

            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            if (m_creature->GetHealthPercent() < 30.0f)
            {
                m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                m_creature->RemoveAllAuras();
                m_creature->DeleteThreatList();
                m_creature->CombatStop(true);
                DoScriptText(SAY_FREE, m_creature);
                return;
            }

            if (m_uiShockTimer < uiDiff)
            {
                DoCastSpellIfCan(m_creature->getVictim(), SPELL_SHOCK);
                m_uiShockTimer = 10000;
            }
            else
            {
                m_uiShockTimer -= uiDiff;
            }

            if (m_uiEnvelopingWindsTimer < uiDiff)
            {
                DoCastSpellIfCan(m_creature->getVictim(), SPELL_ENVELOPING_WINDS);
                m_uiEnvelopingWindsTimer = 25000;
            }
            else
            {
                m_uiEnvelopingWindsTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_aeranasAI(pCreature);
    }
};

/*######
## npc_ancestral_wolf
######*/

enum
{
    EMOTE_WOLF_LIFT_HEAD            = -1000496,
    EMOTE_WOLF_HOWL                 = -1000497,
    SAY_WOLF_WELCOME                = -1000498,

    SPELL_ANCESTRAL_WOLF_BUFF       = 29981,

    NPC_RYGA                        = 17123
};

struct npc_ancestral_wolf : public CreatureScript
{
    npc_ancestral_wolf() : CreatureScript("npc_ancestral_wolf") {}

    struct npc_ancestral_wolfAI : public npc_escortAI
    {
        npc_ancestral_wolfAI(Creature* pCreature) : npc_escortAI(pCreature)
        {
            if (pCreature->GetOwner() && pCreature->GetOwner()->GetTypeId() == TYPEID_PLAYER)
            {
                Start(false, (Player*)pCreature->GetOwner());
            }
            else
            {
                script_error_log("npc_ancestral_wolf can not obtain owner or owner is not a player.");
            }
        }

        void Reset() override
        {
            m_creature->CastSpell(m_creature, SPELL_ANCESTRAL_WOLF_BUFF, true);
        }

        void WaypointReached(uint32 uiPointId) override
        {
            switch (uiPointId)
            {
            case 0:
                DoScriptText(EMOTE_WOLF_LIFT_HEAD, m_creature);
                break;
            case 2:
                DoScriptText(EMOTE_WOLF_HOWL, m_creature);
                break;
            case 50:
                Creature* pRyga = GetClosestCreatureWithEntry(m_creature, NPC_RYGA, 30.0f);
                if (pRyga && pRyga->IsAlive() && !pRyga->IsInCombat())
                {
                    DoScriptText(SAY_WOLF_WELCOME, pRyga);
                }
                break;
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_ancestral_wolfAI(pCreature);
    }
};

/*######
## npc_demoniac_scryer
######*/
//TODO prepare localisation
#define GOSSIP_ITEM_ATTUNE          "Yes, Scryer. You may possess me."

enum
{
    GOSSIP_TEXTID_PROTECT           = 10659,
    GOSSIP_TEXTID_ATTUNED           = 10643,

    QUEST_DEMONIAC                  = 10838,
    NPC_HELLFIRE_WARDLING           = 22259,
    NPC_BUTTRESS                    = 22267,                // the 4x nodes
    NPC_SPAWNER                     = 22260,                // just a dummy, not used

    MAX_BUTTRESS                    = 4,
    TIME_TOTAL                      = MINUTE * 10 * IN_MILLISECONDS,

    SPELL_SUMMONED_DEMON            = 7741,                 // visual spawn-in for demon
    SPELL_DEMONIAC_VISITATION       = 38708,                // create item

    SPELL_BUTTRESS_APPERANCE        = 38719,                // visual on 4x bunnies + the flying ones
    SPELL_SUCKER_CHANNEL            = 38721,                // channel to the 4x nodes
    SPELL_SUCKER_DESPAWN_MOB        = 38691
};

// script is basic support, details like end event are not implemented
struct npc_demoniac_scryer : public CreatureScript
{
    npc_demoniac_scryer() : CreatureScript("npc_demoniac_scryer") {}

    struct npc_demoniac_scryerAI : public ScriptedAI
    {
        npc_demoniac_scryerAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_bIsComplete = false;
            m_uiSpawnDemonTimer = 15000;
            m_uiSpawnButtressTimer = 45000;
            m_uiButtressCount = 0;
        }

        bool m_bIsComplete;

        uint32 m_uiSpawnDemonTimer;
        uint32 m_uiSpawnButtressTimer;
        uint32 m_uiButtressCount;

        // we don't want anything to happen when attacked
        void AttackedBy(Unit* /*pEnemy*/) override {}
        void AttackStart(Unit* /*pEnemy*/) override {}

        void DoSpawnButtress()
        {
            ++m_uiButtressCount;

            float fAngle = 0.0f;

            switch (m_uiButtressCount)
            {
            case 1: fAngle = 0.0f; break;
            case 2: fAngle = M_PI_F + M_PI_F / 2; break;
            case 3: fAngle = M_PI_F / 2; break;
            case 4: fAngle = M_PI_F; break;
            }

            float fX, fY, fZ;
            m_creature->GetNearPoint(m_creature, fX, fY, fZ, 0.0f, 5.0f, fAngle);

            uint32 uiTime = TIME_TOTAL - (m_uiSpawnButtressTimer * m_uiButtressCount);
            m_creature->SummonCreature(NPC_BUTTRESS, fX, fY, fZ, m_creature->GetAngle(fX, fY), TEMPSUMMON_TIMED_DESPAWN, uiTime);
        }

        void DoSpawnDemon()
        {
            float fX, fY, fZ;
            m_creature->GetRandomPoint(m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), 20.0f, fX, fY, fZ);

            m_creature->SummonCreature(NPC_HELLFIRE_WARDLING, fX, fY, fZ, 0.0f, TEMPSUMMON_TIMED_OOC_DESPAWN, 5000);
        }

        void JustSummoned(Creature* pSummoned) override
        {
            if (pSummoned->GetEntry() == NPC_HELLFIRE_WARDLING)
            {
                pSummoned->CastSpell(pSummoned, SPELL_SUMMONED_DEMON, false);
                pSummoned->AI()->AttackStart(m_creature);
            }
            else
            {
                if (pSummoned->GetEntry() == NPC_BUTTRESS)
                {
                    pSummoned->CastSpell(pSummoned, SPELL_BUTTRESS_APPERANCE, false);
                    pSummoned->CastSpell(m_creature, SPELL_SUCKER_CHANNEL, true);
                }
            }
        }

        void SpellHitTarget(Unit* pTarget, const SpellEntry* pSpell) override
        {
            if (pTarget->GetEntry() == NPC_HELLFIRE_WARDLING && pSpell->Id == SPELL_SUCKER_DESPAWN_MOB)
            {
                ((Creature*)pTarget)->ForcedDespawn();
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_bIsComplete || !m_creature->IsAlive())
            {
                return;
            }

            if (m_uiSpawnButtressTimer <= uiDiff)
            {
                if (m_uiButtressCount >= MAX_BUTTRESS)
                {
                    m_creature->CastSpell(m_creature, SPELL_SUCKER_DESPAWN_MOB, false);

                    if (m_creature->IsInCombat())
                    {
                        m_creature->DeleteThreatList();
                        m_creature->CombatStop();
                    }

                    m_bIsComplete = true;
                    return;
                }

                m_uiSpawnButtressTimer = 45000;
                DoSpawnButtress();
            }
            else
            {
                m_uiSpawnButtressTimer -= uiDiff;
            }

            if (m_uiSpawnDemonTimer <= uiDiff)
            {
                DoSpawnDemon();
                m_uiSpawnDemonTimer = 15000;
            }
            else
            {
                m_uiSpawnDemonTimer -= uiDiff;
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_demoniac_scryerAI(pCreature);
    }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override
    {
        if (npc_demoniac_scryerAI* pScryerAI = dynamic_cast<npc_demoniac_scryerAI*>(pCreature->AI()))
        {
            if (pScryerAI->m_bIsComplete)
            {
                if (pPlayer->GetQuestStatus(QUEST_DEMONIAC) == QUEST_STATUS_INCOMPLETE)
                {
                    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_ATTUNE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
                }

                pPlayer->SEND_GOSSIP_MENU(GOSSIP_TEXTID_ATTUNED, pCreature->GetObjectGuid());
                return true;
            }
        }

        pPlayer->SEND_GOSSIP_MENU(GOSSIP_TEXTID_PROTECT, pCreature->GetObjectGuid());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction) override
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_INFO_DEF + 1)
        {
            pPlayer->CLOSE_GOSSIP_MENU();
            pCreature->CastSpell(pPlayer, SPELL_DEMONIAC_VISITATION, false);
        }

        return true;
    }
};

/*######
## npc_wounded_blood_elf
######*/

enum
{
    SAY_ELF_START               = -1000117,
    SAY_ELF_SUMMON1             = -1000118,
    SAY_ELF_RESTING             = -1000119,
    SAY_ELF_SUMMON2             = -1000120,
    SAY_ELF_COMPLETE            = -1000121,
    SAY_ELF_AGGRO               = -1000122,

    NPC_WINDWALKER              = 16966,
    NPC_TALONGUARD              = 16967,

    QUEST_ROAD_TO_FALCON_WATCH  = 9375,
};

struct npc_wounded_blood_elf : public CreatureScript
{
    npc_wounded_blood_elf() : CreatureScript("npc_wounded_blood_elf") {}

    struct npc_wounded_blood_elfAI : public npc_escortAI
    {
        npc_wounded_blood_elfAI(Creature* pCreature) : npc_escortAI(pCreature) { Reset(); }

        void WaypointReached(uint32 uiPointId) override
        {
            Player* pPlayer = GetPlayerForEscort();

            if (!pPlayer)
            {
                return;
            }

            switch (uiPointId)
            {
            case 0:
                DoScriptText(SAY_ELF_START, m_creature, pPlayer);
                break;
            case 9:
                DoScriptText(SAY_ELF_SUMMON1, m_creature, pPlayer);
                // Spawn two Haal'eshi Talonguard
                DoSpawnCreature(NPC_WINDWALKER, -15, -15, 0, 0, TEMPSUMMON_TIMED_OOC_DESPAWN, 5000);
                DoSpawnCreature(NPC_WINDWALKER, -17, -17, 0, 0, TEMPSUMMON_TIMED_OOC_DESPAWN, 5000);
                break;
            case 13:
                DoScriptText(SAY_ELF_RESTING, m_creature, pPlayer);
                break;
            case 14:
                DoScriptText(SAY_ELF_SUMMON2, m_creature, pPlayer);
                // Spawn two Haal'eshi Windwalker
                DoSpawnCreature(NPC_WINDWALKER, -15, -15, 0, 0, TEMPSUMMON_TIMED_OOC_DESPAWN, 5000);
                DoSpawnCreature(NPC_WINDWALKER, -17, -17, 0, 0, TEMPSUMMON_TIMED_OOC_DESPAWN, 5000);
                break;
            case 27:
                DoScriptText(SAY_ELF_COMPLETE, m_creature, pPlayer);
                // Award quest credit
                pPlayer->GroupEventHappens(QUEST_ROAD_TO_FALCON_WATCH, m_creature);
                break;
            }
        }

        void Aggro(Unit* /*pWho*/) override
        {
            if (HasEscortState(STATE_ESCORT_ESCORTING))
            {
                DoScriptText(SAY_ELF_AGGRO, m_creature);
            }
        }

        void JustSummoned(Creature* pSummoned) override
        {
            pSummoned->AI()->AttackStart(m_creature);
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_wounded_blood_elfAI(pCreature);
    }

    bool OnQuestAccept(Player* pPlayer, Creature* pCreature, const Quest* pQuest) override
    {
        if (pQuest->GetQuestId() == QUEST_ROAD_TO_FALCON_WATCH)
        {
            // Change faction so mobs attack
            pCreature->SetFactionTemporary(FACTION_ESCORT_H_PASSIVE, TEMPFACTION_RESTORE_RESPAWN);

            if (npc_wounded_blood_elfAI* pEscortAI = dynamic_cast<npc_wounded_blood_elfAI*>(pCreature->AI()))
            {
                pEscortAI->Start(false, pPlayer, pQuest);
            }
        }

        return true;
    }
};

/*######
## npc_fel_guard_hound
######*/

enum
{
    SPELL_CREATE_POODAD         = 37688,
    SPELL_FAKE_DOG_SPART        = 37692,
    SPELL_INFORM_DOG            = 37689,

    NPC_DERANGED_HELBOAR        = 16863,
};

struct npc_fel_guard_hound : public CreatureScript
{
    npc_fel_guard_hound() : CreatureScript("npc_fel_guard_hound") {}

    struct npc_fel_guard_houndAI : public ScriptedPetAI
    {
        npc_fel_guard_houndAI(Creature* pCreature) : ScriptedPetAI(pCreature) { }

        uint32 m_uiPoodadTimer;

        bool m_bIsPooActive;

        void Reset() override
        {
            m_uiPoodadTimer = 0;
            m_bIsPooActive = false;
        }

        void MovementInform(uint32 uiMoveType, uint32 uiPointId) override
        {
            if (uiMoveType != POINT_MOTION_TYPE || !uiPointId)
            {
                return;
            }

            if (DoCastSpellIfCan(m_creature, SPELL_FAKE_DOG_SPART) == CAST_OK)
            {
                m_uiPoodadTimer = 2000;
            }
        }

        // Function to allow the boar to move to target
        void ReceiveAIEvent(AIEventType eventType, Creature *pSender, Unit *pInvoker, uint32 /*data*/) override //DoMoveToCorpse(Unit* pBoar)
        {
            if (eventType == AI_EVENT_CUSTOM_A && pSender == m_creature)
            {
                if (!pInvoker)
                {
                    return;
                }

                m_bIsPooActive = true;
                m_creature->GetMotionMaster()->MovePoint(1, pInvoker->GetPositionX(), pInvoker->GetPositionY(), pInvoker->GetPositionZ());
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_uiPoodadTimer)
            {
                if (m_uiPoodadTimer <= uiDiff)
                {
                    if (DoCastSpellIfCan(m_creature, SPELL_CREATE_POODAD) == CAST_OK)
                    {
                        m_uiPoodadTimer = 0;
                        m_bIsPooActive = false;
                    }
                }
                else
                {
                    m_uiPoodadTimer -= uiDiff;
                }
            }

            if (!m_bIsPooActive)
            {
                ScriptedPetAI::UpdateAI(uiDiff);
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_fel_guard_houndAI(pCreature);
    }
};

struct spell_inform_dog : public SpellScript
{
    spell_inform_dog() : SpellScript("spell_inform_dog") {}

    bool EffectDummy(Unit* pCaster, uint32 uiSpellId, SpellEffectIndex uiEffIndex, Object* pTarget, ObjectGuid /*originalCasterGuid*/) override
    {
        // always check spellid and effectindex
        if (uiSpellId == SPELL_INFORM_DOG && uiEffIndex == EFFECT_INDEX_0)
        {
            Creature *pCreatureTarget = pTarget->ToCreature();
            if (pCaster->GetEntry() == NPC_DERANGED_HELBOAR)
            {
                if (CreatureAI* pHoundAI = pCreatureTarget->AI())
                {
                    pHoundAI->ReceiveAIEvent(AI_EVENT_CUSTOM_A, pCreatureTarget, pCaster, 0);
                }
            }

            // always return true when we are handling this spell and effect
            return true;
        }

        return false;
    }
};

/*######
## npc_anchorite_barada
######*/

enum
{
    SAY_EXORCISM_1                  = -1000981,
    SAY_EXORCISM_2                  = -1000982,
    SAY_EXORCISM_3                  = -1000983,
    SAY_EXORCISM_4                  = -1000984,
    SAY_EXORCISM_5                  = -1000985,
    SAY_EXORCISM_6                  = -1000986,

    SPELL_BARADA_COMMANDS           = 39277,
    SPELL_BARADA_FALTERS            = 39278,

    SPELL_JULES_THREATENS           = 39284,
    SPELL_JULES_GOES_UPRIGHT        = 39294,
    SPELL_JULES_VOMITS              = 39295,
    SPELL_JULES_RELEASE_DARKNESS    = 39306,                // periodic trigger missing spell 39305

    NPC_ANCHORITE_BARADA            = 22431,
    NPC_COLONEL_JULES               = 22432,
    NPC_DARKNESS_RELEASED           = 22507,                // summoned by missing spell 39305

    GOSSIP_ITEM_EXORCISM            = -3000111,
    QUEST_ID_EXORCISM               = 10935,

    TEXT_ID_CLEANSED                = 10706,
    TEXT_ID_POSSESSED               = 10707,
    TEXT_ID_ANCHORITE               = 10683,
};

static const DialogueEntry aExorcismDialogue[] =
{
    {SAY_EXORCISM_1,        NPC_ANCHORITE_BARADA,   3000},
    {SAY_EXORCISM_2,        NPC_ANCHORITE_BARADA,   2000},
    {QUEST_ID_EXORCISM,     0,                      0},         // start wp movemnet
    {SAY_EXORCISM_3,        NPC_COLONEL_JULES,      3000},
    {SPELL_BARADA_COMMANDS, 0,                      10000},
    {SAY_EXORCISM_4,        NPC_ANCHORITE_BARADA,   10000},
    {SAY_EXORCISM_5,        NPC_COLONEL_JULES,      10000},
    {SPELL_BARADA_FALTERS,  0,                      2000},
    {SPELL_JULES_THREATENS, 0,                      15000},     // start levitating
    {NPC_COLONEL_JULES,     0,                      15000},
    {NPC_ANCHORITE_BARADA,  0,                      15000},
    {NPC_COLONEL_JULES,     0,                      15000},
    {NPC_ANCHORITE_BARADA,  0,                      15000},
    {SPELL_JULES_GOES_UPRIGHT, 0,                   3000},
    {SPELL_JULES_VOMITS,    0,                      7000},      // start moving around the room
    {NPC_COLONEL_JULES,     0,                      10000},
    {NPC_ANCHORITE_BARADA,  0,                      10000},
    {NPC_COLONEL_JULES,     0,                      10000},
    {NPC_ANCHORITE_BARADA,  0,                      10000},
    {NPC_COLONEL_JULES,     0,                      10000},
    {NPC_ANCHORITE_BARADA,  0,                      10000},
    {NPC_COLONEL_JULES,     0,                      10000},
    {NPC_ANCHORITE_BARADA,  0,                      10000},
    {NPC_DARKNESS_RELEASED, 0,                      5000},      // event finished
    {SAY_EXORCISM_6,        NPC_ANCHORITE_BARADA,   3000},
    {TEXT_ID_CLEANSED,      0,                      0},
    {0, 0, 0},
};

static const int32 aAnchoriteTexts[3] = { -1000987, -1000988, -1000989 };
static const int32 aColonelTexts[3] = { -1000990, -1000991, -1000992 };

// Note: script is highly dependent on DBscript implementation
struct npc_anchorite_barada : public CreatureScript
{
    npc_anchorite_barada() : CreatureScript("npc_anchorite_barada") {}

    struct npc_anchorite_baradaAI : public ScriptedAI, private DialogueHelper
    {
        npc_anchorite_baradaAI(Creature* pCreature) : ScriptedAI(pCreature),
        DialogueHelper(aExorcismDialogue)
        {
        }

        bool m_bEventComplete;
        bool m_bEventInProgress;

        ObjectGuid m_colonelGuid;

        void Reset() override
        {
            m_bEventComplete = false;
            m_bEventInProgress = false;
        }

        void AttackStart(Unit* pWho) override
        {
            // no attack during the exorcism
            if (m_bEventInProgress)
                return;

            ScriptedAI::AttackStart(pWho);
        }

        void EnterEvadeMode() override
        {
            // no evade during the exorcism
            if (m_bEventInProgress)
                return;

            ScriptedAI::EnterEvadeMode();
        }

        bool IsExorcismComplete() { return m_bEventComplete; }

        void ReceiveAIEvent(AIEventType eventType, Creature* pSender, Unit* pInvoker, uint32 /*uiMiscValue*/) override
        {
            if (pInvoker->GetTypeId() == TYPEID_PLAYER)
            {
                switch (eventType)
                {
                case AI_EVENT_START_EVENT:  // start the actuall exorcism
                    if (Creature* pColonel = GetClosestCreatureWithEntry(m_creature, NPC_COLONEL_JULES, 15.0f))
                        m_colonelGuid = pColonel->GetObjectGuid();

                    m_creature->SetStandState(UNIT_STAND_STATE_STAND);
                    m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);

                    StartNextDialogueText(SAY_EXORCISM_1);
                    break;
                case AI_EVENT_CUSTOM_A: // event complete - give credit and reset, TODO rethink code distribution between this and the caller
                    if (IsExorcismComplete())
                    {
                        // kill credit
                        ((Player*)pInvoker)->RewardPlayerAndGroupAtEvent(pSender->GetEntry(), pSender);

                        // reset Anchorite and Colonel
                        pSender->AI()->EnterEvadeMode();

                        m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        ((Player*)pInvoker)->SEND_GOSSIP_MENU(TEXT_ID_CLEANSED, pSender->GetObjectGuid());
                        EnterEvadeMode();
                    }
                    break;
                default:
                    break;
                }
            }
        }

        void MovementInform(uint32 uiType, uint32 uiPointId) override
        {
            if (uiType != WAYPOINT_MOTION_TYPE)
                return;

            switch (uiPointId)
            {
            case 3:
                // pause wp and resume dialogue
                m_creature->addUnitState(UNIT_STAT_WAYPOINT_PAUSED);
                m_creature->SetStandState(UNIT_STAND_STATE_KNEEL);
                m_bEventInProgress = true;

                if (Creature* pColonel = m_creature->GetMap()->GetCreature(m_colonelGuid))
                {
                    m_creature->SetFacingToObject(pColonel);
                    pColonel->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                }

                StartNextDialogueText(SAY_EXORCISM_3);
                break;
            case 6:
                // event completed - wait for player to get quest credit by gossip
                if (Creature* pColonel = m_creature->GetMap()->GetCreature(m_colonelGuid))
                    m_creature->SetFacingToObject(pColonel);
                m_creature->GetMotionMaster()->Clear();
                m_creature->SetStandState(UNIT_STAND_STATE_KNEEL);
                m_bEventComplete = true;
                break;
            }
        }

        void JustDidDialogueStep(int32 iEntry) override
        {
            switch (iEntry)
            {
            case QUEST_ID_EXORCISM:
                m_creature->GetMotionMaster()->MoveWaypoint();
                break;
            case SPELL_BARADA_COMMANDS:
                DoCastSpellIfCan(m_creature, SPELL_BARADA_COMMANDS);
                break;
            case SPELL_BARADA_FALTERS:
                DoCastSpellIfCan(m_creature, SPELL_BARADA_FALTERS);
                // start levitating
                if (Creature* pColonel = m_creature->GetMap()->GetCreature(m_colonelGuid))
                {
                    pColonel->SetLevitate(true);
                    pColonel->GetMotionMaster()->MovePoint(0, pColonel->GetPositionX(), pColonel->GetPositionY(), pColonel->GetPositionZ() + 2.0f);
                }
                break;
            case SPELL_JULES_THREATENS:
                if (Creature* pColonel = m_creature->GetMap()->GetCreature(m_colonelGuid))
                {
                    pColonel->CastSpell(pColonel, SPELL_JULES_THREATENS, true);
                    pColonel->CastSpell(pColonel, SPELL_JULES_RELEASE_DARKNESS, true);
                    pColonel->SetFacingTo(0);
                }
                break;
            case SPELL_JULES_GOES_UPRIGHT:
                if (Creature* pColonel = m_creature->GetMap()->GetCreature(m_colonelGuid))
                {
                    pColonel->InterruptNonMeleeSpells(false);
                    pColonel->CastSpell(pColonel, SPELL_JULES_GOES_UPRIGHT, false);
                }
                break;
            case SPELL_JULES_VOMITS:
                if (Creature* pColonel = m_creature->GetMap()->GetCreature(m_colonelGuid))
                {
                    pColonel->CastSpell(pColonel, SPELL_JULES_VOMITS, true);
                    pColonel->GetMotionMaster()->MoveRandomAroundPoint(m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ() + 3.0f, 5.0f);
                }
                break;
            case NPC_COLONEL_JULES:
                if (Creature* pColonel = m_creature->GetMap()->GetCreature(m_colonelGuid))
                    DoScriptText(aColonelTexts[urand(0, 2)], pColonel);
                break;
            case NPC_ANCHORITE_BARADA:
                DoScriptText(aAnchoriteTexts[urand(0, 2)], m_creature);
                break;
            case NPC_DARKNESS_RELEASED:
                if (Creature* pColonel = m_creature->GetMap()->GetCreature(m_colonelGuid))
                {
                    pColonel->RemoveAurasDueToSpell(SPELL_JULES_THREATENS);
                    pColonel->RemoveAurasDueToSpell(SPELL_JULES_RELEASE_DARKNESS);
                    pColonel->RemoveAurasDueToSpell(SPELL_JULES_VOMITS);
                    pColonel->GetMotionMaster()->MoveTargetedHome();
                    pColonel->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                }
                break;
            case TEXT_ID_CLEANSED:
                if (Creature* pColonel = m_creature->GetMap()->GetCreature(m_colonelGuid))
                {
                    pColonel->RemoveAurasDueToSpell(SPELL_JULES_GOES_UPRIGHT);
                    pColonel->SetLevitate(false);
                }
                // resume wp movemnet
                m_creature->RemoveAllAuras();
                m_creature->clearUnitState(UNIT_STAT_WAYPOINT_PAUSED);
                m_creature->SetStandState(UNIT_STAND_STATE_STAND);
                break;
            }
        }

        Creature* GetSpeakerByEntry(uint32 uiEntry) override
        {
            switch (uiEntry)
            {
            case NPC_ANCHORITE_BARADA:      return m_creature;
            case NPC_COLONEL_JULES:         return m_creature->GetMap()->GetCreature(m_colonelGuid);

            default:
                return NULL;
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            DialogueUpdate(uiDiff);

            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_anchorite_baradaAI(pCreature);
    }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override
    {
        // check if quest is active but not completed
        if (pPlayer->IsCurrentQuest(QUEST_ID_EXORCISM, 1))
            pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_EXORCISM, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

        pPlayer->SEND_GOSSIP_MENU(TEXT_ID_ANCHORITE, pCreature->GetObjectGuid());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction) override
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_INFO_DEF + 1)
        {
            pCreature->AI()->SendAIEvent(AI_EVENT_START_EVENT, pPlayer, pCreature);
            pPlayer->CLOSE_GOSSIP_MENU();
        }

        return true;
    }
};

/*######
## npc_colonel_jules
######*/

struct npc_colonel_jules : public CreatureScript
{
    npc_colonel_jules() : CreatureScript("npc_colonel_jules") {}

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override
    {
        // quest already completed
        if (pPlayer->GetQuestStatus(QUEST_ID_EXORCISM) == QUEST_STATUS_COMPLETE)
        {
            pPlayer->SEND_GOSSIP_MENU(TEXT_ID_CLEANSED, pCreature->GetObjectGuid());
            return true;
        }
        // quest active but not complete
        else if (pPlayer->IsCurrentQuest(QUEST_ID_EXORCISM, 1))
        {
            Creature* pAnchorite = GetClosestCreatureWithEntry(pCreature, NPC_ANCHORITE_BARADA, 15.0f);
            if (!pAnchorite)
                return true;

            pCreature->AI()->SendAIEvent(AI_EVENT_CUSTOM_A, pPlayer, pAnchorite);
            return true;
        }

        pPlayer->SEND_GOSSIP_MENU(TEXT_ID_POSSESSED, pCreature->GetObjectGuid());
        return true;
    }
};

struct spell_just_release_darkness : public SpellScript
{
    spell_just_release_darkness() : SpellScript("spell_just_release_darkness") {}

    bool EffectDummy(Unit* pCaster, uint32 uiSpellId, SpellEffectIndex uiEffIndex, Object* pTarget, ObjectGuid /*originalCasterGuid*/) override
    {
        // always check spellid and effectindex
        if (uiSpellId == SPELL_JULES_RELEASE_DARKNESS && uiEffIndex == EFFECT_INDEX_0 && pTarget->GetEntry() == NPC_COLONEL_JULES)
        {
            Creature *pCreatureTarget = pTarget->ToCreature();
            Creature* pAnchorite = GetClosestCreatureWithEntry(pCreatureTarget, NPC_ANCHORITE_BARADA, 15.0f);
            if (!pAnchorite)
                return false;

            // get random point around the Anchorite
            float fX, fY, fZ;
            pCreatureTarget->GetNearPoint(pCreatureTarget, fX, fY, fZ, 5.0f, 10.0f, frand(0, M_PI_F / 2));

            // spawn a Darkness Released npc and move around the room
            if (Creature* pDarkness = pCreatureTarget->SummonCreature(NPC_DARKNESS_RELEASED, 0, 0, 0, 0, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 20000))
                pDarkness->GetMotionMaster()->MovePoint(0, fX, fY, fZ);

            // always return true when we are handling this spell and effect
            return true;
        }

        return false;
    }
};

/*######
## npc_caretaker_dilandrus
######*/

static const float aGraveYardLocation[11][4] =
{
    {-807.52f, 2694.01f, 105.149f, 2.58f},     // spawn point
    {-807.922f, 2692.816f, 104.837f, 2.76f},   // grave location
    {-805.451f, 2696.972f, 105.671f, 2.42f},   // grave location
    {-802.575f, 2702.187f, 106.645f, 2.64f},   // grave location
    {-811.831f, 2699.465f, 106.544f, 2.84f},   // grave location
    {-806.366f, 2708.088f, 108.162f, 2.79f},   // grave location
    {-820.038f, 2701.316f, 107.491f, 2.82f},   // grave location
    {-818.042f, 2705.911f, 108.371f, 2.69f},   // grave location
    {-815.989f, 2709.199f, 108.973f, 2.59f},   // grave location
    {-814.213f, 2712.847f, 109.612f, 2.26f},   // grave location
    {-810.533f, 2718.717f, 110.509f, 2.78f}    // grave location
};

struct npc_caretaker_dilandrus : public CreatureScript
{        
	   npc_caretaker_dilandrus() : CreatureScript("npc_caretaker_dilandrus") {}

    struct npc_caretaker_dilandrusAI : public ScriptedAI
    {
        npc_caretaker_dilandrusAI(Creature* pCreature) : ScriptedAI(pCreature) 
        { 
            Reset(); 
        }
        
        uint32 uVisitGraveTimer, uCurrentStage, uLastGraveVisited;

        void Reset() override 
        {
            uVisitGraveTimer = 0; 
            uCurrentStage = 1;    
            uLastGraveVisited = 0;
        }

        void UpdateAI(const uint32 uiDiff)
        {
            if (uVisitGraveTimer <= uiDiff)
            {
                uint32 uGraveNumber = 0;
                if (uCurrentStage == 1) // walk to grave
                {
                    // time to visit grave
                    uGraveNumber = rand() % 9 + 1;
                    m_creature->GetMotionMaster()->MovePoint(0, aGraveYardLocation[uGraveNumber][0], aGraveYardLocation[uGraveNumber][1], aGraveYardLocation[uGraveNumber][2]);
                    uLastGraveVisited = uGraveNumber;
                    uCurrentStage = 2;
                    uVisitGraveTimer = 10000;
                }
                else if (uCurrentStage == 2) // face gravestone
                {
                    m_creature->SetFacingTo(aGraveYardLocation[uGraveNumber][3]);
                    uCurrentStage = 3;
                    uVisitGraveTimer = 3000;
                }
                else if (uCurrentStage == 3) // kneel for a few second
                {
                    if (rand() % 3 == 0)
                    {
                        m_creature->SetStandState(UNIT_STAND_STATE_KNEEL);
                        uVisitGraveTimer = 5000;
                    }
                    else uVisitGraveTimer = 0;
                    uCurrentStage = 4;
                }
                else if (uCurrentStage == 4)
                {
                    // lay wreath - spawn it

                    m_creature->SetStandState(UNIT_STAND_STATE_STAND);
                    uCurrentStage = 5;
                    uVisitGraveTimer = 5000;
                }
                else if (uCurrentStage == 5)
                {
                    if (rand() % 3 == 0)
                    {
                        m_creature->HandleEmote(EMOTE_ONESHOT_SALUTE);
                    }
                    else if (rand() % 6 == 0)
                    {
                        m_creature->HandleEmote(EMOTE_ONESHOT_CRY);
                    }
                    else uVisitGraveTimer = 5000;
                    uCurrentStage = 6;
                }
                else if (uCurrentStage == 6) // go back to start
                {
                    m_creature->GetMotionMaster()->MovePoint(0, aGraveYardLocation[0][0], aGraveYardLocation[0][1], aGraveYardLocation[0][2]);
                    m_creature->SetFacingTo(aGraveYardLocation[0][3]);
                    uVisitGraveTimer = 900000; // visit a grave every 15 minutes
                    uCurrentStage = 1;
                }

            }
            else
            {
                uVisitGraveTimer -= uiDiff;
            }

        }

    };
		
    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_caretaker_dilandrusAI(pCreature);
    }
};

void AddSC_hellfire_peninsula()
{
    Script* s;

    s = new npc_aeranas();
    s->RegisterSelf();
    s = new npc_ancestral_wolf();
    s->RegisterSelf();
    s = new npc_demoniac_scryer();
    s->RegisterSelf();
    s = new npc_wounded_blood_elf();
    s->RegisterSelf();
    s = new npc_fel_guard_hound();
    s->RegisterSelf();
    s = new npc_anchorite_barada();
    s->RegisterSelf();
    s = new npc_colonel_jules();
    s->RegisterSelf();
    s = new spell_just_release_darkness();
    s->RegisterSelf();
    s = new npc_caretaker_dilandrus();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_aeranas";
    //pNewScript->GetAI = &GetAI_npc_aeranas;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_ancestral_wolf";
    //pNewScript->GetAI = &GetAI_npc_ancestral_wolf;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_demoniac_scryer";
    //pNewScript->GetAI = &GetAI_npc_demoniac_scryer;
    //pNewScript->pGossipHello = &GossipHello_npc_demoniac_scryer;
    //pNewScript->pGossipSelect = &GossipSelect_npc_demoniac_scryer;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_wounded_blood_elf";
    //pNewScript->GetAI = &GetAI_npc_wounded_blood_elf;
    //pNewScript->pQuestAcceptNPC = &QuestAccept_npc_wounded_blood_elf;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_fel_guard_hound";
    //pNewScript->GetAI = &GetAI_npc_fel_guard_hound;
    //pNewScript->pEffectDummyNPC = &EffectDummyCreature_npc_fel_guard_hound;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_anchorite_barada";
    //pNewScript->GetAI = &GetAI_npc_anchorite_barada;
    //pNewScript->pGossipHello = &GossipHello_npc_anchorite_barada;
    //pNewScript->pGossipSelect = &GossipSelect_npc_anchorite_barada;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_colonel_jules";
    //pNewScript->pGossipHello = &GossipHello_npc_colonel_jules;
    //pNewScript->pEffectDummyNPC = &EffectDummyCreature_npc_colonel_jules;
    //pNewScript->RegisterSelf();
}
