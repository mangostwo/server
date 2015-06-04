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
SDName: Instance_Obsidian_Sanctum
SD%Complete: 80%
SDComment:
SDCategory: Obsidian Sanctum
EndScriptData */

#include "precompiled.h"
#include "obsidian_sanctum.h"

/* Obsidian Sanctum encounters:
0 - Sartharion
*/

struct is_obsidian_sanctum : public InstanceScript
{
    is_obsidian_sanctum() : InstanceScript("instance_obsidian_sanctum") {}

    class instance_obsidian_sanctum : public ScriptedInstance
    {
    public:
        instance_obsidian_sanctum(Map* pMap) : ScriptedInstance(pMap),
            m_uiAliveDragons(0)
        {
            Initialize();
        }

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));

            for (uint8 i = 0; i < MAX_TWILIGHT_DRAGONS; ++i)
                m_bPortalActive[i] = false;
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
                // The three dragons below set to active state once created.
                // We must expect bigger raid to encounter main boss, and then three dragons must be active due to grid differences
            case NPC_TENEBRON:
            case NPC_SHADRON:
            case NPC_VESPERON:
                pCreature->SetActiveObjectState(true);
            case NPC_SARTHARION:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
                break;
            case NPC_FIRE_CYCLONE:
                m_lFireCycloneGuidList.push_back(pCreature->GetObjectGuid());
                break;
            }
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_SARTHARION_EVENT:
                m_auiEncounter[0] = uiData;
                if (uiData == IN_PROGRESS)
                    m_sVolcanoBlowFailPlayers.clear();
                break;
            case TYPE_ALIVE_DRAGONS:
                m_uiAliveDragons = uiData;
                break;
            case TYPE_VOLCANO_BLOW_FAILED:
                // Insert the players who fail the achiev and haven't been already inserted in the set
                if (m_sVolcanoBlowFailPlayers.find(uiData) == m_sVolcanoBlowFailPlayers.end())
                    m_sVolcanoBlowFailPlayers.insert(uiData);
                break;
            case TYPE_DATA_PORTAL_OFF:
            case TYPE_DATA_PORTAL_ON:
                SetPortalStatus(uint8(uiData), uiType == TYPE_DATA_PORTAL_ON);
                return;
            default:
                break;
            }

            // No need to save anything here
        }

        uint32 GetData(uint32 uiType) const override
        {
            if (uiType == TYPE_SARTHARION_EVENT)
                return m_auiEncounter[0];

            if (uiType == TYPE_DATA_PORTAL_STATUS)
                return uint32(IsActivePortal());
            return 0;
        }

        uint64 GetData64(uint32 uiType) const override
        {
            if (uiType == DATA64_FIRE_CYCLONE)
                return SelectRandomFireCycloneGuid().GetRawValue();

            return 0;
        }


        bool CheckAchievementCriteriaMeet(uint32 uiCriteriaId, Player const* pSource, Unit const* pTarget, uint32 uiMiscValue1 /* = 0*/) const override
        {
            switch (uiCriteriaId)
            {
            case ACHIEV_DRAGONS_ALIVE_1_N:
            case ACHIEV_DRAGONS_ALIVE_1_H:
                return m_uiAliveDragons >= 1;
            case ACHIEV_DRAGONS_ALIVE_2_N:
            case ACHIEV_DRAGONS_ALIVE_2_H:
                return m_uiAliveDragons >= 2;
            case ACHIEV_DRAGONS_ALIVE_3_N:
            case ACHIEV_DRAGONS_ALIVE_3_H:
                return m_uiAliveDragons >= 3;
            case ACHIEV_CRIT_VOLCANO_BLOW_N:
            case ACHIEV_CRIT_VOLCANO_BLOW_H:
                // Return true if not found in the set
                return m_sVolcanoBlowFailPlayers.find(pSource->GetGUIDLow()) == m_sVolcanoBlowFailPlayers.end();
            default:
                return false;
            }
        }

        bool CheckConditionCriteriaMeet(Player const* pPlayer, uint32 uiInstanceConditionId, WorldObject const* pConditionSource, uint32 conditionSourceType) const override
        {
            switch (uiInstanceConditionId)
            {
            case INSTANCE_CONDITION_ID_HARD_MODE:               // Exactly one dragon alive on event start
            case INSTANCE_CONDITION_ID_HARD_MODE_2:             // Exactly two dragons alive on event start
            case INSTANCE_CONDITION_ID_HARD_MODE_3:             // All three dragons alive on event start
                return m_uiAliveDragons == uiInstanceConditionId;
            }

            script_error_log("instance_obsidian_sanctum::CheckConditionCriteriaMeet called with unsupported Id %u. Called with param plr %s, src %s, condition source type %u",
                uiInstanceConditionId, pPlayer ? pPlayer->GetGuidStr().c_str() : "NULL", pConditionSource ? pConditionSource->GetGuidStr().c_str() : "NULL", conditionSourceType);
            return false;
        }

    private:
        void SetPortalStatus(uint8 uiType, bool bStatus) { m_bPortalActive[uiType] = bStatus; }
        bool IsActivePortal() const
        {
            for (uint8 i = 0; i < MAX_TWILIGHT_DRAGONS; ++i)
            {
                if (m_bPortalActive[i])
                    return true;
            }

            return false;
        }

        ObjectGuid SelectRandomFireCycloneGuid() const
        {
            if (m_lFireCycloneGuidList.empty())
                return ObjectGuid();

            GuidList::const_iterator iter = m_lFireCycloneGuidList.begin();
            advance(iter, urand(0, m_lFireCycloneGuidList.size() - 1));

            return *iter;
        }

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        bool m_bPortalActive[MAX_TWILIGHT_DRAGONS];

        uint8 m_uiAliveDragons;

        std::set<uint32> m_sVolcanoBlowFailPlayers;

        GuidList m_lFireCycloneGuidList;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_obsidian_sanctum(pMap);
    }
};

void AddSC_instance_obsidian_sanctum()
{
    Script* s;

    s = new is_obsidian_sanctum();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_obsidian_sanctum";
    //pNewScript->GetInstanceData = GetInstanceData_instance_obsidian_sanctum;
    //pNewScript->RegisterSelf();
}
