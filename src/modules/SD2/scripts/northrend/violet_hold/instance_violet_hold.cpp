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
SDName: Instance_Violet_Hold
SD%Complete: 75
SDComment: Prison defense system requires more research
SDCategory: Violet Hold
EndScriptData */

#include "precompiled.h"
#include "violet_hold.h"

struct BossInformation
{
    uint32 uiType, uiEntry, uiGhostEntry, uiWayPointId;
    float fX, fY, fZ;                                       // Waypoint for Saboteur
    int32 iSayEntry;
};

struct BossSpawn
{
    uint32 uiEntry;
    float fX, fY, fZ, fO;
};

static const BossInformation aBossInformation[] =
{
    { TYPE_EREKEM, NPC_EREKEM, NPC_ARAKKOA, 1, 1877.03f, 853.84f, 43.33f, SAY_RELEASE_EREKEM },
    { TYPE_ZURAMAT, NPC_ZURAMAT, NPC_VOID_LORD, 1, 1922.41f, 847.95f, 47.15f, SAY_RELEASE_ZURAMAT },
    { TYPE_XEVOZZ, NPC_XEVOZZ, NPC_ETHERAL, 1, 1903.61f, 838.46f, 38.72f, SAY_RELEASE_XEVOZZ },
    { TYPE_ICHORON, NPC_ICHORON, NPC_SWIRLING, 1, 1915.52f, 779.13f, 35.94f, SAY_RELEASE_ICHORON },
    { TYPE_LAVANTHOR, NPC_LAVANTHOR, NPC_LAVA_HOUND, 1, 1855.28f, 760.85f, 38.65f, 0 },
    { TYPE_MORAGG, NPC_MORAGG, NPC_WATCHER, 1, 1890.51f, 752.85f, 47.66f, 0 }
};

static const float fDefenseSystemLoc[4] = { 1888.146f, 803.382f, 58.604f, 3.072f };
static const float fGuardExitLoc[3] = { 1806.955f, 803.851f, 44.36f };

static const uint32 aRandomPortalNpcs[5] = { NPC_AZURE_INVADER, NPC_MAGE_HUNTER, NPC_AZURE_SPELLBREAKER, NPC_AZURE_BINDER, NPC_AZURE_MAGE_SLAYER };
static const uint32 aRandomIntroNpcs[4] = { NPC_AZURE_BINDER_INTRO, NPC_AZURE_INVADER_INTRO, NPC_AZURE_SPELLBREAKER_INTRO, NPC_AZURE_MAGE_SLAYER_INTRO };

static const int32 aSealWeakYell[3] = { SAY_SEAL_75, SAY_SEAL_50, SAY_SEAL_5 };

struct is_violet_hold : public InstanceScript
{
    is_violet_hold() : InstanceScript("instance_violet_hold") {}

    class instance_violet_hold : public ScriptedInstance
    {
    public:
        instance_violet_hold(Map* pMap) : ScriptedInstance(pMap),
            m_uiWorldState(0),
            m_uiWorldStateSealCount(100),
            m_uiWorldStatePortalCount(0),

            m_uiPortalId(0),
            m_uiPortalTimer(0),
            m_uiMaxCountPortalLoc(0),

            m_uiSealYellCount(0),
            m_uiEventResetTimer(0),

            m_bIsVoidDance(false),
            m_bIsDefenseless(false),
            m_bIsDehydratation(false)
        {
            Initialize();
        }

        ~instance_violet_hold()
        {
            // Need to free std::vector<sBossSpawn*> m_vRandomBosses;
            for (std::vector<BossSpawn*>::const_iterator itr = m_vRandomBosses.begin(); itr != m_vRandomBosses.end(); ++itr)
            {
                if (*itr)
                    delete(*itr);
            }
        }

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
            m_uiMaxCountPortalLoc = countof(afPortalLocation) - 1;
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_SINCLARI:
            case NPC_SINCLARI_ALT:
            case NPC_DOOR_SEAL:
            case NPC_EVENT_CONTROLLER:
                break;

            case NPC_EREKEM:
            case NPC_MORAGG:
            case NPC_ICHORON:
            case NPC_XEVOZZ:
            case NPC_LAVANTHOR:
            case NPC_ZURAMAT:
                m_vRandomBossList.push_back(pCreature->GetEntry());
                break;

            case NPC_PORTAL_INTRO:
                m_lIntroPortalList.push_back(pCreature->GetObjectGuid());
                return;
            case NPC_HOLD_GUARD:
                m_lGuardsList.push_back(pCreature->GetObjectGuid());
                return;
            case NPC_EREKEM_GUARD:
                m_lErekemGuardList.push_back(pCreature->GetObjectGuid());
                return;
            case NPC_ARAKKOA_GUARD:
                m_lArakkoaGuardList.push_back(pCreature->GetObjectGuid());
                return;
            case NPC_ICHORON_SUMMON_TARGET:
                m_lIchoronTargetsList.push_back(pCreature->GetObjectGuid());
                return;

            case NPC_ARAKKOA:
            case NPC_VOID_LORD:
            case NPC_ETHERAL:
            case NPC_SWIRLING:
            case NPC_WATCHER:
            case NPC_LAVA_HOUND:
                break;

            default:
                return;
            }
            m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
            case GO_CELL_LAVANTHOR:
                m_mBossToCellMap.insert(BossToCellMap::value_type(NPC_LAVANTHOR, pGo->GetObjectGuid()));
                return;
            case GO_CELL_MORAGG:
                m_mBossToCellMap.insert(BossToCellMap::value_type(NPC_MORAGG, pGo->GetObjectGuid()));
                return;
            case GO_CELL_ZURAMAT:
                m_mBossToCellMap.insert(BossToCellMap::value_type(NPC_ZURAMAT, pGo->GetObjectGuid()));
                return;
            case GO_CELL_XEVOZZ:
                m_mBossToCellMap.insert(BossToCellMap::value_type(NPC_XEVOZZ, pGo->GetObjectGuid()));
                return;
            case GO_CELL_ICHORON:
                m_mBossToCellMap.insert(BossToCellMap::value_type(NPC_ICHORON, pGo->GetObjectGuid()));
                return;
            case GO_CELL_EREKEM:
                m_mBossToCellMap.insert(BossToCellMap::value_type(NPC_EREKEM, pGo->GetObjectGuid()));
                return;
            case GO_CELL_EREKEM_GUARD_L:
                m_mBossToCellMap.insert(BossToCellMap::value_type(NPC_EREKEM, pGo->GetObjectGuid()));
                return;
            case GO_CELL_EREKEM_GUARD_R:
                m_mBossToCellMap.insert(BossToCellMap::value_type(NPC_EREKEM, pGo->GetObjectGuid()));
                return;

            case GO_INTRO_CRYSTAL:
            case GO_PRISON_SEAL_DOOR:
                break;
            }
            m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
        }

        void UpdateCellForBoss(uint32 uiBossEntry, bool bForceClosing = false)
        {
            BossToCellMap::const_iterator itrCellLower = m_mBossToCellMap.lower_bound(uiBossEntry);
            BossToCellMap::const_iterator itrCellUpper = m_mBossToCellMap.upper_bound(uiBossEntry);

            if (itrCellLower == itrCellUpper)
                return;

            for (BossToCellMap::const_iterator itr = itrCellLower; itr != itrCellUpper; ++itr)
            {
                if (!bForceClosing)
                    DoUseDoorOrButton(itr->second);
                else
                {
                    GameObject* pGo = instance->GetGameObject(itr->second);
                    if (pGo && pGo->GetGoType() == GAMEOBJECT_TYPE_DOOR && pGo->GetGoState() == GO_STATE_ACTIVE)
                        pGo->ResetDoorOrButton();
                }
            }
        }

        void ProcessActivationCrystal(Unit* pUser, bool bIsIntro = false)
        {
            if (Creature* pSummon = pUser->SummonCreature(NPC_DEFENSE_SYSTEM, fDefenseSystemLoc[0], fDefenseSystemLoc[1], fDefenseSystemLoc[2], fDefenseSystemLoc[3], TEMPSUMMON_TIMED_DESPAWN, 10000))
            {
                pSummon->CastSpell(pSummon, SPELL_DEFENSE_SYSTEM_VISUAL, true);

                // TODO: figure out how the rest work
                // NPC's NPC_DEFENSE_DUMMY_TARGET are probably channeling some spell to the defense system
            }

            if (bIsIntro)
                DoUseDoorOrButton(GO_INTRO_CRYSTAL);

            // else, kill (and despawn?) certain trash mobs. Also boss affected, but not killed.
        }

        void OnPlayerEnter(Player* pPlayer) override
        {
            UpdateWorldState(m_auiEncounter[TYPE_MAIN] == IN_PROGRESS ? true : false);

            if (m_vRandomBosses.empty())
            {
                SetRandomBosses();
                ResetAll();
            }
        }

        void OnCreatureEnterCombat(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_ZURAMAT:
            case NPC_VOID_LORD:
                SetData(TYPE_ZURAMAT, IN_PROGRESS);
                break;
            case NPC_XEVOZZ:
            case NPC_ETHERAL:
                SetData(TYPE_XEVOZZ, IN_PROGRESS);
                break;
            case NPC_LAVANTHOR:
            case NPC_LAVA_HOUND:
                SetData(TYPE_LAVANTHOR, IN_PROGRESS);
                break;
            case NPC_MORAGG:
            case NPC_WATCHER:
                SetData(TYPE_MORAGG, IN_PROGRESS);
                break;
            case NPC_EREKEM:
            case NPC_ARAKKOA:
                SetData(TYPE_EREKEM, IN_PROGRESS);
                break;
            case NPC_ICHORON:
            case NPC_SWIRLING:
                SetData(TYPE_ICHORON, IN_PROGRESS);
                break;
            case NPC_CYANIGOSA:
                SetData(TYPE_CYANIGOSA, IN_PROGRESS);
                break;
            case NPC_AZURE_CAPTAIN:
            case NPC_AZURE_RAIDER:
            case NPC_AZURE_SORCEROR:
            case NPC_AZURE_STALKER:
            case NPC_AZURE_INVADER:
            case NPC_MAGE_HUNTER:
            case NPC_AZURE_SPELLBREAKER:
            case NPC_AZURE_BINDER:
            case NPC_AZURE_MAGE_SLAYER:
                // Interrupt door seal casting (if necessary)
                pCreature->InterruptNonMeleeSpells(false);
                break;
            }
        }

        void OnCreatureEvade(Creature* pCreature)
        {
            switch (pCreature->GetEntry())
            {
            case NPC_ZURAMAT:
            case NPC_VOID_LORD:
                SetData(TYPE_ZURAMAT, FAIL);
                pCreature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE);
                break;
            case NPC_XEVOZZ:
            case NPC_ETHERAL:
                SetData(TYPE_XEVOZZ, FAIL);
                pCreature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE);
                break;
            case NPC_LAVANTHOR:
            case NPC_LAVA_HOUND:
                SetData(TYPE_LAVANTHOR, FAIL);
                pCreature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE);
                break;
            case NPC_MORAGG:
            case NPC_WATCHER:
                SetData(TYPE_MORAGG, FAIL);
                pCreature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE);
                break;
            case NPC_EREKEM:
            case NPC_ARAKKOA:
                SetData(TYPE_EREKEM, FAIL);
                pCreature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE);
                break;
            case NPC_EREKEM_GUARD:
                pCreature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE);
                break;
            case NPC_ICHORON:
            case NPC_SWIRLING:
                SetData(TYPE_ICHORON, FAIL);
                pCreature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE);
                break;
            case NPC_CYANIGOSA:
                SetData(TYPE_CYANIGOSA, FAIL);
                break;
            case NPC_AZURE_CAPTAIN:
            case NPC_AZURE_RAIDER:
            case NPC_AZURE_SORCEROR:
            case NPC_AZURE_STALKER:
            case NPC_AZURE_INVADER:
            case NPC_MAGE_HUNTER:
            case NPC_AZURE_SPELLBREAKER:
            case NPC_AZURE_BINDER:
            case NPC_AZURE_MAGE_SLAYER:
                // Allow them to finish off the door seal
                pCreature->SetWalk(false);
                pCreature->GetMotionMaster()->MovePoint(1, fSealAttackLoc[0], fSealAttackLoc[1], fSealAttackLoc[2]);
                break;
            }
        }

        void OnCreatureDeath(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_ZURAMAT:
            case NPC_VOID_LORD:
                SetData(TYPE_ZURAMAT, DONE);
                break;
            case NPC_XEVOZZ:
            case NPC_ETHERAL:
                SetData(TYPE_XEVOZZ, DONE);
                break;
            case NPC_LAVANTHOR:
            case NPC_LAVA_HOUND:
                SetData(TYPE_LAVANTHOR, DONE);
                break;
            case NPC_MORAGG:
            case NPC_WATCHER:
                SetData(TYPE_MORAGG, DONE);
                break;
            case NPC_EREKEM:
            case NPC_ARAKKOA:
                SetData(TYPE_EREKEM, DONE);
                break;
            case NPC_ICHORON:
            case NPC_SWIRLING:
                SetData(TYPE_ICHORON, DONE);
                break;
            case NPC_CYANIGOSA:
                SetData(TYPE_CYANIGOSA, DONE);
                break;
            case NPC_VOID_SENTRY:
                if (GetData(TYPE_ZURAMAT) == IN_PROGRESS)
                    m_bIsVoidDance = false;
                break;
            }
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            debug_log("SD2: instance_violet_hold: SetData got type %u, data %u.", uiType, uiData);

            switch (uiType)
            {
            case TYPE_MAIN:
                if (uiData == m_auiEncounter[uiType])
                    return;
                if (m_auiEncounter[uiType] == DONE)
                    return;

                switch (uiData)
                {
                case IN_PROGRESS:
                    // ToDo: enable the prison defense system when implemented
                    DoUseDoorOrButton(GO_PRISON_SEAL_DOOR);
                    UpdateWorldState();
                    m_bIsDefenseless = true;
                    m_uiPortalId = urand(0, 2);
                    m_uiPortalTimer = 15000;
                    break;
                case FAIL:
                    if (Creature* pSinclari = GetSingleCreatureFromStorage(NPC_SINCLARI))
                        pSinclari->DealDamage(pSinclari, pSinclari->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                    if (Creature* pController = GetSingleCreatureFromStorage(NPC_EVENT_CONTROLLER))
                        pController->AI()->EnterEvadeMode();
                    // Reset the event (creature cleanup is handled in creature_linking)
                    DoUseDoorOrButton(GO_PRISON_SEAL_DOOR); // open instance door
                    ResetAll();
                    m_uiEventResetTimer = 20000;            // Timer may not be correct - 20 sec is default reset timer for blizz
                    break;
                case DONE:
                    DoUseDoorOrButton(GO_PRISON_SEAL_DOOR);
                    UpdateWorldState(false);
                    break;
                case SPECIAL:
                    break;
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_SEAL:
                m_auiEncounter[uiType] = uiData;
                if (uiData == SPECIAL)
                {
                    --m_uiWorldStateSealCount;
                    DoUpdateWorldState(WORLD_STATE_SEAL, m_uiWorldStateSealCount);

                    // Yell at 75%, 50% and 25% shield
                    if (m_uiWorldStateSealCount < 100 - 25 * m_uiSealYellCount)
                    {
                        if (Creature* pSinclari = GetSingleCreatureFromStorage(NPC_SINCLARI_ALT))
                        {
                            // ToDo: I'm not sure if the last yell should be at 25% or at 5%. Needs research
                            ++m_uiSealYellCount;
                            DoScriptText(aSealWeakYell[m_uiSealYellCount - 1], pSinclari);
                        }
                    }

                    // set achiev to failed
                    if (m_bIsDefenseless)
                        m_bIsDefenseless = false;

                    if (!m_uiWorldStateSealCount)
                    {
                        SetData(TYPE_MAIN, FAIL);
                        SetData(TYPE_SEAL, NOT_STARTED);
                    }
                }
                break;
            case TYPE_PORTAL:
                switch (uiData)
                {
                case SPECIAL:                               // timer to next
                    m_uiPortalTimer = 90000;
                    break;
                case DONE:                                  // portal done, set timer to 5 secs
                    m_uiPortalTimer = 3000;
                    break;
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_LAVANTHOR:
            case TYPE_MORAGG:
            case TYPE_EREKEM:
            case TYPE_ICHORON:
            case TYPE_XEVOZZ:
            case TYPE_ZURAMAT:
                if (uiData == DONE)
                    m_uiPortalTimer = 35000;
                if (m_auiEncounter[uiType] != DONE)             // Keep the DONE-information stored
                    m_auiEncounter[uiType] = uiData;
                // Handle achievements if necessary
                if (uiData == IN_PROGRESS)
                {
                    if (uiType == TYPE_ZURAMAT)
                        m_bIsVoidDance = true;
                    else if (uiType == TYPE_ICHORON)
                        m_bIsDehydratation = true;
                }
                if (uiData == SPECIAL && uiType == TYPE_ICHORON)
                    m_bIsDehydratation = false;
                if (uiData == FAIL)
                    SetData(TYPE_MAIN, FAIL);
                break;
            case TYPE_CYANIGOSA:
                if (uiData == DONE)
                    SetData(TYPE_MAIN, DONE);
                if (uiData == FAIL)
                    SetData(TYPE_MAIN, FAIL);
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_DO_SINCLARI_BEGIN:
                SetIntroPortals(true);
                CallGuards(false);
                return;
            case TYPE_DO_RELEASE_BOSS:
                DoReleaseBoss();
                return;
            default:
                return;
            }

            if (uiData == DONE)
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " " << m_auiEncounter[2] << " "
                    << m_auiEncounter[3] << " " << m_auiEncounter[4] << " " << m_auiEncounter[5] << " "
                    << m_auiEncounter[6] << " " << m_auiEncounter[7] << " " << m_auiEncounter[8] << " "
                    << m_auiEncounter[9];

                m_strInstData = saveStream.str();

                SaveToDB();
                OUT_SAVE_INST_DATA_COMPLETE;
            }
        }

        uint32 GetData(uint32 uiType) const override
        {
            if (uiType < MAX_ENCOUNTER)
                return m_auiEncounter[uiType];

            switch (uiType)
            {
            case TYPE_DATA_PORTAL_NUMBER:
                return m_uiWorldStatePortalCount;
            case TYPE_DATA_PORTAL_ELITE:
                return (urand(0, 1) ? NPC_PORTAL_GUARDIAN : NPC_PORTAL_KEEPER);
            case TYPE_DATA_IS_TRASH_PORTAL:
                return uint32(IsCurrentPortalForTrash());
            case TYPE_DATA_GET_MOB_NORMAL:
                return aRandomPortalNpcs[urand(0, 4)];
            default:
                break;
            }
            return 0;
        }

        void SetData64(uint32 uiType, uint64 uiData) override
        {
            switch (uiType)
            {
            case DATA64_CRYSTAL_ACTIVATOR:
                if (Player* pl = instance->GetPlayer(ObjectGuid(uiData)))
                    ProcessActivationCrystal(pl);
                break;
            case DATA64_CRYSTAL_ACTIVATOR_INT:
                if (Unit* pl = instance->GetUnit(ObjectGuid(uiData)))
                    ProcessActivationCrystal(pl, true);
                break;
            case DATA64_SABOTEUR:
                if (const BossInformation* pData = GetBossInformation())
                {
                    Creature* pSummoned = instance->GetCreature(ObjectGuid(uiData));
                    pSummoned->SetWalk(false);
                    pSummoned->GetMotionMaster()->MovePoint(pData->uiWayPointId, pData->fX, pData->fY, pData->fZ);
                }
                break;
            default:
                break;
            }
        }

        bool CheckAchievementCriteriaMeet(uint32 uiCriteriaId, Player const* pSource, Unit const* pTarget, uint32 uiMiscValue1 /* = 0*/) const override
        {
            switch (uiCriteriaId)
            {
                // ToDo: uncomment these when they are implemented
                // case ACHIEV_CRIT_DEFENSELES:
                //    return m_bIsDefenseless;
                // case ACHIEV_CRIT_DEHYDRATATION:
                //    return m_bIsDehydratation;
            case ACHIEV_CRIT_VOID_DANCE:
                return m_bIsVoidDance;

            default:
                return false;
            }
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
            loadStream >> m_auiEncounter[0] >> m_auiEncounter[1] >> m_auiEncounter[2] >> m_auiEncounter[3]
                >> m_auiEncounter[4] >> m_auiEncounter[5] >> m_auiEncounter[6] >> m_auiEncounter[7]
                >> m_auiEncounter[8] >> m_auiEncounter[9];

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                    m_auiEncounter[i] = NOT_STARTED;
            }

            OUT_LOAD_INST_DATA_COMPLETE;
        }

        void Update(uint32 uiDiff) override
        {
            if (m_uiEventResetTimer)
            {
                if (m_uiEventResetTimer <= uiDiff)
                {
                    if (Creature* pSinclari = GetSingleCreatureFromStorage(NPC_SINCLARI))
                        pSinclari->Respawn();

                    m_uiEventResetTimer = 0;
                }
                else
                    m_uiEventResetTimer -= uiDiff;
            }

            if (m_auiEncounter[TYPE_MAIN] != IN_PROGRESS)
                return;

            if (m_uiPortalTimer)
            {
                if (m_uiPortalTimer <= uiDiff)
                {
                    DoUpdateWorldState(WORLD_STATE_PORTALS, ++m_uiWorldStatePortalCount);

                    SetPortalId();
                    SpawnPortal();

                    m_uiPortalTimer = 0;
                }
                else
                    m_uiPortalTimer -= uiDiff;
            }
        }

        typedef std::multimap<uint32, ObjectGuid> BossToCellMap;

    private:
        void GetErekemGuardList(GuidList& lGuardList) { lGuardList = GetData(TYPE_EREKEM) != DONE ? m_lErekemGuardList : m_lArakkoaGuardList; }

        bool IsCurrentPortalForTrash() const
        {
            if (m_uiWorldStatePortalCount % MAX_MINIBOSSES)
                return true;

            return false;
        }

        BossInformation const* GetBossInformation(uint32 uiEntry = 0)
        {
            uint32 mEntry = uiEntry;
            if (!mEntry)
            {
                if (m_uiWorldStatePortalCount == 6 && m_vRandomBosses.size() >= 1)
                    mEntry = m_vRandomBosses[0]->uiEntry;
                else if (m_uiWorldStatePortalCount == 12 && m_vRandomBosses.size() >= 2)
                    mEntry = m_vRandomBosses[1]->uiEntry;
            }

            if (!mEntry)
                return NULL;

            for (uint8 i = 0; i < MAX_MINIBOSSES; ++i)
            {
                if (aBossInformation[i].uiEntry == mEntry)
                    return &aBossInformation[i];
            }

            return NULL;
        }

        void SetIntroPortals(bool bDeactivate)
        {
            for (GuidList::const_iterator itr = m_lIntroPortalList.begin(); itr != m_lIntroPortalList.end(); ++itr)
            {
                if (Creature* pPortal = instance->GetCreature(*itr))
                {
                    if (bDeactivate)
                        pPortal->ForcedDespawn();
                    else
                        pPortal->Respawn();
                }
            }
        }

        void CallGuards(bool bRespawn)
        {
            for (GuidList::const_iterator itr = m_lGuardsList.begin(); itr != m_lGuardsList.end(); ++itr)
            {
                if (Creature* pGuard = instance->GetCreature(*itr))
                {
                    if (bRespawn)
                        pGuard->Respawn();
                    else if (pGuard->IsAlive())
                    {
                        pGuard->SetWalk(false);
                        pGuard->GetMotionMaster()->MovePoint(0, fGuardExitLoc[0], fGuardExitLoc[1], fGuardExitLoc[2]);
                        pGuard->ForcedDespawn(6000);
                    }
                }
            }
        }

        // Release a boss from a prison cell
        void DoReleaseBoss()
        {
            if (const BossInformation* pData = GetBossInformation())
            {
                if (Creature* pBoss = GetSingleCreatureFromStorage(GetData(pData->uiType) != DONE ? pData->uiEntry : pData->uiGhostEntry))
                {
                    UpdateCellForBoss(pData->uiEntry);
                    if (pData->iSayEntry)
                        DoScriptText(pData->iSayEntry, pBoss);

                    pBoss->GetMotionMaster()->MovePoint(1, pData->fX, pData->fY, pData->fZ);
                    pBoss->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE);

                    // Handle Erekem guards
                    if (pData->uiType == TYPE_EREKEM)
                    {
                        GuidList lAddGuids;
                        GetErekemGuardList(lAddGuids);

                        float fMoveX;
                        for (GuidList::const_iterator itr = lAddGuids.begin(); itr != lAddGuids.end(); ++itr)
                        {
                            if (Creature* pAdd = instance->GetCreature(*itr))
                            {
                                fMoveX = (pData->fX - pAdd->GetPositionX()) * .25;
                                pAdd->GetMotionMaster()->MovePoint(0, pData->fX - fMoveX, pData->fY, pData->fZ);
                                pAdd->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE);
                            }
                        }
                    }
                }
            }
        }

        PortalData const* GetPortalData() { return &afPortalLocation[m_uiPortalId]; }

        void UpdateWorldState(bool bEnable = true)
        {
            m_uiWorldState = bEnable ? 1 : 0;

            DoUpdateWorldState(WORLD_STATE_ID, m_uiWorldState);
            DoUpdateWorldState(WORLD_STATE_SEAL, m_uiWorldStateSealCount);
            DoUpdateWorldState(WORLD_STATE_PORTALS, m_uiWorldStatePortalCount);
        }

        void SetRandomBosses()
        {
            // Store bosses that are already done
            for (uint8 i = 0; i < MAX_MINIBOSSES; ++i)
            {
                if (m_auiEncounter[aBossInformation[i].uiType] == DONE)
                    m_vRandomBosses.push_back(CreateBossSpawnByEntry(aBossInformation[i].uiEntry));
            }

            if (m_vRandomBosses.size() < 2)                         // Get some new random bosses
            {
                std::random_shuffle(m_vRandomBossList.begin(), m_vRandomBossList.end());
                // two required, in case the first is already pushed to m_vRandomBosses
                if (m_vRandomBossList.size() < 2)
                    script_error_log("instance_violet_hold, Mini Bosses are not properly spawned");
                else
                    m_vRandomBossList.resize(2);

                // Fill up some random bosses
                for (std::vector<uint32>::const_iterator itr = m_vRandomBossList.begin(); itr != m_vRandomBossList.end(); ++itr)
                {
                    if (m_vRandomBosses.empty() || m_vRandomBosses[0]->uiEntry != *itr)
                        m_vRandomBosses.push_back(CreateBossSpawnByEntry(*itr));
                }
            }

            for (uint8 i = 0; i < m_vRandomBosses.size(); ++i)
                debug_log("SD2: instance_violet_hold random boss %u is entry %u", i, m_vRandomBosses[i]->uiEntry);
        }

        void SpawnPortal()
        {
            if (const PortalData* pData = GetPortalData())
            {
                if (Creature* pController = GetSingleCreatureFromStorage(NPC_SINCLARI_ALT))
                {
                    uint32 uiPortalEntry = pData->pPortalType == PORTAL_TYPE_NORM ? NPC_PORTAL : NPC_PORTAL_ELITE;

                    pController->SummonCreature(uiPortalEntry, pData->fX, pData->fY, pData->fZ, pData->fOrient, TEMPSUMMON_TIMED_OOC_OR_CORPSE_DESPAWN, 1800 * IN_MILLISECONDS);
                }
            }
        }

        void SetPortalId()
        {
            if (IsCurrentPortalForTrash())
            {
                // Find another Trash portal position
                uint8 uiTemp = m_uiPortalId + urand(1, m_uiMaxCountPortalLoc - 1);
                // Decrease m_uiMaxCountPortalLoc so that the center position is skipped
                uiTemp %= m_uiMaxCountPortalLoc - 1;

                debug_log("SD2: instance_violet_hold: SetPortalId %u, old was id %u.", uiTemp, m_uiPortalId);

                m_uiPortalId = uiTemp;
            }
            else if (m_uiWorldStatePortalCount == 18)
            {
                debug_log("SD2: instance_violet_hold: SetPortalId %u (Cyanigosa), old was id %u.", 0, m_uiPortalId);
                m_uiPortalId = 0;
            }
            else
            {
                debug_log("SD2: instance_violet_hold: SetPortalId %u (is boss), old was id %u.", m_uiMaxCountPortalLoc, m_uiPortalId);
                m_uiPortalId = m_uiMaxCountPortalLoc;
            }
        }

        void ResetAll()
        {
            ResetVariables();
            UpdateWorldState(false);
            CallGuards(true);
            SetIntroPortals(false);
            // ToDo: reset the activation crystals when implemented

            for (std::vector<BossSpawn*>::const_iterator itr = m_vRandomBosses.begin(); itr != m_vRandomBosses.end(); ++itr)
            {
                const BossInformation* pData = GetBossInformation((*itr)->uiEntry);
                if (pData && m_auiEncounter[pData->uiType] == DONE)
                {
                    // Despawn ghost boss
                    if (Creature* pGhostBoss = GetSingleCreatureFromStorage(pData->uiGhostEntry))
                        pGhostBoss->ForcedDespawn();

                    // Spawn new boss replacement
                    if (Creature* pSummoner = GetSingleCreatureFromStorage(NPC_SINCLARI_ALT))
                        pSummoner->SummonCreature(pData->uiGhostEntry, (*itr)->fX, (*itr)->fY, (*itr)->fZ, (*itr)->fO, TEMPSUMMON_DEAD_DESPAWN, 0);

                    // Replace Erekem guards
                    if (pData->uiType == TYPE_EREKEM)
                    {
                        // Despawn ghost guards
                        for (GuidList::const_iterator itr = m_lArakkoaGuardList.begin(); itr != m_lArakkoaGuardList.end(); ++itr)
                        {
                            if (Creature* pGhostGuard = instance->GetCreature(*itr))
                                pGhostGuard->ForcedDespawn();
                        }

                        m_lArakkoaGuardList.clear();

                        // Spawn new guards replacement
                        float fX, fY, fZ, fO;
                        for (GuidList::const_iterator itr = m_lErekemGuardList.begin(); itr != m_lErekemGuardList.end(); ++itr)
                        {
                            if (Creature* pGuard = instance->GetCreature(*itr))
                            {
                                // Don't allow alive original guards while the boss is dead
                                if (!pGuard->IsDead())
                                    pGuard->ForcedDespawn();

                                // Spawn a ghost guard for each original guard
                                pGuard->GetRespawnCoord(fX, fY, fZ, &fO);
                                pGuard->SummonCreature(NPC_ARAKKOA_GUARD, fX, fY, fZ, fO, TEMPSUMMON_DEAD_DESPAWN, 0);
                            }
                        }
                    }
                }

                // Close Door if still open
                if (pData && (m_auiEncounter[pData->uiType] == DONE || m_auiEncounter[pData->uiType] == FAIL))
                    UpdateCellForBoss(pData->uiEntry, true);
            }
        }

        void ResetVariables()
        {
            m_uiWorldStateSealCount = 100;
            m_uiWorldStatePortalCount = 0;
            m_uiSealYellCount = 0;
        }

        bool IsNextPortalForTrash()
        {
            if ((m_uiWorldStatePortalCount + 1) % MAX_MINIBOSSES)
                return true;

            return false;
        }

        BossSpawn* CreateBossSpawnByEntry(uint32 uiEntry)
        {
            BossSpawn* pBossSpawn = new BossSpawn;
            pBossSpawn->uiEntry = uiEntry;

            if (Creature* pBoss = GetSingleCreatureFromStorage(uiEntry))
                pBoss->GetRespawnCoord(pBossSpawn->fX, pBossSpawn->fY, pBossSpawn->fZ, &(pBossSpawn->fO));

            return pBossSpawn;
        }

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        uint32 m_uiWorldState;
        uint32 m_uiWorldStateSealCount;
        uint32 m_uiWorldStatePortalCount;

        uint8 m_uiPortalId;
        uint32 m_uiPortalTimer;
        uint32 m_uiMaxCountPortalLoc;

        uint32 m_uiSealYellCount;
        uint32 m_uiEventResetTimer;

        bool m_bIsVoidDance;
        bool m_bIsDefenseless;
        bool m_bIsDehydratation;

        BossToCellMap m_mBossToCellMap;

        GuidList m_lIntroPortalList;
        GuidList m_lGuardsList;
        GuidList m_lErekemGuardList;
        GuidList m_lArakkoaGuardList;
        GuidList m_lIchoronTargetsList;
        std::vector<uint32> m_vRandomBossList;

        std::vector<BossSpawn*> m_vRandomBosses;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_violet_hold(pMap);
    }
};

void AddSC_instance_violet_hold()
{
    Script* s;

    s = new is_violet_hold();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_violet_hold";
    //pNewScript->GetInstanceData = GetInstanceData_instance_violet_hold;
    //pNewScript->RegisterSelf();
}
