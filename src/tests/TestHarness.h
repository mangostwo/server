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

#ifndef MANGOS_TESTHARNESS_H
#define MANGOS_TESTHARNESS_H

#include <cstdio>
#include <string>
#include <vector>

/**
 * @brief A test runner in eighty lines, with no external dependency.
 *
 * Deliberately not GoogleTest or Catch2. The whole point of these tests is that
 * they can be built and run anywhere the server itself builds -- three
 * toolchains, one of them FreeBSD -- without adding a package to install or a
 * submodule to vendor. There is nothing here a real framework would give us that
 * these cases need: they are pure functions over bytes.
 */
namespace testing
{
    struct Case
    {
        const char* name;
        void      (*fn)();
    };

    inline std::vector<Case>& Registry()
    {
        static std::vector<Case> cases;
        return cases;
    }

    inline int& Failures()
    {
        static int failures = 0;
        return failures;
    }

    inline const char*& CurrentTest()
    {
        static const char* name = "";
        return name;
    }

    struct Registrar
    {
        Registrar(const char* name, void (*fn)())
        {
            Registry().push_back(Case{name, fn});
        }
    };

    inline void ReportFailure(const char* file, int line, const std::string& what)
    {
        ++Failures();
        std::printf("  FAIL %s\n    %s:%d: %s\n", CurrentTest(), file, line, what.c_str());
    }

    inline int RunAll()
    {
        int failedCases = 0;

        for (const Case& c : Registry())
        {
            CurrentTest() = c.name;
            const int before = Failures();

            c.fn();

            if (Failures() == before)
            {
                std::printf("  ok   %s\n", c.name);
            }
            else
            {
                ++failedCases;
            }
        }

        std::printf("\n%d test(s), %d failed\n",
                    int(Registry().size()), failedCases);
        return failedCases == 0 ? 0 : 1;
    }
}

#define TEST(NAME)                                                            \
    static void NAME();                                                       \
    static testing::Registrar registrar_##NAME(#NAME, &NAME);                 \
    static void NAME()

#define CHECK(COND)                                                           \
    do {                                                                      \
        if (!(COND))                                                          \
        {                                                                     \
            testing::ReportFailure(__FILE__, __LINE__, "expected: " #COND);   \
        }                                                                     \
    } while (0)

// CHECK reports and carries on, so one run finds every failure. Use REQUIRE
// where carrying on would be undefined behaviour rather than merely wrong --
// a crash reports nothing at all.
#define REQUIRE(COND)                                                         \
    do {                                                                      \
        if (!(COND))                                                          \
        {                                                                     \
            testing::ReportFailure(__FILE__, __LINE__, "required: " #COND);   \
            return;                                                           \
        }                                                                     \
    } while (0)

#define CHECK_EQ(A, B)                                                        \
    do {                                                                      \
        const auto lhs_ = (A);                                                \
        const auto rhs_ = (B);                                                \
        if (!(lhs_ == rhs_))                                                  \
        {                                                                     \
            testing::ReportFailure(__FILE__, __LINE__,                        \
                std::string(#A " == " #B ", got ")                            \
                + std::to_string(lhs_) + " vs " + std::to_string(rhs_));      \
        }                                                                     \
    } while (0)

#endif
