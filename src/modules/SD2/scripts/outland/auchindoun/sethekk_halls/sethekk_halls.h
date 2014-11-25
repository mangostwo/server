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

#ifndef DEF_SETHEKK_HALLS_H
#define DEF_SETHEKK_HALLS_H

enum
{
    MAX_ENCOUNTER               = 3,

    TYPE_SYTH                   = 0,
    TYPE_ANZU                   = 1,
    TYPE_IKISS                  = 2,

    NPC_ANZU                    = 23035,
    NPC_RAVEN_GOD_TARGET        = 23057,

    GO_IKISS_DOOR               = 177203,
    GO_IKISS_CHEST              = 187372,
    GO_RAVENS_CLAW              = 185554,

    SAY_ANZU_INTRO_1            = -1556016,
    SAY_ANZU_INTRO_2            = -1556017,

    // possible spells used for Anzu summoning event
    SPELL_PORTAL                = 39952,
    SPELL_SUMMONING_BEAMS       = 39978,
    SPELL_RED_LIGHTNING         = 39990,

    ACHIEV_CRITA_TURKEY_TIME    = 11142,
    ITEM_PILGRIMS_HAT           = 46723,
    ITEM_PILGRIMS_DRESS         = 44785,
    ITEM_PILGRIMS_ROBE          = 46824,
    ITEM_PILGRIMS_ATTIRE        = 46800,
};

class instance_sethekk_halls : public ScriptedInstance
{
    public:
        instance_sethekk_halls(Map* pMap);
        ~instance_sethekk_halls() {}

        void Initialize() override;

        void OnCreatureCreate(Creature* pCreature) override;
        void OnObjectCreate(GameObject* pGo) override;

        void SetData(uint32 uiType, uint32 uiData) override;
        uint32 GetData(uint32 uiType) const override;

        bool CheckAchievementCriteriaMeet(uint32 uiCriteriaId, Player const* pSource, Unit const* pTarget, uint32 uiMiscValue1 /* = 0*/) const override;

        const char* Save() const override { return m_strInstData.c_str(); }
        void Load(const char* chrIn) override;

    private:
        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;
};

#endif
