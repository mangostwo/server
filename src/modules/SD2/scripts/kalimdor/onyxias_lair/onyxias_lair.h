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

#ifndef DEF_ONYXIA_H
#define DEF_ONYXIA_H

enum
{
    TYPE_ONYXIA                 = 0,

    // Special data fields for Onyxia
    DATA_LIFTOFF                = 4,
    DATA_PLAYER_TOASTED         = 5,

    NPC_ONYXIA_WHELP            = 11262,
    NPC_ONYXIA_TRIGGER          = 12758,

    // Achievement Related
    TIME_LIMIT_MANY_WHELPS      = 10,                       // 10s timeframe to kill 50 whelps after liftoff
    ACHIEV_CRIT_REQ_MANY_WHELPS = 50,

    ACHIEV_CRIT_MANY_WHELPS_N   = 12565,                    // Achievements 4403, 4406
    ACHIEV_CRIT_MANY_WHELPS_H   = 12568,
    ACHIEV_CRIT_NO_BREATH_N     = 12566,                    // Acheivements 4404, 4407
    ACHIEV_CRIT_NO_BREATH_H     = 12569,

    ACHIEV_START_ONYXIA_ID      = 6601,
};

class instance_onyxias_lair : public ScriptedInstance
{
    public:
        instance_onyxias_lair(Map* pMap);
        ~instance_onyxias_lair() {}

        void Initialize() override;

        bool IsEncounterInProgress() const override;

        void OnCreatureCreate(Creature* pCreature) override;

        void SetData(uint32 uiType, uint32 uiData) override;

        bool CheckAchievementCriteriaMeet(uint32 uiCriteriaId, Player const* pSource, Unit const* pTarget, uint32 uiMiscValue1 /* = 0*/) const override;

    protected:
        uint32 m_uiEncounter;
        uint32 m_uiAchievWhelpsCount;

        time_t m_tPhaseTwoStart;
};

#endif
