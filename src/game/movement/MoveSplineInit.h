/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2023 MaNGOS <https://getmangos.eu>
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

#ifndef MANGOSSERVER_MOVESPLINEINIT_H
#define MANGOSSERVER_MOVESPLINEINIT_H

#include "MoveSplineInitArgs.h"
#include "PathFinder.h"

class Unit;

namespace Movement
{
    enum AnimType
    {
        ToGround    = 0, // 460 = ToGround, index of AnimationData.dbc
        FlyToFly    = 1, // 461 = FlyToFly?
        ToFly       = 2, // 458 = ToFly
        FlyToGround = 3, // 463 = FlyToGround
    };
    /**
     * @brief Initializes and launches spline movement
     *
     */
    class MoveSplineInit
    {
        public:

            /**
             * @brief
             *
             * @param m
             */
            explicit MoveSplineInit(Unit& m);

            /**
             * @brief Final pass of initialization that launches spline movement.
             *
             * @return int32 duration - estimated travel time
             */
            int32 Launch();

            /**
             * @brief Stop any creature movement
             *
             */
            void Stop();

            /* Adds movement by parabolic trajectory
             * @param amplitude  - the maximum height of parabola, value could be negative and positive
             * @param start_time - delay between movement starting time and beginning to move by parabolic trajectory
             * can't be combined with final animation
             */
            void SetParabolic(float amplitude, float start_time);
            /* Plays animation after movement done
             * can't be combined with parabolic movement
             */
            void SetAnimation(AnimType anim);

            /*
             * @brief  Adds final facing animation
             * sets unit's facing to specified point/angle after all path done.
             * you can have only one final facing: previous will be overriden
             *
             * @param angle
             */
            void SetFacing(float angle);
            /**
             * @brief
             *
             * @param point
             */
            void SetFacing(Vector3 const& point);
            /**
             * @brief
             *
             * @param target
             */
            void SetFacing(const Unit* target);

            /**
             * @brief Initializes movement by path
             *
             * @param path array of points, shouldn't be empty
             * @param pointId Id of fisrt point of the path. Example: when third path point will be done it will notify that pointId + 3 done
             */
            void MovebyPath(const PointsArray& path, int32 pointId = 0);

            /**
             * @brief Initializes simple A to B mition, A is current unit's position, B is destination
             *
             * @param destination
             * @param generatePath
             * @param forceDestination
             * @param maxPathRange
             */
            void MoveTo(const Vector3& destination, bool generatePath = false, bool forceDestination = false);
            /**
             * @brief
             *
             * @param x
             * @param y
             * @param z
             * @param generatePath
             * @param forceDestination
             */
            void MoveTo(float x, float y, float z, bool generatePath = false, bool forceDestination = false);

            /**
             * @brief Sets Id of fisrt point of the path
             *
             * When N-th path point will be done ILisener will notify that pointId + N done
             * Needed for waypoint movement where path splitten into parts
             *
             * @param pointId
             */
            void SetFirstPointId(int32 pointId) { args.path_Idx_offset = pointId; }

            /* Enables CatmullRom spline interpolation mode(makes path smooth)
             * if not enabled linear spline mode will be choosen. Disabled by default
             */
            void SetSmooth();
            /**
             * @brief Enables CatmullRom spline interpolation mode, enables flying animation.
             * Disabled by default
             *
             */
            void SetFly();
            /**
             * @brief Enables walk mode. Disabled by default
             *
             * @param enable
             */
            void SetWalk(bool enable);
            /**
             * @brief Makes movement cyclic. Disabled by default
             *
             */
            void SetCyclic();
            /**
             * @brief Enables falling mode. Disabled by default
             *
             */
            void SetFall();
            /* Inverses unit model orientation. Disabled by default
             */
            void SetOrientationInversed();
            /* Fixes unit's model rotation. Disabled by default
             */
            void SetOrientationFixed(bool enable);

            /**
             * @brief Sets the velocity (in case you want to have custom movement velocity)
             *
             * if no set, speed will be selected based on unit's speeds and current movement mode
             * Has no effect if falling mode enabled
             *
             * @param velocity velocity shouldn't be negative
             */
            void SetVelocity(float velocity);

            /* Sets BoardVehicle flag
             */
            void SetBoardVehicle();

            /* Sets ExitVehicle flag
             */
            void SetExitVehicle();

            /**
             * @brief
             *
             * @return PointsArray
             */
            PointsArray& Path() { return args.path; }

        protected:

            MoveSplineInitArgs args; /**< TODO */
            Unit&  unit; /**< TODO */
    };

    /**
     * @brief
     *
     */
    inline void MoveSplineInit::SetFly() { args.flags.EnableFlying();}
    /**
     * @brief
     *
     * @param enable
     */
    inline void MoveSplineInit::SetWalk(bool enable) { args.flags.walkmode = enable;}
    inline void MoveSplineInit::SetSmooth() { args.flags.EnableCatmullRom();}
    /**
     * @brief
     *
     */
    inline void MoveSplineInit::SetCyclic() { args.flags.cyclic = true;}
    /**
     * @brief
     *
     */
    inline void MoveSplineInit::SetFall() { args.flags.EnableFalling();}
    /**
     * @brief
     *
     * @param vel
     */
    inline void MoveSplineInit::SetVelocity(float vel) { args.velocity = vel;}
    inline void MoveSplineInit::SetOrientationInversed() { args.flags.orientationInversed = true;}
    inline void MoveSplineInit::SetOrientationFixed(bool enable) { args.flags.orientationFixed = enable;}
    inline void MoveSplineInit::SetBoardVehicle() { args.flags.EnableBoardVehicle(); }
    inline void MoveSplineInit::SetExitVehicle() { args.flags.EnableExitVehicle(); }

    /**
     * @brief
     *
     * @param controls
     * @param path_offset
     */
    inline void MoveSplineInit::MovebyPath(const PointsArray& controls, int32 path_offset)
    {
        args.path_Idx_offset = path_offset;
        args.path.assign(controls.begin(), controls.end());
    }

    /**
     * @brief
     *
     * @param x
     * @param y
     * @param z
     * @param generatePath
     * @param forceDestination
     */
    inline void MoveSplineInit::MoveTo(float x, float y, float z, bool generatePath, bool forceDestination)
    {
        Vector3 v(x, y, z);
        MoveTo(v, generatePath, forceDestination);
    }

    /**
     * @brief
     *
     * @param dest
     * @param generatePath
     * @param forceDestination
     */
    inline void MoveSplineInit::MoveTo(const Vector3& dest, bool generatePath, bool forceDestination)
    {
        if (generatePath)
        {
            PathFinder path(&unit);
            path.calculate(dest.x, dest.y, dest.z, forceDestination);
            MovebyPath(path.getPath());
        }
        else
        {
            args.path_Idx_offset = 0;
            args.path.resize(2);
            args.path[1] = dest;
        }
    }

    inline void MoveSplineInit::SetParabolic(float amplitude, float time_shift)
    {
        args.time_perc = time_shift;
        args.parabolic_amplitude = amplitude;
        args.flags.EnableParabolic();
    }

    inline void MoveSplineInit::SetAnimation(AnimType anim)
    {
        args.time_perc = 0.f;
        args.flags.EnableAnimation((uint8)anim);
    }

    /**
     * @brief
     *
     * @param spot
     */
    inline void MoveSplineInit::SetFacing(Vector3 const& spot)
    {
        args.facing.f.x = spot.x;
        args.facing.f.y = spot.y;
        args.facing.f.z = spot.z;
        args.flags.EnableFacingPoint();
    }
}

#endif // MANGOSSERVER_MOVESPLINEINIT_H
