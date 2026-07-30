/*
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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

/**
 * @file Vehicle.h
 * The functionality required by Vehicles -- the ONLY thing in this core that carries a
 * passenger by composing a transform.
 *
 * A vehicle is not a vessel and shares nothing with one. A tank really does carry its rider
 * across the map, so the rider really does have a world position and the server really does
 * own it: seat offset composed through the vehicle's own basis, every tick. A ship is the
 * opposite -- she IS a map, her passengers are on it, and nothing about them is ever
 * composed. The two used to share a base class; they had no business doing so.
 */

#ifndef MANGOSSERVER_VEHICLE_H
#define MANGOSSERVER_VEHICLE_H

#include <unordered_map>
#include "Platform/Define.h"
#include "Object.h"

#include <map>

class Unit;
class TransportInfo;
class VehicleInfo;

typedef std::unordered_map < WorldObject* /*passenger*/, TransportInfo* /*passengerInfo*/ > PassengerMap;

struct VehicleEntry;
struct VehicleSeatEntry;

struct VehicleAccessory
{
    uint32 vehicleEntry;
    uint32 seatId;
    uint32 passengerEntry;
};

typedef std::map<uint8 /*seatPosition*/, VehicleSeatEntry const*> VehicleSeatMap;

/**
 * @brief What a passenger of a VEHICLE carries: which vehicle, which seat, and where that
 *        seat is in the vehicle's own frame.
 *
 * Nothing aboard a ship has one of these, and nothing aboard a ship ever will: there is no
 * seat there, only a map.
 */
class TransportInfo
{
    public:
        explicit TransportInfo(WorldObject* owner, VehicleInfo* transport,
                               Geometry::Placement const& seatPose, uint8 seat);

        /// Writes the passenger's OWN placement too, so `Where()` on a rider is its seat
        /// pose rather than a stale world token.
        void SetSeatPose(Geometry::Placement const& seatPose);
        void SetTransportSeat(uint8 seat) { m_seat = seat; }

        WorldObject* GetTransport() const;
        ObjectGuid GetTransportGuid() const;

        // Required for chain-updating (passenger on vehicle on vehicle)
        bool IsOnVehicle() const;

        // Helper function if a passenger is already boarded somewhere onto the boarded transports
        bool HasOnBoard(WorldObject const* passenger) const;

        uint8 GetTransportSeat() const { return m_seat; }

        /// Where the passenger sits, in the vehicle's own frame.
        Geometry::Placement const& Seat() const { return m_seatPose; }

    private:
        WorldObject* m_owner;                               ///< Passenger
        VehicleInfo* m_transport;                           ///< Vehicle
        Geometry::Placement m_seatPose;
        uint8 m_seat;
};

/*
 * A class to provide support for each vehicle. This includes
 * - Boarding and unboarding of passengers, including support to switch vehicles
 * - Basic checks if a passenger can board
 */
class VehicleInfo
{
    public:
        explicit VehicleInfo(Unit* owner, VehicleEntry const* vehicleEntry, uint32 overwriteNpcEntry);
        ~VehicleInfo();

        WorldObject* GetOwner() const { return m_owner; }

        /// Bring a world point into this vehicle's frame. A vehicle composes both ways --
        /// legitimately, because the server owns its pose.
        Geometry::Placement SeatPoseOf(Geometry::Vector3 const& worldPoint, float worldFacing) const;

        /// The frame the seats are expressed in.
        Geometry::Frame SeatFrame() const;

        /// Is this unit boarded onto this vehicle (or onto a vehicle boarded onto it)?
        bool HasOnBoard(WorldObject const* passenger) const;

        void Initialize();                                  ///< Initializes the accessories
        bool IsInitialized() const { return m_isInitialized; }

        VehicleEntry const* GetVehicleEntry() const { return m_vehicleEntry; }
        VehicleSeatEntry const* GetSeatEntry(uint8 seat) const;

        void Board(Unit* passenger, uint8 seat);            // Board a passenger to a vehicle
        void SwitchSeat(Unit* passenger, uint8 seat);       // Used to switch seats of a passenger
        void UnBoard(Unit* passenger, bool changeVehicle);  // Used to Unboard a passenger from a vehicle

        bool CanBoard(Unit* passenger) const;               // Used to check if a Unit can board a vehicle
        Unit* GetPassenger(uint8 seat) const;

        void RemoveAccessoriesFromMap();                    ///< Unsummones accessory in case of far-teleport or death

        /// Drag the passengers' world positions along with the vehicle. ONLY VEHICLES
        /// COMPOSE, and this is the one place it happens.
        void Update(uint32 diff);
        void UpdateGlobalPositions();

    private:
        void BoardPassenger(WorldObject* passenger, Geometry::Placement const& seatPose, uint8 seat);
        void UnBoardPassenger(WorldObject* passenger);

        void UpdateGlobalPositionOf(WorldObject* passenger, float lx, float ly, float lz, float lo) const;
        void CalculateGlobalPositionOf(float lx, float ly, float lz, float lo, float& gx, float& gy, float& gz, float& go) const;

        // Internal use to calculate the boarding position
        void CalculateBoardingPositionOf(float gx, float gy, float gz, float go, float& lx, float& ly, float& lz, float& lo) const;

        // Seat information
        bool GetUsableSeatFor(Unit* passenger, uint8& seat) const;
        bool IsSeatAvailableFor(Unit* passenger, uint8 seat) const;

        uint8 GetTakenSeatsMask() const;
        uint8 GetEmptySeatsMask() const { return ~GetTakenSeatsMask(); }
        uint8 GetEmptySeats() const { return m_vehicleSeats.size() - m_passengers.size(); }

        bool IsUsableSeatForPlayer(uint32 seatFlags, uint32 seatFlagsB) const;
        bool IsUsableSeatForCreature(uint32 seatFlags) const { return true; } // special flag?, !IsUsableSeatForPlayer(seatFlags)?

        // Apply/ Remove Controlling of the vehicle
        void ApplySeatMods(Unit* passenger, uint32 seatFlags);
        void RemoveSeatMods(Unit* passenger, uint32 seatFlags);

        VehicleEntry const* m_vehicleEntry;
        VehicleSeatMap m_vehicleSeats;                      ///< Stores the available seats of the vehicle (filled in constructor)
        uint8 m_creatureSeats;                              ///< Mask that stores which seats are avaiable for creatures
        uint8 m_playerSeats;                                ///< Mask that stores which seats are avaiable for players

        uint32 m_overwriteNpcEntry;                         // Internal use to store the entry with which the vehicle-accessories are fetched
        bool m_isInitialized;                               // Internal use to store if the accessory is initialized
        GuidSet m_accessoryGuids;                           ///< Stores the summoned accessories of this vehicle

        WorldObject* m_owner;                               ///< The vehicle itself
        PassengerMap m_passengers;                          ///< Passengers and their seat information

        Geometry::Placement m_lastPose;                     ///< Pose the last global update ran at
        uint32 m_updatePositionsTimer;                      ///< Triggers the global position updates
};

#endif
