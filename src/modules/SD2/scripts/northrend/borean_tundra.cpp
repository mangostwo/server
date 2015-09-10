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
SDName: Borean_Tundra
SD%Complete: 100
SDComment: Quest support: 11570, 11590, 11673, 11728, 11865, 11889, 11897, 11919, 11940
SDCategory: Borean Tundra
EndScriptData */

/* ContentData
npc_nesingwary_trapper
npc_sinkhole_kill_credit
npc_lurgglbr
npc_beryl_sorcerer
npc_captured_beryl_sorcerer
npc_nexus_drake_hatchling
npc_scourged_flamespitter
npc_bonker_togglevolt
EndContentData */

#include "precompiled.h"
#include "escort_ai.h"
#include "TemporarySummon.h"
#include "follower_ai.h"

/*######
## npc_nesingwary_trapper
######*/

enum
{
    NPC_NESINGWARY_TRAPPER  = 25835,
    GO_QUALITY_FUR          = 187983,

    SAY_PHRASE_1            = -1000599,
    SAY_PHRASE_2            = -1000600,
    SAY_PHRASE_3            = -1000601,
    SAY_PHRASE_4            = -1000602
};

struct npc_nesingwary_trapper : public CreatureScript
{
    npc_nesingwary_trapper() : CreatureScript("npc_nesingwary_trapper") {}

    struct npc_nesingwary_trapperAI : public ScriptedAI
    {
        npc_nesingwary_trapperAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        uint8 m_uiPhase;
        uint32 m_uiPhaseTimer;
        ObjectGuid m_playerGuid;
        ObjectGuid m_trapGuid;

        void Reset() override
        {
            m_uiPhase = 0;
            m_uiPhaseTimer = 0;
            m_playerGuid.Clear();
            m_trapGuid.Clear();
        }

        void MoveInLineOfSight(Unit* pWho) override
        {
            if (!m_uiPhase && pWho->GetTypeId() == TYPEID_PLAYER && m_creature->IsWithinDistInMap(pWho, 20.0f))
            {
                m_uiPhase = 1;
                m_uiPhaseTimer = 1000;
                m_playerGuid = pWho->GetObjectGuid();

                if (m_creature->IsTemporarySummon())
                {
                    // Get the summoner trap
                    if (GameObject* pTrap = m_creature->GetMap()->GetGameObject(((TemporarySummon*)m_creature)->GetSummonerGuid()))
                        m_trapGuid = pTrap->GetObjectGuid();
                }
            }

            ScriptedAI::MoveInLineOfSight(pWho);
        }

        void MovementInform(uint32 uiType, uint32 uiPointId) override
        {
            if (uiType != POINT_MOTION_TYPE || !uiPointId)
                return;

            if (GameObject* pTrap = m_creature->GetMap()->GetGameObject(m_trapGuid))
            {
                // respawn the Quality Fur
                if (GameObject* pGoFur = GetClosestGameObjectWithEntry(pTrap, GO_QUALITY_FUR, INTERACTION_DISTANCE))
                {
                    if (!pGoFur->isSpawned())
                    {
                        pGoFur->SetRespawnTime(10);
                        pGoFur->Refresh();
                    }
                }
            }

            m_uiPhaseTimer = 2000;
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_creature->getVictim() && m_uiPhaseTimer)
            {
                if (m_uiPhaseTimer <= uiDiff)
                {
                    switch (m_uiPhase)
                    {
                    case 1:
                        if (GameObject* pTrap = m_creature->GetMap()->GetGameObject(m_trapGuid))
                        {
                            float fX, fY, fZ;
                            pTrap->GetContactPoint(m_creature, fX, fY, fZ);

                            m_creature->SetWalk(false);
                            m_creature->GetMotionMaster()->MovePoint(1, fX, fY, fZ);
                        }
                        m_uiPhaseTimer = 0;
                        break;
                    case 2:
                        switch (urand(0, 3))
                        {
                        case 0: DoScriptText(SAY_PHRASE_1, m_creature); break;
                        case 1: DoScriptText(SAY_PHRASE_2, m_creature); break;
                        case 2: DoScriptText(SAY_PHRASE_3, m_creature); break;
                        case 3: DoScriptText(SAY_PHRASE_4, m_creature); break;
                        }
                        m_creature->HandleEmote(EMOTE_ONESHOT_LOOT);
                        m_uiPhaseTimer = 3000;
                        break;
                    case 3:
                        if (GameObject* pTrap = m_creature->GetMap()->GetGameObject(m_trapGuid))
                        {
                            pTrap->Use(m_creature);

                            if (Player* pPlayer = m_creature->GetMap()->GetPlayer(m_playerGuid))
                            {
                                if (pPlayer->IsAlive())
                                    pPlayer->KilledMonsterCredit(m_creature->GetEntry());
                            }
                        }
                        m_uiPhaseTimer = 0;
                        break;
                    }
                    ++m_uiPhase;
                }
                else
                    m_uiPhaseTimer -= uiDiff;
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_nesingwary_trapperAI(pCreature);
    }
};

/*#####
# npc_oil_stained_wolf
#####*/

enum
{
    SPELL_THROW_WOLF_BAIT           = 53326,
    SPELL_PLACE_WOLF_BAIT           = 46072,                // doesn't appear to be used for anything
    SPELL_HAS_EATEN                 = 46073,
    SPELL_SUMMON_DROPPINGS          = 46075,

    FACTION_MONSTER                 = 634,

    POINT_DEST                      = 1
};

struct npc_oil_stained_wolf : public CreatureScript
{
    npc_oil_stained_wolf() : CreatureScript("npc_oil_stained_wolf") {}

    struct npc_oil_stained_wolfAI : public ScriptedAI
    {
        npc_oil_stained_wolfAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        bool m_bCanCrapInPublic;
        uint32 m_uiPooTimer;

        void Reset() override
        {
            m_bCanCrapInPublic = false;
            m_uiPooTimer = 0;
        }

        void MovementInform(uint32 uiType, uint32 uiPointId) override
        {
            if (uiType != POINT_MOTION_TYPE)
                return;

            if (uiPointId == POINT_DEST)
            {
                DoCastSpellIfCan(m_creature, SPELL_HAS_EATEN);
                m_uiPooTimer = 4000;
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                if (m_uiPooTimer)
                {
                    if (m_uiPooTimer <= uiDiff)
                    {
                        if (m_bCanCrapInPublic)
                        {
                            DoCastSpellIfCan(m_creature, SPELL_SUMMON_DROPPINGS);
                            m_creature->GetMotionMaster()->Clear();
                            Reset();
                        }
                        else
                        {
                            m_creature->HandleEmote(EMOTE_ONESHOT_BATTLEROAR);
                            m_bCanCrapInPublic = true;
                            m_uiPooTimer = 3000;
                        }
                    }
                    else
                        m_uiPooTimer -= uiDiff;
                }

                return;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_oil_stained_wolfAI(pCreature);
    }
};

struct spell_throw_wolf_batt : public SpellScript
{
    spell_throw_wolf_batt() : SpellScript("spell_throw_wolf_batt") {}

    bool EffectDummy(Unit* pCaster, uint32 uiSpellId, SpellEffectIndex uiEffIndex, Object* pTarget, ObjectGuid /*originalCasterGuid*/) override
    {
        if (uiSpellId == SPELL_THROW_WOLF_BAIT)
        {
            Creature* pCreatureTarget = pTarget->ToCreature();
            if (uiEffIndex == EFFECT_INDEX_0 && pCreatureTarget->getFaction() != FACTION_MONSTER && !pCreatureTarget->HasAura(SPELL_HAS_EATEN))
            {
                pCreatureTarget->SetFactionTemporary(FACTION_MONSTER);
                pCreatureTarget->SetWalk(false);

                pCreatureTarget->GetMotionMaster()->MoveIdle();

                float fX, fY, fZ;
                pCaster->GetContactPoint(pCreatureTarget, fX, fY, fZ, CONTACT_DISTANCE);
                pCreatureTarget->GetMotionMaster()->MovePoint(POINT_DEST, fX, fY, fZ);
                return true;
            }
        }

        return false;
    }
};

struct aura_wolf_has_eaten : public AuraScript
{
    aura_wolf_has_eaten() : AuraScript("aura_wolf_has_eaten") {}

    bool OnDummyApply(const Aura* pAura, bool bApply) override
    {
        if (pAura->GetId() == SPELL_HAS_EATEN)
        {
            if (pAura->GetEffIndex() != EFFECT_INDEX_0)
                return false;

            if (bApply)
            {
                pAura->GetTarget()->HandleEmote(EMOTE_ONESHOT_CUSTOMSPELL01);
            }
            else
            {
                Creature* pCreature = (Creature*)pAura->GetTarget();
                pCreature->setFaction(pCreature->GetCreatureInfo()->FactionAlliance);
            }

            return true;
        }

        return false;
    }
};

/*#####
# npc_sinkhole_kill_credit
#####*/

enum
{
    SPELL_SUMMON_EXPLOSIVES_CART_FIRE   = 46799,
    SPELL_SUMMON_SCOURGE_BURROWER       = 46800,
    SPELL_COSMETIC_HUGE_EXPLOSION       = 46225,
    SPELL_CANNON_FIRE                   = 42445,
};

struct npc_sinkhole_kill_credit : public CreatureScript
{
    npc_sinkhole_kill_credit() : CreatureScript("npc_sinkhole_kill_credit") {}

    struct npc_sinkhole_kill_creditAI : public ScriptedAI
    {
        npc_sinkhole_kill_creditAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        ObjectGuid m_cartGuid;
        ObjectGuid m_wormGuid;
        uint32 m_uiCartTimer;
        uint32 m_uiCartPhase;

        void Reset() override
        {
            m_cartGuid.Clear();
            m_wormGuid.Clear();
            m_uiCartTimer = 2000;
            m_uiCartPhase = 0;
        }

        void JustSummoned(Creature* pSummoned) override
        {
            m_wormGuid = pSummoned->GetObjectGuid();
        }

        void JustSummoned(GameObject* pGo) override
        {
            // Go is not really needed, but ok to use as a check point so only one "event" can be processed at a time
            if (m_cartGuid)
                return;

            // Expecting summoned from mangos dummy effect 46797
            m_cartGuid = pGo->GetObjectGuid();
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_cartGuid)
            {
                if (m_uiCartTimer <= uiDiff)
                {
                    switch (m_uiCartPhase)
                    {
                    case 0:
                        DoCastSpellIfCan(m_creature, SPELL_SUMMON_EXPLOSIVES_CART_FIRE);
                        m_uiCartTimer = 4000;
                        break;
                    case 1:
                        // Unclear if these should be in a dummy effect or not.
                        // The order of spells are correct though.
                        DoCastSpellIfCan(m_creature, SPELL_COSMETIC_HUGE_EXPLOSION, CAST_TRIGGERED);
                        DoCastSpellIfCan(m_creature, SPELL_CANNON_FIRE, CAST_TRIGGERED);
                        break;
                    case 2:
                        DoCastSpellIfCan(m_creature, SPELL_SUMMON_SCOURGE_BURROWER);
                        m_uiCartTimer = 2000;
                        break;
                    case 3:
                        if (Creature* pWorm = m_creature->GetMap()->GetCreature(m_wormGuid))
                        {
                            pWorm->SetDeathState(JUST_DIED);
                            pWorm->SetHealth(0);
                        }
                        m_uiCartTimer = 10000;
                        break;
                    case 4:
                        if (Creature* pWorm = m_creature->GetMap()->GetCreature(m_wormGuid))
                            pWorm->RemoveCorpse();

                        Reset();
                        return;
                    }

                    ++m_uiCartPhase;
                }
                else
                    m_uiCartTimer -= uiDiff;
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_sinkhole_kill_creditAI(pCreature);
    }
};

/*######
## npc_lurgglbr
######*/

enum
{
    QUEST_ESCAPE_FROM_WINTERFIN_CAVERNS = 11570,
    GO_CAGE                             = 187369,

    SAY_START_1                         = -1000575,
    SAY_START_2                         = -1000576,
    SAY_END_1                           = -1000577,
    SAY_END_2                           = -1000578
};

struct npc_lurgglbr : public CreatureScript
{
    npc_lurgglbr() : CreatureScript("npc_lurgglbr") {}

    struct npc_lurgglbrAI : public npc_escortAI
    {
        npc_lurgglbrAI(Creature* pCreature) : npc_escortAI(pCreature)
        {
            m_uiSayTimer = 0;
            m_uiSpeech = 0;
        }

        uint32 m_uiSayTimer;
        uint8 m_uiSpeech;

        void Reset() override
        {
            if (!HasEscortState(STATE_ESCORT_ESCORTING))
            {
                m_uiSayTimer = 0;
                m_uiSpeech = 0;
            }
        }

        void JustStartedEscort() override
        {
            if (GameObject* pCage = GetClosestGameObjectWithEntry(m_creature, GO_CAGE, INTERACTION_DISTANCE))
            {
                if (pCage->GetGoState() == GO_STATE_READY)
                    pCage->Use(m_creature);
            }
        }

        void WaypointStart(uint32 uiPointId) override
        {
            switch (uiPointId)
            {
            case 1:
                if (Player* pPlayer = GetPlayerForEscort())
                    DoScriptText(SAY_START_2, m_creature, pPlayer);

                // Cage actually closes here, however it's normally determined by GO template and auto close time

                break;
            }
        }

        void WaypointReached(uint32 uiPointId) override
        {
            switch (uiPointId)
            {
            case 0:
                if (Player* pPlayer = GetPlayerForEscort())
                {
                    m_creature->SetFacingToObject(pPlayer);
                    DoScriptText(SAY_START_1, m_creature, pPlayer);
                }
                break;
            case 25:
                if (Player* pPlayer = GetPlayerForEscort())
                {
                    DoScriptText(SAY_END_1, m_creature, pPlayer);
                    m_uiSayTimer = 3000;
                }
                break;
            }
        }

        void UpdateEscortAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                if (m_uiSayTimer)
                {
                    if (m_uiSayTimer <= uiDiff)
                    {
                        Player* pPlayer = GetPlayerForEscort();

                        if (!pPlayer)
                        {
                            m_uiSayTimer = 0;
                            return;
                        }

                        m_creature->SetFacingToObject(pPlayer);

                        switch (m_uiSpeech)
                        {
                        case 0:
                            DoScriptText(SAY_END_2, m_creature, pPlayer);
                            m_uiSayTimer = 3000;
                            break;
                        case 1:
                            pPlayer->GroupEventHappens(QUEST_ESCAPE_FROM_WINTERFIN_CAVERNS, m_creature);
                            m_uiSayTimer = 0;
                            break;
                        }

                        ++m_uiSpeech;
                    }
                    else
                        m_uiSayTimer -= uiDiff;
                }

                return;
            }

            DoMeleeAttackIfReady();
        }
    };

    bool OnQuestAccept(Player* pPlayer, Creature* pCreature, const Quest* pQuest) override
    {
        if (pQuest->GetQuestId() == QUEST_ESCAPE_FROM_WINTERFIN_CAVERNS)
        {
            if (npc_lurgglbrAI* pEscortAI = dynamic_cast<npc_lurgglbrAI*>(pCreature->AI()))
            {
                pCreature->SetFactionTemporary(FACTION_ESCORT_N_NEUTRAL_PASSIVE, TEMPFACTION_RESTORE_RESPAWN);
                pEscortAI->Start(false, pPlayer, pQuest);
            }
        }
        return true;
    }

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_lurgglbrAI(pCreature);
    }
};

/*#####
# npc_beryl_sorcerer
#####*/

enum
{
    SPELL_ARCANE_CHAINS                 = 45611,
    SPELL_ARCANE_CHAINS_CHANNEL         = 45630,
    SPELL_SUMMON_CHAINS_CHARACTER       = 45625,                // triggers 45626
    // SPELL_ENSLAVED_ARCANE_CHAINS     = 45632,                // chain visual - purpose unk, probably used on quest end

    NPC_BERYL_SORCERER                  = 25316,
    NPC_CAPTURED_BERYL_SORCERER         = 25474,
};

struct aura_arcane_chains : public AuraScript
{
    aura_arcane_chains() : AuraScript("aura_arcane_chains") {}

    bool OnDummyApply(const Aura* pAura, bool bApply) override
    {
        if (pAura->GetId() == SPELL_ARCANE_CHAINS)
        {
            if (pAura->GetEffIndex() != EFFECT_INDEX_0 || !bApply)
                return false;

            Creature* pCreature = (Creature*)pAura->GetTarget();
            Unit* pCaster = pAura->GetCaster();
            if (!pCreature || !pCaster || pCaster->GetTypeId() != TYPEID_PLAYER || pCreature->GetEntry() != NPC_BERYL_SORCERER)
                return false;

            // only for wounded creatures
            if (pCreature->GetHealthPercent() > 30.0f)
                return false;

            // spawn the captured sorcerer, apply dummy aura on the summoned and despawn
            pCaster->CastSpell(pCreature, SPELL_SUMMON_CHAINS_CHARACTER, true);
            pCaster->CastSpell(pCaster, SPELL_ARCANE_CHAINS_CHANNEL, true);
            pCreature->ForcedDespawn();
            return true;
        }

        return false;
    }
};

/*#####
# npc_captured_beryl_sorcerer
#####*/
struct aura_arcane_chains_cancel : public AuraScript
{
    aura_arcane_chains_cancel() : AuraScript("aura_arcane_chains_cancel") {}

    bool OnDummyApply(const Aura* pAura, bool bApply) override
    {
        if (pAura->GetId() == SPELL_ARCANE_CHAINS_CHANNEL)
        {
            if (pAura->GetEffIndex() != EFFECT_INDEX_0 || !bApply)
                return false;

            Creature* pCreature = (Creature*)pAura->GetTarget();
            Unit* pCaster = pAura->GetCaster();
            if (!pCreature || !pCaster || pCaster->GetTypeId() != TYPEID_PLAYER || pCreature->GetEntry() != NPC_CAPTURED_BERYL_SORCERER)
                return false;

            // follow the caster
            ((Player*)pCaster)->KilledMonsterCredit(NPC_CAPTURED_BERYL_SORCERER);
            pCreature->GetMotionMaster()->MoveFollow(pCaster, pCreature->GetDistance(pCaster), M_PI_F - pCreature->GetAngle(pCaster));
            return true;
        }

        return false;
    }
};

/*######
## npc_nexus_drake_hatchling
######*/

enum
{
    // combat spells
    SPELL_INTANGIBLE_PRESENCE           = 36513,
    SPELL_NETHERBREATH                  = 36631,

    // quest start spells
    SPELL_DRAKE_HARPOON                 = 46607,                    // initial spell
    SPELL_RED_DRAGONBLOOD               = 46620,                    // applied by aura 46607
    SPELL_CAPTURE_TRIGGER               = 46673,                    // notify the drake that it was captured; triggered by aura 46620 expire
    SPELL_SUBDUED                       = 46675,                    // visual spell; triggered by spell 46673
    SPELL_DRAKE_HATCHLING_SUBDUED       = 46691,                    // inform player that drake has been captured; triggered by spell 46673
    SPELL_DRAKE_VOMIT_PERIODIC          = 46678,                    // visual spell; triggered by spell 46673

    // quest completion spells
    SPELL_DRAKE_TURN_IN                 = 46696,                    // notify the drake that quest is finised
    SPELL_STRIP_AURAS                   = 46693,                    // remove all quest auras
    SPELL_DRAKE_COMPLETION_PING         = 46702,
    SPELL_RAELORASZ_FIREBALL            = 46704,
    SPELL_COMPLETE_IMMOLATION           = 46703,

    NPC_RAELORASZ                       = 26117,                    // quest giver / taker
    NPC_NEXUS_DRAKE_HATCHLING           = 26127,
    NPC_COLDARRA_DRAKE_HUNT_INVISMAN    = 26175,                    // quest credit

    QUEST_DRAKE_HUNT                    = 11919,
    QUEST_DRAKE_HUNT_DAILY              = 11940,

    FACTION_FRIENDLY                    = 35,
};

struct npc_nexus_drake_hatchling : public CreatureScript
{
    npc_nexus_drake_hatchling() : CreatureScript("npc_nexus_drake_hatchling") {}

    struct npc_nexus_drake_hatchlingAI : public FollowerAI
    {
        npc_nexus_drake_hatchlingAI(Creature* pCreature) : FollowerAI(pCreature) { }

        uint32 m_uiNetherbreathTimer;
        uint32 m_uiPresenceTimer;
        uint32 m_uiSubduedTimer;

        void Reset() override
        {
            m_uiNetherbreathTimer = urand(2000, 4000);
            m_uiPresenceTimer = urand(15000, 17000);
            m_uiSubduedTimer = 0;
        }

        void EnterEvadeMode() override
        {
            // force check for evading when the faction is changed
            if (m_uiSubduedTimer)
                return;

            FollowerAI::EnterEvadeMode();
        }

        void MoveInLineOfSight(Unit* pWho) override
        {
            FollowerAI::MoveInLineOfSight(pWho);

            if (!m_creature->HasAura(SPELL_SUBDUED) || m_creature->getVictim())
                return;

            if (pWho->GetEntry() == NPC_COLDARRA_DRAKE_HUNT_INVISMAN && m_creature->IsWithinDistInMap(pWho, 20.0f))
            {
                Player* pPlayer = GetLeaderForFollower();
                if (!pPlayer || !pPlayer->HasAura(SPELL_DRAKE_HATCHLING_SUBDUED))
                    return;

                pWho->CastSpell(pPlayer, SPELL_STRIP_AURAS, true);
                // give kill credit, mark the follow as completed and start the final event
                pPlayer->KilledMonsterCredit(NPC_COLDARRA_DRAKE_HUNT_INVISMAN);
                pPlayer->CastSpell(m_creature, SPELL_DRAKE_TURN_IN, true);
                SetFollowComplete(true);
            }
        }

        void JustRespawned() override
        {
            // reset stand state if required
            m_creature->SetStandState(UNIT_STAND_STATE_STAND);
            FollowerAI::JustRespawned();
        }

        void ReceiveAIEvent(AIEventType eventType, Creature* /*pSender*/, Unit* pInvoker, uint32 /*uiMiscValue*/) override
        {
            // start following
            if (eventType == AI_EVENT_START_EVENT && pInvoker->GetTypeId() == TYPEID_PLAYER)
            {
                StartFollow((Player*)pInvoker);
                m_uiSubduedTimer = 3 * MINUTE * IN_MILLISECONDS;
            }
            // timeout; quest failed
            else if (eventType == AI_EVENT_CUSTOM_A)
            {
                // check if the quest isn't already completed
                if (!HasFollowState(STATE_FOLLOW_COMPLETE))
                {
                    // force reset
                    JustRespawned();
                    ScriptedAI::EnterEvadeMode();
                }
            }
        }

        void UpdateFollowerAI(const uint32 uiDiff)
        {
            if (m_uiSubduedTimer)
            {
                if (m_uiSubduedTimer <= uiDiff)
                    m_uiSubduedTimer = 0;
                else
                    m_uiSubduedTimer -= uiDiff;
            }

            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
                return;

            if (m_uiNetherbreathTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_NETHERBREATH) == CAST_OK)
                    m_uiNetherbreathTimer = urand(17000, 20000);
            }
            else
                m_uiNetherbreathTimer -= uiDiff;

            if (m_uiPresenceTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_INTANGIBLE_PRESENCE) == CAST_OK)
                    m_uiPresenceTimer = urand(18000, 20000);
            }
            else
                m_uiPresenceTimer -= uiDiff;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_nexus_drake_hatchlingAI(pCreature);
    }
};

struct aura_drake_harpoon : public AuraScript
{
    aura_drake_harpoon() : AuraScript("aura_drake_harpoon") {}

    bool OnDummyApply(const Aura* pAura, bool bApply) override
    {
        if (pAura->GetEffIndex() != EFFECT_INDEX_0 || !bApply)
            return false;

        Creature* pCreature = (Creature*)pAura->GetTarget();
        Unit* pCaster = pAura->GetCaster();
        if (!pCreature || !pCaster || pCaster->GetTypeId() != TYPEID_PLAYER || pCreature->GetEntry() != NPC_NEXUS_DRAKE_HATCHLING)
            return false;

        // check if drake is already doing the quest
        if (pCreature->HasAura(SPELL_RED_DRAGONBLOOD) || pCreature->HasAura(SPELL_SUBDUED))
            return false;

        pCaster->CastSpell(pCreature, SPELL_RED_DRAGONBLOOD, true);
        return true;
    }
};

struct aura_red_dragonblood : public AuraScript
{
    aura_red_dragonblood() : AuraScript("aura_red_dragonblood") {}

    bool OnDummyApply(const Aura* pAura, bool bApply) override
    {
        Creature* pCreature = (Creature*)pAura->GetTarget();
        Unit* pCaster = pAura->GetCaster();
        if (!pCreature || !pCaster || pCaster->GetTypeId() != TYPEID_PLAYER || pCreature->GetEntry() != NPC_NEXUS_DRAKE_HATCHLING)
            return false;

        // start attacking on apply and capture on aura expire
        if (bApply)
            pCreature->AI()->AttackStart(pCaster);
        else
            pCaster->CastSpell(pCreature, SPELL_CAPTURE_TRIGGER, true);

        return true;
    }
};

struct aura_spell_drake_subdued : public AuraScript
{
    aura_spell_drake_subdued() : AuraScript("aura_spell_drake_subdued") {}

    bool OnDummyApply(const Aura* pAura, bool bApply) override
    {
        Creature* pCreature = (Creature*)pAura->GetTarget();
        if (!pCreature || pCreature->GetEntry() != NPC_NEXUS_DRAKE_HATCHLING)
            return false;

        // aura expired - evade
        pCreature->AI()->SendAIEvent(AI_EVENT_CUSTOM_A, pCreature, pCreature);
        return true;
    }
};

struct spell_capture_trigger : public SpellScript
{
    spell_capture_trigger() : SpellScript("spell_capture_trigger") {}

    bool EffectDummy(Unit* pCaster, uint32 uiSpellId, SpellEffectIndex uiEffIndex, Object* pTarget, ObjectGuid /*originalCasterGuid*/) override
    {
        if (pCaster->GetTypeId() != TYPEID_PLAYER)
            return true;

        Creature* pCreatureTarget = pTarget->ToCreature();
        if (pCaster->HasAura(SPELL_DRAKE_HATCHLING_SUBDUED) || pCreatureTarget->HasAura(SPELL_SUBDUED))
            return true;

        Player* pPlayer = (Player*)pCaster;
        if (!pPlayer)
            return true;

        // check the quest
        if (pPlayer->GetQuestStatus(QUEST_DRAKE_HUNT) != QUEST_STATUS_INCOMPLETE && pPlayer->GetQuestStatus(QUEST_DRAKE_HUNT_DAILY) != QUEST_STATUS_INCOMPLETE)
            return true;

        // evade and set friendly and start following @TODO move to creature script
        pCreatureTarget->SetFactionTemporary(FACTION_FRIENDLY, TEMPFACTION_RESTORE_REACH_HOME | TEMPFACTION_RESTORE_RESPAWN);
        pCreatureTarget->DeleteThreatList();
        pCreatureTarget->CombatStop(true);
        pCreatureTarget->AI()->SendAIEvent(AI_EVENT_START_EVENT, pCaster, pCreatureTarget);

        // cast visual spells
        pCreatureTarget->CastSpell(pCreatureTarget, SPELL_DRAKE_VOMIT_PERIODIC, true);
        pCreatureTarget->CastSpell(pCreatureTarget, SPELL_SUBDUED, true);
        pCreatureTarget->CastSpell(pCaster, SPELL_DRAKE_HATCHLING_SUBDUED, true);
        return true;
    }
};

struct spell_drake_turn_in : public SpellScript
{
    spell_drake_turn_in() : SpellScript("spell_drake_turn_in") {}

    bool EffectDummy(Unit* pCaster, uint32 uiSpellId, SpellEffectIndex uiEffIndex, Object* pTarget, ObjectGuid /*originalCasterGuid*/) override
    {
        Creature* pCreatureTarget = pTarget->ToCreature();

        if (Creature* pRaelorasz = GetClosestCreatureWithEntry(pCreatureTarget, NPC_RAELORASZ, 30.0f))
        {
            // Inform Raelorasz and move in front of him
            pCreatureTarget->CastSpell(pRaelorasz, SPELL_DRAKE_COMPLETION_PING, true);
            float fX, fY, fZ;
            pRaelorasz->GetContactPoint(pCreatureTarget, fX, fY, fZ, CONTACT_DISTANCE);
            pCreatureTarget->GetMotionMaster()->Clear(true, true);
            pCreatureTarget->GetMotionMaster()->MovePoint(0, fX, fY, fZ);
        }
        return true;
    }
};

struct spell_raelorasz_fireball : public SpellScript
{
    spell_raelorasz_fireball() : SpellScript("spell_raelorasz_fireball") {}

    bool EffectDummy(Unit* pCaster, uint32 uiSpellId, SpellEffectIndex uiEffIndex, Object* pTarget, ObjectGuid /*originalCasterGuid*/) override
    {
        Creature* pCreatureTarget = pTarget->ToCreature();
        pCreatureTarget->CastSpell(pCreatureTarget, SPELL_COMPLETE_IMMOLATION, true);
        pCreatureTarget->SetStandState(UNIT_STAND_STATE_DEAD);
        pCreatureTarget->ForcedDespawn(10000);
        return true;
    }
};

/*#####
# npc_scourged_flamespitter
#####*/

enum
{
    SPELL_REINFORCED_NET                = 46361,
    SPELL_NET                           = 47021,

    SPELL_INCINERATE_COSMETIC           = 45863,
    SPELL_INCINERATE                    = 32707,

    NPC_FLAMESPITTER                    = 25582,
};

struct npc_scourged_flamespitter : public CreatureScript
{
    npc_scourged_flamespitter() : CreatureScript("npc_scourged_flamespitter") {}

    struct  npc_scourged_flamespitterAI : public ScriptedAI
    {
        npc_scourged_flamespitterAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        uint32 m_uiIncinerateTimer;
        uint32 m_uiNetExpireTimer;

        void Reset() override
        {
            m_uiIncinerateTimer = urand(1000, 2000);
            m_uiNetExpireTimer = 0;
        }

        void AttackStart(Unit* pWho) override
        {
            if (m_creature->Attack(pWho, false))
            {
                m_creature->AddThreat(pWho);
                m_creature->SetInCombatWith(pWho);
                pWho->SetInCombatWith(m_creature);
                DoStartMovement(pWho, 10.0f);
            }
        }

        void MovementInform(uint32 uiMoveType, uint32 uiPointId) override
        {
            if (uiMoveType != POINT_MOTION_TYPE || !uiPointId)
                return;

            if (DoCastSpellIfCan(m_creature, SPELL_NET) == CAST_OK)
                m_uiNetExpireTimer = 20000;
        }

        void UpdateAI(const uint32 uiDiff)
        {
            if (m_uiNetExpireTimer)
            {
                if (m_uiNetExpireTimer <= uiDiff)
                {
                    // evade when the net root has expired
                    if (!m_creature->getVictim())
                        EnterEvadeMode();

                    m_uiNetExpireTimer = 0;
                }
                else
                    m_uiNetExpireTimer -= uiDiff;
            }

            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                // incinerate visual on OOC timer, unless creature is rooted
                if (!m_uiNetExpireTimer)
                {
                    if (m_uiIncinerateTimer < uiDiff)
                    {
                        if (DoCastSpellIfCan(m_creature, SPELL_INCINERATE_COSMETIC) == CAST_OK)
                            m_uiIncinerateTimer = urand(3000, 5000);
                    }
                    else
                        m_uiIncinerateTimer -= uiDiff;
                }

                return;
            }

            if (m_uiIncinerateTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_INCINERATE) == CAST_OK)
                    m_uiIncinerateTimer = urand(3000, 5000);
            }
            else
                m_uiIncinerateTimer -= uiDiff;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_scourged_flamespitterAI(pCreature);
    }
};

struct aura_reinforced_net : public AuraScript
{
    aura_reinforced_net() : AuraScript("aura_reinforced_net") {}

    bool OnDummyApply(const Aura* pAura, bool bApply)
    {
        if (pAura->GetId() == SPELL_REINFORCED_NET && pAura->GetEffIndex() == EFFECT_INDEX_0 && bApply)
        {
            Creature* pCreature = (Creature*)pAura->GetTarget();
            Unit* pCaster = pAura->GetCaster();
            if (!pCreature || !pCaster || pCaster->GetTypeId() != TYPEID_PLAYER || pCreature->GetEntry() != NPC_FLAMESPITTER)
                return false;

            // move the flamespitter to the ground level
            pCreature->GetMotionMaster()->Clear();
            pCreature->SetWalk(false);

            float fGroundZ = pCreature->GetMap()->GetHeight(pCreature->GetPhaseMask(), pCreature->GetPositionX(), pCreature->GetPositionY(), pCreature->GetPositionZ());
            pCreature->GetMotionMaster()->MovePoint(1, pCreature->GetPositionX(), pCreature->GetPositionY(), fGroundZ);
            return true;
        }

        return false;
    }
};

/*#####
## npc_bonker_togglevolt
#####*/

enum
{
    SAY_BONKER_START            = -1001013,
    SAY_BONKER_GO               = -1001014,
    SAY_BONKER_AGGRO            = -1001015,
    SAY_BONKER_LEFT             = -1001016,
    SAY_BONKER_COMPLETE         = -1001017,

    QUEST_ID_GET_ME_OUTA_HERE   = 11673,
};

struct npc_bonker_togglevolt : public CreatureScript
{
    npc_bonker_togglevolt() : CreatureScript("npc_bonker_togglevolt") {}

    struct npc_bonker_togglevoltAI : public npc_escortAI
    {
        npc_bonker_togglevoltAI(Creature* pCreature) : npc_escortAI(pCreature) { }

        void Reset() override { }

        void Aggro(Unit* /*pWho*/) override
        {
            if (urand(0, 1))
                DoScriptText(SAY_BONKER_AGGRO, m_creature);
        }

        void ReceiveAIEvent(AIEventType eventType, Creature* /*pSender*/, Unit* pInvoker, uint32 uiMiscValue) override
        {
            if (eventType == AI_EVENT_START_ESCORT && pInvoker->GetTypeId() == TYPEID_PLAYER)
            {
                m_creature->SetStandState(UNIT_STAND_STATE_STAND);
                Start(false, (Player*)pInvoker, GetQuestTemplateStore(uiMiscValue));
            }
        }

        void WaypointReached(uint32 uiPointId) override
        {
            switch (uiPointId)
            {
            case 0:
                DoScriptText(SAY_BONKER_START, m_creature);
                break;
            case 1:
                DoScriptText(SAY_BONKER_GO, m_creature);
                // WORKAROUND ALERT - temp ignore pathfinding until we pass the pool
                // creature cannont find a proper swimming path in this area, so ignore pathfinding for the moment
                m_creature->addUnitState(UNIT_STAT_IGNORE_PATHFINDING);
                break;
            case 3:
                DoScriptText(SAY_BONKER_LEFT, m_creature);
                // WORKAROUND END - resume pathfinding
                m_creature->clearUnitState(UNIT_STAT_IGNORE_PATHFINDING);
                break;
            case 32:
                if (Player* pPlayer = GetPlayerForEscort())
                {
                    pPlayer->GroupEventHappens(QUEST_ID_GET_ME_OUTA_HERE, m_creature);
                    DoScriptText(SAY_BONKER_COMPLETE, m_creature, pPlayer);
                }
                break;
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_bonker_togglevoltAI(pCreature);
    }

    bool OnQuestAccept(Player* pPlayer, Creature* pCreature, const Quest* pQuest) override
    {
        if (pQuest->GetQuestId() == QUEST_ID_GET_ME_OUTA_HERE)
        {
            pCreature->AI()->SendAIEvent(AI_EVENT_START_ESCORT, pPlayer, pCreature, pQuest->GetQuestId());
            return true;
        }

        return false;
    }
};

void AddSC_borean_tundra()
{
    Script* s;

    s = new npc_nesingwary_trapper();
    s->RegisterSelf();
    s = new npc_oil_stained_wolf();
    s->RegisterSelf();
    s = new npc_sinkhole_kill_credit();
    s->RegisterSelf();
    s = new npc_lurgglbr();
    s->RegisterSelf();
    s = new npc_nexus_drake_hatchling();
    s->RegisterSelf();
    s = new npc_scourged_flamespitter();
    s->RegisterSelf();
    s = new npc_bonker_togglevolt();
    s->RegisterSelf();

    s = new spell_throw_wolf_batt();
    s->RegisterSelf();
    s = new spell_capture_trigger();
    s->RegisterSelf();
    s = new spell_drake_turn_in();
    s->RegisterSelf();
    s = new spell_raelorasz_fireball();
    s->RegisterSelf();

    s = new aura_wolf_has_eaten();
    s->RegisterSelf();
    s = new aura_arcane_chains();
    s->RegisterSelf();
    s = new aura_arcane_chains_cancel();
    s->RegisterSelf();
    s = new aura_drake_harpoon();
    s->RegisterSelf();
    s = new aura_red_dragonblood();
    s->RegisterSelf();
    s = new spell_capture_trigger();
    s->RegisterSelf();
    s = new aura_reinforced_net();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_nesingwary_trapper";
    //pNewScript->GetAI = &GetAI_npc_nesingwary_trapper;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_oil_stained_wolf";
    //pNewScript->GetAI = &GetAI_npc_oil_stained_wolf;
    //pNewScript->pEffectDummyNPC = &EffectDummyCreature_npc_oil_stained_wolf;
    //pNewScript->pEffectAuraDummy = &EffectAuraDummy_npc_oil_stained_wolf;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_sinkhole_kill_credit";
    //pNewScript->GetAI = &GetAI_npc_sinkhole_kill_credit;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_lurgglbr";
    //pNewScript->GetAI = &GetAI_npc_lurgglbr;
    //pNewScript->pQuestAcceptNPC = &QuestAccept_npc_lurgglbr;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_beryl_sorcerer";
    //pNewScript->pEffectAuraDummy = &EffectAuraDummy_npc_beryl_sorcerer;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_captured_beryl_sorcerer";
    //pNewScript->pEffectAuraDummy = &EffectAuraDummy_npc_captured_beryl_sorcerer;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_nexus_drake_hatchling";
    //pNewScript->GetAI = &GetAI_npc_nexus_drake_hatchling;
    //pNewScript->pEffectAuraDummy = &EffectAuraDummy_npc_nexus_drake_hatchling;
    //pNewScript->pEffectDummyNPC = &EffectDummyCreature_npc_nexus_drake_hatchling;
    //pNewScript->RegisterSelf();
    //
    //pNewScript = new Script;
    //pNewScript->Name = "npc_scourged_flamespitter";
    //pNewScript->GetAI = &GetAI_npc_scourged_flamespitter;
    //pNewScript->pEffectAuraDummy = &EffectAuraDummy_npc_scourged_flamespitter;
    //pNewScript->RegisterSelf();
    //
    //pNewScript = new Script;
    //pNewScript->Name = "npc_bonker_togglevolt";
    //pNewScript->GetAI = &GetAI_npc_bonker_togglevolt;
    //pNewScript->pQuestAcceptNPC = &QuestAccept_npc_bonker_togglevolt;
    //pNewScript->RegisterSelf();
}
