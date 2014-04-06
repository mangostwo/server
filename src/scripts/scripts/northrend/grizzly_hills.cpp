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
 */

/* ScriptData
SDName: Grizzly_Hills
SD%Complete:
SDComment: Quest support: 12138, 12198
SDCategory: Grizzly Hills
EndScriptData */

/* ContentData
npc_depleted_war_golem
EndContentData */

#include "precompiled.h"
#include "pet_ai.h"

/*######
## npc_depleted_war_golem
######*/

enum
{
    SAY_GOLEM_CHARGE            = -1000626,
    SAY_GOLEM_COMPLETE          = -1000627,

    NPC_LIGHTNING_SENTRY        = 26407,

    SPELL_CHARGE_GOLEM          = 47799,
    SPELL_GOLEM_CHARGE_CREDIT   = 47797,
};

struct MANGOS_DLL_DECL npc_depleted_war_golemAI : public ScriptedPetAI
{
    npc_depleted_war_golemAI(Creature* pCreature) : ScriptedPetAI(pCreature) { Reset(); }

    void Reset() override { }

    void OwnerKilledUnit(Unit* pVictim) override
    {
        if (pVictim->GetTypeId() == TYPEID_UNIT && pVictim->GetEntry() == NPC_LIGHTNING_SENTRY)
        {
            // Is distance expected?
            if (m_creature->IsWithinDistInMap(pVictim, 10.0f))
                m_creature->CastSpell(m_creature, SPELL_CHARGE_GOLEM, true);
        }
    }
};

CreatureAI* GetAI_npc_depleted_war_golem(Creature* pCreature)
{
    return new npc_depleted_war_golemAI(pCreature);
}

bool EffectAuraDummy_npc_depleted_war_golem(const Aura* pAura, bool bApply)
{
    if (pAura->GetId() != SPELL_CHARGE_GOLEM)
        return true;

    Creature* pCreature = (Creature*)pAura->GetTarget();

    if (!pCreature)
        return true;

    if (pAura->GetEffIndex() == EFFECT_INDEX_0)
    {
        if (bApply)
        {
            DoScriptText(SAY_GOLEM_CHARGE, pCreature);
            pCreature->addUnitState(UNIT_STAT_STUNNED);
        }
        else
        {
            DoScriptText(SAY_GOLEM_COMPLETE, pCreature);
            pCreature->clearUnitState(UNIT_STAT_STUNNED);

            // targets master
            pCreature->CastSpell(pCreature, SPELL_GOLEM_CHARGE_CREDIT, true);
        }
    }

    return true;
}

void AddSC_grizzly_hills()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "npc_depleted_war_golem";
    pNewScript->GetAI = &GetAI_npc_depleted_war_golem;
    pNewScript->pEffectAuraDummy = &EffectAuraDummy_npc_depleted_war_golem;
    pNewScript->RegisterSelf();
}
