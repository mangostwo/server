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
SDName: boss_yogg_saron
SD%Complete: 50%
SDComment: Partially implemented. Not fully playable yet.
SDCategory: Ulduar
EndScriptData */

#include "precompiled.h"
#include "ulduar.h"

enum
{
    SAY_SARA_INTRO_1                            = -1603197,
    SAY_SARA_INTRO_2                            = -1603198,
    SAY_SARA_AGGRO                              = -1603199,
    SAY_SARA_HELP_1                             = -1603201,
    SAY_SARA_HELP_2                             = -1603202,
    SAY_SARA_SLAY_1                             = -1603203,
    SAY_SARA_SLAY_2                             = -1603204,
    SAY_WIPE_PHASE_1                            = -1603205,

    SAY_PHASE_2_INTRO_1                         = -1603206,
    SAY_PHASE_2_INTRO_2                         = -1603262,
    SAY_PHASE_2_INTRO_3                         = -1603263,
    SAY_PHASE_2_INTRO_4                         = -1603264,
    SAY_PHASE_2_INTRO_5                         = -1603265,
    SAY_SARA_PHASE_2_INTRO_A                    = -1603206,
    SAY_SARA_PHASE_2_INTRO_B                    = -1603207,

    SAY_MADNESS                                 = -1603209,
    SAY_PHASE_3                                 = -1603210,
    SAY_SLAY_1                                  = -1603211,
    SAY_SLAY_2                                  = -1603212,
    SAY_DEATH                                   = -1603213,
    SAY_TO_INSANE_1                             = -1603214,
    SAY_TO_INSANE_2                             = -1603215,

    SAY_LICH_KING_1                             = -1603216,
    SAY_CHAMPION_1                              = -1603217,
    SAY_CHAMPION_2                              = -1603218,
    SAY_LICH_KING_2                             = -1603219,
    SAY_YOGG_V3_1                               = -1603220,
    SAY_YOGG_V3_2                               = -1603221,

    SAY_NELTHARION_1                            = -1603222,
    SAY_YSERA                                   = -1603223,
    SAY_NELTHARION_2                            = -1603224,
    SAY_MALYGOS                                 = -1603225,
    SAY_YOGG_V2                                 = -1603226,

    SAY_GARONA_1                                = -1603227,
    SAY_GARONA_2                                = -1603228,
    SAY_YOGG_V1_1                               = -1603229,
    SAY_YOGG_V1_2                               = -1603230,
    SAY_GARONA_3                                = -1603231,
    SAY_GARONA_4                                = -1603232,
    SAY_YOGG_V1_3                               = -1603233,

    EMOTE_VISION_BLAST                          = -1603234,
    EMOTE_SHATTER_BLAST                         = -1603235,
    EMOTE_CLOUD_BOIL                            = -1603261,

    // generic spells
    SPELL_EXTINGUISH_LIFE                       = 64166,                    // berserk spell

    // Sara phase spells
    SPELL_SARAS_FERVOR                          = 63747,                    // triggers 63138
    SPELL_SARAS_BLESSING                        = 63745,                    // triggers 63134
    SPELL_SARAS_ANGER                           = 63744,                    // triggers 63147

    // ominous cloud spells
    // SPELL_OMINOUS_CLOUD_VISUAL               = 63084,                    // in c_t_a
    SPELL_BOIL_OMNIOUSLY                        = 63030,                    // cast when a player is in range; triggers 63031

    // guardian of yogg spells
    SPELL_SHADOW_NOVA                           = 62714,                    // used by the guardians when it dies
    SPELL_SHADOW_NOVA_H                         = 65209,
    SPELL_DARK_VOLLEY                           = 63038,
    SPELL_DOMINATE_MIND                         = 63713,

    // Voice of Yogg spells
    SPELL_SANITY                                = 63786,                    // add sanity when encounter starts
    SPELL_INSANE_PERIODIC                       = 64554,                    // decrease sanity
    SPELL_CLEAR_INSANE                          = 63122,                    // clear all the sanity and insane on wipe / death
    SPELL_SUMMON_GUARDIAN_YOGG                  = 62978,                    // cast by npc 33280 on an Ominus cloud

    // Yogg transition spells
    SPELL_SHADOWY_BARRIER_YOGG                  = 63894,
    SPELL_KNOCK_AWAY                            = 64022,
    SPELL_MATCH_HEALTH                          = 64066, // periodic aura on the Brain

    // Sara transition spells
    SPELL_SHADOWY_BARRIER                       = 64775, // damage immunity spells
    SPELL_FULL_HEAL                             = 43978,
    SPELL_PHASE_2_TRANSFORM                     = 65157,
    SPELL_RIDE_VEHICLE_YOGG                     = 61791, // mount vehicle Yogg

    // Vision phase spells
    SPELL_PHYCHOSIS                             = 63795,                    // Sara combat spells
    SPELL_PHYCHOSIS_H                           = 65301,
    SPELL_MALADY_OF_THE_MIND                    = 63830,
    SPELL_BRAIN_LINK                            = 63802,
    SPELL_DEATH_RAY_SUMMON                      = 63891,

    // Tentacle spawn spells
    SPELL_CONSTRICTOR_TENTACLE                  = 64132,                    // triggers 64133
    SPELL_CORRUPTOR_TENTACLE                    = 64143,                    // tentacle spawn spells
    SPELL_CRUSHER_TENTACLE                      = 64139,

    // Tentacle spells
    SPELL_TENTACLE_VOID_ZONE                    = 64384,
    SPELL_ERUPT                                 = 64144,
    SPELL_LUNGE                                 = 64131,                    // triggers 64123
    SPELL_TENTACLE_VOID_ZONE_BIG                = 64017,
    SPELL_CRUSH                                 = 64146,
    SPELL_DIMINISH_POWER                        = 64148,
    SPELL_FOCUSED_ANGER                         = 57688,
    
    // Old God phase spells
    SPELL_LUNATIC_GAZE_YOGG                     = 64163,
    SPELL_SHADOW_BEACON                         = 64465,                    // trigger 64468
    // SPELL_EMPOWERING_SHADOWS                 = 64468,
    // SPELL_EMPOWERING_SHADOWS_H               = 64486,
    SPELL_DEAFENING_ROAR                        = 64189,
    SPELL_IMMORTAL_GUARDIAN                     = 64158,

    // death ray spells
    SPELL_DEATH_RAY_TRIGG                       = 63883,
    SPELL_DEATH_RAY_VISUAL_WARN                 = 63882,
    SPELL_DEATH_RAY_VISUAL_DAMAGE               = 63886,
    SPELL_DEATH_RAY_VISUAL_ORIGIN               = 63893,

    // descend into madness spells
    SPELL_TELEPORT_PORTAL_VISUAL                = 64416,
    SPELL_TELEPORT_TO_STORMWIND_ILLUSION        = 63989,
    SPELL_TELEPORT_TO_CHAMBER_ILLUSION          = 63997,
    SPELL_TELEPORT_TO_ICEECROWN_ILLUSION        = 63998,
    SPELL_TELEPORT_BACK_TO_MAIN_ROOM            = 63992,

    // summoned creatures
    NPC_DEATH_RAY                               = 33881,
    NPC_DEATH_ORB                               = 33882,
    NPC_CONSTRICTOR_TENTACLE                    = 33983,
    NPC_CRUSHER_TENTACLE                        = 33966,
    NPC_CORRUPTOR_TENTACLE                      = 33985,
    NPC_DESCEND_INTO_MADNESS                    = 34072,
    NPC_IMMORTAL_GUARDIAN                       = 33988,

    // keepers
    // Freya spells
    SPELL_RESILIENCE_OF_NATURE                  = 62670,
    SPELL_SUMMON_SANITY_WELL                    = 64170,                    // sends event 21432; used to spawn npc 33991

    // sanity well spells
    SPELL_SANITY_WELL                           = 64169,
    SPELL_SANITY_WELL_VISUAL                    = 63288,

    // Hodir spells
    SPELL_FORTITUDE_OF_FROST                    = 62650,
    SPELL_HODIRS_PROTECTIVE_GAZE                = 64174,

    // Mimiron spells
    SPELL_SPEED_OF_INVENTION                    = 62671,
    SPELL_DESTABILIZATION_MATRIX                = 65206,

    // Thorim spells
    SPELL_FURY_OF_THE_STORM                     = 62702,
    SPELL_TITANIC_STORM                         = 64171,
    
    // other
    FACTION_SARA_HOSTILE                        = 16,

    // encounter phases
    PHASE_INTRO                                 = 0,
    PHASE_SARA                                  = 1,
    PHASE_VISIONS                               = 2,
    PHASE_OLD_GOD                               = 3,
    PHASE_TRANSITION                            = 4,
};

static const DialogueEntry aYoggSaronDialog[] =
{
    {SAY_PHASE_2_INTRO_1,       NPC_SARA,       4000},
    {SAY_PHASE_2_INTRO_2,       NPC_SARA,       5000},
    {SAY_PHASE_2_INTRO_3,       NPC_SARA,       5000},
    {SAY_PHASE_2_INTRO_4,       NPC_SARA,       1000},
    {SPELL_PHASE_2_TRANSFORM,   0,              3000},
    {SAY_PHASE_2_INTRO_5,       NPC_YOGGSARON,  0},
    {0, 0, 0},
};

static const float afYoggSaronSpawn[4] = {1980.43f, -25.7708f, 324.9724f, 3.141f};

/*######
## boss_sara
######*/

struct MANGOS_DLL_DECL boss_saraAI : public Scripted_NoMovementAI, private DialogueHelper
{
    boss_saraAI(Creature* pCreature) : Scripted_NoMovementAI(pCreature),
        DialogueHelper(aYoggSaronDialog)
    {
        m_pInstance = (instance_ulduar*)pCreature->GetInstanceData();
        m_bIsRegularMode = pCreature->GetMap()->IsRegularDifficulty();
        InitializeDialogueHelper(m_pInstance);
        Reset();
    }

    instance_ulduar* m_pInstance;
    bool m_bIsRegularMode;
    
    bool m_bIsHostile;

    uint8 m_uiPhase;
    uint32 m_uiSarasSpellTimer;
    
    uint32 m_uiPsychosisTimer;
    uint32 m_uiMaladyTimer;
    uint32 m_uiBrainLinkTimer;
    uint32 m_uiDeathRayTimer;

    void Reset() override
    {
        m_uiPhase                       = PHASE_INTRO;
        m_uiSarasSpellTimer             = 15000;
        m_bIsHostile                    = false;
        
        m_uiPsychosisTimer              = 2000;
        m_uiMaladyTimer                 = 15000;
        m_uiBrainLinkTimer              = 25000;
        m_uiDeathRayTimer               = 20000;
    }

    void AttackStart(Unit* pWho) override
    {
        if (m_uiPhase == PHASE_SARA)
            return;

        ScriptedAI::AttackStart(pWho);
    }
    
    void EnterEvadeMode() override
    {
        if (!m_bIsHostile)
            return;
        
        ScriptedAI::EnterEvadeMode();
    }

    void MoveInLineOfSight(Unit* pWho) override
    {
        // start the encounter on range check
        // ToDo: research if there is any intro available before the actual encounter starts
        if (m_uiPhase == PHASE_INTRO && pWho->GetTypeId() == TYPEID_PLAYER && pWho->IsAlive() && !((Player*)pWho)->isGameMaster() && m_creature->IsWithinDistInMap(pWho, 70.0f))
        {
            m_uiPhase = PHASE_SARA;
            DoScriptText(SAY_SARA_AGGRO, m_creature);

            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_YOGGSARON, IN_PROGRESS);
                
                if (Creature* pVoice = m_pInstance->GetSingleCreatureFromStorage(NPC_VOICE_OF_YOGG))
                    SendAIEvent(AI_EVENT_START_EVENT, m_creature, pVoice);
            }
        }

        ScriptedAI::MoveInLineOfSight(pWho);
    }

    void KilledUnit(Unit* pVictim) override
    {
        if (pVictim->GetTypeId() != TYPEID_PLAYER)
            return;

        DoScriptText(urand(0, 1) ? SAY_SARA_SLAY_1 : SAY_SARA_SLAY_2, m_creature);
    }
    
    void DamageTaken(Unit* /*pDealer*/, uint32& uiDamage) override
    {
        if (uiDamage >= m_creature->GetHealth())
        {
            uiDamage = 0;

            if (m_uiPhase == PHASE_SARA)
            {
                m_uiPhase = PHASE_TRANSITION;
                StartNextDialogueText(SAY_PHASE_2_INTRO_1);

                // despawn all clouds for phase 2
                if (!m_pInstance)
                    return;

                GuidList m_lCloudGuids;
                m_pInstance->GetOminousCloudGuids(m_lCloudGuids);

                for (GuidList::const_iterator itr = m_lCloudGuids.begin(); itr != m_lCloudGuids.end(); ++itr)
                {
                    if (Creature* pCloud = m_creature->GetMap()->GetCreature(*itr))
                        pCloud->ForcedDespawn();
                }
            }
        }
    }

    void JustSummoned(Creature* /*pSummoned*/) override
    {
        if (pSummoned->GetEntry() == NPC_YOGGSARON)
        {
            pSummoned->CastSpell(pSummoned, SPELL_SHADOWY_BARRIER_YOGG, true);
            pSummoned->CastSpell(pSummoned, SPELL_KNOCK_AWAY, true);
            pSummoned->SetInCombatWithZone();
        }
        else if (pSummoned->GetEntry() == NPC_DEATH_ORB)
        {
            // ToDo: summon 2 death rays
            pSummoned->CastSpell(pSummoned, SPELL_DEATH_RAY_VISUAL_ORIGIN, true);
        }
    }
    
    void JustDidDialogueStep(int32 iEntry) override
    {
        switch (iEntry)
        {
            case SAY_PHASE_2_INTRO_4:
                DoCastSpellIfCan(m_creature, SPELL_FULL_HEAL, CAST_TRIGGERED);
                DoCastSpellIfCan(m_creature, SPELL_PHASE_2_TRANSFORM, CAST_TRIGGERED);
                DoCastSpellIfCan(m_creature, SPELL_SHADOWY_BARRIER, CAST_TRIGGERED);

                m_creature->SetFactionTemporary(FACTION_SARA_HOSTILE, TEMPFACTION_RESTORE_RESPAWN);
                m_creature->SummonCreature(NPC_YOGGSARON, afYoggSaronSpawn[0], afYoggSaronSpawn[1], afYoggSaronSpawn[2], afYoggSaronSpawn[3], TEMPSUMMON_DEAD_DESPAWN, 0);
                m_bIsHostile = true;
                m_creature->SetInCombatWithZone();
                break;
            case SPELL_PHASE_2_TRANSFORM:
                if (m_pInstance)
                {
                    if (Creature* pYogg = m_pInstance->GetSingleCreatureFromStorage(NPC_YOGGSARON))
                        DoCastSpellIfCan(pYogg, SPELL_RIDE_VEHICLE_YOGG, CAST_TRIGGERED);
                    if (Creature* pVoice = m_pInstance->GetSingleCreatureFromStorage(NPC_VOICE_OF_YOGG))
                        SendAIEvent(AI_EVENT_START_EVENT_A, m_creature, pVoice);
                    m_uiPhase = PHASE_VISIONS;
                }
                break;
        }
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        DialogueUpdate(uiDiff);

        if (m_uiPhase == PHASE_SARA)
        {
            if (m_uiSarasSpellTimer < uiDiff)
            {
                CanCastResult castResult;
                switch (urand(0, 2))
                {
                    case 0: castResult = DoCastSpellIfCan(m_creature, SPELL_SARAS_FERVOR);   break;
                    case 1: castResult = DoCastSpellIfCan(m_creature, SPELL_SARAS_BLESSING); break;
                    case 2: castResult = DoCastSpellIfCan(m_creature, SPELL_SARAS_ANGER);    break;
                }

                if (castResult == CAST_OK)
                {
                    if (urand(0, 1))
                        DoScriptText(urand(0, 1) ? SAY_SARA_HELP_1 : SAY_SARA_HELP_2, m_creature);

                    m_uiSarasSpellTimer = 5000;
                }
            }
            else
                m_uiSarasSpellTimer -= uiDiff;
        }

        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;
        
        if (m_uiPhase == PHASE_VISIONS)
        {
            if (m_uiPsychosisTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, m_bIsRegularMode ? SPELL_PHYCHOSIS : SPELL_PHYCHOSIS_H) == CAST_OK)
                    m_uiPsychosisTimer = urand(3000, 4000);
            }
            else
                m_uiPsychosisTimer -= uiDiff;

            if (m_uiMaladyTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_MALADY_OF_THE_MIND) == CAST_OK)
                    m_uiMaladyTimer = 15000;
            }
            else
                m_uiMaladyTimer -= uiDiff;

            if (m_uiBrainLinkTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_BRAIN_LINK) == CAST_OK)
                    m_uiBrainLinkTimer = 25000;
            }
            else
                m_uiBrainLinkTimer -= uiDiff;

            if (m_uiDeathRayTimer < uiDiff)
            {
                // spawn death orb at predefined location
                m_creature->CastSpell(1980.43f, -25.7708f, 351.5418f, SPELL_DEATH_RAY_SUMMON, true);
                m_uiDeathRayTimer = 20000;
            }
            else
                m_uiDeathRayTimer -= uiDiff;
        }
    }
};

CreatureAI* GetAI_boss_sara(Creature* pCreature)
{
    return new boss_saraAI(pCreature);
}

/*######
## boss_yogg_saron
######*/

struct MANGOS_DLL_DECL boss_yogg_saronAI : public Scripted_NoMovementAI
{
    boss_yogg_saronAI(Creature* pCreature) : Scripted_NoMovementAI(pCreature)
    {
        m_pInstance = (instance_ulduar*)pCreature->GetInstanceData();
        m_bIsRegularMode = pCreature->GetMap()->IsRegularDifficulty();
        Reset();
    }

    instance_ulduar* m_pInstance;
    bool m_bIsRegularMode;

    uint8 m_uiPhase;
    
    uint32 m_uiLunaticGazeTimer;
    uint32 m_uiDeafeningRoarTimer;

    void Reset() override
    {
        m_uiPhase = PHASE_VISIONS;
        m_uiLunaticGazeTimer = 15000;
        
        if (m_pInstance)
            m_uiDeafeningRoarTimer = (!m_bIsRegularMode && m_pInstance->GetData(TYPE_YOGGSARON_HARD) >= 1) ? 20000 : 0;
    }

    void JustReachedHome() override
    {
        // unboard passengers first to avoid issues
        m_creature->RemoveSpellsCausingAura(SPELL_AURA_CONTROL_VEHICLE);

        if (m_pInstance)
        {
            if (m_pInstance->GetData(TYPE_YOGGSARON) != FAIL)
                m_pInstance->SetData(TYPE_YOGGSARON, FAIL);
        }

        m_creature->ForcedDespawn();
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        // switch to last phase when HP is dropped to 30%
        if (m_uiPhase == PHASE_VISIONS && m_creature->GetHealthPercent() < 30.0f)
        {
            DoScriptText(SAY_PHASE_3, m_creature);
            m_uiPhase = PHASE_OLD_GOD;
            DoCastSpellIfCan(m_creature, SPELL_SHADOW_BEACON);
            
            if (m_pInstance)
            {
                if (Creature* pSara = m_pInstance->GetSingleCreatureFromStorage(NPC_SARA))
                    pSara->ForcedDespawn();
                if (Creature* pVoice = m_pInstance->GetSingleCreatureFromStorage(NPC_VOICE_OF_YOGG))
                    SendAIEvent(AI_EVENT_START_EVENT_B, m_creature, pVoice);
            }
        }
    
        if (m_uiPhase == PHASE_OLD_GOD)
        {
            if (m_uiLunaticGazeTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_LUNATIC_GAZE_YOGG) == CAST_OK)
                    m_uiLunaticGazeTimer = 15000;
            }
            else
                m_uiLunaticGazeTimer -= uiDiff;
            
            if (m_uiDeafeningRoarTimer)
            {
                if (m_uiDeafeningRoarTimer <= uiDiff)
                {
                    if (DoCastSpellIfCan(m_creature, SPELL_DEAFENING_ROAR) == CAST_OK)
                        m_uiDeafeningRoarTimer = 20000;
                }
                else
                    m_uiDeafeningRoarTimer -= uiDiff;
            }
        }
    }
};

CreatureAI* GetAI_boss_yogg_saron(Creature* pCreature)
{
    return new boss_yogg_saronAI(pCreature);
}

/*######
## npc_voice_yogg_saron
######*/

struct MANGOS_DLL_DECL npc_voice_yogg_saronAI : public Scripted_NoMovementAI
{
    npc_voice_yogg_saronAI(Creature* pCreature) : Scripted_NoMovementAI(pCreature)
    {
        m_bIsRegularMode = pCreature->GetMap()->IsRegularDifficulty();
        Reset();
    }

    bool m_bIsRegularMode;

    uint8 m_uiPhase;
    uint8 m_uiMaxPortals;

    uint32 m_uiBerserkTimer;
    uint32 m_uiSummonGuardianTimer;
    uint32 m_uiCrusherTentacleTimer;
    uint32 m_uiCorruptorTentacleTimer;
    uint32 m_uiConstrictorTentacleTimer;
    uint32 m_uiMadnessTimer;
    uint32 m_uiGuardianTimer;

    void Reset() override
    {
        m_uiPhase                       = PHASE_INTRO;
        m_uiBerserkTimer                = 0;
        m_uiSummonGuardianTimer         = 1000;
        m_uiCrusherTentacleTimer        = 1000;
        m_uiCorruptorTentacleTimer      = 1000;
        m_uiConstrictorTentacleTimer    = 1000;
        m_uiMadnessTimer                = 60000;
        m_uiGuardianTimer               = 1000;

        m_uiMaxPortals                  = m_bIsRegularMode ? 4 : 10;
    }

    void AttackStart(Unit* /*pWho*/) override { }
    void MoveInLineOfSight(Unit* /*pWho*/) override { }

    void ReceiveAIEvent(AIEventType eventType, Creature* /*pSender*/, Unit* pInvoker, uint32 uiMiscValue) override
    {
        switch (eventType)
        {
            case AI_EVENT_START_EVENT:
                m_uiPhase = PHASE_SARA;
                m_uiBerserkTimer = 15 * MINUTE * IN_MILLISECONDS;
                break;
            case AI_EVENT_START_EVENT_A:
                m_uiPhase = PHASE_VISIONS;
                break;
            case AI_EVENT_START_EVENT_B:
                m_uiPhase = PHASE_OLD_GOD;
                break;
        }
    }

    void JustSummoned(Creature* pSummoned) override
    {
        switch (pSummoned->GetEntry())
        {
            case NPC_DESCEND_INTO_MADNESS:
                // ToDo: add the teleporting spells
                pSummoned->CastSpell(pSummoned, SPELL_TELEPORT_PORTAL_VISUAL, true);
                break;
            case NPC_CONSTRICTOR_TENTACLE:
                pSummoned->CastSpell(pSummoned, SPELL_TENTACLE_VOID_ZONE, true);
                pSummoned->CastSpell(pSummoned, SPELL_ERUPT, true);
                pSummoned->CastSpell(pSummoned, SPELL_LUNGE, true);
                pSummoned->SetInCombatWithZone();
                break;
            case NPC_CRUSHER_TENTACLE:
                pSummoned->CastSpell(pSummoned, SPELL_TENTACLE_VOID_ZONE_BIG, true);
                pSummoned->CastSpell(pSummoned, SPELL_CRUSH, true);
                pSummoned->CastSpell(pSummoned, SPELL_DIMINISH_POWER, true);
                pSummoned->CastSpell(pSummoned, SPELL_FOCUSED_ANGER, true);
                pSummoned->CastSpell(pSummoned, SPELL_ERUPT, true);
                pSummoned->SetInCombatWithZone();
                break;
            case NPC_CORRUPTOR_TENTACLE:
                pSummoned->CastSpell(pSummoned, SPELL_TENTACLE_VOID_ZONE_BIG, true);
                pSummoned->CastSpell(pSummoned, SPELL_ERUPT, true);
                pSummoned->SetInCombatWithZone();
                break;
        }
    }
    
    void UpdateAI(const uint32 uiDiff) override
    {
        if (m_uiBerserkTimer)
        {
            if (m_uiBerserkTimer <= uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_EXTINGUISH_LIFE) == CAST_OK)
                    m_uiBerserkTimer = 0;
            }
            else
                m_uiBerserkTimer -= uiDiff;
        }
        
        if (m_uiPhase == PHASE_SARA)
        {
            if (m_uiSummonGuardianTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_SUMMON_GUARDIAN_YOGG) == CAST_OK)
                    m_uiSummonGuardianTimer = 20000;
            }
            else
                m_uiSummonGuardianTimer -= uiDiff;
        }
        else if (m_uiPhase == PHASE_VISIONS)
        {
            if (m_uiCorruptorTentacleTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_CORRUPTOR_TENTACLE) == CAST_OK)
                    m_uiCorruptorTentacleTimer = 30000;
            }
            else
                m_uiCorruptorTentacleTimer -= uiDiff;

            if (m_uiCrusherTentacleTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_CRUSHER_TENTACLE) == CAST_OK)
                    m_uiCrusherTentacleTimer = 60000;
            }
            else
                m_uiCrusherTentacleTimer -= uiDiff;

            if (m_uiConstrictorTentacleTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_CONSTRICTOR_TENTACLE) == CAST_OK)
                    m_uiConstrictorTentacleTimer = 25000;
            }
            else
                m_uiConstrictorTentacleTimer -= uiDiff;

            if (m_uiMadnessTimer < uiDiff)
            {
                float fX, fY, fZ, fAng;
                for (uint8 i = 0; i < m_uiMaxPortals; ++i)
                {
                    fAng = (2 * M_PI_F / m_uiMaxPortals) * i;
                    m_creature->GetClosePoint(fX, fY, fZ, 0, 22.0f, fAng);
                    m_creature->SummonCreature(NPC_DESCEND_INTO_MADNESS, fX, fY, fZ, 0, TEMPSUMMON_DEAD_DESPAWN, 0);
                }
                m_uiMadnessTimer = 90000;
            }
            else
                m_uiMadnessTimer -= uiDiff;
        }
        else if (m_uiPhase == PHASE_OLD_GOD)
        {
            if (m_uiGuardianTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_IMMORTAL_GUARDIAN) == CAST_OK)
                    m_uiGuardianTimer = 15000;
            }
            else
                m_uiGuardianTimer -= uiDiff;
        }
    }
};

CreatureAI* GetAI_npc_voice_yogg_saron(Creature* pCreature)
{
    return new npc_voice_yogg_saronAI(pCreature);
}

/*######
## npc_guardian_of_yogg
######*/

struct MANGOS_DLL_DECL npc_guardian_of_yoggAI : public ScriptedAI
{
    npc_guardian_of_yoggAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (instance_ulduar*)pCreature->GetInstanceData();
        m_bIsRegularMode = pCreature->GetMap()->IsRegularDifficulty();
        Reset();
    }

    instance_ulduar* m_pInstance;
    bool m_bIsRegularMode;

    uint32 m_uiDarkVolleyTimer;
    uint32 m_uiDominateMindTimer;

    void Reset() override
    {
        m_uiDarkVolleyTimer = 10000;
        m_uiDominateMindTimer = 25000;
    }

    void AttackStart(Unit* pWho) override
    {
        if (pWho->GetEntry() == NPC_SARA)
            return;

        ScriptedAI::AttackStart(pWho);
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        DoCastSpellIfCan(m_creature, m_bIsRegularMode ? SPELL_SHADOW_NOVA : SPELL_SHADOW_NOVA_H, CAST_TRIGGERED);
    }

    void JustReachedHome() override
    {
        if (m_pInstance)
        {
            if (m_pInstance->GetData(TYPE_YOGGSARON) != FAIL)
                m_pInstance->SetData(TYPE_YOGGSARON, FAIL);
        }

        m_creature->ForcedDespawn();
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiDarkVolleyTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_DARK_VOLLEY) == CAST_OK)
                m_uiDarkVolleyTimer = urand(10000, 25000);
        }
        else
            m_uiDarkVolleyTimer -= uiDiff;

        if (m_uiDominateMindTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_DOMINATE_MIND) == CAST_OK)
                m_uiDominateMindTimer = urand(30000, 40000);
        }
        else
            m_uiDominateMindTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_guardian_of_yogg(Creature* pCreature)
{
    return new npc_guardian_of_yoggAI(pCreature);
}

/*######
## npc_ominous_cloud
######*/

struct MANGOS_DLL_DECL npc_ominous_cloudAI : public Scripted_NoMovementAI
{
    npc_ominous_cloudAI(Creature* pCreature) : Scripted_NoMovementAI(pCreature) { Reset(); }

    uint32 m_uiDelayTimer;

    void Reset() override
    {
        m_uiDelayTimer = 0;
    }

    void AttackStart(Unit* /*pWho*/) override { }

    void MoveInLineOfSight(Unit* pWho) override
    {
        if (!m_uiDelayTimer && pWho->GetTypeId() == TYPEID_PLAYER && !((Player*)pWho)->isGameMaster() && m_creature->IsWithinDistInMap(pWho, 7.0f))
        {
            if (DoCastSpellIfCan(m_creature, SPELL_BOIL_OMNIOUSLY) == CAST_OK)
            {
                DoScriptText(EMOTE_CLOUD_BOIL, m_creature, pWho);
                m_uiDelayTimer = 10000;
            }
        }
    }

    void SpellHit(Unit* /*pCaster*/, const SpellEntry* pSpell) override
    {
        if (pSpell->Id == SPELL_SUMMON_GUARDIAN_YOGG)
            m_uiDelayTimer = 10000;
    }

    void JustSummoned(Creature* pSummoned) override
    {
        if (pSummoned->GetEntry() == NPC_GUARDIAN_OF_YOGG)
            pSummoned->SetInCombatWithZone();
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (m_uiDelayTimer)
        {
            if (m_uiDelayTimer <= uiDiff)
                m_uiDelayTimer = 0;
            else
                m_uiDelayTimer -= uiDiff;
        }
    }
};

CreatureAI* GetAI_npc_ominous_cloud(Creature* pCreature)
{
    return new npc_ominous_cloudAI(pCreature);
}

void AddSC_boss_yogg_saron()
{
    Script* pNewScript;
    
    pNewScript = new Script;
    pNewScript->Name = "boss_yogg_saron";
    pNewScript->GetAI = &GetAI_boss_yogg_saron;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "boss_sara";
    pNewScript->GetAI = &GetAI_boss_sara;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_voice_yogg_saron";
    pNewScript->GetAI = &GetAI_npc_voice_yogg_saron;
    pNewScript->RegisterSelf();
    
    pNewScript = new Script;
    pNewScript->Name = "npc_guardian_of_yogg";
    pNewScript->GetAI = &GetAI_npc_guardian_of_yogg;
    pNewScript->RegisterSelf();
    
    pNewScript = new Script;
    pNewScript->Name = "npc_ominous_cloud";
    pNewScript->GetAI = &GetAI_npc_ominous_cloud;
    pNewScript->RegisterSelf();
}
