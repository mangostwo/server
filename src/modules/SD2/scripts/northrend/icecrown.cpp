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
SDName: Icecrown
SD%Complete: 100
SDComment: Quest support: 13221, 13229, 13284, 13300, 13301, 13302, 13481, 13482.
SDCategory: Icecrown
EndScriptData */

/* ContentData
npc_squad_leader
npc_infantry
npc_father_kamaros
npc_saronite_mine_slave
EndContentData */

#include "precompiled.h"
#include "escort_ai.h"

/*######
## npc_squad_leader
######*/

enum
{
    // yells
    SAY_HORDE_SQUAD_RUN                 = -1001018,
    SAY_ALLIANCE_SQUAD_RUN              = -1001019,
    SAY_SQUAD_AGGRO_1                   = -1001020,
    SAY_SQUAD_AGGRO_2                   = -1001021,
    SAY_HORDE_SQUAD_AGGRO_1             = -1001022,
    SAY_HORDE_SQUAD_AGGRO_2             = -1001023,
    SAY_HORDE_SQUAD_AGGRO_3             = -1001024,
    SAY_HORDE_SQUAD_AGGRO_4             = -1001025,
    SAY_ALLIANCE_SQUAD_AGGRO_1          = -1001026,
    SAY_ALLIANCE_SQUAD_AGGRO_2          = -1001027,
    SAY_ALLIANCE_SQUAD_AGGRO_3          = -1001028,
    SAY_ALLIANCE_SQUAD_AGGRO_4          = -1001029,
    SAY_HORDE_SQUAD_BREAK               = -1001030,
    SAY_HORDE_SQUAD_BREAK_DONE          = -1001031,
    SAY_ALLIANCE_SQUAD_BREAK            = -1001032,
    SAY_ALLIANCE_SQUAD_BREAK_DONE       = -1001033,
    SAY_EVENT_COMPLETE                  = -1001034,
    SAY_DEFENDER_AGGRO_1                = -1001035,
    SAY_DEFENDER_AGGRO_2                = -1001036,
    SAY_DEFENDER_AGGRO_3                = -1001037,
    SAY_DEFENDER_AGGRO_4                = -1001038,
    SAY_DEFENDER_AGGRO_5                = -1001039,
    SAY_DEFENDER_AGGRO_6                = -1001040,
    SAY_DEFENDER_AGGRO_7                = -1001041,
    SAY_DEFENDER_AGGRO_8                = -1001042,
    SAY_DEFENDER_AGGRO_9                = -1001043,

    // combat spells
    SPELL_CLEAVE                        = 15496,
    SPELL_FROST_SHOT                    = 12551,
    SPELL_ALLIANCE_TROOP_CREDIT         = 59677,
    SPELL_HORDE_TROOP_CREDIT            = 59764,

    NPC_SKYBREAKER_SQUAD_LEADER         = 31737,
    NPC_SKYBREAKER_INFANTRY             = 31701,
    NPC_KORKRON_SQUAD_LEADER            = 31833,
    NPC_KORKRON_INFANTRY                = 31832,
    NPC_YMIRHEIM_DEFENDER               = 31746,

    // quests
    QUEST_ID_ASSAULT_BY_GROUND_A        = 13284,
    QUEST_ID_ASSAULT_BY_GROUND_H        = 13301,
};

struct npc_squad_leader : public CreatureScript
{
    npc_squad_leader() : CreatureScript("npc_squad_leader") {}

    struct npc_squad_leaderAI : public npc_escortAI
    {
        npc_squad_leaderAI(Creature* pCreature) : npc_escortAI(pCreature) { }

        uint32 m_uiCleaveTimer;
        uint32 m_uiFrostShotTimer;

        void Reset() override
        {
            m_uiCleaveTimer = urand(3000, 5000);
            m_uiFrostShotTimer = urand(1000, 3000);
        }

        void Aggro(Unit* pWho) override
        {
            switch (urand(0, 5))
            {
            case 0: DoScriptText(SAY_SQUAD_AGGRO_1, m_creature); break;
            case 1: DoScriptText(SAY_SQUAD_AGGRO_2, m_creature); break;
            case 2: DoScriptText(m_creature->GetEntry() == NPC_SKYBREAKER_SQUAD_LEADER ? SAY_ALLIANCE_SQUAD_AGGRO_1 : SAY_HORDE_SQUAD_AGGRO_1, m_creature); break;
            case 3: DoScriptText(m_creature->GetEntry() == NPC_SKYBREAKER_SQUAD_LEADER ? SAY_ALLIANCE_SQUAD_AGGRO_2 : SAY_HORDE_SQUAD_AGGRO_2, m_creature); break;
            case 4: DoScriptText(m_creature->GetEntry() == NPC_SKYBREAKER_SQUAD_LEADER ? SAY_ALLIANCE_SQUAD_AGGRO_3 : SAY_HORDE_SQUAD_AGGRO_3, m_creature); break;
            case 5: DoScriptText(m_creature->GetEntry() == NPC_SKYBREAKER_SQUAD_LEADER ? SAY_ALLIANCE_SQUAD_AGGRO_4 : SAY_HORDE_SQUAD_AGGRO_4, m_creature); break;
            }

            if (pWho->GetEntry() == NPC_YMIRHEIM_DEFENDER && urand(0, 1))
            {
                switch (urand(0, 8))
                {
                case 0: DoScriptText(SAY_DEFENDER_AGGRO_1, pWho); break;
                case 1: DoScriptText(SAY_DEFENDER_AGGRO_2, pWho); break;
                case 2: DoScriptText(SAY_DEFENDER_AGGRO_3, pWho); break;
                case 3: DoScriptText(SAY_DEFENDER_AGGRO_4, pWho); break;
                case 4: DoScriptText(SAY_DEFENDER_AGGRO_5, pWho); break;
                case 5: DoScriptText(SAY_DEFENDER_AGGRO_6, pWho); break;
                case 6: DoScriptText(SAY_DEFENDER_AGGRO_7, pWho); break;
                case 7: DoScriptText(SAY_DEFENDER_AGGRO_8, pWho); break;
                case 8: DoScriptText(SAY_DEFENDER_AGGRO_9, pWho); break;
                }
            }
        }

        void ReceiveAIEvent(AIEventType eventType, Creature* /*pSender*/, Unit* pInvoker, uint32 uiMiscValue) override
        {
            if (eventType == AI_EVENT_START_ESCORT && pInvoker->GetTypeId() == TYPEID_PLAYER)
            {
                Start(false, (Player*)pInvoker, GetQuestTemplateStore(uiMiscValue));
                SendAIEventAround(AI_EVENT_CUSTOM_A, pInvoker, 0, 12.0f);
            }
        }

        void JustSummoned(Creature* pSummoned) override
        {
            if (pSummoned->GetEntry() == NPC_YMIRHEIM_DEFENDER)
                pSummoned->AI()->AttackStart(m_creature);
        }

        void WaypointReached(uint32 uiPointId) override
        {
            switch (uiPointId)
            {
            case 2:
                SetRun();
                DoScriptText(m_creature->GetEntry() == NPC_SKYBREAKER_SQUAD_LEADER ? SAY_ALLIANCE_SQUAD_RUN : SAY_HORDE_SQUAD_RUN, m_creature);
                break;
            case 4:
                // first horde attack
                if (m_creature->GetEntry() == NPC_KORKRON_SQUAD_LEADER)
                {
                    m_creature->SummonCreature(NPC_YMIRHEIM_DEFENDER, 7433.193f, 1838.199f, 402.43f, 0, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                    m_creature->SummonCreature(NPC_YMIRHEIM_DEFENDER, 7441.071f, 1848.997f, 401.03f, 0, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                    m_creature->SummonCreature(NPC_YMIRHEIM_DEFENDER, 7451.976f, 1850.776f, 402.96f, 0, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                }
                break;
            case 6:
                // first alliance attack
                if (m_creature->GetEntry() == NPC_SKYBREAKER_SQUAD_LEADER)
                {
                    m_creature->SummonCreature(NPC_YMIRHEIM_DEFENDER, 7217.792f, 1602.024f, 378.86f, 0, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                    m_creature->SummonCreature(NPC_YMIRHEIM_DEFENDER, 7235.733f, 1597.831f, 381.08f, 0, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                }
                break;
            case 9:
                // second horde attack
                if (m_creature->GetEntry() == NPC_KORKRON_SQUAD_LEADER)
                {
                    m_creature->SummonCreature(NPC_YMIRHEIM_DEFENDER, 7420.511f, 1813.180f, 425.14f, 0, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                    m_creature->SummonCreature(NPC_YMIRHEIM_DEFENDER, 7411.768f, 1784.054f, 427.84f, 0, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                    m_creature->SummonCreature(NPC_YMIRHEIM_DEFENDER, 7418.514f, 1805.596f, 425.50f, 0, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                }
                break;
            case 13:
                if (m_creature->GetEntry() == NPC_KORKRON_SQUAD_LEADER)
                {
                    DoScriptText(SAY_HORDE_SQUAD_BREAK, m_creature);
                    m_creature->SetStandState(UNIT_STAND_STATE_KNEEL);
                }
                break;
            case 14:
                if (m_creature->GetEntry() == NPC_KORKRON_SQUAD_LEADER)
                {
                    DoScriptText(SAY_HORDE_SQUAD_BREAK_DONE, m_creature);
                    m_creature->SetStandState(UNIT_STAND_STATE_STAND);
                }
                break;
            case 15:
                // second alliance attack
                if (m_creature->GetEntry() == NPC_SKYBREAKER_SQUAD_LEADER)
                {
                    m_creature->SummonCreature(NPC_YMIRHEIM_DEFENDER, 7328.375f, 1631.935f, 416.06f, 0, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                    m_creature->SummonCreature(NPC_YMIRHEIM_DEFENDER, 7334.475f, 1618.401f, 412.93f, 0, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                    m_creature->SummonCreature(NPC_YMIRHEIM_DEFENDER, 7341.556f, 1632.023f, 423.01f, 0, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                }
                break;
            case 20:
                if (m_creature->GetEntry() == NPC_SKYBREAKER_SQUAD_LEADER)
                {
                    DoScriptText(SAY_ALLIANCE_SQUAD_BREAK, m_creature);
                    m_creature->SetStandState(UNIT_STAND_STATE_KNEEL);
                }
                break;
            case 21:
                if (m_creature->GetEntry() == NPC_SKYBREAKER_SQUAD_LEADER)
                {
                    DoScriptText(SAY_ALLIANCE_SQUAD_BREAK_DONE, m_creature);
                    m_creature->SetStandState(UNIT_STAND_STATE_STAND);
                }
                break;
            case 22:
                // horde gate attack
                if (m_creature->GetEntry() == NPC_KORKRON_SQUAD_LEADER)
                {
                    m_creature->SummonCreature(NPC_YMIRHEIM_DEFENDER, 7280.229f, 1725.829f, 471.37f, 0, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                    m_creature->SummonCreature(NPC_YMIRHEIM_DEFENDER, 7272.390f, 1732.530f, 472.43f, 0, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                    m_creature->SummonCreature(NPC_YMIRHEIM_DEFENDER, 7285.863f, 1690.997f, 483.35f, 0, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                    m_creature->SummonCreature(NPC_YMIRHEIM_DEFENDER, 7334.487f, 1690.376f, 443.32f, 0, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                    m_creature->SummonCreature(NPC_YMIRHEIM_DEFENDER, 7371.765f, 1699.052f, 442.50f, 0, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                }
                break;
            case 25:
                // alliance gate attack
                if (m_creature->GetEntry() == NPC_SKYBREAKER_SQUAD_LEADER)
                {
                    m_creature->SummonCreature(NPC_YMIRHEIM_DEFENDER, 7205.636f, 1648.500f, 453.59f, 0, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                    m_creature->SummonCreature(NPC_YMIRHEIM_DEFENDER, 7224.602f, 1677.164f, 454.65f, 0, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                    m_creature->SummonCreature(NPC_YMIRHEIM_DEFENDER, 7220.114f, 1667.603f, 451.01f, 0, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                    m_creature->SummonCreature(NPC_YMIRHEIM_DEFENDER, 7220.528f, 1634.114f, 434.81f, 0, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                    m_creature->SummonCreature(NPC_YMIRHEIM_DEFENDER, 7237.092f, 1687.461f, 459.81f, 0, TEMPSUMMON_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                }
                break;
            case 26:
            {
                       // event complete
                       if (Player* pPlayer = GetPlayerForEscort())
                           m_creature->SetFacingToObject(pPlayer);
                       DoScriptText(SAY_EVENT_COMPLETE, m_creature);

                       // get all the soldiers around
                       std::list<Creature*> lSoldiersList;
                       GetCreatureListWithEntryInGrid(lSoldiersList, m_creature, m_creature->GetEntry() == NPC_SKYBREAKER_SQUAD_LEADER ? NPC_SKYBREAKER_INFANTRY : NPC_KORKRON_INFANTRY, 30.0f);

                       // for each soldier alive cast the kill credit
                       for (std::list<Creature*>::const_iterator itr = lSoldiersList.begin(); itr != lSoldiersList.end(); ++itr)
                       {
                           if ((*itr) && (*itr)->IsAlive())
                           {
                               (*itr)->CastSpell(*itr, (*itr)->GetEntry() == NPC_SKYBREAKER_INFANTRY ? SPELL_ALLIANCE_TROOP_CREDIT : SPELL_HORDE_TROOP_CREDIT, true);
                               (*itr)->ForcedDespawn(10000);
                           }
                       }

                       // set to pause and despawn on timer
                       SetEscortPaused(true);
                       m_creature->ForcedDespawn(10000);
                       break;
            }
            }
        }

        void UpdateEscortAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
                return;

            if (m_uiFrostShotTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_FROST_SHOT) == CAST_OK)
                    m_uiFrostShotTimer = urand(1000, 3000);
            }
            else
                m_uiFrostShotTimer -= uiDiff;

            if (m_uiCleaveTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_CLEAVE) == CAST_OK)
                    m_uiCleaveTimer = urand(3000, 5000);
            }
            else
                m_uiCleaveTimer -= uiDiff;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_squad_leaderAI(pCreature);
    }

    bool OnQuestAccept(Player* pPlayer, Creature* pCreature, const Quest* pQuest) override
    {
        if (pQuest->GetQuestId() == QUEST_ID_ASSAULT_BY_GROUND_A || pQuest->GetQuestId() == QUEST_ID_ASSAULT_BY_GROUND_H)
        {
            pCreature->AI()->SendAIEvent(AI_EVENT_START_ESCORT, pPlayer, pCreature, pQuest->GetQuestId());
            return true;
        }

        return false;
    }
};

/*######
## npc_infantry
######*/

enum
{
    SPELL_SHOOT                         = 15547,
    SPELL_HEROIC_STRIKE                 = 29426,
};

struct npc_infantry : public CreatureScript
{
    npc_infantry() : CreatureScript("npc_infantry") {}

    struct npc_infantryAI : public ScriptedAI
    {
        npc_infantryAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_bEscortActive = false;
        }

        uint32 m_uiShootTimer;
        uint32 m_uiHeroicStrikeTimer;

        bool m_bEscortActive;

        ObjectGuid m_squadLeaderGuid;

        void Reset() override
        {
            m_uiShootTimer = urand(1000, 3000);
            m_uiHeroicStrikeTimer = urand(3000, 5000);
        }

        void EnterEvadeMode() override
        {
            m_creature->RemoveAllAurasOnEvade();
            m_creature->DeleteThreatList();
            m_creature->CombatStop(true);
            m_creature->LoadCreatureAddon(true);

            m_creature->SetLootRecipient(NULL);

            Reset();

            if (!m_creature->IsAlive())
                return;

            if (m_bEscortActive)
            {
                if (Creature* pLeader = m_creature->GetMap()->GetCreature(m_squadLeaderGuid))
                    m_creature->GetMotionMaster()->MoveFollow(pLeader, m_creature->GetDistance(pLeader), M_PI_F / 2 + m_creature->GetAngle(pLeader));
            }
            else
                m_creature->GetMotionMaster()->MoveTargetedHome();
        }

        void JustRespawned() override
        {
            m_bEscortActive = false;
        }

        void ReceiveAIEvent(AIEventType eventType, Creature* pSender, Unit* /*pInvoker*/, uint32 uiMiscValue) override
        {
            // start following the squad leader
            if (eventType == AI_EVENT_CUSTOM_A && (pSender->GetEntry() == NPC_SKYBREAKER_SQUAD_LEADER || pSender->GetEntry() == NPC_KORKRON_SQUAD_LEADER))
            {
                m_creature->SetStandState(UNIT_STAND_STATE_STAND);
                m_creature->GetMotionMaster()->MoveFollow(pSender, m_creature->GetDistance(pSender), M_PI_F / 2 + m_creature->GetAngle(pSender));
                m_squadLeaderGuid = pSender->GetObjectGuid();
                m_bEscortActive = true;
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
                return;

            if (m_uiShootTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_SHOOT) == CAST_OK)
                    m_uiShootTimer = urand(1000, 3000);
            }
            else
                m_uiShootTimer -= uiDiff;

            if (m_uiHeroicStrikeTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_HEROIC_STRIKE) == CAST_OK)
                    m_uiHeroicStrikeTimer = urand(3000, 5000);
            }
            else
                m_uiHeroicStrikeTimer -= uiDiff;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_infantryAI(pCreature);
    }
};

/*######
## npc_father_kamaros
######*/

enum
{
    // yells
    SAY_KAMAROS_START_1                 = -1001044,
    SAY_KAMAROS_START_2                 = -1001045,
    SAY_KAMAROS_AGGRO_1                 = -1001046,
    SAY_KAMAROS_AGGRO_2                 = -1001047,
    SAY_KAMAROS_AGGRO_3                 = -1001048,
    SAY_KAMAROS_COMPLETE_1              = -1001050,
    SAY_KAMAROS_COMPLETE_2              = -1001049,

    // combat spells
    SPELL_HOLY_SMITE                    = 25054,
    SPELL_POWER_WORD_FORTITUDE          = 58921,
    SPELL_POWER_WORD_SHIELD             = 32595,
    SPELL_SHADOW_WORD_PAIN              = 17146,

    NPC_SPIKED_GHOUL                    = 30597,
    NPC_KAMAROS_1                       = 31279,                    // phase 1 npc
    NPC_KAMAROS_2                       = 32800,                    // phase 64 npc

    // quests
    // all four quests have the same script;
    // they depend only on faction and quest giver (same npc, different entries in different phases);
    QUEST_ID_NOT_DEAD_YET_A             = 13221,
    QUEST_ID_NOT_DEAD_YET_H             = 13229,
    QUEST_ID_GET_OUT_OF_HERE_A          = 13482,
    QUEST_ID_GET_OUT_OF_HERE_H          = 13481,
};

struct npc_father_kamaros : public CreatureScript
{
    npc_father_kamaros() : CreatureScript("npc_father_kamaros") {}

    struct npc_father_kamarosAI : public npc_escortAI
    {
        npc_father_kamarosAI(Creature* pCreature) : npc_escortAI(pCreature)
        {
            m_uiCurrentQuestId = 0;
        }

        uint32 m_uiSmiteTimer;
        uint32 m_uiShadowWordTimer;
        uint32 m_uiCurrentQuestId;

        void Reset() override
        {
            m_uiSmiteTimer = urand(3000, 5000);
            m_uiShadowWordTimer = urand(1000, 3000);
        }

        void Aggro(Unit* /*pWho*/) override
        {
            switch (urand(0, 2))
            {
            case 0: DoScriptText(SAY_KAMAROS_AGGRO_1, m_creature); break;
            case 1: DoScriptText(SAY_KAMAROS_AGGRO_2, m_creature); break;
            case 2: DoScriptText(SAY_KAMAROS_AGGRO_3, m_creature); break;
            }

            if (DoCastSpellIfCan(m_creature, SPELL_POWER_WORD_SHIELD) != CAST_OK)
            {
                if (Player* pPlayer = GetPlayerForEscort())
                    DoCastSpellIfCan(pPlayer, SPELL_POWER_WORD_SHIELD);
            }
        }

        void ReceiveAIEvent(AIEventType eventType, Creature* /*pSender*/, Unit* pInvoker, uint32 uiMiscValue) override
        {
            if (eventType == AI_EVENT_START_ESCORT && pInvoker->GetTypeId() == TYPEID_PLAYER)
            {
                m_uiCurrentQuestId = uiMiscValue;
                m_creature->SetStandState(UNIT_STAND_STATE_STAND);
                Start(false, (Player*)pInvoker, GetQuestTemplateStore(uiMiscValue));
            }
        }

        void WaypointReached(uint32 uiPointId) override
        {
            switch (uiPointId)
            {
            case 0:
                if (Player* pPlayer = GetPlayerForEscort())
                {
                    DoScriptText(SAY_KAMAROS_START_1, m_creature, pPlayer);
                    DoCastSpellIfCan(pPlayer, SPELL_POWER_WORD_FORTITUDE);
                }
                break;
            case 1:
                DoScriptText(SAY_KAMAROS_START_2, m_creature);
                break;
            case 11:
            case 13:
            case 16:
                if (Creature* pGhoul = GetClosestCreatureWithEntry(m_creature, NPC_SPIKED_GHOUL, 25.0f))
                    pGhoul->AI()->AttackStart(m_creature);
                break;
            case 23:
                if (Player* pPlayer = GetPlayerForEscort())
                    m_creature->SetFacingToObject(pPlayer);
                DoScriptText(SAY_KAMAROS_COMPLETE_1, m_creature);
                break;
            case 24:
                SetRun();
                DoScriptText(SAY_KAMAROS_COMPLETE_2, m_creature);
                if (Player* pPlayer = GetPlayerForEscort())
                    pPlayer->GroupEventHappens(m_uiCurrentQuestId, m_creature);
                break;
            }
        }

        void UpdateEscortAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
                return;

            if (m_uiSmiteTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_HOLY_SMITE) == CAST_OK)
                    m_uiSmiteTimer = urand(3000, 4000);
            }
            else
                m_uiSmiteTimer -= uiDiff;

            if (m_uiShadowWordTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_SHADOW_WORD_PAIN) == CAST_OK)
                    m_uiShadowWordTimer = urand(15000, 20000);
            }
            else
                m_uiShadowWordTimer -= uiDiff;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_father_kamarosAI(pCreature);
    }

    bool OnQuestAccept(Player* pPlayer, Creature* pCreature, const Quest* pQuest) override
    {
        if (pQuest->GetQuestId() == QUEST_ID_NOT_DEAD_YET_A || pQuest->GetQuestId() == QUEST_ID_NOT_DEAD_YET_H ||
            pQuest->GetQuestId() == QUEST_ID_GET_OUT_OF_HERE_A || pQuest->GetQuestId() == QUEST_ID_GET_OUT_OF_HERE_H)
        {
            pCreature->AI()->SendAIEvent(AI_EVENT_START_ESCORT, pPlayer, pCreature, pQuest->GetQuestId());
            return true;
        }

        return false;
    }
};

/*######
## npc_saronite_mine_slave
######*/

enum
{
    SAY_MINER_SUICIDE_1                 = -1001117,
    SAY_MINER_SUICIDE_2                 = -1001118,
    SAY_MINER_SUICIDE_3                 = -1001119,
    SAY_MINER_SUICIDE_4                 = -1001120,
    SAY_MINER_SUICIDE_5                 = -1001121,
    SAY_MINER_SUICIDE_6                 = -1001122,
    SAY_MINER_SUICIDE_7                 = -1001123,
    SAY_MINER_SUICIDE_8                 = -1001124,

    GOSSIP_ITEM_SLAVE_FREE              = -3000113,
    TEXT_ID                             = 14068,

    NPC_SARONITE_KILL_CREDIT_BUNNY      = 31866,

    FACTION_HOSTILE                     = 14,

    QUEST_SLAVES_TO_SARONITE_A          = 13300,
    QUEST_SLAVES_TO_SARONITE_H          = 13302,
};

static const float afPointSlaveSalvation[3] = {7030.59f, 1866.73f, 533.94f};
static const float afPointSlaveSuicide1[3] = {6965.99f, 2051.44f, 519.49f};
static const float afPointSlaveSuicide2[3] = {6920.47f, 1973.46f, 523.38f};
static const float afPointSlaveSuicide3[3] = {6915.35f, 2026.35f, 518.53f};

struct npc_saronite_mine_slave : public CreatureScript
{
    npc_saronite_mine_slave() : CreatureScript("npc_saronite_mine_slave") {}

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override
    {
        if (pPlayer->GetQuestStatus(QUEST_SLAVES_TO_SARONITE_A) == QUEST_STATUS_INCOMPLETE || pPlayer->GetQuestStatus(QUEST_SLAVES_TO_SARONITE_H) == QUEST_STATUS_INCOMPLETE)
            pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_SLAVE_FREE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

        pPlayer->SEND_GOSSIP_MENU(TEXT_ID, pCreature->GetObjectGuid());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction) override
    {
        if (uiAction != GOSSIP_ACTION_INFO_DEF + 1)
            return false;

        pPlayer->CLOSE_GOSSIP_MENU();

        switch (urand(0, 5))
        {
        case 0:
        case 1:
        case 2:
            pCreature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            pPlayer->KilledMonsterCredit(NPC_SARONITE_KILL_CREDIT_BUNNY);

            pCreature->SetWalk(false);
            pCreature->GetMotionMaster()->MovePoint(0, afPointSlaveSalvation[0], afPointSlaveSalvation[1], afPointSlaveSalvation[2]);
            pCreature->ForcedDespawn(20000);
            break;
        case 3:
        case 4:
            pCreature->SetFactionTemporary(FACTION_HOSTILE, TEMPFACTION_RESTORE_RESPAWN | TEMPFACTION_RESTORE_REACH_HOME);
            pCreature->AI()->AttackStart(pPlayer);
            break;
        case 5:
            switch (urand(0, 7))
            {
            case 0: DoScriptText(SAY_MINER_SUICIDE_1, pCreature); break;
            case 1: DoScriptText(SAY_MINER_SUICIDE_2, pCreature); break;
            case 2: DoScriptText(SAY_MINER_SUICIDE_3, pCreature); break;
            case 3: DoScriptText(SAY_MINER_SUICIDE_4, pCreature); break;
            case 4: DoScriptText(SAY_MINER_SUICIDE_5, pCreature); break;
            case 5: DoScriptText(SAY_MINER_SUICIDE_6, pCreature); break;
            case 6: DoScriptText(SAY_MINER_SUICIDE_7, pCreature); break;
            case 7: DoScriptText(SAY_MINER_SUICIDE_8, pCreature); break;
            }

            pCreature->SetWalk(false);
            switch (urand(0, 2))
            {
            case 0:
                pCreature->GetMotionMaster()->MovePoint(0, afPointSlaveSuicide1[0], afPointSlaveSuicide1[1], afPointSlaveSuicide1[2]);
                break;
            case 1:
                pCreature->GetMotionMaster()->MovePoint(0, afPointSlaveSuicide2[0], afPointSlaveSuicide2[1], afPointSlaveSuicide2[2]);
                break;
            case 2:
                pCreature->GetMotionMaster()->MovePoint(0, afPointSlaveSuicide3[0], afPointSlaveSuicide3[1], afPointSlaveSuicide3[2]);
                break;
            }
            pCreature->ForcedDespawn(20000);
            break;
        }

        return true;
    }
};

void AddSC_icecrown()
{
    Script* s;
    s = new npc_squad_leader();
    s->RegisterSelf();
    s = new npc_infantry();
    s->RegisterSelf();
    s = new npc_father_kamaros();
    s->RegisterSelf();
    s = new npc_saronite_mine_slave();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_squad_leader";
    //pNewScript->GetAI = &GetAI_npc_squad_leader;
    //pNewScript->pQuestAcceptNPC = &QuestAccept_npc_squad_leader;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_infantry";
    //pNewScript->GetAI = &GetAI_npc_infantry;
    //pNewScript->RegisterSelf();
    //
    //pNewScript = new Script;
    //pNewScript->Name = "npc_father_kamaros";
    //pNewScript->GetAI = &GetAI_npc_father_kamaros;
    //pNewScript->pQuestAcceptNPC = &QuestAccept_npc_father_kamaros;
    //pNewScript->RegisterSelf();
    //
    //pNewScript = new Script;
    //pNewScript->Name = "npc_saronite_mine_slave";
    //pNewScript->pGossipHello = &GossipHello_npc_saronite_mine_slave;
    //pNewScript->pGossipSelect = &GossipSelect_npc_saronite_mine_slave;
    //pNewScript->RegisterSelf();
}
