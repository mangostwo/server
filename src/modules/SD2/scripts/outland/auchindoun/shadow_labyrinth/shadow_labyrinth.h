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

#ifndef DEF_SHADOW_LABYRINTH_H
#define DEF_SHADOW_LABYRINTH_H

enum
{
    MAX_ENCOUNTER           = 4,

    TYPE_HELLMAW            = 1,
    // TYPE_OVERSEER        = 2,                            // obsolete id used by acid
    TYPE_INCITER            = 3,
    TYPE_VORPIL             = 4,
    TYPE_MURMUR             = 5,

    DATA_CABAL_RITUALIST    = 1,                            // DO NOT CHANGE! Used by Acid. - used to check the Cabal Ritualists alive

    NPC_HELLMAW             = 18731,
    NPC_VORPIL              = 18732,
    NPC_CABAL_RITUALIST     = 18794,

    GO_REFECTORY_DOOR       = 183296,                       // door opened when blackheart the inciter dies
    GO_SCREAMING_HALL_DOOR  = 183295,                       // door opened when grandmaster vorpil dies

    SAY_HELLMAW_INTRO       = -1555000,

    SPELL_BANISH            = 30231,                        // spell is handled in creature_template_addon;
};

class instance_shadow_labyrinth : public ScriptedInstance
{
    public:
        instance_shadow_labyrinth(Map* pMap);

        void Initialize() override;

        void OnObjectCreate(GameObject* pGo) override;
        void OnCreatureCreate(Creature* pCreature) override;

        void OnCreatureDeath(Creature* pCreature) override;

        void SetData(uint32 uiType, uint32 uiData) override;
        uint32 GetData(uint32 uiType) const override;

        void SetData64(uint32 uiType, uint64 uiGuid) override;

        const char* Save() const override { return m_strInstData.c_str(); }
        void Load(const char* chrIn) override;

        bool IsHellmawUnbanished() { return m_sRitualistsAliveGUIDSet.empty(); }

    private:
        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        GuidSet m_sRitualistsAliveGUIDSet;
};

#endif
