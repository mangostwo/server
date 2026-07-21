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

#ifndef MANGOS_LOCALES_H
#define MANGOS_LOCALES_H

#include <string>
#include <vector>

/**
 * @brief Client locales, in the order the client itself numbers them.
 *
 * The values are wire and database values, so the order is not ours to change.
 */
enum LocaleConstant
{
    LOCALE_enUS = 0,    ///< also enGB
    LOCALE_koKR = 1,
    LOCALE_frFR = 2,
    LOCALE_deDE = 3,
    LOCALE_zhCN = 4,
    LOCALE_zhTW = 5,
    LOCALE_esES = 6,
    LOCALE_esMX = 7,
    LOCALE_ruRU = 8
};

#define MAX_LOCALE     9
#define DEFAULT_LOCALE LOCALE_enUS

/// Canonical name of each locale, indexed by LocaleConstant.
extern char const* localeNames[MAX_LOCALE];

/**
 * @brief A locale name paired with the constant it maps to.
 *
 * Distinct from localeNames because several names can map to one locale (enGB
 * and enUS are the same locale to the server).
 */
struct LocaleNameStr
{
    char const*    name;
    LocaleConstant locale;
};

/// Every accepted name, including aliases. NULL-name terminated.
extern LocaleNameStr const fullLocaleNameList[];

/// Resolve a locale name; unknown names fall back to LOCALE_enUS.
LocaleConstant GetLocaleByName(const std::string& name);

typedef std::vector<std::string> StringVector;

#endif
