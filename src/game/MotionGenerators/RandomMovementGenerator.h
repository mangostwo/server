/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2026 MaNGOS <https://www.getmangos.eu>
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

#ifndef MANGOS_RANDOMMOTIONGENERATOR_H
#define MANGOS_RANDOMMOTIONGENERATOR_H

#include "IntentMovementGenerator.h"

class Creature;

/**
 * @brief Leashed wander: hop to a random reachable point within a radius of a fixed
 *        centre, rest a beat, repeat.
 *
 * The whole generator is that rhythm and nothing else. Picking the point is a question
 * for the mover's frame; routing and launching the leg is the driver's.
 */
class RandomMovementGenerator final : public IntentMovementGenerator
{
    public:
        /// Wander around the creature's own respawn point, at its DB wander distance.
        explicit RandomMovementGenerator(Creature const& creature);

        /**
         * @brief Wander around an explicit centre.
         * @param verticalZ Half-height of the vertical band the creature may roam, for
         *        a flier that should drift up and down as well as around. Zero keeps it
         *        on the ground.
         */
        RandomMovementGenerator(float x, float y, float z, float radius,
                                float verticalZ = 0.0f);

        void Initialize(Unit& owner) override;
        void Finalize(Unit& owner) override;
        void Interrupt(Unit& owner) override;
        void Reset(Unit& owner) override;

        MovementGeneratorType GetMovementGeneratorType() const override { return RANDOM_MOTION_TYPE; }

    protected:
        Motion::MoveIntent Intent(Unit& owner, Motion::MoveStatus const& status,
                                  uint32 diff) override;

    private:
        /// True when this wander roams a vertical band rather than the ground: it was
        /// given a band AND the creature can actually fly. A ground creature handed a
        /// verticalZ ignores it rather than levitating.
        bool Airborne(Unit& owner) const;

        /// The next point on the orbit. See the note in the .cpp: a circle in xy with a
        /// sinusoidal z of the SAME period is exactly an ellipse in an inclined plane, so
        /// advancing one angle smoothly is all a smooth 3D glide takes.
        Motion::Vector3 NextOrbitPoint(Unit& owner);

        Motion::Vector3 m_centre;    ///< What the wander is leashed to.
        float m_radius = 0.0f;       ///< How far from it the creature may stray.
        float m_verticalZ = 0.0f;    ///< Vertical half-band for a flying wander.

        /// Where we are around the orbit, and which way its plane is tilted. The angle
        /// only ever advances, so the creature never reverses into a zig-zag.
        float m_orbitAngle = 0.0f;
        float m_orbitTilt = 0.0f;

        TimeTracker m_restTime{0};   ///< Time left standing before the next hop.
        Motion::Vector3 m_hop;       ///< Where the current hop is heading.
        bool m_haveHop = false;      ///< False before the first point has been picked.
};

#endif // MANGOS_RANDOMMOTIONGENERATOR_H
