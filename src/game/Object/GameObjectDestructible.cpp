/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2025 MaNGOS <https://www.getmangos.eu>
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
 * @file GameObjectDestructible.cpp
 * @brief Cohesion split of GameObject.cpp -- capture-point slider/tick and
 *        destructible-building damage/rebuild/force-health. Same
 *        `GameObject` class; no behaviour change.
 */

#include <list>
#include "Utilities/Errors.h"
#include "GameObject.h"
#include "GameObjectModel.h"
#include "Geometry/Quat.h"
#include "QuestDef.h"
#include "ObjectMgr.h"
#include "PoolManager.h"
#include "SpellMgr.h"
#include "Spell.h"
#include "UpdateMask.h"
#include "Opcodes.h"
#include "WorldPacket.h"
#include "World.h"
#include "Database/DatabaseEnv.h"
#include "LootMgr.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "InstanceData.h"
#include "MapManager.h"
#include "MapPersistentStateMgr.h"
#include "BattleGround/BattleGround.h"
#include "BattleGround/BattleGroundAV.h"
#include "OutdoorPvP/OutdoorPvP.h"
#include "Util.h"
#include "ScriptMgr.h"
#include "CreatureAISelector.h"
#include "SQLStorages.h"
#include "GameObjectAI.h"
#include <memory>
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /* ENABLE_ELUNA */

/**
 * @brief Sets the capture point slider and derived state.
 *
 * @param value The slider value.
 * @param isLocked true if the capture point is locked.
 */
void GameObject::SetCapturePointSlider(float value, bool isLocked)
{
    GameObjectInfo const* info = GetGOInfo();

    m_captureSlider = value;

    // only activate non-locked capture point
    if (!isLocked)
    {
        SetLootState(GO_ACTIVATED);
    }

    // set the state of the capture point based on the slider value
    if ((int)m_captureSlider == CAPTURE_SLIDER_ALLIANCE)
    {
        m_captureState = CAPTURE_STATE_WIN_ALLIANCE;
    }
    else if ((int)m_captureSlider == CAPTURE_SLIDER_HORDE)
    {
        m_captureState = CAPTURE_STATE_WIN_HORDE;
    }
    else if (m_captureSlider > CAPTURE_SLIDER_MIDDLE + info->capturePoint.neutralPercent * 0.5f)
    {
        m_captureState = CAPTURE_STATE_PROGRESS_ALLIANCE;
    }
    else if (m_captureSlider < CAPTURE_SLIDER_MIDDLE - info->capturePoint.neutralPercent * 0.5f)
    {
        m_captureState = CAPTURE_STATE_PROGRESS_HORDE;
    }
    else
    {
        m_captureState = CAPTURE_STATE_NEUTRAL;
    }
}

/**
 * @brief Updates capture point progress and sends related world state changes.
 */
void GameObject::TickCapturePoint()
{
    // TODO: On retail: Ticks every 5.2 seconds. slider value increase when new player enters on tick

    GameObjectInfo const* info = GetGOInfo();
    float radius = info->capturePoint.radius;

    // search for players in radius
    std::list<Player*> capturingPlayers;
    MaNGOS::AnyPlayerInCapturePointRange u_check(this, radius);
    MaNGOS::PlayerListSearcher<MaNGOS::AnyPlayerInCapturePointRange> checker(capturingPlayers, u_check);
    Cell::VisitWorldObjects(this, checker, radius);

    GuidSet tempUsers(m_UniqueUsers);
    uint32 neutralPercent = info->capturePoint.neutralPercent;
    int oldValue = m_captureSlider;
    int rangePlayers = 0;

    for (std::list<Player*>::iterator itr = capturingPlayers.begin(); itr != capturingPlayers.end(); ++itr)
    {
        if ((*itr)->GetTeam() == ALLIANCE)
        {
            ++rangePlayers;
        }
        else
        {
            --rangePlayers;
        }

        ObjectGuid guid = (*itr)->GetObjectGuid();
        if (!tempUsers.erase(guid))
        {
            // new player entered capture point zone
            m_UniqueUsers.insert(guid);

            // send capture point enter packets
            (*itr)->SendUpdateWorldState(info->capturePoint.worldState3, neutralPercent);
            (*itr)->SendUpdateWorldState(info->capturePoint.worldState2, oldValue);
            (*itr)->SendUpdateWorldState(info->capturePoint.worldState1, WORLD_STATE_ADD);
            (*itr)->SendUpdateWorldState(info->capturePoint.worldState2, oldValue); // also redundantly sent on retail to prevent displaying the initial capture direction on client capture slider incorrectly
        }
    }

    for (GuidSet::iterator itr = tempUsers.begin(); itr != tempUsers.end(); ++itr)
    {
        // send capture point leave packet
        if (Player* owner = GetMap()->GetPlayer(*itr))
        {
            owner->SendUpdateWorldState(info->capturePoint.worldState1, WORLD_STATE_REMOVE);
        }

        // player left capture point zone
        m_UniqueUsers.erase(*itr);
    }

    // return if there are not enough players capturing the point (works because minSuperiority is always 1)
    if (rangePlayers == 0)
    {
        // set to inactive if all players left capture point zone
        if (m_UniqueUsers.empty())
        {
            SetActiveObjectState(false);
        }
        return;
    }

    // prevents unloading gameobject before all players left capture point zone (to prevent m_UniqueUsers not being cleared if grid is set to idle)
    SetActiveObjectState(true);

    // cap speed
    int maxSuperiority = info->capturePoint.maxSuperiority;
    if (rangePlayers > maxSuperiority)
    {
        rangePlayers = maxSuperiority;
    }
    else if (rangePlayers < -maxSuperiority)
    {
        rangePlayers = -maxSuperiority;
    }

    // time to capture from 0% to 100% is maxTime for minSuperiority amount of players and minTime for maxSuperiority amount of players (linear function: y = dy/dx*x+d)
    float deltaSlider = info->capturePoint.minTime;

    if (int deltaSuperiority = maxSuperiority - info->capturePoint.minSuperiority)
    {
        deltaSlider += (float)(maxSuperiority - abs(rangePlayers)) / deltaSuperiority * (info->capturePoint.maxTime - info->capturePoint.minTime);
    }

    // calculate changed slider value for a duration of 5 seconds (5 * 100%)
    deltaSlider = 500.0f / deltaSlider;

    Team progressFaction;
    if (rangePlayers > 0)
    {
        progressFaction = ALLIANCE;
        m_captureSlider += deltaSlider;
        if (m_captureSlider > CAPTURE_SLIDER_ALLIANCE)
        {
            m_captureSlider = CAPTURE_SLIDER_ALLIANCE;
        }
    }
    else
    {
        progressFaction = HORDE;
        m_captureSlider -= deltaSlider;
        if (m_captureSlider < CAPTURE_SLIDER_HORDE)
        {
            m_captureSlider = CAPTURE_SLIDER_HORDE;
        }
    }

    // return if slider did not move a whole percent
    if ((int)m_captureSlider == oldValue)
    {
        return;
    }

    // on retail this is also sent to newly added players even though they already received a slider value
    for (std::list<Player*>::iterator itr = capturingPlayers.begin(); itr != capturingPlayers.end(); ++itr)
    {
        (*itr)->SendUpdateWorldState(info->capturePoint.worldState2, (uint32)m_captureSlider);
    }

    // send capture point events
    uint32 eventId = 0;

    /* WIN EVENTS */
    // alliance wins tower with max points
    if (m_captureState != CAPTURE_STATE_WIN_ALLIANCE && (int)m_captureSlider == CAPTURE_SLIDER_ALLIANCE)
    {
        eventId = info->capturePoint.winEventID1;
        m_captureState = CAPTURE_STATE_WIN_ALLIANCE;
    }
    // horde wins tower with max points
    else if (m_captureState != CAPTURE_STATE_WIN_HORDE && (int)m_captureSlider == CAPTURE_SLIDER_HORDE)
    {
        eventId = info->capturePoint.winEventID2;
        m_captureState = CAPTURE_STATE_WIN_HORDE;
    }

    /* PROGRESS EVENTS */
    // alliance takes the tower from neutral, contested or horde (if there is no neutral area) to alliance
    else if (m_captureState != CAPTURE_STATE_PROGRESS_ALLIANCE && m_captureSlider > CAPTURE_SLIDER_MIDDLE + neutralPercent * 0.5f && progressFaction == ALLIANCE)
    {
        eventId = info->capturePoint.progressEventID1;

        // handle objective complete
        if (m_captureState == CAPTURE_STATE_NEUTRAL)
            if (OutdoorPvP* outdoorPvP = sOutdoorPvPMgr.GetScript((*capturingPlayers.begin())->GetCachedZoneId()))
            {
                outdoorPvP->HandleObjectiveComplete(eventId, capturingPlayers, progressFaction);
            }

        // set capture state to alliance
        m_captureState = CAPTURE_STATE_PROGRESS_ALLIANCE;
    }
    // horde takes the tower from neutral, contested or alliance (if there is no neutral area) to horde
    else if (m_captureState != CAPTURE_STATE_PROGRESS_HORDE && m_captureSlider < CAPTURE_SLIDER_MIDDLE - neutralPercent * 0.5f && progressFaction == HORDE)
    {
        eventId = info->capturePoint.progressEventID2;

        // handle objective complete
        if (m_captureState == CAPTURE_STATE_NEUTRAL)
            if (OutdoorPvP* outdoorPvP = sOutdoorPvPMgr.GetScript((*capturingPlayers.begin())->GetCachedZoneId()))
            {
                outdoorPvP->HandleObjectiveComplete(eventId, capturingPlayers, progressFaction);
            }

        // set capture state to horde
        m_captureState = CAPTURE_STATE_PROGRESS_HORDE;
    }

    /* NEUTRAL EVENTS */
    // alliance takes the tower from horde to neutral
    else if (m_captureState != CAPTURE_STATE_NEUTRAL && m_captureSlider >= CAPTURE_SLIDER_MIDDLE - neutralPercent * 0.5f && m_captureSlider <= CAPTURE_SLIDER_MIDDLE + neutralPercent * 0.5f && progressFaction == ALLIANCE)
    {
        eventId = info->capturePoint.neutralEventID1;
        m_captureState = CAPTURE_STATE_NEUTRAL;
    }
    // horde takes the tower from alliance to neutral
    else if (m_captureState != CAPTURE_STATE_NEUTRAL && m_captureSlider >= CAPTURE_SLIDER_MIDDLE - neutralPercent * 0.5f && m_captureSlider <= CAPTURE_SLIDER_MIDDLE + neutralPercent * 0.5f && progressFaction == HORDE)
    {
        eventId = info->capturePoint.neutralEventID2;
        m_captureState = CAPTURE_STATE_NEUTRAL;
    }

    /* CONTESTED EVENTS */
    // alliance attacks tower which is in control or progress by horde (except if alliance also gains control in that case)
    else if ((m_captureState == CAPTURE_STATE_WIN_HORDE || m_captureState == CAPTURE_STATE_PROGRESS_HORDE) && progressFaction == ALLIANCE)
    {
        eventId = info->capturePoint.contestedEventID1;
        m_captureState = CAPTURE_STATE_CONTEST_HORDE;
    }
    // horde attacks tower which is in control or progress by alliance (except if horde also gains control in that case)
    else if ((m_captureState == CAPTURE_STATE_WIN_ALLIANCE || m_captureState == CAPTURE_STATE_PROGRESS_ALLIANCE) && progressFaction == HORDE)
    {
        eventId = info->capturePoint.contestedEventID2;
        m_captureState = CAPTURE_STATE_CONTEST_ALLIANCE;
    }

    if (eventId)
    {
        StartEvents_Event(GetMap(), eventId, this, this, true, *capturingPlayers.begin());
    }
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
//                              Destructible GO handling
// ////////////////////////////////////////////////////////////////////////////////////////////////
void GameObject::DealGameObjectDamage(uint32 damage, uint32 spell, Unit* caster)
{
    MANGOS_ASSERT(GetGoType() == GAMEOBJECT_TYPE_DESTRUCTIBLE_BUILDING);
    MANGOS_ASSERT(spell && sSpellStore.LookupEntry(spell) && caster);

    if (!damage)
    {
        return;
    }

    ForceGameObjectHealth(-int32(damage), caster);

    WorldPacket data(SMSG_DESTRUCTIBLE_BUILDING_DAMAGE, 9 + 9 + 9 + 4 + 4);
    data << GetPackGUID();
    data << caster->GetPackGUID();
    data << caster->GetCharmerOrOwnerOrSelf()->GetPackGUID();
    data << uint32(damage);
    data << uint32(spell);
    SendMessageToSet(&data, false);
}

void GameObject::RebuildGameObject(Unit* caster)
{
    MANGOS_ASSERT(GetGoType() == GAMEOBJECT_TYPE_DESTRUCTIBLE_BUILDING);
    MANGOS_ASSERT(caster);

    ForceGameObjectHealth(0, caster);
}

void GameObject::ForceGameObjectHealth(int32 diff, Unit* caster)
{
    MANGOS_ASSERT(GetGoType() == GAMEOBJECT_TYPE_DESTRUCTIBLE_BUILDING);
    MANGOS_ASSERT(caster || diff >= 0);

    if (diff < 0)                                           // Taken damage
    {
        DEBUG_FILTER_LOG(LOG_FILTER_DAMAGE, "DestructibleGO: %s taken damage %u dealt by %s", GetGuidStr().c_str(), uint32(-diff), caster->GetGuidStr().c_str());
#ifdef ENABLE_ELUNA
        if (caster && caster->ToPlayer())
        {
            if (Eluna* e = caster->GetEluna())
            {
                e->OnDamaged(this, caster->ToPlayer());
            }
        }
#endif
        if (m_useTimes > uint32(-diff))
        {
            m_useTimes += diff;
        }
        else
        {
            m_useTimes = 0;
        }
    }
    else if (diff == 0 && GetMaxHealth())                   // Rebuild - TODO: Rebuilding over time with special display-id?
    {
        DEBUG_FILTER_LOG(LOG_FILTER_DAMAGE, "DestructibleGO: %s start rebuild by %s", GetGuidStr().c_str(), caster->GetGuidStr().c_str());

        m_useTimes = GetMaxHealth();
        // Start Event if exist
        if (caster && m_goInfo->destructibleBuilding.rebuildingEvent)
        {
            StartEvents_Event(GetMap(), m_goInfo->destructibleBuilding.rebuildingEvent, this, caster->GetCharmerOrOwnerOrSelf(), true, caster->GetCharmerOrOwnerOrSelf());
        }
    }
    else                                                    // Set to value
    {
        m_useTimes = uint32(diff);
    }

    uint32 newDisplayId = 0xFFFFFFFF;                       // Set to invalid -1 to track if we switched to a change state
    DestructibleModelDataEntry const* destructibleInfo = sDestructibleModelDataStore.LookupEntry(m_goInfo->destructibleBuilding.destructibleData);

    // Get Current State - Note about order: Important for GetMaxHealth() == 0
    if (m_useTimes == GetMaxHealth())                       // Full Health
    {
        DEBUG_FILTER_LOG(LOG_FILTER_DAMAGE, "DestructibleGO: %s set to full health %u", GetGuidStr().c_str(), m_useTimes);

        RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_UNK_9 | GO_FLAG_UNK_10 | GO_FLAG_UNK_11);
        newDisplayId = m_goInfo->displayId;

        // Start Event if exist
        if (caster && m_goInfo->destructibleBuilding.intactEvent)
        {
            StartEvents_Event(GetMap(), m_goInfo->destructibleBuilding.intactEvent, this, caster->GetCharmerOrOwnerOrSelf(), true, caster->GetCharmerOrOwnerOrSelf());
        }
    }
    else if (m_useTimes == 0)                               // Destroyed
    {
        if (!HasFlag(GAMEOBJECT_FLAGS, GO_FLAG_UNK_11))     // Was not destroyed before
        {
            DEBUG_FILTER_LOG(LOG_FILTER_DAMAGE, "DestructibleGO: %s got destroyed", GetGuidStr().c_str());
#ifdef ENABLE_ELUNA
            if (caster && caster->ToPlayer())
            {
                if (Eluna* e = caster->GetEluna())
                {
                    e->OnDestroyed(this, caster->ToPlayer());
                }
            }
#endif
            RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_UNK_9 | GO_FLAG_UNK_10);
            SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_UNK_11);

            // Get destroyed DisplayId
            if ((!m_goInfo->destructibleBuilding.destroyedDisplayId || m_goInfo->destructibleBuilding.destroyedDisplayId == 1) && destructibleInfo)
            {
                newDisplayId = destructibleInfo->destroyedDisplayId;
            }
            else
            {
                newDisplayId = m_goInfo->destructibleBuilding.destroyedDisplayId;
            }

            if (!newDisplayId)                              // No proper destroyed display ID exists, fetch damaged
            {
                if ((!m_goInfo->destructibleBuilding.damagedDisplayId || m_goInfo->destructibleBuilding.damagedDisplayId == 1) && destructibleInfo)
                {
                    newDisplayId = destructibleInfo->damagedDisplayId;
                }
                else
                {
                    newDisplayId = m_goInfo->destructibleBuilding.damagedDisplayId;
                }
            }

            // Start Event if exist
            if (caster && m_goInfo->destructibleBuilding.destroyedEvent)
            {
                StartEvents_Event(GetMap(), m_goInfo->destructibleBuilding.destroyedEvent, this, caster->GetCharmerOrOwnerOrSelf(), true, caster->GetCharmerOrOwnerOrSelf());
            }
        }
    }
    else if (m_useTimes <= m_goInfo->destructibleBuilding.damagedNumHits) // Damaged
    {
        if (!HasFlag(GAMEOBJECT_FLAGS, GO_FLAG_UNK_10))     // Was not damaged before
        {
            DEBUG_FILTER_LOG(LOG_FILTER_DAMAGE, "DestructibleGO: %s got damaged (health now %u)", GetGuidStr().c_str(), m_useTimes);

            SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_UNK_10);

            // Get damaged DisplayId
            if ((!m_goInfo->destructibleBuilding.damagedDisplayId || m_goInfo->destructibleBuilding.damagedDisplayId == 1) && destructibleInfo)
            {
                newDisplayId = destructibleInfo->damagedDisplayId;
            }
            else
            {
                newDisplayId = m_goInfo->destructibleBuilding.damagedDisplayId;
            }

            // Start Event if exist
            if (caster && m_goInfo->destructibleBuilding.damagedEvent)
            {
                StartEvents_Event(GetMap(), m_goInfo->destructibleBuilding.damagedEvent, this, caster->GetCharmerOrOwnerOrSelf(), true, caster->GetCharmerOrOwnerOrSelf());
            }
        }
    }

    // Set display Id
    if (newDisplayId != 0xFFFFFFFF && newDisplayId != GetDisplayId() && newDisplayId)
    {
        SetDisplayId(newDisplayId);
    }

    // Set health
    SetGoAnimProgress(GetMaxHealth() ? m_useTimes * 255 / GetMaxHealth() : 255);
}
