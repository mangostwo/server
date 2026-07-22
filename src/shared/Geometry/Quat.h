#pragma once

// Quaternion (not necessarily unit). Self-contained replacement for G3D::Quat, ported to
// the subset the game core used: game-object world rotation storage, quaternion
// composition (rotate a GO about Z), and unitize(). The multiply follows Watt & Watt
// exactly so results match the former g3dlite dependency. See [[project_g3d_removal]].
//
// Layout note: x,y,z,w must stay the first data members with no virtuals so imag()'s
// reinterpret_cast onto a Vector3 is valid.

#include "Geometry/GeometryMath.h"
#include "Geometry/Vector3.h"

#include <cmath>

namespace Geometry
{
    class Quat
    {
        public:
            /// q = [sin(angle/2) * axis, cos(angle/2)]; (x,y,z) imaginary, w real.
            float x, y, z, w;

            /// Identity rotation (0, 0, 0, 1).
            Quat() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
            Quat(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}

            /// Defaults to a pure-vector quaternion.
            Quat(const Vector3& v, float _w = 0.0f) : x(v.x), y(v.y), z(v.z), w(_w) {}

            const float& operator[](int i) const { return reinterpret_cast<const float*>(this)[i]; }
            float& operator[](int i) { return reinterpret_cast<float*>(this)[i]; }

            /// The imaginary part (x, y, z) aliased as a Vector3.
            const Vector3& imag() const { return *reinterpret_cast<const Vector3*>(this); }
            Vector3& imag() { return *reinterpret_cast<Vector3*>(this); }

            const float& real() const { return w; }
            float& real() { return w; }

            Quat operator-() const { return Quat(-x, -y, -z, -w); }
            Quat conj() const { return Quat(-x, -y, -z, w); }

            Quat operator*(float s) const { return Quat(x * s, y * s, z * s, w * s); }
            Quat operator/(float s) const { return Quat(x / s, y / s, z / s, w / s); }
            Quat& operator*=(float s) { x *= s; y *= s; z *= s; w *= s; return *this; }

            float dot(const Quat& other) const
            {
                return (x * other.x) + (y * other.y) + (z * other.z) + (w * other.w);
            }

            /// Quaternion multiplication (composition of rotations); does not commute.
            /// @cite Watt & Watt, page 360.
            Quat operator*(const Quat& other) const
            {
                const Vector3& v1 = imag();
                const Vector3& v2 = other.imag();
                float s1 = w;
                float s2 = other.w;

                return Quat(s1 * v2 + s2 * v1 + v1.cross(v2), s1 * s2 - v1.dot(v2));
            }

            /// Make unit length in place.
            void unitize() { *this *= rsq(dot(*this)); }

            float magnitude() const { return std::sqrt(dot(*this)); }

            bool isUnit(float tolerance = 1e-5f) const { return std::fabs(dot(*this) - 1.0f) < tolerance; }
    };

    inline Quat operator*(float s, const Quat& q) { return q * s; }
}
