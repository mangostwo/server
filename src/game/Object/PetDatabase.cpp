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
 * @file PetDatabase.cpp
 * @brief Cohesion split of Pet.cpp -- pet persistence: load from / save to /
 *        delete from the characters database. Same `Pet` class; no behaviour
 *        change.
 */

#include <sstream>
#include "Pet.h"
#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "WorldPacket.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Formulas.h"
#include "SpellAuras.h"
#include "CreatureAI.h"
#include "Unit.h"
#include "Util.h"

/**
 * @brief Loads a pet from the database for an owner.
 *
 * @param owner The player owning the pet.
 * @param petentry Optional creature entry filter.
 * @param petnumber Optional pet number filter.
 * @param current true to load the current pet.
 * @return true if the pet was loaded successfully; otherwise, false.
 */
bool Pet::LoadPetFromDB(Player* owner, uint32 petentry, uint32 petnumber, bool current)
{
    m_loading = true;

    uint32 ownerid = owner->GetGUIDLow();

    QueryResult* result;

    if (petnumber)
        // known petnumber entry                   0     1        2(?)     3          4        5      6             7       8       9          10           11         12              13        14          15                   16                   17                18
        result = CharacterDatabase.PQuery("SELECT `id`, `entry`, `owner`, `modelid`, `level`, `exp`, `Reactstate`, `slot`, `name`, `renamed`, `curhealth`, `curmana`, `curhappiness`, `abdata`, `savetime`, `resettalents_cost`, `resettalents_time`, `CreatedBySpell`, `PetType` "
                                          "FROM `character_pet` WHERE `owner` = '%u' AND `id` = '%u'",
                                          ownerid, petnumber);
    else if (current)
        // current pet (slot 0)                    0     1        2(?)     3          4        5      6             7       8       9          10           11         12              13        14          15                   16                   17                18
        result = CharacterDatabase.PQuery("SELECT `id`, `entry`, `owner`, `modelid`, `level`, `exp`, `Reactstate`, `slot`, `name`, `renamed`, `curhealth`, `curmana`, `curhappiness`, `abdata`, `savetime`, `resettalents_cost`, `resettalents_time`, `CreatedBySpell`, `PetType` "
                                          "FROM `character_pet` WHERE `owner` = '%u' AND `slot` = '%u'",
                                          ownerid, PET_SAVE_AS_CURRENT);
    else if (petentry)
        // known petentry entry (unique for summoned pet, but non unique for hunter pet (only from current or not stabled pets)
        //                                         0     1        2(?)     3          4        5      6             7       8       9          10           11         12              13        14          15                   16                   17                18
        result = CharacterDatabase.PQuery("SELECT `id`, `entry`, `owner`, `modelid`, `level`, `exp`, `Reactstate`, `slot`, `name`, `renamed`, `curhealth`, `curmana`, `curhappiness`, `abdata`, `savetime`, `resettalents_cost`, `resettalents_time`, `CreatedBySpell`, `PetType` "
                                          "FROM `character_pet` WHERE `owner` = '%u' AND `entry` = '%u' AND (`slot` = '%u' OR `slot` > '%u') ",
                                          ownerid, petentry, PET_SAVE_AS_CURRENT, PET_SAVE_LAST_STABLE_SLOT);
    else
        // any current or other non-stabled pet (for hunter "call pet")
        //                                         0     1        2(?)     3          4        5      6             7       8       9          10           11         12              13        14          15                   16                   17                18
        result = CharacterDatabase.PQuery("SELECT `id`, `entry`, `owner`, `modelid`, `level`, `exp`, `Reactstate`, `slot`, `name`, `renamed`, `curhealth`, `curmana`, `curhappiness`, `abdata`, `savetime`, `resettalents_cost`, `resettalents_time`, `CreatedBySpell`, `PetType` "
                                          "FROM `character_pet` WHERE `owner` = '%u' AND (`slot` = '%u' OR `slot` > '%u') ",
                                          ownerid, PET_SAVE_AS_CURRENT, PET_SAVE_LAST_STABLE_SLOT);

    if (!result)
    {
        return false;
    }

    Field* fields = result->Fetch();

    // update for case of current pet "slot = 0"
    petentry = fields[1].GetUInt32();
    if (!petentry)
    {
        delete result;
        return false;
    }

    CreatureInfo const* creatureInfo = ObjectMgr::GetCreatureTemplate(petentry);
    if (!creatureInfo)
    {
        sLog.outError("Pet entry %u does not exist but used at pet load (owner: %s).", petentry, owner->GetGuidStr().c_str());
        delete result;
        return false;
    }

    uint32 summon_spell_id = fields[17].GetUInt32();
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(summon_spell_id);

    bool is_temporary_summoned = spellInfo && GetSpellDuration(spellInfo) > 0;

    // check temporary summoned pets like mage water elemental
    if (current && is_temporary_summoned)
    {
        delete result;
        return false;
    }

    PetType pet_type = PetType(fields[18].GetUInt8());
    if (pet_type == HUNTER_PET)
    {
        if (!creatureInfo->isTameable(owner->CanTameExoticPets()))
        {
            delete result;
            return false;
        }
    }

    uint32 pet_number = fields[0].GetUInt32();

    if (current && owner->IsPetNeedBeTemporaryUnsummoned())
    {
        owner->SetTemporaryUnsummonedPetNumber(pet_number);
        delete result;
        return false;
    }

    Map* map = owner->GetMap();

    CreatureCreatePos pos(owner, owner->GetOrientation(), PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);

    uint32 guid = pos.GetMap()->GenerateLocalLowGuid(HIGHGUID_PET);
    if (!Create(guid, pos, creatureInfo, pet_number))
    {
        delete result;
        return false;
    }

    setPetType(pet_type);
    setFaction(owner->getFaction());
    SetUInt32Value(UNIT_CREATED_BY_SPELL, summon_spell_id);

    // reget for sure use real creature info selected for Pet at load/creating
    CreatureInfo const* cinfo = GetCreatureInfo();
    if (cinfo->CreatureType == CREATURE_TYPE_CRITTER)
    {
        AIM_Initialize();
        pos.GetMap()->Add((Creature*)this);
        delete result;
        return true;
    }

    m_charmInfo->SetPetNumber(pet_number, IsPermanentPetFor(owner));

    SetOwnerGuid(owner->GetObjectGuid());
    SetDisplayId(fields[3].GetUInt32());
    SetNativeDisplayId(fields[3].GetUInt32());
    uint32 petlevel = fields[4].GetUInt32();
    SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);
    SetName(fields[8].GetString());

    switch (getPetType())
    {
        case SUMMON_PET:
            petlevel = owner->getLevel();
            break;
        case HUNTER_PET:
            SetByteFlag(UNIT_FIELD_BYTES_2, 2, fields[9].GetBool() ? UNIT_CAN_BE_ABANDONED : UNIT_CAN_BE_RENAMED | UNIT_CAN_BE_ABANDONED);
            SetMaxPower(POWER_HAPPINESS, GetCreatePowers(POWER_HAPPINESS));
            SetPower(POWER_HAPPINESS, fields[12].GetUInt32());
            SetPowerType(POWER_FOCUS);
            break;
        default:
            sLog.outError("Pet have incorrect type (%u) for pet loading.", getPetType());
    }

    if (owner->IsPvP())
    {
        SetPvP(true);
    }

    if (owner->IsFFAPvP())
    {
        SetFFAPvP(true);
    }

    SetCanModifyStats(true);
    InitStatsForLevel(petlevel);
    InitTalentForLevel();                                   // set original talents points before spell loading

    SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(time(NULL)));
    SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, fields[5].GetUInt32());
    SetCreatorGuid(owner->GetObjectGuid());

    m_charmInfo->SetReactState(ReactStates(fields[6].GetUInt8()));

    uint32 savedhealth = fields[10].GetUInt32();
    uint32 savedpower = fields[11].GetUInt32();

    // set current pet as current
    // 0=current
    // 1..MAX_PET_STABLES in stable slot
    // PET_SAVE_NOT_IN_SLOT(100) = not stable slot (summoning))
    if (fields[7].GetUInt32() != 0)
    {
        CharacterDatabase.BeginTransaction();

        static SqlStatementID id_1;
        static SqlStatementID id_2;

        SqlStatement stmt = CharacterDatabase.CreateStatement(id_1, "UPDATE `character_pet` SET `slot` = ? WHERE `owner` = ? AND `slot` = ? AND `id` <> ?");
        stmt.PExecute(uint32(PET_SAVE_NOT_IN_SLOT), ownerid, uint32(PET_SAVE_AS_CURRENT), m_charmInfo->GetPetNumber());

        stmt = CharacterDatabase.CreateStatement(id_2, "UPDATE `character_pet` SET `slot` = ? WHERE `owner` = ? AND `id` = ?");
        stmt.PExecute(uint32(PET_SAVE_AS_CURRENT), ownerid, m_charmInfo->GetPetNumber());

        CharacterDatabase.CommitTransaction();
    }

    // load action bar, if data broken will fill later by default spells.
    if (!is_temporary_summoned)
    {
        m_charmInfo->LoadPetActionBar(fields[13].GetCppString());
    }

    // since last save (in seconds)
    uint32 timediff = uint32(time(NULL) - fields[14].GetUInt64());

    m_resetTalentsCost = fields[15].GetUInt32();
    m_resetTalentsTime = fields[16].GetUInt64();

    delete result;

    // load spells/cooldowns/auras
    _LoadAuras(timediff);

    // init AB
    if (is_temporary_summoned)
    {
        // Temporary summoned pets always have initial spell list at load
        InitPetCreateSpells();
    }
    else
    {
        LearnPetPassives();
        CastPetAuras(current);
        CastOwnerTalentAuras();
    }

    Powers powerType = GetPowerType();

    if (getPetType() == SUMMON_PET && !current)             // all (?) summon pets come with full health when called, but not when they are current
    {
        SetHealth(GetMaxHealth());
        SetPower(powerType, GetMaxPower(powerType));
    }
    else
    {
        SetHealth(savedhealth > GetMaxHealth() ? GetMaxHealth() : savedhealth);
        SetPower(powerType, savedpower > GetMaxPower(powerType) ? GetMaxPower(powerType) : savedpower);
    }

    map->Add((Creature*)this);
    AIM_Initialize();

    // Spells should be loaded after pet is added to map, because in CheckCast is check on it
    _LoadSpells();
    InitLevelupSpellsForLevel();

    CleanupActionBar();                                     // remove unknown spells from action bar after load

    _LoadSpellCooldowns();

    owner->SetPet(this);                                    // in DB stored only full controlled creature
    DEBUG_LOG("New Pet has guid %u", GetGUIDLow());

    if (owner->GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)owner)->PetSpellInitialize();
        if (((Player*)owner)->GetGroup())
        {
            ((Player*)owner)->SetGroupUpdateFlag(GROUP_UPDATE_PET);
        }

        ((Player*)owner)->SendTalentsInfoData(true);
    }

    if (owner->GetTypeId() == TYPEID_PLAYER && getPetType() == HUNTER_PET)
    {
        result = CharacterDatabase.PQuery("SELECT `genitive`, `dative`, `accusative`, `instrumental`, `prepositional` FROM `character_pet_declinedname` WHERE `owner` = '%u' AND `id` = '%u'", owner->GetGUIDLow(), GetCharmInfo()->GetPetNumber());

        if (result)
        {
            delete m_declinedname;
            m_declinedname = new DeclinedName;

            Field* fields2 = result->Fetch();
            for (int i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
            {
                m_declinedname->name[i] = fields2[i].GetCppString();
            }

            delete result;
        }
    }

    m_loading = false;

    SynchronizeLevelWithOwner();
    return true;
}

/**
 * @brief Saves the pet to the database using the specified save mode.
 *
 * @param mode The pet save mode to apply.
 */
void Pet::SavePetToDB(PetSaveMode mode)
{
    if (!GetEntry())
    {
        return;
    }

    // save only fully controlled creature
    if (!isControlled())
    {
        return;
    }

    // not save not player pets
    if (!GetOwnerGuid().IsPlayer())
    {
        return;
    }

    Player* pOwner = (Player*)GetOwner();
    if (!pOwner)
    {
        return;
    }

    // current/stable/not_in_slot
    if (mode >= PET_SAVE_AS_CURRENT)
    {
        // reagents must be returned before save call
        if (mode == PET_SAVE_REAGENTS)
        {
            mode = PET_SAVE_NOT_IN_SLOT;
        }
        // not save pet as current if another pet temporary unsummoned
        else if (mode == PET_SAVE_AS_CURRENT && pOwner->GetTemporaryUnsummonedPetNumber() &&
                 pOwner->GetTemporaryUnsummonedPetNumber() != m_charmInfo->GetPetNumber())
        {
            // pet will lost anyway at restore temporary unsummoned
            if (getPetType() == HUNTER_PET)
            {
                return;
            }

            // for warlock case
            mode = PET_SAVE_NOT_IN_SLOT;
        }

        uint32 curhealth = GetHealth();
        uint32 curpower = GetPower(GetPowerType());

        // stable and not in slot saves
        if (mode != PET_SAVE_AS_CURRENT)
        {
            RemoveAllAuras();
        }

        // save pet's data as one single transaction
        CharacterDatabase.BeginTransaction();
        _SaveSpells();
        _SaveSpellCooldowns();
        _SaveAuras();

        uint32 ownerLow = GetOwnerGuid().GetCounter();
        // remove current data
        static SqlStatementID delPet ;
        static SqlStatementID insPet ;

        SqlStatement stmt = CharacterDatabase.CreateStatement(delPet, "DELETE FROM `character_pet` WHERE `owner` = ? AND `id` = ?");
        stmt.PExecute(ownerLow, m_charmInfo->GetPetNumber());

        // prevent duplicate using slot (except PET_SAVE_NOT_IN_SLOT)
        if (mode <= PET_SAVE_LAST_STABLE_SLOT)
        {
            static SqlStatementID updPet ;

            stmt = CharacterDatabase.CreateStatement(updPet, "UPDATE `character_pet` SET `slot` = ? WHERE `owner` = ? AND `slot` = ?");
            stmt.PExecute(uint32(PET_SAVE_NOT_IN_SLOT), ownerLow, uint32(mode));
        }

        // prevent existence another hunter pet in PET_SAVE_AS_CURRENT and PET_SAVE_NOT_IN_SLOT
        if (getPetType() == HUNTER_PET && (mode == PET_SAVE_AS_CURRENT || mode > PET_SAVE_LAST_STABLE_SLOT))
        {
            static SqlStatementID del ;

            stmt = CharacterDatabase.CreateStatement(del, "DELETE FROM `character_pet` WHERE `owner` = ? AND (`slot` = ? OR `slot` > ?)");
            stmt.PExecute(ownerLow, uint32(PET_SAVE_AS_CURRENT), uint32(PET_SAVE_LAST_STABLE_SLOT));
        }

        // save pet
        SqlStatement savePet = CharacterDatabase.CreateStatement(insPet, "INSERT INTO `character_pet` "
                               "(`id`, `entry`,  `owner`, `modelid`, `level`, `exp`, `Reactstate`, `slot`, `name`, `renamed`, `curhealth`, `curmana`, `curhappiness`, `abdata`, `savetime`, `resettalents_cost`, `resettalents_time`, `CreatedBySpell`, `PetType`) "
                               "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

        savePet.addUInt32(m_charmInfo->GetPetNumber());
        savePet.addUInt32(GetEntry());
        savePet.addUInt32(ownerLow);
        savePet.addUInt32(GetNativeDisplayId());
        savePet.addUInt32(getLevel());
        savePet.addUInt32(GetUInt32Value(UNIT_FIELD_PETEXPERIENCE));
        savePet.addUInt32(uint32(m_charmInfo->GetReactState()));
        savePet.addUInt32(uint32(mode));
        savePet.addString(m_name.c_str());
        savePet.addUInt32(uint32(HasByteFlag(UNIT_FIELD_BYTES_2, 2, UNIT_CAN_BE_RENAMED) ? 0 : 1));
        savePet.addUInt32((curhealth < 1 ? 1 : curhealth));
        savePet.addUInt32(curpower);
        savePet.addUInt32(GetPower(POWER_HAPPINESS));

        std::ostringstream ss;
        for (uint32 i = ACTION_BAR_INDEX_START; i < ACTION_BAR_INDEX_END; ++i)
        {
            ss << uint32(m_charmInfo->GetActionBarEntry(i)->GetType()) << " "
               << uint32(m_charmInfo->GetActionBarEntry(i)->GetAction()) << " ";
        };
        savePet.addString(ss);

        savePet.addUInt64(uint64(time(NULL)));
        savePet.addUInt32(uint32(m_resetTalentsCost));
        savePet.addUInt64(uint64(m_resetTalentsTime));
        savePet.addUInt32(GetUInt32Value(UNIT_CREATED_BY_SPELL));
        savePet.addUInt32(uint32(getPetType()));

        savePet.Execute();
        CharacterDatabase.CommitTransaction();
    }
    else
    {
        RemoveAllAuras(AURA_REMOVE_BY_DELETE);
        DeleteFromDB(m_charmInfo->GetPetNumber());
    }
}

/**
 * @brief Deletes pet-related database records.
 *
 * @param guidlow The pet number identifier.
 * @param separate_transaction true to wrap deletion in its own transaction.
 */
void Pet::DeleteFromDB(uint32 guidlow, bool separate_transaction)
{
    if (separate_transaction)
    {
        CharacterDatabase.BeginTransaction();
    }

    static SqlStatementID delPet ;
    static SqlStatementID delDeclName ;
    static SqlStatementID delAuras ;
    static SqlStatementID delSpells ;
    static SqlStatementID delSpellCD ;

    SqlStatement stmt = CharacterDatabase.CreateStatement(delPet, "DELETE FROM `character_pet` WHERE `id` = ?");
    stmt.PExecute(guidlow);

    stmt = CharacterDatabase.CreateStatement(delDeclName, "DELETE FROM `character_pet_declinedname` WHERE `id` = ?");
    stmt.PExecute(guidlow);

    stmt = CharacterDatabase.CreateStatement(delAuras, "DELETE FROM `pet_aura` WHERE `guid` = ?");
    stmt.PExecute(guidlow);

    stmt = CharacterDatabase.CreateStatement(delSpells, "DELETE FROM `pet_spell` WHERE `guid` = ?");
    stmt.PExecute(guidlow);

    stmt = CharacterDatabase.CreateStatement(delSpellCD, "DELETE FROM `pet_spell_cooldown` WHERE `guid` = ?");
    stmt.PExecute(guidlow);

    if (separate_transaction)
    {
        CharacterDatabase.CommitTransaction();
    }
}
