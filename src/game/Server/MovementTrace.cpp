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

#include "MovementTrace.h"

#include "Config/Config.h"
#include "Creature.h"
#include "GameTime.h"
#include "Log.h"
#include "Map.h"
#include "MotionMaster.h"
#include "movement/MoveSpline.h"
#include "OpcodeTable.h"
#include "Player.h"
#include "Transports.h"
#include "TransportMap.h"
#include "WorldSession.h"

#include <cstdio>
#include <mutex>

namespace
{
    std::mutex g_mutex;
    FILE* g_file = NULL;
    std::string g_path;

    FILE* g_deckFile = NULL;
    std::string g_deckPath;
    uint32 g_deckMapId = 0;

    struct FlagName
    {
        uint32 flag;
        char const* name;
    };

    const FlagName MOVE_FLAG_NAMES[] =
    {
        { MOVEFLAG_FORWARD,                 "FWD"          },
        { MOVEFLAG_BACKWARD,                "BACK"         },
        { MOVEFLAG_STRAFE_LEFT,             "STRAFE_L"     },
        { MOVEFLAG_STRAFE_RIGHT,            "STRAFE_R"     },
        { MOVEFLAG_TURN_LEFT,               "TURN_L"       },
        { MOVEFLAG_TURN_RIGHT,              "TURN_R"       },
        { MOVEFLAG_PITCH_UP,                "PITCH_UP"     },
        { MOVEFLAG_PITCH_DOWN,              "PITCH_DN"     },
        { MOVEFLAG_WALK_MODE,               "WALK"         },
        { MOVEFLAG_ONTRANSPORT,             "ONTRANSPORT"  },
        { MOVEFLAG_LEVITATING,              "LEVITATE"     },
        { MOVEFLAG_ROOT,                    "ROOT"         },
        { MOVEFLAG_FALLING,                 "FALLING"      },
        { MOVEFLAG_FALLINGFAR,              "FALLFAR"      },
        { MOVEFLAG_PENDINGSTOP,             "PEND_STOP"    },
        { MOVEFLAG_PENDINGSTRAFESTOP,       "PEND_SSTOP"   },
        { MOVEFLAG_PENDINGFORWARD,          "PEND_FWD"     },
        { MOVEFLAG_PENDINGBACKWARD,         "PEND_BACK"    },
        { MOVEFLAG_PENDINGSTRAFELEFT,       "PEND_SL"      },
        { MOVEFLAG_PENDINGSTRAFERIGHT,      "PEND_SR"      },
        { MOVEFLAG_PENDINGROOT,             "PEND_ROOT"    },
        { MOVEFLAG_SWIMMING,                "SWIM"         },
        { MOVEFLAG_ASCENDING,               "ASCEND"       },
        { MOVEFLAG_DESCENDING,              "DESCEND"      },
        { MOVEFLAG_CAN_FLY,                 "CAN_FLY"      },
        { MOVEFLAG_FLYING,                  "FLYING"       },
        { MOVEFLAG_SPLINE_ELEVATION,        "SPL_ELEV"     },
        { MOVEFLAG_SPLINE_ENABLED,          "SPL_ON"       },
        { MOVEFLAG_WATERWALKING,            "WATERWALK"    },
        { MOVEFLAG_SAFE_FALL,               "SAFE_FALL"    },
        { MOVEFLAG_HOVER,                   "HOVER"        },
    };

    const FlagName MOVE_FLAG2_NAMES[] =
    {
        { MOVEFLAG2_NO_STRAFE,              "NO_STRAFE"    },
        { MOVEFLAG2_NO_JUMPING,             "NO_JUMP"      },
        { MOVEFLAG2_UNK3,                   "UNK3"         },
        { MOVEFLAG2_FULLSPEEDTURNING,       "FULLTURN"     },
        { MOVEFLAG2_FULLSPEEDPITCHING,      "FULLPITCH"    },
        { MOVEFLAG2_ALLOW_PITCHING,         "ALLOW_PITCH"  },
        { MOVEFLAG2_UNK4,                   "UNK4"         },
        { MOVEFLAG2_UNK5,                   "UNK5"         },
        { MOVEFLAG2_UNK6,                   "UNK6_TRANSP"  },
        { MOVEFLAG2_UNK7,                   "UNK7"         },
        { MOVEFLAG2_INTERP_MOVEMENT,        "INTERP_MOVE"  },
        { MOVEFLAG2_INTERP_TURNING,         "INTERP_TURN"  },
        { MOVEFLAG2_INTERP_PITCHING,        "INTERP_PITCH" },
        { MOVEFLAG2_UNK8,                   "UNK8"         },
        { MOVEFLAG2_UNK9,                   "UNK9"         },
        { MOVEFLAG2_UNK10,                  "UNK10"        },
    };

    template<size_t N>
    std::string Decode(uint32 value, FlagName const (&table)[N])
    {
        std::string out;
        for (size_t i = 0; i < N; ++i)
        {
            if (value & table[i].flag)
            {
                if (!out.empty())
                {
                    out += '|';
                }
                out += table[i].name;
            }
        }
        return out.empty() ? std::string("-") : out;
    }

    /// The vessel this session's player rides, by the two routes there are: the pointer
    /// the boarding path set, or -- before it is set -- the guid the packet itself names.
    Transport const* VesselOf(Player const* player, MovementInfo const& mi)
    {
        if (!player)
        {
            return NULL;
        }

        if (Transport const* known = player->GetTransport())
        {
            return known;
        }

        if (!player->IsInWorld() || !mi.GetTransportGuid())
        {
            return NULL;
        }

        return Transport::GetTransport(player->GetMap(), mi.GetTransportGuid());
    }

    char const* const TRACE_HEADER =
        "recv_ms,stage,event,acct,player,guid,opcode,opcode_id,bytes,latency,"
        "cli_time,map,flags,flags2,flag_names,flag2_names,"
        "own_x,own_y,own_z,has_transport,"
        "wx,wy,wz,wo,"
        "t_guid,t_entry,tx,ty,tz,to,t_time,t_seat,t_time2,"
        "fall_ms,pitch,jump_vel,jump_sin,jump_cos,jump_xy,"
        "v_entry,v_prog,v_map,vx,vy,vz,vo,detail\n";

    char const* const DECK_HEADER =
        "recv_ms,map,guid,entry,name,x,y,z,o,dx,dy,dz,"
        "movegen,spline_active,alive,in_combat,"
        "v_entry,v_prog,vx,vy,vz,vo\n";

    /// Caller holds g_mutex.
    bool Open(FILE*& file, std::string& path, char const* configKey,
              char const* fallback, char const* header)
    {
        const std::string name = sConfig.GetStringDefault(configKey, fallback);
        if (name.empty())
        {
            return false;
        }

        std::string dir = sConfig.GetStringDefault("LogsDir", "");
        if (!dir.empty() && dir.back() != '/' && dir.back() != '\\')
        {
            dir += '/';
        }

        path = dir + name;

        bool fresh = true;
        if (FILE* probe = fopen(path.c_str(), "r"))
        {
            fseek(probe, 0, SEEK_END);
            fresh = ftell(probe) == 0;
            fclose(probe);
        }

        file = fopen(path.c_str(), "a");
        if (!file)
        {
            sLog.outError("MovementTrace: cannot open %s", path.c_str());
            path.clear();
            return false;
        }

        setvbuf(file, NULL, _IOFBF, 64 * 1024);
        if (fresh)
        {
            fputs(header, file);
        }
        return true;
    }

    /// Caller holds g_mutex.
    void Close(FILE*& file, std::string& path)
    {
        if (file)
        {
            fclose(file);
            file = NULL;
        }
        path.clear();
    }

    /**
     * @brief The one and only row writer.
     *
     * Both a packet and a bare state change go through here, so the column count cannot
     * drift between them -- a second format string is exactly the duplicated truth that
     * makes a CSV unparseable three months later.
     */
    void Row(WorldSession const* session, char const* stage, char const* event,
             uint16 opcode, MovementInfo const& mi, size_t bytes, char const* detail)
    {
        Player const* player = session ? session->GetPlayer() : NULL;
        Position const* t = mi.GetTransportPos();
        MovementInfo::JumpInfo const& jump = mi.GetJumpInfo();
        Transport const* vessel = VesselOf(player, mi);

        const std::string flags = Decode(mi.GetMovementFlags(), MOVE_FLAG_NAMES);
        const std::string flags2 = Decode(mi.GetMovementFlags2(), MOVE_FLAG2_NAMES);

        std::lock_guard<std::mutex> lock(g_mutex);
        if (!g_file)
        {
            return;
        }

        fprintf(g_file,
                "%u,%s,%s,%u,%s,%u,%s,%u,%u,%u,"
                "%u,%u,0x%08X,0x%04X,%s,%s,"
                "%.4f,%.4f,%.4f,%u,"
                "%.4f,%.4f,%.4f,%.4f,"
                "%u,%u,%.4f,%.4f,%.4f,%.4f,%u,%d,%u,"
                "%u,%.4f,%.4f,%.4f,%.4f,%.4f,"
                "%u,%u,%u,%.4f,%.4f,%.4f,%.4f,%s\n",
                GameTime::GetGameTimeMS(), stage, event,
                session ? session->GetAccountId() : 0,
                player ? player->GetName() : "-",
                player ? player->GetGUIDLow() : 0,
                opcode ? LookupOpcodeName(opcode) : "-", uint32(opcode), uint32(bytes),
                session ? session->GetLatency() : 0,
                mi.GetTime(), player ? player->GetMapId() : 0,
                uint32(mi.GetMovementFlags()), uint32(mi.GetMovementFlags2()),
                flags.c_str(), flags2.c_str(),
                // What the SERVER holds, beside what the client claims. On a deck map the
                // two must be different frames; when own_* reads like wx/wy/wz, the
                // placement has been written in the wrong one and every command that
                // reads Where() has already inherited it.
                player ? player->Where().X() : 0.0f,
                player ? player->Where().Y() : 0.0f,
                player ? player->Where().Z() : 0.0f,
                uint32(player && player->GetTransport() ? 1 : 0),
                mi.Reported().X(), mi.Reported().Y(), mi.Reported().Z(),
                mi.Reported().Facing(),
                mi.GetTransportGuid().GetCounter(), mi.GetTransportGuid().GetEntry(),
                t->x, t->y, t->z, t->o,
                mi.GetTransportTime(), int32(mi.GetTransportSeat()),
                mi.GetTransportTime2(),
                mi.GetFallTime(), mi.GetPitch(),
                jump.velocity, jump.sinAngle, jump.cosAngle, jump.xyspeed,
                vessel ? vessel->GetEntry() : 0,
                vessel ? vessel->GetPathProgress() : 0,
                vessel ? vessel->VesselMapId() : 0,
                vessel ? vessel->Where().X() : 0.0f,
                vessel ? vessel->Where().Y() : 0.0f,
                vessel ? vessel->Where().Z() : 0.0f,
                vessel ? vessel->Where().Facing() : 0.0f,
                detail);
    }
}

namespace MovementTrace
{
    bool Enabled()
    {
        return g_file != NULL;
    }

    std::string const& FileName()
    {
        return g_path;
    }

    std::string const& DeckFileName()
    {
        return g_deckPath;
    }

    uint32 DeckMapId()
    {
        return g_deckMapId;
    }

    bool SetEnabled(bool on, uint32 deckMapId)
    {
        std::lock_guard<std::mutex> lock(g_mutex);

        if (!on)
        {
            Close(g_file, g_path);
            Close(g_deckFile, g_deckPath);
            g_deckMapId = 0;
            return true;
        }

        if (g_file)
        {
            return true;
        }

        if (!Open(g_file, g_path, "MovementTrace.File", "MovementTrace.csv",
                  TRACE_HEADER))
        {
            return false;
        }

        // The crew file is opened only when a deck is named: without one the hook has
        // nothing to select on, and every relocation on the server would land here.
        if (deckMapId)
        {
            if (Open(g_deckFile, g_deckPath, "MovementTrace.DeckFile",
                     "MovementTraceDeck.csv", DECK_HEADER))
            {
                g_deckMapId = deckMapId;
            }
        }

        return true;
    }

    void Initialize()
    {
        if (sConfig.GetBoolDefault("MovementTrace.Enabled", false))
        {
            SetEnabled(true, sConfig.GetIntDefault("MovementTrace.DeckMap", 0));
        }
    }

    void Packet(WorldSession const* session, uint16 opcode, MovementInfo const& mi,
                char const* stage, size_t bytes)
    {
        if (!g_file)
        {
            return;
        }

        Row(session, stage, "", opcode, mi, bytes, "");
    }

    void Event(WorldSession const* session, char const* event, char const* detail)
    {
        if (!g_file)
        {
            return;
        }

        // The session's own last-known state is the right body for an event row: it is
        // what the event happened TO, and it makes the row diffable against the packets
        // on either side of it.
        Player const* player = session ? session->GetPlayer() : NULL;
        static const MovementInfo empty;

        Row(session, "event", event, 0, player ? player->m_movementInfo : empty, 0,
            detail ? detail : "");

        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_file)
        {
            fflush(g_file);
        }
    }

    void DeckMove(Map* on, Creature* creature, float x, float y, float z, float o)
    {
        if (!g_deckFile || !creature)
        {
            return;
        }

        Geometry::Placement const& from = creature->Where();

        // The map comes from the caller. GetMap() ASSERTS on a mapless object rather than
        // returning NULL, so testing its result is not a guard -- it is the crash.
        Transport const* vessel = NULL;
        if (on && on->AsTransport())
        {
            vessel = on->AsTransport()->Vessel();
        }

        std::lock_guard<std::mutex> lock(g_mutex);
        if (!g_deckFile)
        {
            return;
        }

        fprintf(g_deckFile,
                "%u,%u,%u,%u,%s,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,"
                "%u,%u,%u,%u,"
                "%u,%u,%.4f,%.4f,%.4f,%.4f\n",
                GameTime::GetGameTimeMS(), creature->GetMapId(),
                creature->GetGUIDLow(), creature->GetEntry(), creature->GetName(),
                x, y, z, o,
                x - from.X(), y - from.Y(), z - from.Z(),
                uint32(creature->GetMotionMaster()->GetCurrentMovementGeneratorType()),
                uint32(!creature->movespline->Finalized()),
                uint32(creature->IsAlive()), uint32(creature->IsInCombat()),
                vessel ? vessel->GetEntry() : 0,
                vessel ? vessel->GetPathProgress() : 0,
                vessel ? vessel->Where().X() : 0.0f,
                vessel ? vessel->Where().Y() : 0.0f,
                vessel ? vessel->Where().Z() : 0.0f,
                vessel ? vessel->Where().Facing() : 0.0f);
    }
}
