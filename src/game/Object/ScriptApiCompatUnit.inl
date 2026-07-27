// Unit's share of the old spatial API, for SD3 and Eluna only -- see ScriptApiCompat.inl.
// Combat reach became free functions beside Unit; these hand back the member spelling.

public:

    float GetCombatReach(Unit const* victim, bool forMeleeRange = true,
                         float flatMod = 0.0f) const
    {
        return victim ? CombatReachBetween(*this, *victim, forMeleeRange, flatMod) : 0.0f;
    }

    float GetCombatDistance(Unit const* target, bool forMeleeRange) const
    {
        return target ? CombatDistanceBetween(*this, *target, forMeleeRange) : 0.0f;
    }

    bool CanReachWithMeleeAttack(Unit const* victim, float flatMod = 0.0f) const
    {
        return victim && InMeleeReach(*this, *victim, flatMod);
    }
