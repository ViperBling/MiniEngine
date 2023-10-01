#pragma once

#include <cassert>
#include <cmath>

namespace MiniEngine
{
    class Vector2
    {
    public:
        Vector2() = default;
        Vector2(float _x, float _y) : x(_x), y(_y) {};
        explicit Vector2(float scala) : x(scala), y(scala) {}
        explicit Vector2(const float v[2]) : x(v[0]), y(v[1]) {}
        explicit Vector2(float* const r) : x(r[0]), y(r[1]) {}

        float operator[](size_t i) const
        {
            assert(i < 2);
            return (i == 0 ? x : y);
        }
        float& operator[](size_t i)
        {
            assert(i < 2);
            return (i == 0 ? x : y);
        }
        bool operator==(const Vector2& rhs) const { return (x == rhs.x && y == rhs.y); }
        bool operator!=(const Vector2& rhs) const { return (x != rhs.x || y != rhs.y); }

        // arithmetic operations
        Vector2 operator+(const Vector2& rhs) const { return Vector2(x + rhs.x, y + rhs.y); }
        Vector2 operator-(const Vector2& rhs) const { return Vector2(x - rhs.x, y - rhs.y); }
        Vector2 operator*(float scalar) const { return Vector2(x * scalar, y * scalar); }
        Vector2 operator*(const Vector2& rhs) const { return Vector2(x * rhs.x, y * rhs.y); }
        Vector2 operator/(float scale) const
        {
            assert(scale != 0.0);
            float inv = 1.0f / scale;
            return Vector2(x * inv, y * inv);
        }
        Vector2        operator/(const Vector2& rhs) const { return Vector2(x / rhs.x, y / rhs.y); }
        const Vector2& operator+() const { return *this; }
        Vector2        operator-() const { return Vector2(-x, -y); }

        // overloaded operators to help Vector2
        friend Vector2 operator*(float scalar, const Vector2& rhs) { return Vector2(scalar * rhs.x, scalar * rhs.y); }
        friend Vector2 operator/(float fScalar, const Vector2& rhs)
        {
            return Vector2(fScalar / rhs.x, fScalar / rhs.y);
        }
        friend Vector2 operator+(const Vector2& lhs, float rhs) { return Vector2(lhs.x + rhs, lhs.y + rhs); }
        friend Vector2 operator+(float lhs, const Vector2& rhs) { return Vector2(lhs + rhs.x, lhs + rhs.y); }
        friend Vector2 operator-(const Vector2& lhs, float rhs) { return Vector2(lhs.x - rhs, lhs.y - rhs); }
        friend Vector2 operator-(float lhs, const Vector2& rhs) { return Vector2(lhs - rhs.x, lhs - rhs.y); }

        // arithmetic updates
        Vector2& operator+=(const Vector2& rhs)
        {
            x += rhs.x;
            y += rhs.y;

            return *this;
        }

        Vector2& operator+=(float scalar)
        {
            x += scalar, y += scalar;
            return *this;
        }
        Vector2& operator-=(const Vector2& rhs)
        {
            x -= rhs.x, y -= rhs.y;
            return *this;
        }
        Vector2& operator-=(float scalar)
        {
            x -= scalar, y -= scalar;
            return *this;
        }
        Vector2& operator*=(float scalar)
        {
            x *= scalar, y *= scalar;
            return *this;
        }
        Vector2& operator*=(const Vector2& rhs)
        {
            x *= rhs.x, y *= rhs.y;
            return *this;
        }
        Vector2& operator/=(float scalar)
        {
            assert(scalar != 0.0);
            float inv = 1.0f / scalar;
            x *= inv, y *= inv;
            return *this;
        }
        Vector2& operator/=(const Vector2& rhs)
        {
            x /= rhs.x, y /= rhs.y;
            return *this;
        }

    public:
        float x = 0.0f;
        float y = 0.0f;
    };


    class Vector3
    {
    public:
        Vector3() = default;
        Vector3(float x_, float y_, float z_) : x {x_}, y {y_}, z {z_} {};
        explicit Vector3(const float coords[3]) : x {coords[0]}, y {coords[1]}, z {coords[2]} {}

        float operator[](size_t i) const
        {
            assert(i < 3);
            return *(&x + i);
        }
        float& operator[](size_t i)
        {
            assert(i < 3);
            return *(&x + i);
        }

        bool operator==(const Vector3& rhs) const { return (x == rhs.x && y == rhs.y && z == rhs.z); }
        bool operator!=(const Vector3& rhs) const { return x != rhs.x || y != rhs.y || z != rhs.z; }

        // arithmetic operations
        Vector3 operator+(const Vector3& rhs) const { return Vector3(x + rhs.x, y + rhs.y, z + rhs.z); }
        Vector3 operator-(const Vector3& rhs) const { return Vector3(x - rhs.x, y - rhs.y, z - rhs.z); }
        Vector3 operator*(float scalar) const { return Vector3(x * scalar, y * scalar, z * scalar); }
        Vector3 operator*(const Vector3& rhs) const { return Vector3(x * rhs.x, y * rhs.y, z * rhs.z); }
        Vector3 operator/(float scalar) const
        {
            assert(scalar != 0.0);
            return Vector3(x / scalar, y / scalar, z / scalar);
        }
        Vector3 operator/(const Vector3& rhs) const
        {
            assert((rhs.x != 0 && rhs.y != 0 && rhs.z != 0));
            return Vector3(x / rhs.x, y / rhs.y, z / rhs.z);
        }
        const Vector3& operator+() const { return *this; }
        Vector3        operator-() const { return Vector3(-x, -y, -z); }

        // overloaded operators to help Vector3
        friend Vector3 operator*(float scalar, const Vector3& rhs)
        {
            return Vector3(scalar * rhs.x, scalar * rhs.y, scalar * rhs.z);
        }
        friend Vector3 operator/(float scalar, const Vector3& rhs)
        {
            assert(rhs.x != 0 && rhs.y != 0 && rhs.z != 0);
            return Vector3(scalar / rhs.x, scalar / rhs.y, scalar / rhs.z);
        }
        friend Vector3 operator+(const Vector3& lhs, float rhs)
        {
            return Vector3(lhs.x + rhs, lhs.y + rhs, lhs.z + rhs);
        }
        friend Vector3 operator+(float lhs, const Vector3& rhs)
        {
            return Vector3(lhs + rhs.x, lhs + rhs.y, lhs + rhs.z);
        }
        friend Vector3 operator-(const Vector3& lhs, float rhs)
        {
            return Vector3(lhs.x - rhs, lhs.y - rhs, lhs.z - rhs);
        }
        friend Vector3 operator-(float lhs, const Vector3& rhs)
        {
            return Vector3(lhs - rhs.x, lhs - rhs.y, lhs - rhs.z);
        }

        // arithmetic updates
        Vector3& operator+=(const Vector3& rhs)
        {
            x += rhs.x, y += rhs.y, z += rhs.z;
            return *this;
        }
        Vector3& operator+=(float scalar)
        {
            x += scalar, y += scalar, z += scalar;
            return *this;
        }
        Vector3& operator-=(const Vector3& rhs)
        {
            x -= rhs.x, y -= rhs.y, z -= rhs.z;
            return *this;
        }
        Vector3& operator-=(float scalar)
        {
            x -= scalar, y -= scalar, z -= scalar;
            return *this;
        }
        Vector3& operator*=(float scalar)
        {
            x *= scalar, y *= scalar, z *= scalar;
            return *this;
        }
        Vector3& operator*=(const Vector3& rhs)
        {
            x *= rhs.x, y *= rhs.y, z *= rhs.z;
            return *this;
        }
        Vector3& operator/=(float scalar)
        {
            assert(scalar != 0.0);
            x /= scalar, y /= scalar, z /= scalar;
            return *this;
        }
        Vector3& operator/=(const Vector3& rhs)
        {
            assert(rhs.x != 0 && rhs.y != 0 && rhs.z != 0);
            x /= rhs.x, y /= rhs.y, z /= rhs.z;
            return *this;
        }

    public:
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };
}