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

#ifndef MANGOSSERVER_MOVESPLINEFLAG_H
#define MANGOSSERVER_MOVESPLINEFLAG_H

#include "typedefs.h"
#include <string>

namespace Movement
{
#if defined( __GNUC__ )
#pragma pack(1)
#else
#pragma pack(push,1)
#endif

    /**
     * @brief Class representing movement spline flags.
     */
    class MoveSplineFlag
    {
        public:
            /**
             * @brief Enum for movement spline flags.
             */
            enum eFlags
            {
                None         = 0x00000000,
                // x00-xFF(first byte) used as animation Ids storage in pair with Animation flag
                Done         = 0x00000100,
                Falling      = 0x00000200,           // Affects elevation computation, can't be combined with Parabolic flag
                No_Spline    = 0x00000400,
                Parabolic    = 0x00000800,           // Affects elevation computation, can't be combined with Falling flag
                Walkmode     = 0x00001000,
                Flying       = 0x00002000,           // Smooth movement(Catmullrom interpolation mode), flying animation
                OrientationFixed = 0x00004000,       // Model orientation fixed
                Final_Point  = 0x00008000,
                Final_Target = 0x00010000,
                Final_Angle  = 0x00020000,
                Catmullrom   = 0x00040000,           // Used Catmullrom interpolation mode
                Cyclic       = 0x00080000,           // Movement by cycled spline
                Enter_Cycle  = 0x00100000,           // Everytimes appears with cyclic flag in monster move packet, erases first spline vertex after first cycle done
                Animation    = 0x00200000,           // Plays animation after some time passed
                Frozen       = 0x00400000,           // Will never arrive
                BoardVehicle = 0x00800000,
                ExitVehicle  = 0x01000000,
                Unknown7     = 0x02000000,
                Unknown8     = 0x04000000,
                OrientationInversed = 0x08000000,
                Unknown10    = 0x10000000,
                Unknown11    = 0x20000000,
                Unknown12    = 0x40000000,
                Unknown13    = 0x80000000,

                // Masks
                Mask_Final_Facing = Final_Point | Final_Target | Final_Angle,
                // animation ids stored here, see AnimType enum, used with Animation flag
                Mask_Animations = 0xFF,
                // Flags that shouldn't be appended into SMSG_MONSTER_MOVE\SMSG_MONSTER_MOVE_TRANSPORT packet
                Mask_No_Monster_Move = Mask_Final_Facing | Mask_Animations | Done,
                // CatmullRom interpolation mode used
                Mask_CatmullRom = Flying | Catmullrom,
                // Unused, not suported flags
                Mask_Unused = No_Spline | Enter_Cycle | Frozen | Unknown7 | Unknown8 | Unknown10 | Unknown11 | Unknown12 | Unknown13,
            };

            /**
             * @brief Gets the raw flag value.
             * @return uint32& Reference to the raw flag value.
             */
            inline uint32& raw() { return (uint32&) * this;}
            /**
             * @brief Gets the raw flag value.
             * @return const uint32& Reference to the raw flag value.
             */
            inline const uint32& raw() const { return (const uint32&) * this;}

            /**
             * @brief Default constructor for MoveSplineFlag.
             */
            MoveSplineFlag() { raw() = 0; }
            /**
             * @brief Constructor for MoveSplineFlag with a flag value.
             * @param f The flag value.
             */
            MoveSplineFlag(uint32 f) { raw() = f; }
            /**
             * @brief Copy constructor for MoveSplineFlag.
             * @param f The MoveSplineFlag to copy.
             */
            MoveSplineFlag(const MoveSplineFlag& f) { raw() = f.raw(); }

            // Constant interface

            /**
             * @brief Checks if the movement is smooth.
             * @return bool True if the movement is smooth, false otherwise.
             */
            bool isSmooth() const { return raw() & Mask_CatmullRom;}
            bool isLinear() const { return !isSmooth();}
            /**
             * @brief Checks if the movement has a final facing.
             * @return bool True if the movement has a final facing, false otherwise.
             */
            bool isFacing() const { return raw() & Mask_Final_Facing;}

            uint8 getAnimationId() const { return animId;}
            /**
             * @brief Checks if all specified flags are set.
             * @param f The flags to check.
             * @return bool True if all specified flags are set, false otherwise.
             */
            bool hasAllFlags(uint32 f) const { return (raw() & f) == f;}
            bool hasFlag(uint32 f) const { return (raw() & f) != 0;}
            /**
             * @brief Bitwise AND operator for flags.
             * @param f The flags to AND.
             * @return uint32 The result of the AND operation.
             */
            uint32 operator & (uint32 f) const { return (raw() & f);}
            /**
             * @brief Bitwise OR operator for flags.
             * @param f The flags to OR.
             * @return uint32 The result of the OR operation.
             */
            uint32 operator | (uint32 f) const { return (raw() | f);}
            /**
             * @brief Converts the flags to a string representation.
             * @return std::string The string representation of the flags.
             */
            std::string ToString() const;

            // Not constant interface

            /**
             * @brief Bitwise AND assignment operator for flags.
             * @param f The flags to AND.
             */
            void operator &= (uint32 f) { raw() &= f;}
            /**
             * @brief Bitwise OR assignment operator for flags.
             * @param f The flags to OR.
             */
            void operator |= (uint32 f) { raw() |= f;}

            void EnableAnimation(uint8 anim) { raw() = (raw() & ~(Mask_Animations | Falling | Parabolic))   | Animation | anim;}
            void EnableParabolic()           { raw() = (raw() & ~(Mask_Animations | Falling | Animation))   | Parabolic;}
            void EnableFalling()             { raw() = (raw() & ~(Mask_Animations | Parabolic | Animation)) | Falling;}
            void EnableFlying()              { raw() = (raw() & ~Catmullrom)                                | Flying; }
            void EnableCatmullRom()          { raw() = (raw() & ~Flying)                                    | Catmullrom; }
            /**
             * @brief Enables facing a point.
             */
            void EnableFacingPoint()    { raw() = (raw() & ~Mask_Final_Facing) | Final_Point;}
            /**
             * @brief Enables facing an angle.
             */
            void EnableFacingAngle()    { raw() = (raw() & ~Mask_Final_Facing) | Final_Angle;}
            /**
             * @brief Enables facing a target.
             */
            void EnableFacingTarget()   { raw() = (raw() & ~Mask_Final_Facing) | Final_Target;}
            void EnableBoardVehicle()        { raw() = (raw() & ~(Catmullrom | ExitVehicle))                | BoardVehicle; }
            void EnableExitVehicle()         { raw() = (raw() & ~BoardVehicle)                              | ExitVehicle; }

            uint8 animId       : 8;
            bool done          : 1;
            bool falling       : 1;
            bool no_spline     : 1;
            bool parabolic     : 1;
            bool walkmode      : 1;
            bool flying        : 1;
            bool orientationFixed : 1;
            bool final_point   : 1;
            bool final_target  : 1;
            bool final_angle   : 1;
            bool catmullrom    : 1;
            bool cyclic        : 1;
            bool enter_cycle   : 1;
            bool animation     : 1;
            bool frozen        : 1;
            bool boardVehicle  : 1;
            bool exitVehicle   : 1;
            bool unknown7      : 1;
            bool unknown8      : 1;
            bool orientationInversed : 1;
            bool unknown10     : 1;
            bool unknown11     : 1;
            bool unknown12     : 1;
            bool unknown13     : 1;
    };
#if defined( __GNUC__ )
#pragma pack()
#else
#pragma pack(pop)
#endif
}

#endif // MANGOSSERVER_MOVESPLINEFLAG_H

