#pragma once

// Scalar math helpers for the game-core geometry types (Vector2/3/4, Matrix4, Quat).
//
// These are direct ports of the handful of G3D::g3dmath functions the server used
// (pi/twoPi/halfPi, wrap, rsq, iFloor, fuzzyEq/fuzzyNe, isFinite). Values and formulas
// are reproduced verbatim so movement/rotation math stays numerically identical to the
// former g3dlite dependency. See [[project_g3d_removal]].

#include <cmath>
#include <limits>

namespace Geometry
{
    inline double pi()     { return 3.1415926535898; }
    inline float  pif()    { return 3.1415926535898f; }
    inline double halfPi() { return 1.57079633; }
    inline double twoPi()  { return 6.28318531; }

    /// 1 / sqrt(x)
    inline float rsq(float x)
    {
        return 1.0f / std::sqrt(x);
    }

    inline int iFloor(double value)
    {
        return int(std::floor(value));
    }

    inline bool isFinite(double x)
    {
        return std::isfinite(x);
    }

    /// Magnitude-scaled comparison epsilon (G3D fuzzyEpsilon == 1e-5f).
    inline double eps(double a, double b)
    {
        (void)b;
        const double aa = std::fabs(a) + 1.0;
        if (aa == std::numeric_limits<double>::infinity())
        {
            return 0.00001f;
        }
        else
        {
            return 0.00001f * aa;
        }
    }

    inline bool fuzzyEq(double a, double b)
    {
        return (a == b) || (std::fabs(a - b) <= eps(a, b));
    }

    inline bool fuzzyNe(double a, double b)
    {
        return !fuzzyEq(a, b);
    }

    /// Wraps t into the half-open interval [lo, hi).
    ///
    /// Non-finite input and a non-positive interval fail closed to lo, and the
    /// quotient is floored in double rather than through iFloor. This is reachable
    /// from a client packet: MoveSpline normalizes the orientation the client
    /// reports, so a NaN or a value beyond 2^31 intervals from lo used to convert a
    /// double to int out of range -- undefined behaviour, not a wrong angle. Inside
    /// the range every caller actually uses, the value is unchanged.
    inline float wrap(float t, float lo, float hi)
    {
        const float interval = hi - lo;
        if (!(interval > 0.0f) || !isFinite(t))
        {
            return lo;
        }

        if ((t >= lo) && (t < hi))
        {
            return t;
        }

        const double turns = std::floor((double(t) - double(lo)) / double(interval));
        return float(double(t) - double(interval) * turns);
    }
}
