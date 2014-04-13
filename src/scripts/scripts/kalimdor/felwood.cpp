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
SDName: Felwood
SD%Complete: 95
SDComment: Quest support: 4506, 7603, 7603 (Summon Pollo Grande)
SDCategory: Felwood
EndScriptData */

/* ContentData
npc_kitten
npc_niby_the_almighty
npc_kroshius
EndContentData */

#include "precompiled.h"
#include "follower_ai.h"
#include "ObjectMgr.h"

/*####
# npc_kitten
####*/

enum
{
    EMOTE_SAB_JUMP              = -1000541,
    EMOTE_SAB_FOLLOW            = -1000542,

    SPELL_CORRUPT_SABER_VISUAL  = 16510,

    QUEST_CORRUPT_SABER         = 4506,
    NPC_WINNA                   = 9996,
    NPC_CORRUPT_SABER           = 10042
};

#define GOSSIP_ITEM_RELEASE     "I want to release the corrupted saber to Winna."

struct MANGOS_DLL_DECL npc_kittenAI : public FollowerAI
{
    npc_kittenAI(Creature* pCreature) : FollowerAI(pCreature)
    {
        if (pCreature->GetOwner() && pCreature->GetOwner()->GetTypeId() == TYPEID_PLAYER)
        {
            StartFollow((Player*)pCreature->GetOwner());
            SetFollowPaused(true);
            DoScriptText(EMOTE_SAB_JUMP, m_creature);

            pCreature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

            // find a decent way to move to center of moonwell
        }

        m_uiMoonwellCooldown = 7500;
        Reset();
    }

    uint32 m_uiMoonwellCooldown;

    void Reset() override { }

    void MoveInLineOfSight(Unit* pWho) override
    {
        // should not have npcflag by default, so set when expected
        if (!m_creature->getVictim() && !m_creature->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP) && HasFollowState(STATE_FOLLOW_INPROGRESS) && pWho->GetEntry() == NPC_WINNA)
        {
            if (m_creature->IsWithinDistInMap(pWho, INTERACTION_DISTANCE))
                m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        }
    }

    void UpdateFollowerAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
        {
            if (HasFollowState(STATE_FOLLOW_PAUSED))
            {
                if (m_uiMoonwellCooldown < uiDiff)
                {
                    m_creature->CastSpell(m_creature, SPELL_CORRUPT_SABER_VISUAL, false);
                    SetFollowPaused(false);
                }
                else
                    m_uiMoonwellCooldown -= uiDiff;
            }

            return;
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_kitten(Creature* pCreature)
{
    return new npc_kittenAI(pCreature);
}

bool EffectDummyCreature_npc_kitten(Unit* /*pCaster*/, uint32 uiSpellId, SpellEffectIndex uiEffIndex, Creature* pCreatureTarget, ObjectGuid /*originalCasterGuid*/)
{
    // always check spellid and effectindex
    if (uiSpellId == SPELL_CORRUPT_SABER_VISUAL && uiEffIndex == EFFECT_INDEX_0)
    {
        // Not nice way, however using UpdateEntry will not be correct.
        if (const CreatureInfo* pTemp = GetCreatureTemplateStore(NPC_CORRUPT_SABER))
        {
            pCreatureTarget->SetEntry(pTemp->Entry);
            pCreatureTarget->SetDisplayId(Creature::ChooseDisplayId(pTemp));
            pCreatureTarget->SetName(pTemp->Name);
            pCreatureTarget->SetFloatValue(OBJECT_FIELD_SCALE_X, pTemp->Scale);
        }

        if (Unit* pOwner = pCreatureTarget->GetOwner())
            DoScriptText(EMOTE_SAB_FOLLOW, pCreatureTarget, pOwner);

        // always return true when we are handling this spell and effect
        return true;
    }
    return false;
}

bool GossipHello_npc_corrupt_saber(Player* pPlayer, Creature* pCreature)
{
    if (pPlayer->GetQuestStatus(QUEST_CORRUPT_SABER) == QUEST_STATUS_INCOMPLETE)
    {
        if (GetClosestCreatureWithEntry(pCreature, NPC_WINNA, INTERACTION_DISTANCE))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_RELEASE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
    }

    pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetObjectGuid());
    return true;
}

bool GossipSelect_npc_corrupt_saber(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
{
    if (uiAction == GOSSIP_ACTION_INFO_DEF + 1)
    {
        pPlayer->CLOSE_GOSSIP_MENU();

        if (npc_kittenAI* pKittenAI = dynamic_cast<npc_kittenAI*>(pCreature->AI()))
            pKittenAI->SetFollowComplete();

        pPlayer->AreaExploredOrEventHappens(QUEST_CORRUPT_SABER);
    }

    return true;
}

/*######
## npc_niby_the_almighty (summons el pollo grande)
######*/

enum
{
    QUEST_KROSHIUS     = 7603,

    NPC_IMPSY          = 14470,

    SPELL_SUMMON_POLLO = 23056,

    SAY_NIBY_1         = -1000566,
    SAY_NIBY_2         = -1000567,
    EMOTE_IMPSY_1      = -1000568,
    SAY_IMPSY_1        = -1000569,
    SAY_NIBY_3         = -1000570
};

struct MANGOS_DLL_DECL npc_niby_the_almightyAI : public ScriptedAI
{
    npc_niby_the_almightyAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    uint32 m_uiSummonTimer;
    uint8  m_uiSpeech;

    bool m_bEventStarted;

    void Reset() override
    {
        m_uiSummonTimer = 500;
        m_uiSpeech = 0;

        m_bEventStarted = false;
    }

    void StartEvent()
    {
        Reset();
        m_bEventStarted = true;
        m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (m_bEventStarted)
        {
            if (m_uiSummonTimer <= uiDiff)
            {
                switch (m_uiSpeech)
                {
                    case 1:
                        m_creature->GetMotionMaster()->Clear();
                        m_creature->GetMotionMaster()->MovePoint(0, 5407.19f, -753.00f, 350.82f);
                        m_uiSummonTimer = 6200;
                        break;
                    case 2:
                        m_creature->SetFacingTo(1.2f);
                        DoScriptText(SAY_NIBY_1, m_creature);
                        m_uiSummonTimer = 3000;
                        break;
                    case 3:
                        DoScriptText(SAY_NIBY_2, m_creature);
                        DoCastSpellIfCan(m_creature, SPELL_SUMMON_POLLO);
                        m_uiSummonTimer = 2000;
                        break;
                    case 4:
                        if (Creature* pImpsy = GetClosestCreatureWithEntry(m_creature, NPC_IMPSY, 20.0))
                        {
                            DoScriptText(EMOTE_IMPSY_1, pImpsy);
                            DoScriptText(SAY_IMPSY_1, pImpsy);
                            m_uiSummonTimer = 2500;
                        }
                        else
                        {
                            // Skip Speech 5
                            m_uiSummonTimer = 40000;
                            ++m_uiSpeech;
                        }
                        break;
                    case 5:
                        DoScriptText(SAY_NIBY_3, m_creature);
                        m_uiSummonTimer = 40000;
                        break;
                    case 6:
                        m_creature->GetMotionMaster()->MoveTargetedHome();
                        m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                        m_bEventStarted = false;
                        break;
                }
                ++m_uiSpeech;
            }
            else
                m_uiSummonTimer -= uiDiff;
        }
    }
};

CreatureAI* GetAI_npc_niby_the_almighty(Creature* pCreature)
{
    return new npc_niby_the_almightyAI(pCreature);
}

bool QuestRewarded_npc_niby_the_almighty(Player* /*pPlayer*/, Creature* pCreature, Quest const* pQuest)
{
    if (pQuest->GetQuestId() == QUEST_KROSHIUS)
    {
        if (npc_niby_the_almightyAI* pNibyAI = dynamic_cast<npc_niby_the_almightyAI*>(pCreature->AI()))
        {
            pNibyAI->StartEvent();
        }
    }
    return true;
}

/*######
## npc_kroshius
######*/

enum
{
    NPC_KROSHIUS            = 14467,
    SPELL_KNOCKBACK         = 10101,
    SAY_KROSHIUS_REVIVE     = -1000589,
    EVENT_KROSHIUS_REVIVE   = 8328,
    FACTION_HOSTILE         = 16,
};

struct MANGOS_DLL_DECL npc_kroshiusAI : public ScriptedAI
{
    npc_kroshiusAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_uiPhase = 0;
        Reset();
    }

    ObjectGuid m_playerGuid;
    uint32 m_uiKnockBackTimer;
    uint32 m_uiPhaseTimer;

    uint8 m_uiPhase;

    void Reset() override
    {
        m_uiKnockBackTimer = urand(5000, 8000);
        m_playerGuid.Clear();

        if (!m_uiPhase)
            m_creature->SetStandState(UNIT_STAND_STATE_DEAD);
    }

    void DoRevive(Player* pSource)
    {
        if (m_uiPhase)
            return;

        m_uiPhase = 1;
        m_uiPhaseTimer = 2500;
        m_playerGuid = pSource->GetObjectGuid();

        // TODO: A visual Flame Circle around the mob still missing
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        m_uiPhase = 0;
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_uiPhase)
            return;

        if (m_uiPhase < 4)
        {
            if (m_uiPhaseTimer < uiDiff)
            {
                switch (m_uiPhase)
                {
                    case 1:                                 // Revived
                        m_creature->SetStandState(UNIT_STAND_STATE_STAND);
                        m_uiPhaseTimer = 1000;
                        break;
                    case 2:
                        DoScriptText(SAY_KROSHIUS_REVIVE, m_creature);
                        m_uiPhaseTimer = 3500;
                        break;
                    case 3:                                 // Attack
                        m_creature->SetFactionTemporary(FACTION_HOSTILE, TEMPFACTION_RESTORE_COMBAT_STOP | TEMPFACTION_RESTORE_RESPAWN |  TEMPFACTION_TOGGLE_OOC_NOT_ATTACK | TEMPFACTION_TOGGLE_PASSIVE);
                        if (Player* pPlayer = m_creature->GetMap()->GetPlayer(m_playerGuid))
                        {
                            if (m_creature->IsWithinDistInMap(pPlayer, 30.0f))
                                AttackStart(pPlayer);
                        }
                        break;
                }
                ++m_uiPhase;
            }
            else
                m_uiPhaseTimer -= uiDiff;
        }
        else
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
                return;

            if (m_uiKnockBackTimer < uiDiff)
            {
                DoCastSpellIfCan(m_creature->getVictim(), SPELL_KNOCKBACK);
                m_uiKnockBackTimer = urand(9000, 12000);
            }
            else
                m_uiKnockBackTimer -= uiDiff;

            DoMeleeAttackIfReady();
        }
    }
};

CreatureAI* GetAI_npc_kroshius(Creature* pCreature)
{
    return new npc_kroshiusAI(pCreature);
}

bool ProcessEventId_npc_kroshius(uint32 uiEventId, Object* pSource, Object* /*pTarget*/, bool /*bIsStart*/)
{
    if (uiEventId == EVENT_KROSHIUS_REVIVE)
    {
        if (pSource->GetTypeId() == TYPEID_PLAYER)
        {
            if (Creature* pKroshius = GetClosestCreatureWithEntry((Player*)pSource, NPC_KROSHIUS, 20.0f))
            {
                if (npc_kroshiusAI* pKroshiusAI = dynamic_cast<npc_kroshiusAI*>(pKroshius->AI()))
                    pKroshiusAI->DoRevive((Player*)pSource);
            }
        }

        return true;
    }
    return false;
}

void AddSC_felwood()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "npc_kitten";
    pNewScript->GetAI = &GetAI_npc_kitten;
    pNewScript->pEffectDummyNPC = &EffectDummyCreature_npc_kitten;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_corrupt_saber";
    pNewScript->pGossipHello = &GossipHello_npc_corrupt_saber;
    pNewScript->pGossipSelect = &GossipSelect_npc_corrupt_saber;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_niby_the_almighty";
    pNewScript->GetAI = &GetAI_npc_niby_the_almighty;
    pNewScript->pQuestRewardedNPC = &QuestRewarded_npc_niby_the_almighty;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_kroshius";
    pNewScript->GetAI = &GetAI_npc_kroshius;
    pNewScript->pProcessEventId = &ProcessEventId_npc_kroshius;
    pNewScript->RegisterSelf();
}
