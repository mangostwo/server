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
SDName: trial_of_the_crusader
SD%Complete: 0
SDComment:
SDCategory: Crusader Coliseum
EndScriptData */

#include "precompiled.h"
#include "trial_of_the_crusader.h"

enum
{
    SAY_AGGRO                           = -1649056,
    SAY_BERSERK                         = -1649057,
    SAY_COLORSWITCH                     = -1649058,
    SAY_DEATH                           = -1649059,
    SAY_SLAY_1                          = -1649060,
    SAY_SLAY_2                          = -1649061,
    SAY_TO_BLACK                        = -1649062,
    SAY_TO_WHITE                        = -1649063,
};

/*######
## boss_fjola
######*/

struct boss_fjola : CreatureScript
{
    boss_fjola() : CreatureScript("boss_fjola") {}

    struct boss_fjolaAI : public ScriptedAI
    {
        boss_fjolaAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

        ScriptedInstance* m_pInstance;

        void Reset() override {}

        void Aggro(Unit* /*pWho*/) override
        {
            m_creature->SetInCombatWithZone();
        }

        void UpdateAI(const uint32 /*uiDiff*/) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_fjolaAI(pCreature);
    }
};

/*######
## boss_eydis
######*/

struct boss_eydis : public CreatureScript
{
    boss_eydis() : CreatureScript("boss_eydis") {}

    struct boss_eydisAI : public ScriptedAI
    {
        boss_eydisAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

        void Reset() override {}

        void Aggro(Unit* /*pWho*/) override
        {
            m_creature->SetInCombatWithZone();
        }

        void UpdateAI(const uint32 /*uiDiff*/) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_eydisAI(pCreature);
    }
};

void AddSC_twin_valkyr()
{
    Script* s;

    s = new boss_fjola();
    s->RegisterSelf();
    s = new boss_eydis();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "boss_fjola";
    //pNewScript->GetAI = &GetAI_boss_fjola;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "boss_eydis";
    //pNewScript->GetAI = &GetAI_boss_fjola;
    //pNewScript->RegisterSelf();
}
