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
SDName: Hellfire_Peninsula
SD%Complete: 100
SDComment: Quest support: 9375, 9410, 9418, 10629, 10838
SDCategory: Hellfire Peninsula
EndScriptData */

/* ContentData
npc_aeranas
npc_ancestral_wolf
npc_demoniac_scryer
npc_wounded_blood_elf
npc_fel_guard_hound
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

struct MANGOS_DLL_DECL npc_aeranasAI : public ScriptedAI
{
    npc_aeranasAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    uint32 m_uiFactionTimer;
    uint32 m_uiEnvelopingWindsTimer;
    uint32 m_uiShockTimer;

    void Reset() override
    {
        m_uiFactionTimer         = 8000;
        m_uiEnvelopingWindsTimer = 9000;
        m_uiShockTimer           = 5000;

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
                m_uiFactionTimer -= uiDiff;
        }

        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

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
            m_uiShockTimer -= uiDiff;

        if (m_uiEnvelopingWindsTimer < uiDiff)
        {
            DoCastSpellIfCan(m_creature->getVictim(), SPELL_ENVELOPING_WINDS);
            m_uiEnvelopingWindsTimer = 25000;
        }
        else
            m_uiEnvelopingWindsTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_aeranas(Creature* pCreature)
{
    return new npc_aeranasAI(pCreature);
}

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

struct MANGOS_DLL_DECL npc_ancestral_wolfAI : public npc_escortAI
{
    npc_ancestral_wolfAI(Creature* pCreature) : npc_escortAI(pCreature)
    {
        if (pCreature->GetOwner() && pCreature->GetOwner()->GetTypeId() == TYPEID_PLAYER)
            Start(false, (Player*)pCreature->GetOwner());
        else
            script_error_log("npc_ancestral_wolf can not obtain owner or owner is not a player.");

        Reset();
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
                if (pRyga && pRyga->isAlive() && !pRyga->isInCombat())
                    DoScriptText(SAY_WOLF_WELCOME, pRyga);
                break;
        }
    }
};

CreatureAI* GetAI_npc_ancestral_wolf(Creature* pCreature)
{
    return new npc_ancestral_wolfAI(pCreature);
}

/*######
## npc_demoniac_scryer
######*/

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
struct MANGOS_DLL_DECL npc_demoniac_scryerAI : public ScriptedAI
{
    npc_demoniac_scryerAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_bIsComplete = false;
        m_uiSpawnDemonTimer = 15000;
        m_uiSpawnButtressTimer = 45000;
        m_uiButtressCount = 0;
        Reset();
    }

    bool m_bIsComplete;

    uint32 m_uiSpawnDemonTimer;
    uint32 m_uiSpawnButtressTimer;
    uint32 m_uiButtressCount;

    void Reset() override {}

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
            ((Creature*)pTarget)->ForcedDespawn();
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (m_bIsComplete || !m_creature->isAlive())
            return;

        if (m_uiSpawnButtressTimer <= uiDiff)
        {
            if (m_uiButtressCount >= MAX_BUTTRESS)
            {
                m_creature->CastSpell(m_creature, SPELL_SUCKER_DESPAWN_MOB, false);

                if (m_creature->isInCombat())
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
            m_uiSpawnButtressTimer -= uiDiff;

        if (m_uiSpawnDemonTimer <= uiDiff)
        {
            DoSpawnDemon();
            m_uiSpawnDemonTimer = 15000;
        }
        else
            m_uiSpawnDemonTimer -= uiDiff;
    }
};

CreatureAI* GetAI_npc_demoniac_scryer(Creature* pCreature)
{
    return new npc_demoniac_scryerAI(pCreature);
}

bool GossipHello_npc_demoniac_scryer(Player* pPlayer, Creature* pCreature)
{
    if (npc_demoniac_scryerAI* pScryerAI = dynamic_cast<npc_demoniac_scryerAI*>(pCreature->AI()))
    {
        if (pScryerAI->m_bIsComplete)
        {
            if (pPlayer->GetQuestStatus(QUEST_DEMONIAC) == QUEST_STATUS_INCOMPLETE)
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_ATTUNE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

            pPlayer->SEND_GOSSIP_MENU(GOSSIP_TEXTID_ATTUNED, pCreature->GetObjectGuid());
            return true;
        }
    }

    pPlayer->SEND_GOSSIP_MENU(GOSSIP_TEXTID_PROTECT, pCreature->GetObjectGuid());
    return true;
}

bool GossipSelect_npc_demoniac_scryer(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
{
    if (uiAction == GOSSIP_ACTION_INFO_DEF + 1)
    {
        pPlayer->CLOSE_GOSSIP_MENU();
        pCreature->CastSpell(pPlayer, SPELL_DEMONIAC_VISITATION, false);
    }

    return true;
}

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

struct MANGOS_DLL_DECL npc_wounded_blood_elfAI : public npc_escortAI
{
    npc_wounded_blood_elfAI(Creature* pCreature) : npc_escortAI(pCreature) {Reset();}

    void WaypointReached(uint32 uiPointId) override
    {
        Player* pPlayer = GetPlayerForEscort();

        if (!pPlayer)
            return;

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

    void Reset() override { }

    void Aggro(Unit* /*pWho*/) override
    {
        if (HasEscortState(STATE_ESCORT_ESCORTING))
            DoScriptText(SAY_ELF_AGGRO, m_creature);
    }

    void JustSummoned(Creature* pSummoned) override
    {
        pSummoned->AI()->AttackStart(m_creature);
    }
};

CreatureAI* GetAI_npc_wounded_blood_elf(Creature* pCreature)
{
    return new npc_wounded_blood_elfAI(pCreature);
}

bool QuestAccept_npc_wounded_blood_elf(Player* pPlayer, Creature* pCreature, const Quest* pQuest)
{
    if (pQuest->GetQuestId() == QUEST_ROAD_TO_FALCON_WATCH)
    {
        // Change faction so mobs attack
        pCreature->SetFactionTemporary(FACTION_ESCORT_H_PASSIVE, TEMPFACTION_RESTORE_RESPAWN);

        if (npc_wounded_blood_elfAI* pEscortAI = dynamic_cast<npc_wounded_blood_elfAI*>(pCreature->AI()))
            pEscortAI->Start(false, pPlayer, pQuest);
    }

    return true;
}

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

struct MANGOS_DLL_DECL npc_fel_guard_houndAI : public ScriptedPetAI
{
    npc_fel_guard_houndAI(Creature* pCreature) : ScriptedPetAI(pCreature) { Reset(); }

    uint32 m_uiPoodadTimer;

    bool m_bIsPooActive;

    void Reset() override
    {
        m_uiPoodadTimer = 0;
        m_bIsPooActive  = false;
    }

    void MovementInform(uint32 uiMoveType, uint32 uiPointId) override
    {
        if (uiMoveType != POINT_MOTION_TYPE || !uiPointId)
            return;

        if (DoCastSpellIfCan(m_creature, SPELL_FAKE_DOG_SPART) == CAST_OK)
            m_uiPoodadTimer = 2000;
    }

    // Function to allow the boar to move to target
    void DoMoveToCorpse(Unit* pBoar)
    {
        if (!pBoar)
            return;

        m_bIsPooActive = true;
        m_creature->GetMotionMaster()->MovePoint(1, pBoar->GetPositionX(), pBoar->GetPositionY(), pBoar->GetPositionZ());
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
                m_uiPoodadTimer -= uiDiff;
        }

        if (!m_bIsPooActive)
            ScriptedPetAI::UpdateAI(uiDiff);
    }
};

CreatureAI* GetAI_npc_fel_guard_hound(Creature* pCreature)
{
    return new npc_fel_guard_houndAI(pCreature);
}

bool EffectDummyCreature_npc_fel_guard_hound(Unit* pCaster, uint32 uiSpellId, SpellEffectIndex uiEffIndex, Creature* pCreatureTarget, ObjectGuid /*originalCasterGuid*/)
{
    // always check spellid and effectindex
    if (uiSpellId == SPELL_INFORM_DOG && uiEffIndex == EFFECT_INDEX_0)
    {
        if (pCaster->GetEntry() == NPC_DERANGED_HELBOAR)
        {
            if (npc_fel_guard_houndAI* pHoundAI = dynamic_cast<npc_fel_guard_houndAI*>(pCreatureTarget->AI()))
                pHoundAI->DoMoveToCorpse(pCaster);
        }

        // always return true when we are handling this spell and effect
        return true;
    }

    return false;
}

void AddSC_hellfire_peninsula()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "npc_aeranas";
    pNewScript->GetAI = &GetAI_npc_aeranas;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_ancestral_wolf";
    pNewScript->GetAI = &GetAI_npc_ancestral_wolf;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_demoniac_scryer";
    pNewScript->GetAI = &GetAI_npc_demoniac_scryer;
    pNewScript->pGossipHello = &GossipHello_npc_demoniac_scryer;
    pNewScript->pGossipSelect = &GossipSelect_npc_demoniac_scryer;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_wounded_blood_elf";
    pNewScript->GetAI = &GetAI_npc_wounded_blood_elf;
    pNewScript->pQuestAcceptNPC = &QuestAccept_npc_wounded_blood_elf;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_fel_guard_hound";
    pNewScript->GetAI = &GetAI_npc_fel_guard_hound;
    pNewScript->pEffectDummyNPC = &EffectDummyCreature_npc_fel_guard_hound;
    pNewScript->RegisterSelf();
}
