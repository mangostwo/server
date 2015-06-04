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
SDName: Dalaran
SD%Complete: 100
SDComment:
SDCategory: Dalaran
EndScriptData */

/* ContentData
npc_dalaran_guardian_mage
EndContentData */

#include "precompiled.h"

enum
{
    SPELL_TRESPASSER_H          = 54029,
    SPELL_TRESPASSER_A          = 54028,

    // Exception auras - used for quests 20439 and 24451
    SPELL_COVENANT_DISGUISE_1   = 70971,
    SPELL_COVENANT_DISGUISE_2   = 70972,
    SPELL_SUNREAVER_DISGUISE_1  = 70973,
    SPELL_SUNREAVER_DISGUISE_2  = 70974,

    AREA_ID_SUNREAVER           = 4616,
    AREA_ID_SILVER_ENCLAVE      = 4740
};

struct npc_dalaran_guardian_mage : public CreatureScript
{
    npc_dalaran_guardian_mage() : CreatureScript("npc_dalaran_guardian_mage") {}

    struct npc_dalaran_guardian_mageAI : public ScriptedAI
    {
        npc_dalaran_guardian_mageAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        void MoveInLineOfSight(Unit* pWho) override
        {
            if (m_creature->GetDistanceZ(pWho) > CREATURE_Z_ATTACK_RANGE)
                return;

            if (pWho->IsTargetableForAttack() && m_creature->IsHostileTo(pWho))
            {
                // exception for quests 20439 and 24451
                if (pWho->HasAura(SPELL_COVENANT_DISGUISE_1) || pWho->HasAura(SPELL_COVENANT_DISGUISE_2) ||
                    pWho->HasAura(SPELL_SUNREAVER_DISGUISE_1) || pWho->HasAura(SPELL_SUNREAVER_DISGUISE_2))
                    return;

                if (m_creature->IsWithinDistInMap(pWho, m_creature->GetAttackDistance(pWho)) && m_creature->IsWithinLOSInMap(pWho))
                {
                    if (Player* pPlayer = pWho->GetCharmerOrOwnerPlayerOrPlayerItself())
                    {
                        // it's mentioned that pet may also be teleported, if so, we need to tune script to apply to those in addition.

                        if (pPlayer->GetAreaId() == AREA_ID_SILVER_ENCLAVE)
                            DoCastSpellIfCan(pPlayer, SPELL_TRESPASSER_A);
                        else if (pPlayer->GetAreaId() == AREA_ID_SUNREAVER)
                            DoCastSpellIfCan(pPlayer, SPELL_TRESPASSER_H);
                    }
                }
            }
        }

        void AttackedBy(Unit* /*pAttacker*/) override {}

        void Reset() override {}

        void UpdateAI(const uint32 /*uiDiff*/) override {}
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_dalaran_guardian_mageAI(pCreature);
    }
};

void AddSC_dalaran()
{
    Script* s;

    s = new npc_dalaran_guardian_mage();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_dalaran_guardian_mage";
    //pNewScript->GetAI = &GetAI_npc_dalaran_guardian_mage;
    //pNewScript->RegisterSelf();
}
