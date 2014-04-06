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
SDName: Howling_Fjord
SD%Complete: ?
SDComment: Quest support: 11300, 11343, 11344, 11464.
SDCategory: Howling Fjord
EndScriptData */

/* ContentData
npc_ancient_male_vrykul
at_ancient_male_vrykul
npc_daegarn
npc_silvermoon_harry
npc_lich_king_village
npc_king_ymiron
EndContentData */

#include "precompiled.h"

enum
{
    SPELL_ECHO_OF_YMIRON                    = 42786,
    SPELL_SECRET_OF_WYRMSKULL               = 43458,
    QUEST_ECHO_OF_YMIRON                    = 11343,
    NPC_MALE_VRYKUL                         = 24314,
    NPC_FEMALE_VRYKUL                       = 24315,

    SAY_VRYKUL_CURSED                       = -1000635,
    EMOTE_VRYKUL_POINT                      = -1000636,
    EMOTE_VRYKUL_SOB                        = -1000637,
    SAY_VRYKUL_DISPOSE                      = -1000638,
    SAY_VRYKUL_BEG                          = -1000639,
    SAY_VRYKUL_WHAT                         = -1000640,
    SAY_VRYKUL_HIDE                         = -1000641,
};

struct MANGOS_DLL_DECL npc_ancient_male_vrykulAI : public ScriptedAI
{
    npc_ancient_male_vrykulAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    bool m_bEventInProgress;
    uint32 m_uiPhase;
    uint32 m_uiPhaseTimer;

    void Reset() override
    {
        m_bEventInProgress = false;
        m_uiPhase = 0;
        m_uiPhaseTimer = 0;
    }

    void StartEvent()
    {
        if (m_bEventInProgress)
            return;

        m_bEventInProgress = true;
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_bEventInProgress)
            return;

        if (m_uiPhaseTimer < uiDiff)
            m_uiPhaseTimer = 5000;
        else
        {
            m_uiPhaseTimer -= uiDiff;
            return;
        }

        Creature* pFemale = GetClosestCreatureWithEntry(m_creature, NPC_FEMALE_VRYKUL, 10.0f);

        switch (m_uiPhase)
        {
            case 0:
                DoScriptText(SAY_VRYKUL_CURSED, m_creature);
                DoScriptText(EMOTE_VRYKUL_POINT, m_creature);
                break;
            case 1:
                if (pFemale)
                    DoScriptText(EMOTE_VRYKUL_SOB, pFemale);
                DoScriptText(SAY_VRYKUL_DISPOSE, m_creature);
                break;
            case 2:
                if (pFemale)
                    DoScriptText(SAY_VRYKUL_BEG, pFemale);
                break;
            case 3:
                DoScriptText(SAY_VRYKUL_WHAT, m_creature);
                break;
            case 4:
                if (pFemale)
                    DoScriptText(SAY_VRYKUL_HIDE, pFemale);
                break;
            case 5:
                DoCastSpellIfCan(m_creature, SPELL_SECRET_OF_WYRMSKULL);
                break;
            case 6:
                Reset();
                return;
        }

        ++m_uiPhase;
    }
};

CreatureAI* GetAI_npc_ancient_male_vrykul(Creature* pCreature)
{
    return new npc_ancient_male_vrykulAI(pCreature);
}

bool AreaTrigger_at_ancient_male_vrykul(Player* pPlayer, AreaTriggerEntry const* /*pAt*/)
{
    if (pPlayer->isAlive() && pPlayer->GetQuestStatus(QUEST_ECHO_OF_YMIRON) == QUEST_STATUS_INCOMPLETE &&
            pPlayer->HasAura(SPELL_ECHO_OF_YMIRON))
    {
        if (Creature* pCreature = GetClosestCreatureWithEntry(pPlayer, NPC_MALE_VRYKUL, 20.0f))
        {
            if (npc_ancient_male_vrykulAI* pVrykulAI = dynamic_cast<npc_ancient_male_vrykulAI*>(pCreature->AI()))
                pVrykulAI->StartEvent();
        }
    }

    return true;
}

/*######
## npc_daegarn
######*/

enum
{
    QUEST_DEFEAT_AT_RING            = 11300,

    NPC_FIRJUS                      = 24213,
    NPC_JLARBORN                    = 24215,
    NPC_YOROS                       = 24214,
    NPC_OLUF                        = 23931,

    NPC_PRISONER_1                  = 24253,                // looks the same but has different abilities
    NPC_PRISONER_2                  = 24254,
    NPC_PRISONER_3                  = 24255,
};

static float afSummon[] = {838.81f, -4678.06f, -94.182f};
static float afCenter[] = {801.88f, -4721.87f, -96.143f};

// TODO: make prisoners help (unclear if summoned or using npc's from surrounding cages (summon inside small cages?))
struct MANGOS_DLL_DECL npc_daegarnAI : public ScriptedAI
{
    npc_daegarnAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    bool m_bEventInProgress;
    ObjectGuid m_playerGuid;

    void Reset() override
    {
        m_bEventInProgress = false;
        m_playerGuid.Clear();
    }

    void StartEvent(Player* pPlayer)
    {
        if (m_bEventInProgress)
            return;

        m_playerGuid = pPlayer->GetObjectGuid();

        SummonGladiator(NPC_FIRJUS);
    }

    void JustSummoned(Creature* pSummon) override
    {
        if (Player* pPlayer = m_creature->GetMap()->GetPlayer(m_playerGuid))
        {
            if (pPlayer->isAlive())
            {
                pSummon->SetWalk(false);
                pSummon->GetMotionMaster()->MovePoint(0, afCenter[0], afCenter[1], afCenter[2]);
                return;
            }
        }

        Reset();
    }

    void SummonGladiator(uint32 uiEntry)
    {
        m_creature->SummonCreature(uiEntry, afSummon[0], afSummon[1], afSummon[2], 0.0f, TEMPSUMMON_TIMED_OOC_DESPAWN, 20 * IN_MILLISECONDS);
    }

    void SummonedMovementInform(Creature* pSummoned, uint32 /*uiMotionType*/, uint32 /*uiPointId*/) override
    {
        Player* pPlayer = m_creature->GetMap()->GetPlayer(m_playerGuid);

        // could be group, so need additional here.
        if (!pPlayer || !pPlayer->isAlive())
        {
            Reset();
            return;
        }

        if (pSummoned->IsWithinDistInMap(pPlayer, 75.0f))   // ~the radius of the ring
            pSummoned->AI()->AttackStart(pPlayer);
    }

    void SummonedCreatureDespawn(Creature* pSummoned) override
    {
        uint32 uiEntry = 0;

        // will eventually reset the event if something goes wrong
        switch (pSummoned->GetEntry())
        {
            case NPC_FIRJUS:    uiEntry = NPC_JLARBORN; break;
            case NPC_JLARBORN:  uiEntry = NPC_YOROS;    break;
            case NPC_YOROS:     uiEntry = NPC_OLUF;     break;
            case NPC_OLUF:      Reset();                return;
        }

        SummonGladiator(uiEntry);
    }
};

bool QuestAccept_npc_daegarn(Player* pPlayer, Creature* pCreature, const Quest* pQuest)
{
    if (pQuest->GetQuestId() == QUEST_DEFEAT_AT_RING)
    {
        if (npc_daegarnAI* pDaegarnAI = dynamic_cast<npc_daegarnAI*>(pCreature->AI()))
            pDaegarnAI->StartEvent(pPlayer);
    }

    return true;
}

CreatureAI* GetAI_npc_daegarn(Creature* pCreature)
{
    return new npc_daegarnAI(pCreature);
}

/*######
## npc_silvermoon_harry
######*/

enum
{
    QUEST_GAMBLING_DEBT         = 11464,

    SAY_AGGRO                   = -1000603,
    SAY_BEATEN                  = -1000604,

    GOSSIP_ITEM_GAMBLING_DEBT   = -3000101,
    GOSSIP_ITEM_PAYING          = -3000102,

    SPELL_BLAST_WAVE            = 15091,
    SPELL_SCORCH                = 50183,

    ITEM_HARRY_DEBT             = 34115,
    FACTION_HOSTILE_SH          = 90,                       // guessed, possibly not correct
};

struct MANGOS_DLL_DECL npc_silvermoon_harryAI : public ScriptedAI
{
    npc_silvermoon_harryAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    bool m_bHarryBeaten;
    uint32 m_uiBlastWaveTimer;
    uint32 m_uiScorchTimer;
    uint32 m_uiResetBeatenTimer;

    void Reset() override
    {
        m_bHarryBeaten = false;

        // timers guessed
        m_uiScorchTimer = 5 * IN_MILLISECONDS;
        m_uiBlastWaveTimer = 7 * IN_MILLISECONDS;

        m_uiResetBeatenTimer = MINUTE * IN_MILLISECONDS;
    }

    void AttackedBy(Unit* pAttacker) override
    {
        if (m_creature->getVictim())
            return;

        if (m_creature->IsHostileTo(pAttacker))
            AttackStart(pAttacker);
    }

    void DamageTaken(Unit* pDoneBy, uint32& uiDamage) override
    {
        if (uiDamage > m_creature->GetHealth() || (m_creature->GetHealth() - uiDamage) * 100 / m_creature->GetMaxHealth() < 20)
        {
            if (Player* pPlayer = pDoneBy->GetCharmerOrOwnerPlayerOrPlayerItself())
            {
                if (!m_bHarryBeaten && pPlayer->GetQuestStatus(QUEST_GAMBLING_DEBT) == QUEST_STATUS_INCOMPLETE)
                {
                    uiDamage = 0;                           // Take 0 damage

                    m_creature->RemoveAllAurasOnDeath();
                    m_creature->DeleteThreatList();
                    m_creature->CombatStop(true);

                    DoScriptText(SAY_BEATEN, m_creature);
                    m_bHarryBeaten = true;
                }
            }
        }
    }

    bool IsBeaten()
    {
        return m_bHarryBeaten;
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (m_bHarryBeaten)
        {
            if (m_uiResetBeatenTimer < uiDiff)
                EnterEvadeMode();
            else
                m_uiResetBeatenTimer -= uiDiff;
        }

        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiScorchTimer < uiDiff)
        {
            DoCastSpellIfCan(m_creature->getVictim(), SPELL_SCORCH);
            m_uiScorchTimer = 10 * IN_MILLISECONDS;
        }
        else
            m_uiScorchTimer -= uiDiff;

        if (m_uiBlastWaveTimer < uiDiff)
        {
            DoCastSpellIfCan(m_creature, SPELL_BLAST_WAVE);
            m_uiBlastWaveTimer = 50 * IN_MILLISECONDS;
        }
        else
            m_uiBlastWaveTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_silvermoon_harry(Creature* pCreature)
{
    return new npc_silvermoon_harryAI(pCreature);
}

bool GossipHello_npc_silvermoon_harry(Player* pPlayer, Creature* pCreature)
{
    if (pCreature->isQuestGiver())
        pPlayer->PrepareQuestMenu(pCreature->GetObjectGuid());

    if (pCreature->isVendor())
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, GOSSIP_TEXT_BROWSE_GOODS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);

    if (pPlayer->GetQuestStatus(QUEST_GAMBLING_DEBT) == QUEST_STATUS_INCOMPLETE)
    {
        if (npc_silvermoon_harryAI* pHarryAI = dynamic_cast<npc_silvermoon_harryAI*>(pCreature->AI()))
        {
            if (!pHarryAI->IsBeaten())
                pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_GAMBLING_DEBT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            else
                pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_PAYING, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
        }
    }

    pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetObjectGuid());

    return true;
}

bool GossipSelect_npc_silvermoon_harry(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
{
    switch (uiAction)
    {
        case GOSSIP_ACTION_TRADE:
            pPlayer->SEND_VENDORLIST(pCreature->GetObjectGuid());
            break;
        case GOSSIP_ACTION_INFO_DEF+1:
            pPlayer->CLOSE_GOSSIP_MENU();

            DoScriptText(SAY_AGGRO, pCreature, pPlayer);
            pCreature->SetFactionTemporary(FACTION_HOSTILE_SH, TEMPFACTION_RESTORE_RESPAWN | TEMPFACTION_RESTORE_COMBAT_STOP);
            pCreature->AI()->AttackStart(pPlayer);
            break;
        case GOSSIP_ACTION_INFO_DEF+2:
            if (!pPlayer->HasItemCount(ITEM_HARRY_DEBT, 1))
            {
                if (Item* pItem = pPlayer->StoreNewItemInInventorySlot(ITEM_HARRY_DEBT, 1))
                {
                    pPlayer->SendNewItem(pItem, 1, true, false);
                    pPlayer->CLOSE_GOSSIP_MENU();
                    pCreature->AI()->EnterEvadeMode();
                }
            }
            break;
    }

    return true;
}

/*######
## npc_lich_king_village
######*/

enum
{
    EMOTE_LICH_KING_FACE            = -1000920,
    SAY_LICH_KING_1                 = -1000921,
    SAY_PREPARE                     = -1000922,
    SAY_LICH_KING_2                 = -1000923,
    SAY_LICH_KING_3                 = -1000924,
    SAY_LICH_KING_4                 = -1000925,
    SAY_LICH_KING_5                 = -1000926,
    SAY_PERSISTANCE                 = -1000927,

    SPELL_GRASP_OF_THE_LICH_KING    = 43489,
    SPELL_MAGNETIC_PULL             = 29661,
    SPELL_WRATH_LICH_KING_FIRST     = 43488,
    SPELL_WRATH_LICH_KING           = 50156,

    NPC_VALKYR_SOULCLAIMER          = 24327,
    NPC_LICH_KING_WYRMSKULL         = 24248,

    QUEST_ID_LK_FLAG                = 12485,            // Server side dummy quest
};

static const DialogueEntry aLichDialogue[] =
{
    // first time dialogue only
    {EMOTE_LICH_KING_FACE,          NPC_LICH_KING_WYRMSKULL, 4000},
    {QUEST_ID_LK_FLAG,              0,                       3000},
    {SAY_LICH_KING_1,               NPC_LICH_KING_WYRMSKULL, 20000},
    {NPC_VALKYR_SOULCLAIMER,        0,                       4000},
    {SAY_LICH_KING_2,               NPC_LICH_KING_WYRMSKULL, 10000},
    {SAY_LICH_KING_3,               NPC_LICH_KING_WYRMSKULL, 25000},
    {SAY_LICH_KING_4,               NPC_LICH_KING_WYRMSKULL, 25000},
    {SAY_LICH_KING_5,               NPC_LICH_KING_WYRMSKULL, 20000},
    {SPELL_WRATH_LICH_KING_FIRST,   0,                       10000},
    {NPC_LICH_KING_WYRMSKULL,       0,                       0},
    // if the player persists...
    {SAY_PERSISTANCE,               NPC_LICH_KING_WYRMSKULL, 15000},
    {SPELL_WRATH_LICH_KING,         0,                       10000},
    {NPC_LICH_KING_WYRMSKULL,       0,                       0},
    {0, 0, 0},
};

struct MANGOS_DLL_DECL npc_lich_king_villageAI : public ScriptedAI, private DialogueHelper
{
    npc_lich_king_villageAI(Creature* pCreature) : ScriptedAI(pCreature),
        DialogueHelper(aLichDialogue)
    {
        Reset();
    }

    ObjectGuid m_pHeldPlayer;
    bool m_bEventInProgress;

    void Reset() override
    {
        m_bEventInProgress = false;
    }

    void JustDidDialogueStep(int32 iEntry) override
    {
        switch (iEntry)
        {
            case QUEST_ID_LK_FLAG:
                m_creature->HandleEmote(EMOTE_ONESHOT_LAUGH);
                break;
            case NPC_VALKYR_SOULCLAIMER:
                if (Creature* pCreature = GetClosestCreatureWithEntry(m_creature, NPC_VALKYR_SOULCLAIMER, 20.0f))
                    DoScriptText(SAY_PREPARE, pCreature);
                break;
            case SPELL_WRATH_LICH_KING_FIRST:
                if (Player* pPlayer = m_creature->GetMap()->GetPlayer(m_pHeldPlayer))
                {
                    DoCastSpellIfCan(pPlayer, SPELL_WRATH_LICH_KING_FIRST);
                    // handle spell scriptEffect in the script
                    m_creature->DealDamage(pPlayer, pPlayer->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                }
                break;
            case SPELL_WRATH_LICH_KING:
                if (Player* pPlayer = m_creature->GetMap()->GetPlayer(m_pHeldPlayer))
                {
                    DoCastSpellIfCan(pPlayer, SPELL_WRATH_LICH_KING);
                    // handle spell scriptEffect in the script
                    m_creature->DealDamage(pPlayer, pPlayer->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                }
                break;
            case NPC_LICH_KING_WYRMSKULL:
                EnterEvadeMode();
                break;
        }
    }

    void MoveInLineOfSight(Unit* pWho) override
    {
        if (!m_bEventInProgress && pWho->GetTypeId() == TYPEID_PLAYER)
        {
            if (pWho->isAlive() && m_creature->IsWithinDistInMap(pWho, 15.0) && pWho->HasAura(SPELL_ECHO_OF_YMIRON))
            {
                m_pHeldPlayer = pWho->GetObjectGuid();
                m_bEventInProgress = true;

                DoCastSpellIfCan(pWho, SPELL_MAGNETIC_PULL, CAST_TRIGGERED);
                DoCastSpellIfCan(pWho, SPELL_GRASP_OF_THE_LICH_KING, CAST_TRIGGERED);

                if (((Player*)pWho)->GetQuestStatus(QUEST_ID_LK_FLAG) == QUEST_STATUS_COMPLETE)
                    StartNextDialogueText(SAY_PERSISTANCE);
                else
                    StartNextDialogueText(EMOTE_LICH_KING_FACE);
            }
        }
    }

    Creature* GetSpeakerByEntry(uint32 uiEntry) override
    {
        if (uiEntry == NPC_LICH_KING_WYRMSKULL)
            return m_creature;

        return NULL;
    }

    void UpdateAI(const uint32 uiDiff) override { DialogueUpdate(uiDiff); }
};

CreatureAI* GetAI_npc_lich_king_village(Creature* pCreature)
{
    return new npc_lich_king_villageAI(pCreature);
}

/*######
## npc_king_ymiron
######*/

enum
{
    EMOTE_KING_SILENCE                      = -1000928,
    SAY_KING_YMIRON_SPEECH_1                = -1000929,
    SAY_KING_YMIRON_SPEECH_2                = -1000930,
    EMOTE_YMIRON_CROWD_1                    = -1000931,
    SAY_KING_YMIRON_SPEECH_3                = -1000932,
    SAY_KING_YMIRON_SPEECH_4                = -1000933,
    SAY_KING_YMIRON_SPEECH_5                = -1000934,
    SAY_KING_YMIRON_SPEECH_6                = -1000935,
    SAY_KING_YMIRON_SPEECH_7                = -1000936,
    EMOTE_YMIRON_CROWD_2                    = -1000937,
    SAY_KING_YMIRON_SPEECH_8                = -1000938,
    EMOTE_YMIRON_CROWD_3                    = -1000939,
    SAY_KING_YMIRON_SPEECH_9                = -1000940,

    SPELL_ECHO_OF_YMIRON_NIFFLEVAR          = 43466,
    SPELL_SECRETS_OF_NIFFLEVAR              = 43468,

    NPC_CITIZEN_OF_NIFFLEVAR_MALE           = 24322,
    NPC_CITIZEN_OF_NIFFLEVAR_FEMALE         = 24323,
    NPC_KING_YMIRON                         = 24321,

    QUEST_ID_ANGUISH_OF_NIFFLEVAR           = 11344,

    MAX_CROWD_TEXT_ENTRIES                  = 7
};

static const DialogueEntry aNifflevarDialogue[] =
{
    {EMOTE_KING_SILENCE,            NPC_KING_YMIRON,    3000},
    {SAY_KING_YMIRON_SPEECH_1,      NPC_KING_YMIRON,    5000},
    {SAY_KING_YMIRON_SPEECH_2,      NPC_KING_YMIRON,    2000},
    {EMOTE_YMIRON_CROWD_1,          NPC_KING_YMIRON,    5000},
    {SAY_KING_YMIRON_SPEECH_3,      NPC_KING_YMIRON,    10000},
    {SAY_KING_YMIRON_SPEECH_4,      NPC_KING_YMIRON,    9000},
    {SAY_KING_YMIRON_SPEECH_5,      NPC_KING_YMIRON,    7000},
    {SAY_KING_YMIRON_SPEECH_6,      NPC_KING_YMIRON,    5000},
    {SAY_KING_YMIRON_SPEECH_7,      NPC_KING_YMIRON,    9000},
    {EMOTE_YMIRON_CROWD_2,          NPC_KING_YMIRON,    5000},
    {SAY_KING_YMIRON_SPEECH_8,      NPC_KING_YMIRON,    8000},
    {EMOTE_YMIRON_CROWD_3,          NPC_KING_YMIRON,    4000},
    {SAY_KING_YMIRON_SPEECH_9,      NPC_KING_YMIRON,    10000},
    {SPELL_SECRETS_OF_NIFFLEVAR,    0,                  10000},
    {QUEST_ID_ANGUISH_OF_NIFFLEVAR, 0,                  0},
    {0, 0, 0},
};

static const int32 aRandomTextEntries[MAX_CROWD_TEXT_ENTRIES] = { -1000941, -1000942, -1000943, -1000944, -1000945, -1000946, -1000947};

struct MANGOS_DLL_DECL npc_king_ymironAI : public ScriptedAI, private DialogueHelper
{
    npc_king_ymironAI(Creature* pCreature) : ScriptedAI(pCreature),
        DialogueHelper(aNifflevarDialogue)
    {
        Reset();
    }

    uint32 m_uiCrowdSpeechTimer;

    bool m_bEventInProgress;
    bool m_bEventInit;

    GuidList m_lCrowdGuidList;

    void Reset() override
    {
        m_uiCrowdSpeechTimer = 0;
        m_bEventInit = false;
        m_bEventInProgress = false;
        m_lCrowdGuidList.clear();
    }

    void MoveInLineOfSight(Unit* pWho) override
    {
        if (!m_bEventInit && pWho->GetTypeId() == TYPEID_PLAYER)
        {
            // Get all the citizen around the king for future use
            if (pWho->isAlive() && m_creature->IsWithinDistInMap(pWho, 60.0) && ((Player*)pWho)->GetQuestStatus(QUEST_ID_ANGUISH_OF_NIFFLEVAR) == QUEST_STATUS_INCOMPLETE
                && pWho->HasAura(SPELL_ECHO_OF_YMIRON_NIFFLEVAR))
            {
                std::list<Creature*> lCrowdList;
                GetCreatureListWithEntryInGrid(lCrowdList, m_creature, NPC_CITIZEN_OF_NIFFLEVAR_MALE, 60.0f);
                GetCreatureListWithEntryInGrid(lCrowdList, m_creature, NPC_CITIZEN_OF_NIFFLEVAR_FEMALE, 60.0f);

                for (std::list<Creature*>::const_iterator itr = lCrowdList.begin(); itr != lCrowdList.end(); ++itr)
                    m_lCrowdGuidList.push_back( (*itr)->GetObjectGuid() );

                m_uiCrowdSpeechTimer = 1000;
                m_bEventInit = true;
            }
        }
    }

    void JustDidDialogueStep(int32 iEntry) override
    {
        switch (iEntry)
        {
            case SPELL_SECRETS_OF_NIFFLEVAR:
                DoCastSpellIfCan(m_creature, SPELL_SECRETS_OF_NIFFLEVAR);
                break;
            case QUEST_ID_ANGUISH_OF_NIFFLEVAR:
                EnterEvadeMode();
                break;
        }
    }

    Creature* GetSpeakerByEntry(uint32 uiEntry) override
    {
        if (uiEntry == NPC_KING_YMIRON)
            return m_creature;

        return NULL;
    }

    void ReceiveAIEvent(AIEventType eventType, Creature* /*pSender*/, Unit* pInvoker, uint32 /*uiMiscValue*/) override
    {
        if (eventType == AI_EVENT_CUSTOM_A && pInvoker->GetTypeId() == TYPEID_PLAYER)
        {
            if (m_bEventInProgress)
                return;

            StartNextDialogueText(EMOTE_KING_SILENCE);
            m_uiCrowdSpeechTimer = 0;
            m_bEventInProgress = true;
        }
    }

    ObjectGuid SelectRandomCrowdNpc()
    {
        if (m_lCrowdGuidList.empty())
            return ObjectGuid();

        GuidList::iterator iter = m_lCrowdGuidList.begin();
        advance(iter, urand(0, m_lCrowdGuidList.size() - 1));

        return *iter;
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        DialogueUpdate(uiDiff);

        if (m_uiCrowdSpeechTimer)
        {
            if (m_uiCrowdSpeechTimer <= uiDiff)
            {
                // only 15% chance to yell (guessed)
                if (roll_chance_i(15))
                {
                    if (Creature* pCitizen = m_creature->GetMap()->GetCreature(SelectRandomCrowdNpc()))
                        DoScriptText(aRandomTextEntries[urand(0, MAX_CROWD_TEXT_ENTRIES - 1)], pCitizen);
                }

                m_uiCrowdSpeechTimer = 1000;
            }
            else
                m_uiCrowdSpeechTimer -= uiDiff;
        }
    }
};

CreatureAI* GetAI_npc_king_ymiron(Creature* pCreature)
{
    return new npc_king_ymironAI(pCreature);
}

bool AreaTrigger_at_nifflevar(Player* pPlayer, AreaTriggerEntry const* pAt)
{
    if (pPlayer->isAlive() && pPlayer->GetQuestStatus(QUEST_ID_ANGUISH_OF_NIFFLEVAR) == QUEST_STATUS_INCOMPLETE && pPlayer->HasAura(SPELL_ECHO_OF_YMIRON_NIFFLEVAR))
    {
        if (Creature* pCreature = GetClosestCreatureWithEntry(pPlayer, NPC_KING_YMIRON, 30.0f))
            pCreature->AI()->SendAIEvent(AI_EVENT_CUSTOM_A, pPlayer, pCreature);

        return true;
    }

    return false;
}

void AddSC_howling_fjord()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "npc_ancient_male_vrykul";
    pNewScript->GetAI = &GetAI_npc_ancient_male_vrykul;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "at_ancient_male_vrykul";
    pNewScript->pAreaTrigger = &AreaTrigger_at_ancient_male_vrykul;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_daegarn";
    pNewScript->GetAI = &GetAI_npc_daegarn;
    pNewScript->pQuestAcceptNPC = &QuestAccept_npc_daegarn;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_silvermoon_harry";
    pNewScript->GetAI = &GetAI_npc_silvermoon_harry;
    pNewScript->pGossipHello = &GossipHello_npc_silvermoon_harry;
    pNewScript->pGossipSelect = &GossipSelect_npc_silvermoon_harry;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_lich_king_village";
    pNewScript->GetAI = &GetAI_npc_lich_king_village;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_king_ymiron";
    pNewScript->GetAI = &GetAI_npc_king_ymiron;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "at_nifflevar";
    pNewScript->pAreaTrigger = &AreaTrigger_at_nifflevar;
    pNewScript->RegisterSelf();
}
