// Creature's share of the old spatial API, for SD3 and Eluna only. The respawn pose and the
// leash point are placements now (Spawn(), CombatAnchor()); these are the old spellings.

public:

    void SetCombatStartPosition(float x, float y, float z)
    {
        SetCombatAnchor(Geometry::Vector3(x, y, z));
    }

    void GetCombatStartPosition(float& x, float& y, float& z) const
    {
        x = CombatAnchor().x;
        y = CombatAnchor().y;
        z = CombatAnchor().z;
    }

    void SetRespawnCoord(CreatureCreatePos const& pos) { SetSpawn(pos); }

    void SetRespawnCoord(float x, float y, float z, float ori)
    {
        SetSpawn(Geometry::Vector3(x, y, z), ori);
    }

    void ResetRespawnCoord() { ResetSpawn(); }

    void GetRespawnCoord(float& x, float& y, float& z, float* ori = NULL,
                         float* dist = NULL) const
    {
        x = Spawn().X();
        y = Spawn().Y();
        z = Spawn().Z();
        if (ori)
        {
            *ori = Spawn().Facing();
        }
        if (dist)
        {
            *dist = GetRespawnRadius();
        }
    }
