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
SDName: Storm_Peaks
SD%Complete: 100
SDComment: Quest support: 12977.
SDCategory: Storm Peaks
EndScriptData */

/* ContentData
npc_floating_spirit
npc_restless_frostborn
EndContentData */

#include "precompiled.h"

/*######
## npc_floating_spirit
######*/

enum
{
    SPELL_BLOW_HODIRS_HORN              = 55983,
    SPELL_SUMMON_FROST_GIANG_SPIRIT     = 55986,
    SPELL_SUMMON_FROST_WARRIOR_SPIRIT   = 55991,
    SPELL_SUMMON_FROST_GHOST_SPIRIT     = 55992,

    NPC_FROST_GIANT_GHOST_KC            = 30138,
    NPC_FROST_DWARF_GHOST_KC            = 30139,

    NPC_NIFFELEM_FOREFATHER             = 29974,
    NPC_FROSTBORN_WARRIOR               = 30135,
    NPC_FROSTBORN_GHOST                 = 30144,
};

struct MANGOS_DLL_DECL npc_floating_spiritAI : public ScriptedAI
{
    npc_floating_spiritAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    void Reset() override
    {
        // Simple animation for the floating spirit
        m_creature->SetLevitate(true);
        m_creature->ForcedDespawn(5000);

        m_creature->GetMotionMaster()->MovePoint(0, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ() + 50.0f);
    }
};

CreatureAI* GetAI_npc_floating_spirit(Creature* pCreature)
{
    return new npc_floating_spiritAI(pCreature);
}

/*######
## npc_restless_frostborn
######*/

bool EffectDummyCreature_npc_restless_frostborn(Unit* pCaster, uint32 uiSpellId, SpellEffectIndex uiEffIndex, Creature* pCreatureTarget, ObjectGuid /*originalCasterGuid*/)
{
    if (uiSpellId == SPELL_BLOW_HODIRS_HORN && uiEffIndex == EFFECT_INDEX_0 && !pCreatureTarget->isAlive() && pCaster->GetTypeId() == TYPEID_PLAYER)
    {
        uint32 uiCredit = 0;
        uint32 uiSpawnSpell = 0;
        switch (pCreatureTarget->GetEntry())
        {
            case NPC_NIFFELEM_FOREFATHER:
                uiCredit = NPC_FROST_GIANT_GHOST_KC;
                uiSpawnSpell = SPELL_SUMMON_FROST_GIANG_SPIRIT;
                break;
            case NPC_FROSTBORN_WARRIOR:
                uiCredit = NPC_FROST_DWARF_GHOST_KC;
                uiSpawnSpell = SPELL_SUMMON_FROST_WARRIOR_SPIRIT;
                break;
            case NPC_FROSTBORN_GHOST:
                uiCredit = NPC_FROST_DWARF_GHOST_KC;
                uiSpawnSpell = SPELL_SUMMON_FROST_GHOST_SPIRIT;
                break;
        }

        // spawn the spirit and give the credit; spirit animation is handled by the script above
        pCaster->CastSpell(pCaster, uiSpawnSpell, true);
        ((Player*)pCaster)->KilledMonsterCredit(uiCredit);
        return true;
    }

    return false;
}

void AddSC_storm_peaks()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "npc_floating_spirit";
    pNewScript->GetAI = &GetAI_npc_floating_spirit;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_restless_frostborn";
    pNewScript->pEffectDummyNPC = &EffectDummyCreature_npc_restless_frostborn;
    pNewScript->RegisterSelf();
}
