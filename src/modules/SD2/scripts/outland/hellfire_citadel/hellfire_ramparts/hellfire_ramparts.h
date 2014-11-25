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

#ifndef DEF_RAMPARTS_H
#define DEF_RAMPARTS_H

enum
{
    MAX_ENCOUNTER               = 2,

    TYPE_VAZRUDEN               = 1,
    TYPE_NAZAN                  = 2,                        // Do not change, used in ACID (SetData(SPECIAL) on death of 17517

    NPC_HELLFIRE_SENTRY         = 17517,
    NPC_VAZRUDEN_HERALD         = 17307,
    NPC_VAZRUDEN                = 17537,

    GO_FEL_IRON_CHEST           = 185168,
    GO_FEL_IRON_CHEST_H         = 185169,
};

class instance_ramparts : public ScriptedInstance
{
    public:
        instance_ramparts(Map* pMap);

        void Initialize() override;

        void OnCreatureCreate(Creature* pCreature) override;
        void OnObjectCreate(GameObject* pGo) override;

        void SetData(uint32 uiType, uint32 uiData) override;
        uint32 GetData(uint32 uiType) const override;

        // No need to save and load this instance (only one encounter needs special handling, no doors used)

    private:
        void DoFailVazruden();

        uint32 m_auiEncounter[MAX_ENCOUNTER];

        uint32 m_uiSentryCounter;
        GuidList m_lSentryGUIDs;
};

#endif
