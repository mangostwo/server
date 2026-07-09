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
 * @file ChatMessageValidation.cpp
 * @brief Cohesion split of Chat.cpp -- chat shift-link validation.
 */

#include "Chat.h"
#include "Log.h"
#include "World.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "DBCStores.h"

/**
 * @brief Validates chat text and embedded shift-links against configured strictness rules.
 *
 * @param message The message text to validate.
 * @return true if the message is valid; otherwise false.
 */
bool ChatHandler::isValidChatMessage(const char* message)
{
    /*

    valid examples:
    |cffa335ee|Hitem:812:0:0:0:0:0:0:0:70|h[Glowing Brightwood Staff]|h|r
    |cff808080|Hquest:2278:47|h[The Platinum Discs]|h|r
    |cffffd000|Htrade:4037:1:150:1:6AAAAAAAAAAAAAAAAAAAAAAOAADAAAAAAAAAAAAAAAAIAAAAAAAAA|h[Engineering]|h|r
    |cff4e96f7|Htalent:2232:-1|h[Taste for Blood]|h|r
    |cff71d5ff|Hspell:21563|h[Command]|h|r
    |cffffd000|Henchant:3919|h[Engineering: Rough Dynamite]|h|r
    |cffffff00|Hachievement:546:0000000000000001:0:0:0:-1:0:0:0:0|h[Safe Deposit]|h|r
    |cff66bbff|Hglyph:21:762|h[Glyph of Bladestorm]|h|r

    | will be escaped to ||
    */

    if (strlen(message) > 255)
    {
        return false;
    }

    const char validSequence[6] = "cHhhr";
    const char* validSequenceIterator = validSequence;

    // more simple checks
    if (sWorld.getConfig(CONFIG_UINT32_CHAT_STRICT_LINK_CHECKING_SEVERITY) < 3)
    {
        const std::string validCommands = "cHhr|";

        while (*message)
        {
            // find next pipe command
            message = strchr(message, '|');

            if (!message)
            {
                return true;
            }

            ++message;
            char commandChar = *message;
            if (validCommands.find(commandChar) == std::string::npos)
            {
                return false;
            }

            ++message;
            // validate sequence
            if (sWorld.getConfig(CONFIG_UINT32_CHAT_STRICT_LINK_CHECKING_SEVERITY) == 2)
            {
                if (commandChar == *validSequenceIterator)
                {
                    if (validSequenceIterator == validSequence + 4)
                    {
                        validSequenceIterator = validSequence;
                    }
                    else
                    {
                        ++validSequenceIterator;
                    }
                }
                else if (commandChar != '|')
                {
                    return false;
                }
            }
        }
        return true;
    }

    std::istringstream reader(message);
    char buffer[256];

    uint32 color = 0;

    ItemPrototype const* linkedItem = NULL;
    Quest const* linkedQuest = NULL;
    SpellEntry const* linkedSpell = NULL;
    AchievementEntry const* linkedAchievement = NULL;
    ItemRandomPropertiesEntry const* itemProperty = NULL;
    ItemRandomSuffixEntry const* itemSuffix = NULL;

    while (!reader.eof())
    {
        if (validSequence == validSequenceIterator)
        {
            linkedItem = NULL;
            linkedQuest = NULL;
            linkedSpell = NULL;
            linkedAchievement = NULL;
            itemProperty = NULL;
            itemSuffix = NULL;

            reader.ignore(255, '|');
        }
        else if (reader.get() != '|')
        {
            DEBUG_LOG("ChatHandler::isValidChatMessage sequence aborted unexpectedly");
            return false;
        }

        // pipe has always to be followed by at least one char
        if (reader.peek() == '\0')
        {
            DEBUG_LOG("ChatHandler::isValidChatMessage pipe followed by \\0");
            return false;
        }

        // no further pipe commands
        if (reader.eof())
        {
            break;
        }

        char commandChar;
        reader >> commandChar;

        // | in normal messages is escaped by ||
        if (commandChar != '|')
        {
            if (commandChar == *validSequenceIterator)
            {
                if (validSequenceIterator == validSequence + 4)
                {
                    validSequenceIterator = validSequence;
                }
                else
                {
                    ++validSequenceIterator;
                }
            }
            else
            {
                DEBUG_LOG("ChatHandler::isValidChatMessage invalid sequence, expected %c but got %c", *validSequenceIterator, commandChar);
                return false;
            }
        }
        else if (validSequence != validSequenceIterator)
        {
            // no escaped pipes in sequences
            DEBUG_LOG("ChatHandler::isValidChatMessage got escaped pipe in sequence");
            return false;
        }

        switch (commandChar)
        {
            case 'c':
                color = 0;
                // validate color, expect 8 hex chars
                for (int i = 0; i < 8; ++i)
                {
                    char c;
                    reader >> c;
                    if (!c)
                    {
                        DEBUG_LOG("ChatHandler::isValidChatMessage got \\0 while reading color in |c command");
                        return false;
                    }

                    color <<= 4;
                    // check for hex char
                    if (c >= '0' && c <= '9')
                    {
                        color |= c - '0';
                        continue;
                    }
                    if (c >= 'a' && c <= 'f')
                    {
                        color |= 10 + c - 'a';
                        continue;
                    }
                    DEBUG_LOG("ChatHandler::isValidChatMessage got non hex char '%c' while reading color", c);
                    return false;
                }
                break;
            case 'H':
                // read chars up to colon  = link type
                reader.getline(buffer, 256, ':');
                if (reader.eof())                           // : must be
                {
                    return false;
                }

                if (strcmp(buffer, "item") == 0)
                {
                    // read item entry
                    reader.getline(buffer, 256, ':');
                    if (reader.eof())                       // : must be
                    {
                        return false;
                    }

                    linkedItem = ObjectMgr::GetItemPrototype(atoi(buffer));
                    if (!linkedItem)
                    {
                        DEBUG_LOG("ChatHandler::isValidChatMessage got invalid itemID %u in |item command", atoi(buffer));
                        return false;
                    }

                    if (color != ItemQualityColors[linkedItem->Quality])
                    {
                        DEBUG_LOG("ChatHandler::isValidChatMessage linked item has color %u, but user claims %u", ItemQualityColors[linkedItem->Quality],
                                  color);
                        return false;
                    }

                    // the itementry is followed by several integers which describe an instance of this item

                    // position relative after itemEntry
                    const uint8 randomPropertyPosition = 6;

                    int32 propertyId = 0;
                    bool negativeNumber = false;
                    char c;
                    for (uint8 i = 0; i < randomPropertyPosition; ++i)
                    {
                        propertyId = 0;
                        negativeNumber = false;
                        while ((c = reader.get()) != ':')
                        {
                            if (c >= '0' && c <= '9')
                            {
                                propertyId *= 10;
                                propertyId += c - '0';
                            }
                            else if (c == '-')
                            {
                                negativeNumber = true;
                            }
                            else
                            {
                                return false;
                            }
                        }
                    }
                    if (negativeNumber)
                    {
                        propertyId *= -1;
                    }

                    if (propertyId > 0)
                    {
                        itemProperty = sItemRandomPropertiesStore.LookupEntry(propertyId);
                        if (!itemProperty)
                        {
                            return false;
                        }
                    }
                    else if (propertyId < 0)
                    {
                        itemSuffix = sItemRandomSuffixStore.LookupEntry(-propertyId);
                        if (!itemSuffix)
                        {
                            return false;
                        }
                    }

                    // ignore other integers
                    while ((c >= '0' && c <= '9') || c == ':')
                    {
                        reader.ignore(1);
                        c = reader.peek();
                    }
                }
                else if (strcmp(buffer, "quest") == 0)
                {
                    // no color check for questlinks, each client will adapt it anyway
                    uint32 questid = 0;
                    // read questid
                    char c = reader.peek();
                    while (c >= '0' && c <= '9')
                    {
                        reader.ignore(1);
                        questid *= 10;
                        questid += c - '0';
                        c = reader.peek();
                    }

                    linkedQuest = sObjectMgr.GetQuestTemplate(questid);

                    if (!linkedQuest)
                    {
                        DEBUG_LOG("ChatHandler::isValidChatMessage Questtemplate %u not found", questid);
                        return false;
                    }

                    if (c != ':')
                    {
                        DEBUG_LOG("ChatHandler::isValidChatMessage Invalid quest link structure");
                        return false;
                    }

                    reader.ignore(1);
                    c = reader.peek();
                    // level
                    uint32 questlevel = 0;
                    while (c >= '0' && c <= '9')
                    {
                        reader.ignore(1);
                        questlevel *= 10;
                        questlevel += c - '0';
                        c = reader.peek();
                    }

                    if (questlevel >= STRONG_MAX_LEVEL)
                    {
                        DEBUG_LOG("ChatHandler::isValidChatMessage Quest level %u too big", questlevel);
                        return false;
                    }

                    if (c != '|')
                    {
                        DEBUG_LOG("ChatHandler::isValidChatMessage Invalid quest link structure");
                        return false;
                    }
                }
                else if (strcmp(buffer, "trade") == 0)
                {
                    if (color != CHAT_LINK_COLOR_TRADE)
                    {
                        return false;
                    }

                    // read spell entry
                    reader.getline(buffer, 256, ':');
                    if (reader.eof())                       // : must be
                    {
                        return false;
                    }

                    linkedSpell = sSpellStore.LookupEntry(atoi(buffer));
                    if (!linkedSpell)
                    {
                        return false;
                    }

                    char c = reader.peek();
                    // base64 encoded stuff
                    while (c != '|' && c != '\0')
                    {
                        reader.ignore(1);
                        c = reader.peek();
                    }
                }
                else if (strcmp(buffer, "talent") == 0)
                {
                    // talent links are always supposed to be blue
                    if (color != CHAT_LINK_COLOR_TALENT)
                    {
                        return false;
                    }

                    // read talent entry
                    reader.getline(buffer, 256, ':');
                    if (reader.eof())                       // : must be
                    {
                        return false;
                    }

                    TalentEntry const* talentInfo = sTalentStore.LookupEntry(atoi(buffer));
                    if (!talentInfo)
                    {
                        return false;
                    }

                    linkedSpell = sSpellStore.LookupEntry(talentInfo->SpellRank[0]);
                    if (!linkedSpell)
                    {
                        return false;
                    }

                    char c = reader.peek();
                    // skillpoints? whatever, drop it
                    while (c != '|' && c != '\0')
                    {
                        reader.ignore(1);
                        c = reader.peek();
                    }
                }
                else if (strcmp(buffer, "spell") == 0)
                {
                    if (color != CHAT_LINK_COLOR_SPELL)
                    {
                        return false;
                    }

                    uint32 spellid = 0;
                    // read spell entry
                    char c = reader.peek();
                    while (c >= '0' && c <= '9')
                    {
                        reader.ignore(1);
                        spellid *= 10;
                        spellid += c - '0';
                        c = reader.peek();
                    }
                    linkedSpell = sSpellStore.LookupEntry(spellid);
                    if (!linkedSpell)
                    {
                        return false;
                    }
                }
                else if (strcmp(buffer, "enchant") == 0)
                {
                    if (color != CHAT_LINK_COLOR_ENCHANT)
                    {
                        return false;
                    }

                    uint32 spellid = 0;
                    // read spell entry
                    char c = reader.peek();
                    while (c >= '0' && c <= '9')
                    {
                        reader.ignore(1);
                        spellid *= 10;
                        spellid += c - '0';
                        c = reader.peek();
                    }
                    linkedSpell = sSpellStore.LookupEntry(spellid);
                    if (!linkedSpell)
                    {
                        return false;
                    }
                }
                else if (strcmp(buffer, "achievement") == 0)
                {
                    if (color != CHAT_LINK_COLOR_ACHIEVEMENT)
                    {
                        return false;
                    }

                    reader.getline(buffer, 256, ':');
                    if (reader.eof())                       // : must be
                    {
                        return false;
                    }

                    uint32 achievementId = atoi(buffer);
                    linkedAchievement = sAchievementStore.LookupEntry(achievementId);

                    if (!linkedAchievement)
                    {
                        return false;
                    }

                    char c = reader.peek();
                    // skip progress
                    while (c != '|' && c != '\0')
                    {
                        reader.ignore(1);
                        c = reader.peek();
                    }
                }
                else if (strcmp(buffer, "glyph") == 0)
                {
                    if (color != CHAT_LINK_COLOR_GLYPH)
                    {
                        return false;
                    }

                    // first id is slot, drop it
                    reader.getline(buffer, 256, ':');
                    if (reader.eof())                       // : must be
                    {
                        return false;
                    }

                    uint32 glyphId = 0;
                    char c = reader.peek();
                    while (c >= '0' && c <= '9')
                    {
                        glyphId *= 10;
                        glyphId += c - '0';
                        reader.ignore(1);
                        c = reader.peek();
                    }
                    GlyphPropertiesEntry const* glyph = sGlyphPropertiesStore.LookupEntry(glyphId);
                    if (!glyph)
                    {
                        return false;
                    }

                    linkedSpell = sSpellStore.LookupEntry(glyph->SpellID);

                    if (!linkedSpell)
                    {
                        return false;
                    }
                }
                else
                {
                    DEBUG_LOG("ChatHandler::isValidChatMessage user sent unsupported link type '%s'", buffer);
                    return false;
                }
                break;
            case 'h':
                // if h is next element in sequence, this one must contain the linked text :)
                if (*validSequenceIterator == 'h')
                {
                    // links start with '['
                    if (reader.get() != '[')
                    {
                        DEBUG_LOG("ChatHandler::isValidChatMessage link caption doesn't start with '['");
                        return false;
                    }
                    reader.getline(buffer, 256, ']');
                    if (reader.eof())                       // ] must be
                    {
                        return false;
                    }

                    // verify the link name
                    if (linkedSpell)
                    {
                        // spells with that flag have a prefix of "$PROFESSION: "
                        if (linkedSpell->HasAttribute(SPELL_ATTR_TRADESPELL))
                        {
                            // lookup skillid
                            SkillLineAbilityMapBounds bounds = sSpellMgr.GetSkillLineAbilityMapBounds(linkedSpell->Id);
                            if (bounds.first == bounds.second)
                            {
                                return false;
                            }

                            SkillLineAbilityEntry const* skillInfo = bounds.first->second;

                            if (!skillInfo)
                            {
                                return false;
                            }

                            SkillLineEntry const* skillLine = sSkillLineStore.LookupEntry(skillInfo->skillId);
                            if (!skillLine)
                            {
                                return false;
                            }

                            for (uint8 i = 0; i < MAX_LOCALE; ++i)
                            {
                                uint32 skillLineNameLength = strlen(skillLine->name[i]);
                                if (skillLineNameLength > 0 && strncmp(skillLine->name[i], buffer, skillLineNameLength) == 0)
                                {
                                    // found the prefix, remove it to perform spellname validation below
                                    // -2 = strlen(": ")
                                    uint32 spellNameLength = strlen(buffer) - skillLineNameLength - 2;
                                    memmove(buffer, buffer + skillLineNameLength + 2, spellNameLength + 1);
                                }
                            }
                        }
                        bool foundName = false;
                        for (uint8 i = 0; i < MAX_LOCALE; ++i)
                        {
                            if (*linkedSpell->SpellName[i] && strcmp(linkedSpell->SpellName[i], buffer) == 0)
                            {
                                foundName = true;
                                break;
                            }
                        }
                        if (!foundName)
                        {
                            return false;
                        }
                    }
                    else if (linkedQuest)
                    {
                        if (linkedQuest->GetTitle() != buffer)
                        {
                            QuestLocale const* ql = sObjectMgr.GetQuestLocale(linkedQuest->GetQuestId());

                            if (!ql)
                            {
                                DEBUG_LOG("ChatHandler::isValidChatMessage default questname didn't match and there is no locale");
                                return false;
                            }

                            bool foundName = false;
                            for (uint8 i = 0; i < ql->Title.size(); ++i)
                            {
                                if (ql->Title[i] == buffer)
                                {
                                    foundName = true;
                                    break;
                                }
                            }
                            if (!foundName)
                            {
                                DEBUG_LOG("ChatHandler::isValidChatMessage no quest locale title matched");
                                return false;
                            }
                        }
                    }
                    else if (linkedItem)
                    {
                        char* const* suffix = itemSuffix ? itemSuffix->nameSuffix : (itemProperty ? itemProperty->nameSuffix : NULL);

                        std::string expectedName = std::string(linkedItem->Name1);
                        if (suffix)
                        {
                            expectedName += " ";
                            expectedName += suffix[LOCALE_enUS];
                        }

                        if (expectedName != buffer)
                        {
                            ItemLocale const* il = sObjectMgr.GetItemLocale(linkedItem->ItemId);

                            bool foundName = false;
                            for (uint8 i = LOCALE_koKR; i < MAX_LOCALE; ++i)
                            {
                                int8 dbIndex = sObjectMgr.GetIndexForLocale(LocaleConstant(i));
                                if (dbIndex == -1 || il == NULL || (size_t)dbIndex >= il->Name.size())
                                    // using strange database/client combinations can lead to this case
                                {
                                    expectedName = linkedItem->Name1;
                                }
                                else
                                {
                                    expectedName = il->Name[dbIndex];
                                }
                                if (suffix)
                                {
                                    expectedName += " ";
                                    expectedName += suffix[i];
                                }
                                if (expectedName == buffer)
                                {
                                    foundName = true;
                                    break;
                                }
                            }
                            if (!foundName)
                            {
                                DEBUG_LOG("ChatHandler::isValidChatMessage linked item name wasn't found in any localization");
                                return false;
                            }
                        }
                    }
                    else if (linkedAchievement)
                    {
                        bool foundName = false;
                        for (uint8 i = 0; i < MAX_LOCALE; ++i)
                        {
                            if (*linkedAchievement->name[i] && strcmp(linkedAchievement->name[i], buffer) == 0)
                            {
                                foundName = true;
                                break;
                            }
                        }
                        if (!foundName)
                        {
                            return false;
                        }
                    }
                    // that place should never be reached - if nothing linked has been set in |H
                    // it will return false before
                    else
                    {
                        return false;
                    }
                }
                break;
            case 'r':
            case '|':
                // no further payload
                break;
            default:
                DEBUG_LOG("ChatHandler::isValidChatMessage got invalid command |%c", commandChar);
                return false;
        }
    }

    // check if every opened sequence was also closed properly
    if (validSequence != validSequenceIterator)
    {
        DEBUG_LOG("ChatHandler::isValidChatMessage EOF in active sequence");
    }

    return validSequence == validSequenceIterator;
}
