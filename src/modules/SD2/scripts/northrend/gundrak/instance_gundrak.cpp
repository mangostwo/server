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
SDName: instance_gundrak
SD%Complete: 80
SDComment: Reload case for bridge support is missing, achievement support is missing
SDCategory: Gundrak
EndScriptData */

#include "precompiled.h"
#include "gundrak.h"

typedef std::map<uint8, uint32>  TypeTimerMap;
typedef std::pair<uint8, uint32> TypeTimerPair;

static bool sortFromEastToWest(Creature* pFirst, Creature* pSecond)
{
    return pFirst && pSecond && pFirst->GetPositionY() < pSecond->GetPositionY();
}

struct is_gundrak : public InstanceScript
{
    is_gundrak() : InstanceScript("instance_gundrak") {}

    class instance_gundrak : public ScriptedInstance
    {
    public:
        instance_gundrak(Map* pMap) : ScriptedInstance(pMap),
            m_bLessRabi(false)
        {
            Initialize();
        }

        ~instance_gundrak() {}

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));

            m_vStalkerCasterGuids.reserve(3);
            m_vStalkerTargetGuids.reserve(3);
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_SLADRAN:
            case NPC_ELEMENTAL:
            case NPC_COLOSSUS:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
                break;

            case NPC_INVISIBLE_STALKER:
                m_luiStalkerGUIDs.push_back(pCreature->GetObjectGuid());
                break;
            case NPC_SLADRAN_SUMMON_T:
                m_lSummonTargetsGuids.push_back(pCreature->GetObjectGuid());
                break;

            case NPC_LIVIN_MOJO:
                // Store only the Mojos used to activate the Colossus
                if (pCreature->GetPositionX() > 1650.0f)
                    m_sColossusMojosGuids.insert(pCreature->GetObjectGuid());
                break;
            }
        }

        /* TODO: Reload case need some love!
        *  Problem is to get the bridge/ collision work correct in relaod case.
        *  To provide correct functionality(expecting testers to activate all altars in reload case), the Keys aren't loaded, too
        *  TODO: When fixed, also remove the SPECIAL->DONE data translation in Load().
        *
        *  For the Keys should be used something like this, and for bridge and collision similar
        *
        *   if (m_auiEncounter[0] == SPECIAL && m_auiEncounter[1] == SPECIAL && m_auiEncounter[2] == SPECIAL)
        *       pGo->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
        *   else
        *       pGo->SetGoState(GO_STATE_READY);
        */
        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
            case GO_ECK_DOOR:
                if (m_auiEncounter[TYPE_MOORABI] == DONE && !instance->IsRegularDifficulty())
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_ECK_UNDERWATER_DOOR:
                if (m_auiEncounter[TYPE_ECK] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_GALDARAH_DOOR:
                pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_EXIT_DOOR_L:
                if (m_auiEncounter[TYPE_GALDARAH] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_EXIT_DOOR_R:
                if (m_auiEncounter[TYPE_GALDARAH] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_ALTAR_OF_SLADRAN:
                if (m_auiEncounter[TYPE_SLADRAN] == DONE)
                    pGo->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NO_INTERACT);
                break;
            case GO_ALTAR_OF_MOORABI:
                if (m_auiEncounter[TYPE_MOORABI] == DONE)
                    pGo->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NO_INTERACT);
                break;
            case GO_ALTAR_OF_COLOSSUS:
                if (m_auiEncounter[TYPE_COLOSSUS] == DONE)
                    pGo->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NO_INTERACT);
                break;
            case GO_SNAKE_KEY:
            case GO_TROLL_KEY:
            case GO_MAMMOTH_KEY:
            case GO_RHINO_KEY:
            case GO_BRIDGE:
            case GO_COLLISION:
                break;

            default:
                return;
            }
            m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
        }

        void OnCreatureEnterCombat(Creature* pCreature) override
        {
            if (pCreature->GetEntry() == NPC_LIVIN_MOJO)
            {
                // If not found in the set, or the event is already started, return
                if (m_sColossusMojosGuids.find(pCreature->GetObjectGuid()) == m_sColossusMojosGuids.end())
                    return;

                // Move all 4 Mojos to evade and move to the Colossus position
                for (GuidSet::const_iterator itr = m_sColossusMojosGuids.begin(); itr != m_sColossusMojosGuids.end(); ++itr)
                {
                    if (Creature* pMojo = instance->GetCreature(*itr))
                        pMojo->AI()->EnterEvadeMode();
                }
            }
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            debug_log("SD2: Instance Gundrak: SetData received for type %u with data %u", uiType, uiData);

            switch (uiType)
            {
            case TYPE_SLADRAN:
                m_auiEncounter[TYPE_SLADRAN] = uiData;
                if (uiData == DONE)
                if (GameObject* pGo = GetSingleGameObjectFromStorage(GO_ALTAR_OF_SLADRAN))
                    pGo->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NO_INTERACT);
                if (uiData == FAIL)
                    m_uisWhySnakesAchievPlayers.clear();
                if (uiData == SPECIAL)
                    m_mAltarInProgress.insert(TypeTimerPair(TYPE_SLADRAN, TIMER_VISUAL_ALTAR));
                break;
            case TYPE_MOORABI:
                m_auiEncounter[TYPE_MOORABI] = uiData;
                if (uiData == DONE)
                {
                    if (!instance->IsRegularDifficulty())
                        DoUseDoorOrButton(GO_ECK_DOOR);
                    if (GameObject* pGo = GetSingleGameObjectFromStorage(GO_ALTAR_OF_MOORABI))
                        pGo->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NO_INTERACT);
                }
                if (uiData == IN_PROGRESS)
                    SetLessRabiAchievementCriteria(true);
                if (uiData == SPECIAL)
                    m_mAltarInProgress.insert(TypeTimerPair(TYPE_MOORABI, TIMER_VISUAL_ALTAR));
                break;
            case TYPE_COLOSSUS:
                m_auiEncounter[TYPE_COLOSSUS] = uiData;
                if (uiData == DONE)
                if (GameObject* pGo = GetSingleGameObjectFromStorage(GO_ALTAR_OF_COLOSSUS))
                    pGo->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NO_INTERACT);
                if (uiData == FAIL)
                {
                    for (GuidSet::const_iterator itr = m_sColossusMojosGuids.begin(); itr != m_sColossusMojosGuids.end(); ++itr)
                    {
                        if (Creature* pMojo = instance->GetCreature(*itr))
                            pMojo->Respawn();
                    }
                }
                if (uiData == SPECIAL)
                    m_mAltarInProgress.insert(TypeTimerPair(TYPE_COLOSSUS, TIMER_VISUAL_ALTAR));
                break;
            case TYPE_GALDARAH:
                m_auiEncounter[TYPE_GALDARAH] = uiData;
                DoUseDoorOrButton(GO_GALDARAH_DOOR);
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_EXIT_DOOR_L);
                    DoUseDoorOrButton(GO_EXIT_DOOR_R);
                }
                if (uiData == FAIL)
                    m_uisShareLoveAchievPlayers.clear();
                break;
            case TYPE_ECK:
                m_auiEncounter[TYPE_ECK] = uiData;
                if (uiData == DONE)
                    DoUseDoorOrButton(GO_ECK_UNDERWATER_DOOR);
                break;
            case TYPE_ACHIEV_WHY_SNAKES:
                // insert the players who failed the achiev and haven't been already inserted in the set
                if (m_uisWhySnakesAchievPlayers.find(uiData) == m_uisWhySnakesAchievPlayers.end())
                    m_uisWhySnakesAchievPlayers.insert(uiData);
                return;
            case TYPE_ACHIEV_SHARE_LOVE:
                // insert players who got stampeled and haven't been already inserted in the set
                if (m_uisShareLoveAchievPlayers.find(uiData) == m_uisShareLoveAchievPlayers.end())
                    m_uisShareLoveAchievPlayers.insert(uiData);
                return;
            case TYPE_ACHIEV_LESS_RABI:
                SetLessRabiAchievementCriteria(bool(uiData));
                return;
            default:
                script_error_log("Instance Gundrak: ERROR SetData = %u for type %u does not exist/not implemented.", uiType, uiData);
                return;
            }

            if (uiData == DONE || uiData == SPECIAL)                // Save activated altars, too
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << m_auiEncounter[TYPE_SLADRAN] << " " << m_auiEncounter[TYPE_MOORABI] << " " << m_auiEncounter[TYPE_COLOSSUS] << " " << m_auiEncounter[TYPE_GALDARAH] << " "
                    << m_auiEncounter[TYPE_ECK];

                m_strInstData = saveStream.str();

                SaveToDB();
                OUT_SAVE_INST_DATA_COMPLETE;
            }
        }

        uint32 GetData(uint32 uiType) const override
        {
            if (uiType < MAX_ENCOUNTER)
                return m_auiEncounter[uiType];

            return 0;
        }

        uint64 GetData64(uint32 uiType) const override
        {
            switch (uiType)
            {
            case TYPE_SLADRAN:
                if (!m_lSummonTargetsGuids.empty())
                {
                    GuidList::const_iterator iter = m_lSummonTargetsGuids.begin();
                    advance(iter, urand(0, m_lSummonTargetsGuids.size() - 1));
                    return *iter;
                }
                break;
            default:
                break;
            }
            return 0;
        }

        const char* Save() const override { return m_strInstData.c_str(); }
        void Load(const char* chrIn) override
        {
            if (!chrIn)
            {
                OUT_LOAD_INST_DATA_FAIL;
                return;
            }

            OUT_LOAD_INST_DATA(chrIn);

            std::istringstream loadStream(chrIn);
            loadStream >> m_auiEncounter[TYPE_SLADRAN] >> m_auiEncounter[TYPE_MOORABI] >> m_auiEncounter[TYPE_COLOSSUS] >> m_auiEncounter[TYPE_GALDARAH] >> m_auiEncounter[TYPE_ECK];

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                    m_auiEncounter[i] = NOT_STARTED;

                // TODO: REMOVE when bridge/ collision reloading correctly working
                if (m_auiEncounter[i] == SPECIAL)
                    m_auiEncounter[i] = DONE;
            }

            OUT_LOAD_INST_DATA_COMPLETE;
        }

        bool CheckAchievementCriteriaMeet(uint32 uiCriteriaId, Player const* pSource, Unit const* pTarget, uint32 uiMiscValue1 /* = 0*/) const override
        {
            switch (uiCriteriaId)
            {
            case ACHIEV_CRIT_LESS_RABI:
                return m_bLessRabi;
            case ACHIEV_CRIT_SHARE_LOVE:
                // Return true if all the players in the group got stampeled
                return m_uisShareLoveAchievPlayers.size() == MIN_LOVE_SHARE_PLAYERS;
                // ToDo: enable this criteria when the script will be implemented
                // case ACHIEV_CRIT_WHY_SNAKES:
                //    // Return true if not found in the set
                //    return m_uisWhySnakesAchievPlayers.find(pSource->GetGUIDLow()) == m_uisWhySnakesAchievPlayers.end();

            default:
                return false;
            }
        }

        void Update(uint32 uiDiff) override
        {
            // Possible multible altars used at the same time, process their timers
            if (!m_mAltarInProgress.empty())
            {
                for (TypeTimerMap::iterator itr = m_mAltarInProgress.begin(); itr != m_mAltarInProgress.end();)
                {
                    if (itr->second < uiDiff)
                    {
                        // Do Visual Effect
                        DoAltarVisualEffect(itr->first);
                        // Set Timer for Beam-Duration
                        m_mBeamInProgress.insert(TypeTimerPair(itr->first, TIMER_VISUAL_BEAM));
                        // Remove this timer, as processed
                        m_mAltarInProgress.erase(itr++);
                    }
                    else
                    {
                        itr->second -= uiDiff;
                        ++itr;
                    }
                }
            }

            // Possible multible beams used at the same time, process their timers
            if (!m_mBeamInProgress.empty())
            {
                for (TypeTimerMap::iterator itr = m_mBeamInProgress.begin(); itr != m_mBeamInProgress.end();)
                {
                    if (itr->second < uiDiff)
                    {
                        // Use Key
                        switch (itr->first)
                        {
                        case TYPE_SLADRAN: DoUseDoorOrButton(GO_SNAKE_KEY); break;
                        case TYPE_MOORABI: DoUseDoorOrButton(GO_MAMMOTH_KEY); break;
                        case TYPE_COLOSSUS: DoUseDoorOrButton(GO_TROLL_KEY); break;
                        }
                        // Set Timer for Beam-Duration
                        m_mKeyInProgress.insert(TypeTimerPair(itr->first, TIMER_VISUAL_KEY));
                        m_mBeamInProgress.erase(itr++);
                    }
                    else
                    {
                        itr->second -= uiDiff;
                        ++itr;
                    }
                }
            }

            // Activate Bridge if all Three Encounters are used
            if (!m_mKeyInProgress.empty())
            {
                for (TypeTimerMap::iterator itr = m_mKeyInProgress.begin(); itr != m_mKeyInProgress.end();)
                {
                    if (itr->second < uiDiff)
                    {
                        // Activate Bridge (and all other Keys) if we are on the last Key, and all other keys are already set
                        if (m_auiEncounter[0] == SPECIAL && m_auiEncounter[1] == SPECIAL && m_auiEncounter[2] == SPECIAL
                            && m_mAltarInProgress.empty() && m_mBeamInProgress.empty() && m_mKeyInProgress.size() == 1)
                        {
                            DoUseDoorOrButton(GO_COLLISION);
                            DoUseDoorOrButton(GO_RHINO_KEY, 0, true);

                            // The already closed keys cannot be done with DoUseDoorOrButton
                            if (GameObject* pTrollKey = GetSingleGameObjectFromStorage(GO_TROLL_KEY))
                                pTrollKey->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
                            if (GameObject* pMammothKey = GetSingleGameObjectFromStorage(GO_MAMMOTH_KEY))
                                pMammothKey->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
                            if (GameObject* pSnakeKey = GetSingleGameObjectFromStorage(GO_SNAKE_KEY))
                                pSnakeKey->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);

                            // GO_BRIDGE is type 35 (TRAP_DOOR) and needs to be handled directly
                            // Real Use of this GO is unknown, but this change of state is expected
                            DoUseDoorOrButton(GO_BRIDGE);
                        }
                        // Remove this timer, as processed
                        m_mKeyInProgress.erase(itr++);
                    }
                    else
                    {
                        itr->second -= uiDiff;
                        ++itr;
                    }
                }
            }
        }

    private:
        void SetLessRabiAchievementCriteria(bool bIsMet) { m_bLessRabi = bIsMet; }
        void DoAltarVisualEffect(uint8 uiType)
        {
            // Sort the lists if not yet done
            if (!m_luiStalkerGUIDs.empty())
            {
                float fHeight = 10.0f;                              // A bit higher than the altar is needed
                if (GameObject* pCollusAltar = GetSingleGameObjectFromStorage(GO_ALTAR_OF_COLOSSUS))
                    fHeight += pCollusAltar->GetPositionZ();

                std::list<Creature*> lStalkerTargets, lStalkerCasters;
                for (GuidList::const_iterator itr = m_luiStalkerGUIDs.begin(); itr != m_luiStalkerGUIDs.end(); ++itr)
                {
                    if (Creature* pStalker = instance->GetCreature(*itr))
                    {
                        if (pStalker->GetPositionZ() > fHeight)
                            lStalkerTargets.push_back(pStalker);
                        else
                            lStalkerCasters.push_back(pStalker);
                    }
                }
                m_luiStalkerGUIDs.clear();

                lStalkerTargets.sort(sortFromEastToWest);
                lStalkerCasters.sort(sortFromEastToWest);

                for (std::list<Creature*>::const_iterator itr = lStalkerTargets.begin(); itr != lStalkerTargets.end(); ++itr)
                    m_vStalkerTargetGuids.push_back((*itr)->GetObjectGuid());
                for (std::list<Creature*>::const_iterator itr = lStalkerCasters.begin(); itr != lStalkerCasters.end(); ++itr)
                    m_vStalkerCasterGuids.push_back((*itr)->GetObjectGuid());
            }

            // Verify that the DB has enough trigger spawned
            if (m_vStalkerTargetGuids.size() < 3 || m_vStalkerCasterGuids.size() < 3)
                return;

            // Get the Index from the bosses
            uint8 uiIndex = 0;
            switch (uiType)
            {
            case TYPE_SLADRAN:  uiIndex = 0; break;
            case TYPE_COLOSSUS: uiIndex = 1; break;
            case TYPE_MOORABI:  uiIndex = 2; break;
            default:
                return;
            }

            Creature* pTarget = instance->GetCreature(m_vStalkerTargetGuids[uiIndex]);
            Creature* pCaster = instance->GetCreature(m_vStalkerCasterGuids[uiIndex]);

            if (!pTarget || !pCaster)
                return;

            uint32 auiFireBeamSpells[3] = { SPELL_BEAM_SNAKE, SPELL_BEAM_ELEMENTAL, SPELL_BEAM_MAMMOTH };

            // Cast from Caster to Target
            pCaster->CastSpell(pTarget, auiFireBeamSpells[uiIndex], false);
        }

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        TypeTimerMap m_mAltarInProgress;
        TypeTimerMap m_mBeamInProgress;
        TypeTimerMap m_mKeyInProgress;

        GuidList m_luiStalkerGUIDs;
        GuidList m_lSummonTargetsGuids;
        GuidVector m_vStalkerCasterGuids;
        GuidVector m_vStalkerTargetGuids;
        GuidSet m_sColossusMojosGuids;

        bool m_bLessRabi;

        std::set<uint32> m_uisShareLoveAchievPlayers;
        std::set<uint32> m_uisWhySnakesAchievPlayers;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_gundrak(pMap);
    }
};

struct go_gundrak_altar : public GameObjectScript
{
    go_gundrak_altar() : GameObjectScript("go_gundrak_altar") {}

    bool OnUse(Player* /*pPlayer*/, GameObject* pGo) override
    {
        ScriptedInstance* pInstance = (ScriptedInstance*)pGo->GetInstanceData();

        if (!pInstance)
            return false;

        switch (pGo->GetEntry())
        {
        case GO_ALTAR_OF_SLADRAN:  pInstance->SetData(TYPE_SLADRAN, SPECIAL);  break;
        case GO_ALTAR_OF_MOORABI:  pInstance->SetData(TYPE_MOORABI, SPECIAL);  break;
        case GO_ALTAR_OF_COLOSSUS: pInstance->SetData(TYPE_COLOSSUS, SPECIAL); break;
        }

        pGo->UseDoorOrButton(0, true);
        return true;
    }
};

void AddSC_instance_gundrak()
{
    Script* s;

    s = new is_gundrak();
    s->RegisterSelf();

    s = new go_gundrak_altar();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "go_gundrak_altar";
    //pNewScript->pGOUse = &GOUse_go_gundrak_altar;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_gundrak";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_gundrak;
    //pNewScript->RegisterSelf();
}
