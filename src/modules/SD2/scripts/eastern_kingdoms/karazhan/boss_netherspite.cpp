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

/**
* ScriptData
* SDName:      Boss Netherspite
* SD%Complete: 75
* SDComment:   Nether portal partially implemented. Find spell ID for tail swipe added in patch 3.0.2
* SDCategory:  Karazhan
* EndScriptData
*/

#include "precompiled.h"
#include "karazhan.h"

enum
{
    // netherspite spells
    SPELL_NETHERBURN = 30522,
    SPELL_VOID_ZONE = 37063,
    SPELL_NETHERBREATH = 38523,
    SPELL_EMPOWERMENT = 38549,
    SPELL_NETHER_INFUSION = 38688,                // hard enrage spell
    SPELL_NETHERSPITE_ROAR = 38684,                // on banish phase begin
    SPELL_SHADOWFORM = 38542,                // banish visual spell
    SPELL_FACE_RANDOM_TARGET = 38546,                // triggered by spell 38684 - currently not used
    SPELL_PORTAL_ATTUNEMENT = 30425,

    // void zone spells
    SPELL_CONSUMPTION = 28865,

    // beam buffs
    SPELL_SERENITY_NS = 30467,
    SPELL_SERENITY_PLR = 30422,
    SPELL_DOMINANCE_NS = 30468,
    SPELL_DOMINANCE_PLR = 30423,
    SPELL_PERSEVERENCE_NS = 30466,
    SPELL_PERSEVERENCE_PLR = 30421,

    // beam debuffs
    SPELL_EXHAUSTION_SER = 38638,
    SPELL_EXHAUSTION_DOM = 38639,
    SPELL_EXHAUSTION_PER = 38637,

    // beam visual spells which hit players
    SPELL_BEAM_SER = 30401,
    SPELL_BEAM_DOM = 30402,
    SPELL_BEAM_PER = 30400,

    // beam visual spells which hit Netherspite
    SPELL_BEAM_GREEN = 30464,
    SPELL_BEAM_BLUE = 30463,
    SPELL_BEAM_RED = 30465,

    // portal visual spells
    SPELL_GREEN_PORTAL = 30490,
    SPELL_BLUE_PORTAL = 30491,
    SPELL_RED_PORTAL = 30487,

    // emotes
    EMOTE_PHASE_BEAM = -1532089,
    EMOTE_PHASE_BANISH = -1532090,

    // npcs
    NPC_VOID_ZONE = 16697,
    NPC_PORTAL_GREEN = 17367, // Nether Portal - Serenity
    NPC_PORTAL_BLUE = 17368,  // Nether Portal - Dominance
    NPC_PORTAL_RED = 17369,   // Nether Portal - Perseverence

    MAX_PORTALS = 3,
    MAX_PORTAL_PROPERTIES = 7,
};

struct SpawnLocation
{
    float fX, fY, fZ, fO;
};

// at first spawn portals got fixed coords, should be shuffled in subsequent beam phases
static const SpawnLocation aPortalCoordinates[MAX_PORTALS] =
{
    { -11195.14f, -1616.375f, 278.3217f, 6.230825f },
    { -11108.13f, -1602.839f, 280.0323f, 3.717551f },
    { -11139.78f, -1681.278f, 278.3217f, 1.396263f },
};

enum NetherspitePhases
{
    BEAM_PHASE = 0,
    BANISH_PHASE = 1,
};

enum PortalProperties
{
    PORTAL_ENTRY = 0,    
    PORTAL_VISUAL = 1,
    PORTAL_VISUAL_PLR = 2,
    PORTAL_VISUAL_NS = 3,
    PORTAL_BUFF_PLR = 4,
    PORTAL_BUFF_NS = 5,
    PORTAL_DEBUFF = 6,
};

static const uint32 auiPortalVector[MAX_PORTAL_PROPERTIES][MAX_PORTALS] =
{
    //      0 - GREEN               1 - BLUE                2 - RED
    { NPC_PORTAL_GREEN, NPC_PORTAL_BLUE, NPC_PORTAL_RED },	// 0 - portal entries
    { SPELL_GREEN_PORTAL, SPELL_BLUE_PORTAL, SPELL_RED_PORTAL },	// 1 - visual spells for portals
    { SPELL_BEAM_SER, SPELL_BEAM_DOM, SPELL_BEAM_PER },  // 2 - visual spells for players
    { SPELL_BEAM_GREEN, SPELL_BEAM_BLUE, SPELL_BEAM_RED },	// 3 - visual spells for netherspite
    { SPELL_SERENITY_PLR, SPELL_DOMINANCE_PLR, SPELL_PERSEVERENCE_PLR },  // 4 - buffs for players
    { SPELL_SERENITY_NS, SPELL_DOMINANCE_NS, SPELL_PERSEVERENCE_NS },  // 5 - buffs for netherspite
    { SPELL_EXHAUSTION_SER, SPELL_EXHAUSTION_DOM, SPELL_EXHAUSTION_PER },	// 6 - debuffs	
};

//adjust how easy it is to catch the beam
static const float beamHitbox = 0.8f;

struct MANGOS_DLL_DECL boss_netherspiteAI : public ScriptedAI
{
    boss_netherspiteAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    NetherspitePhases m_uiActivePhase;

    Creature* m_portal[MAX_PORTALS];
    Unit* m_portalHolder[MAX_PORTALS];

    uint32 m_uiEnrageTimer;
    uint32 m_uiVoidZoneTimer;
    uint32 m_uiPhaseSwitchTimer;
    uint32 m_uiNetherbreathTimer;
    uint32 m_uiEmpowermentTimer;
    uint32 m_uiConnectionTimer;

    std::vector<uint32> m_vPortalEntryList;

    void Reset() override
    {
        m_uiActivePhase = BEAM_PHASE;

        m_uiEmpowermentTimer = 10000;
        m_uiEnrageTimer = 9 * MINUTE * IN_MILLISECONDS;
        m_uiVoidZoneTimer = 15000;
        m_uiPhaseSwitchTimer = MINUTE * IN_MILLISECONDS;
        m_uiConnectionTimer = 10000;

        SetCombatMovement(true);

        // initialize the portal list
        m_vPortalEntryList.clear();
        m_vPortalEntryList.resize(MAX_PORTALS);

        for (uint8 i = 0; i < MAX_PORTALS; ++i)
            m_vPortalEntryList[i] = auiPortalVector[PORTAL_ENTRY][i];

        DoResetPortals();
    }

    void Aggro(Unit* /*pWho*/) override
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_NETHERSPITE, IN_PROGRESS);

        DoSummonPortals();
        DoCastSpellIfCan(m_creature, SPELL_NETHERBURN);
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_NETHERSPITE, DONE);

        DoDespawnPortalsImmediately();
    }

    void JustReachedHome() override
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_NETHERSPITE, FAIL);

        DoDespawnPortalsImmediately();
    }

    void SwitchPhases()
    {
        if (m_uiActivePhase == BEAM_PHASE)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_NETHERSPITE_ROAR) == CAST_OK)
            {
                DoDebuffPortalHolderInBanishPhase();
                DoResetPortals();

                DoCastSpellIfCan(m_creature, SPELL_SHADOWFORM, CAST_TRIGGERED);
                m_creature->RemoveAurasDueToSpell(SPELL_EMPOWERMENT);

                SetCombatMovement(false);
                m_creature->GetMotionMaster()->MoveIdle();

                m_uiActivePhase = BANISH_PHASE;
                DoScriptText(EMOTE_PHASE_BANISH, m_creature);

                m_uiNetherbreathTimer = 2000;
                m_uiPhaseSwitchTimer = 30000;
            }
        }
        else
        {
            m_creature->RemoveAurasDueToSpell(SPELL_SHADOWFORM);
            SetCombatMovement(true);
            DoStartMovement(m_creature->getVictim());

            m_uiActivePhase = BEAM_PHASE;
            DoScriptText(EMOTE_PHASE_BEAM, m_creature);

            DoSummonPortals();
            m_uiConnectionTimer = 5000;
            m_uiEmpowermentTimer = 10000;
            m_uiPhaseSwitchTimer = MINUTE * IN_MILLISECONDS;
        }

        // reset threat every phase switch
        DoResetThreat();
    }

    void DoSummonPortals()
    {
        for (uint8 i = 0; i < MAX_PORTALS; ++i)
            m_creature->SummonCreature(m_vPortalEntryList[i], aPortalCoordinates[i].fX, aPortalCoordinates[i].fY, aPortalCoordinates[i].fZ, aPortalCoordinates[i].fO, TEMPSUMMON_TIMED_DESPAWN, 60000);

        // randomize the portals after the first summon
        std::random_shuffle(m_vPortalEntryList.begin(), m_vPortalEntryList.end());
    }

    void DoDespawnPortalsImmediately()
    {
        if (m_pInstance)
        {
            for (uint8 i = 0; i < MAX_PORTALS; ++i)
            {
                if (Creature* pPortal = m_pInstance->GetSingleCreatureFromStorage(auiPortalVector[PORTAL_ENTRY][i]))
                {
                    pPortal->ForcedDespawn();
                    pPortal->RemoveFromWorld();
                }
            }
        }
    }

    void DoResetPortals()
    {
        for (uint8 i = 0; i < MAX_PORTALS; ++i)
        {
            m_portal[i] = 0;
            m_portalHolder[i] = 0;
        }
    }

    void DoDebuffPortalHolder(uint8 portalIndex)
    {
        if (m_portalHolder[portalIndex] && m_portalHolder[portalIndex] != m_creature)
            m_portalHolder[portalIndex]->CastSpell(m_portalHolder[portalIndex], auiPortalVector[PORTAL_DEBUFF][portalIndex], true);
    }

    void DoDebuffPortalHolderInBanishPhase()
    {
        for (uint8 i = 0; i < MAX_PORTALS; ++i)
        {
            DoDebuffPortalHolder(i);
        }
    }

    void JustSummoned(Creature* pSummoned) override
    {
        //do this at db later
        //we need to adjust the modells of the portals -> need invisible modells
        pSummoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        pSummoned->setFaction(m_creature->getFaction());

        //we need the portals not moveing and auto attacking but staying in fight so that we can turn the triggered spells to false
        //pSummoned->SetInCombatWithZone();
        //pSummoned->addUnitState(UNIT_STAT_NO_COMBAT_MOVEMENT);

        switch (pSummoned->GetEntry())
        {
        case NPC_VOID_ZONE:
            pSummoned->CastSpell(pSummoned, SPELL_CONSUMPTION, true);
            break;
        case NPC_PORTAL_RED:
            pSummoned->CastSpell(pSummoned, SPELL_RED_PORTAL, true);
            m_portal[2] = pSummoned;
            break;
        case NPC_PORTAL_GREEN:
            pSummoned->CastSpell(pSummoned, SPELL_GREEN_PORTAL, true);
            m_portal[0] = pSummoned;
            break;
        case NPC_PORTAL_BLUE:
            pSummoned->CastSpell(pSummoned, SPELL_BLUE_PORTAL, true);
            m_portal[1] = pSummoned;
            break;
        }
    }

    void UpdatePortalConnection()
    {
        for (uint8 i = 0; i < MAX_PORTALS; ++i)
        {
            if (m_portal[i])
            {
                if (Player* pPlayer = GetClosestPlayerToPortalBetweenPortalAndNetherspite(i))
                {
                    m_portal[i]->CastSpell(pPlayer, auiPortalVector[PORTAL_BUFF_PLR][i], true);

                    if (!m_portalHolder[i] || pPlayer != m_portalHolder[i])
                    {
                        DoDebuffPortalHolder(i);

                        m_portal[i]->CastSpell(pPlayer, auiPortalVector[PORTAL_VISUAL_PLR][i], true);
                        m_portalHolder[i] = pPlayer;
                    }
                }
                else
                {
                    m_portal[i]->CastSpell(m_creature, auiPortalVector[PORTAL_BUFF_NS][i], true);

                    if (!m_portalHolder[i] || m_creature != m_portalHolder[i])
                    {
                        DoDebuffPortalHolder(i);

                        m_portal[i]->CastSpell(m_creature, auiPortalVector[PORTAL_VISUAL_NS][i], true);
                        m_portalHolder[i] = m_creature;
                    }
                }
            }
        }
    }

    bool IsPlayerPositionBetweenPortalAndNetherspite(uint8 portalIndex, Player* pPlayer)
    {
        bool success = false;

        if (m_portal[portalIndex] && pPlayer)
        {
            float delta = m_portal[portalIndex]->GetDistance2d(pPlayer) + pPlayer->GetDistance2d(m_creature) - m_portal[portalIndex]->GetDistance2d(m_creature);

            if (delta >= -beamHitbox && delta <= 0)
                success = true;
        }

        return success;
    }

    Player* GetClosestPlayerToPortalBetweenPortalAndNetherspite(uint8 portalIndex)
    {
        Player* closestPlayer = 0;
        Map::PlayerList const& lPlayers = m_pInstance->instance->GetPlayers();

        for (Map::PlayerList::const_iterator itr = lPlayers.begin(); itr != lPlayers.end(); ++itr)
        {
            if (Player* pPlayer = itr->getSource())
            {
                if (m_portal[portalIndex] && !pPlayer->HasAura(auiPortalVector[PORTAL_DEBUFF][portalIndex]) && IsPlayerPositionBetweenPortalAndNetherspite(portalIndex, pPlayer) && m_portal[portalIndex]->GetDistance2d(m_creature) > m_portal[portalIndex]->GetDistance2d(pPlayer))
                {
                    if (!closestPlayer || m_portal[portalIndex]->GetDistance2d(closestPlayer) > m_portal[portalIndex]->GetDistance2d(pPlayer))
                        closestPlayer = pPlayer;
                }
            }
        }

        return closestPlayer;
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        //PHASE SWITCH
        if (m_uiPhaseSwitchTimer <= uiDiff)
            SwitchPhases();
        else
            m_uiPhaseSwitchTimer -= uiDiff;

        //ENRAGE
        if (m_uiEnrageTimer)
        {
            if (m_uiEnrageTimer <= uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_NETHER_INFUSION) == CAST_OK)
                    m_uiEnrageTimer = 0;
            }
            else
                m_uiEnrageTimer -= uiDiff;
        }

        if (m_uiVoidZoneTimer < uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_VOID_ZONE) == CAST_OK)
                    m_uiVoidZoneTimer = 15000;
            }
        }
        else
            m_uiVoidZoneTimer -= uiDiff;

        //BEAM PHASE
        if (m_uiActivePhase == BEAM_PHASE)
        {
            if (m_uiConnectionTimer < uiDiff)
            {
                UpdatePortalConnection();
                m_uiConnectionTimer = 1000;
            }
            else
                m_uiConnectionTimer -= uiDiff;

            if (m_uiEmpowermentTimer)
            {
                if (m_uiEmpowermentTimer <= uiDiff)
                {
                    if (DoCastSpellIfCan(m_creature, SPELL_EMPOWERMENT) == CAST_OK)
                    {
                        DoCastSpellIfCan(m_creature, SPELL_PORTAL_ATTUNEMENT, CAST_TRIGGERED);
                        m_uiEmpowermentTimer = 0;
                    }
                }
                else
                    m_uiEmpowermentTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
        //BANISH PHASE
        else
        {
            if (m_uiNetherbreathTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_NETHERBREATH) == CAST_OK)
                    m_uiNetherbreathTimer = urand(4000, 5000);
            }
            else
                m_uiNetherbreathTimer -= uiDiff;
        }
    }
};

CreatureAI* GetAI_boss_netherspite(Creature* pCreature)
{
    return new boss_netherspiteAI(pCreature);
}

void AddSC_boss_netherspite()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "boss_netherspite";
    pNewScript->GetAI = &GetAI_boss_netherspite;
    pNewScript->RegisterSelf();
}