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

#ifndef MANGOS_WAYPOINTMOVEMENTGENERATOR_H
#define MANGOS_WAYPOINTMOVEMENTGENERATOR_H

/** @page PathMovementGenerator is used to generate movements
 * of waypoints and flight paths.  Each serves the purpose
 * of generate activities so that it generates updated
 * packets for the players.
 */

#include <sstream>
#include "Common/TimeConstants.h"
#include "IntentMovementGenerator.h"
#include "MovementGenerator.h"
#include "WaypointManager.h"
#include "DBCStructure.h"
#include "WaypointSmoothing.h"
#include "movement/MoveSplineInitArgs.h"

#include <vector>
#include <set>

using Movement::PointsArray;

#define FLIGHT_TRAVEL_UPDATE  100
#define STOP_TIME_FOR_PLAYER  (3 * MINUTE * IN_MILLISECONDS)// 3 Minutes

/**
 * @brief Base class for path movement generators
 *
 * Provides common functionality for path-based movement.
 *
 * @tparam T Type of the unit (Player or Creature)
 * @tparam P Type of the path
 */
template<class T, class P>
class PathMovementBase
{
    public:
        /**
         * @brief Constructor
         */
        PathMovementBase() : i_path(nullptr), i_currentNode(0) {}

        /**
         * @brief Virtual destructor
         */
        virtual ~PathMovementBase() {};

        /**
         * @brief Load path for the unit
         * @param unit Reference to the unit
         */
        void LoadPath(T&);

        /**
         * @brief Get current node in the path
         * @return Current node index
         */
        uint32 GetCurrentNode() const { return i_currentNode; }

    protected:
        P i_path; ///< Path for the movement
        uint32 i_currentNode; ///< Current node in the path
};

/**
 * @brief Patrol: walk a list of waypoints, pausing, emoting and running scripts at the
 *        ones that say to.
 *
 * This is the one movement kind that must dictate the EXACT geometry of its leg rather
 * than name a destination and let the driver route to it: a smoothed segment welds
 * several waypoint legs into a single spline so the creature does not visibly stop and
 * relaunch at every node. It therefore hands the driver its own points on the intent.
 */
class WaypointMovementGenerator final : public IntentMovementGenerator
{
    public:
        explicit WaypointMovementGenerator(Creature&) {}

        void Initialize(Unit& owner) override;
        void Finalize(Unit& owner) override;
        void Interrupt(Unit& owner) override;
        void Reset(Unit& owner) override;

        MovementGeneratorType GetMovementGeneratorType() const override { return WAYPOINT_MOTION_TYPE; }

        bool GetResetPosition(Unit& owner, float& x, float& y, float& z, float& o) const override;

        /// Load a path and start walking it after `initialDelay` ms.
        void InitializeWaypointPath(Unit& owner, int32 pathId, WaypointPathOrigin wpSource,
                                    uint32 initialDelay, uint32 overwriteEntry);

        uint32 getLastReachedWaypoint() const { return m_lastReachedWaypoint; }

        void GetPathInformation(int32& pathId, WaypointPathOrigin& wpOrigin) const
        {
            pathId = m_pathId;
            wpOrigin = m_pathOrigin;
        }

        void GetPathInformation(std::ostringstream& oss) const;

        /// Extend (or cut short) the pause at the current node.
        void AddToWaypointPauseTime(int32 waitTimeDiff);

        /// Jump the patrol to a given node; it moves on the next tick.
        bool SetNextWaypoint(uint32 pointId);

    protected:
        Motion::MoveIntent Intent(Unit& owner, Motion::MoveStatus const& status,
                                  uint32 diff) override;

    private:
        /// A waypoint reached inside an active smoothed segment.
        struct SegmentWaypoint
        {
            uint32 pointId;        ///< Waypoint id in the path.
            size_t pathPointIndex; ///< Index of its endpoint within the spline points.
        };

        void LoadPath(Creature& creature, int32 pathId, WaypointPathOrigin wpOrigin,
                      uint32 overwriteEntry);

        /// Advance to the next waypoint and prepare the leg that reaches it. Still fires
        /// the AI informs and applies the node's model change, and still builds the
        /// smoothed geometry -- but hands that to the driver as an intent rather than
        /// pushing a spline itself.
        Motion::MoveIntent PrepareMove(Creature& creature);

        /// The intent that walks the leg PrepareMove built.
        Motion::MoveIntent WalkPreparedLeg() const;

        /// Everything that happens on getting to a node: scripts, emotes, AI informs, and
        /// the pause the node asks for.
        void OnArrived(Creature& creature);

        /// Fire arrival handling for any smoothed waypoints the spline has now passed.
        void ProcessSegmentProgress(Creature& creature, int32 pathIndex);

        /// Weld as many upcoming legs as will fit into one spline. Leaves m_legPoints
        /// empty when the segment cannot be smoothed, and the driver then routes a plain
        /// leg to the next node instead.
        void BuildSmoothPath(Creature& creature, WaypointPath::const_iterator startPoint);

        bool Stopped(Unit const& owner) const;
        bool CanMove(Unit const& owner, uint32 diff);
        void Stop(int32 time) { m_nextMoveTime.Reset(time); }

        void ClearSegment()
        {
            m_segment.clear();
            m_segmentArrivals = 0;
        }

        WaypointPath const* m_path = nullptr;
        uint32 m_currentNode = 0;
        uint32 m_lastReachedWaypoint = 0;
        int32 m_pathId = 0;
        WaypointPathOrigin m_pathOrigin = PATH_NO_PATH;

        TimeTracker m_nextMoveTime{0};
        bool m_isArrivalDone = false;

        std::vector<SegmentWaypoint> m_segment; ///< Waypoints inside the smoothed leg.
        size_t m_segmentArrivals = 0;           ///< How many of them we have passed.

        /// The leg PrepareMove built. Empty points mean "not smoothed -- route to m_legEnd
        /// instead". The driver holds a pointer to these while the leg is in flight, so
        /// they must not be rebuilt until the leg ends.
        Movement::PointsArray m_legPoints;
        Motion::Vector3 m_legEnd;
        Motion::Facing m_legFacing;
        bool m_legWalk = true; ///< Pace of the leg; false only for a DB-flagged runner.
        bool m_haveLeg = false;
};

/**
 * @brief Flight path movement generator for players
 *
 * Generates movement of the player along taxi flight paths.
 * Handles ground and activities for the player during flight.
 */
class FlightPathMovementGenerator
    : public MovementGeneratorMedium< Player, FlightPathMovementGenerator >,
  public PathMovementBase<Player, TaxiPathNodeList const*>
{
    public:
        /**
         * @brief Constructor
         * @param pathnodes Reference to path nodes
         * @param startNode Starting node index
         */
        explicit FlightPathMovementGenerator(TaxiPathNodeList const& pathnodes, uint32 startNode = 0)
        {
            i_path = &pathnodes;
            i_currentNode = startNode;
        }

        /**
         * @brief Initialize the movement generator
         * @param player Reference to the player
         */
        void Initialize(Player&);

        /**
         * @brief Finalize the movement generator
         * @param player Reference to the player
         */
        void Finalize(Player&);

        /**
         * @brief Interrupt the movement generator
         * @param player Reference to the player
         */
        void Interrupt(Player&);

        /**
         * @brief Reset the movement generator
         * @param player Reference to the player
         */
        void Reset(Player&);

        /**
         * @brief Update the movement generator
         * @param player Reference to the player
         * @param diff Time difference in milliseconds
         * @return True if update successful
         */
        bool Update(Player&, const uint32&);

        /**
         * @brief Get movement generator type
         * @return FLIGHT_MOTION_TYPE
         */
        MovementGeneratorType GetMovementGeneratorType() const override { return FLIGHT_MOTION_TYPE; }

        /**
         * @brief Get the flight path
         * @return Reference to path nodes
         */
        TaxiPathNodeList const& GetPath() { return *i_path; }

        /**
         * @brief Get node index at map end
         * @return Node index at map end
         */
        uint32 GetPathAtMapEnd() const;

        /**
         * @brief Check if player has arrived at destination
         * @return True if arrived
         */
        bool HasArrived() const { return (i_currentNode >= i_path->size()); }

        /**
         * @brief Set current node after teleport
         */
        void SetCurrentNodeAfterTeleport();

        /**
         * @brief Skip current node
         */
        void SkipCurrentNode() { ++i_currentNode; }
        void DoEventIfAny(Player& player, TaxiPathNodeEntry const& node, bool departure);

        /**
         * @brief Get reset position for evade
         * @param player Reference to the player
         * @param x X-coordinate output
         * @param y Y-coordinate output
         * @param z Z-coordinate output
         * @param o Orientation output
         * @return True if reset position obtained
         */
        bool GetResetPosition(Player&, float& /*x*/, float& /*y*/, float& /*z*/, float& /*o*/) const;
};

#endif // MANGOS_WAYPOINTMOVEMENTGENERATOR_H
