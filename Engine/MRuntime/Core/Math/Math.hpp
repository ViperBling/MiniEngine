#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

#include "Random.hpp"

#define CMP(x, y) (fabsf(x - y) < FLT_EPSILON * fmaxf(1.0f, fmaxf(fabsf(x), fabsf(y))))

namespace MiniEngine
{
    static const float MATH_POS_INFINITY = std::numeric_limits<float>::infinity();
    static const float MATH_NEG_INFINITY = -std::numeric_limits<float>::infinity();
    static const float MATH_PI           = 3.14159265358979323846264338327950288f;
    static const float MATH_ONE_OVER_PI  = 1.0f / MATH_PI;
    static const float MATH_TWO_PI       = 2.0f * MATH_PI;
    static const float MATH_HALF_PI      = 0.5f * MATH_PI;
    static const float MATH_fDeg2Rad     = MATH_PI / 180.0f;
    static const float MATH_fRad2Deg     = 180.0f / MATH_PI;
    static const float MATH_LOG2         = log(2.0f);
    static const float MATH_EPSILON      = 1e-6f;

    static const float FLOAT_EPSILON  = FLT_EPSILON;
    static const float DOUBLE_EPSILON = DBL_EPSILON;

    class Radian;
    class Angle;
    class Degree;

    class Vector2;
    class Vector3;
    class Vector4;
    class Matrix3x3;
    class Matrix4x4;
    class Quaternion;

    class Radian
    {
    public:
        explicit Radian(float r = 0) : mRad(r) {}
        explicit Radian(const Degree& d);
        Radian& operator=(float f)
        {
            mRad = f;
            return *this;
        }
        Radian& operator=(const Degree& d);

        float ValueRadians() const { return mRad; }
        float ValueDegrees() const; // see bottom of this file
        float ValueAngleUnits() const;

        void SetValue(float f) { mRad = f; }

        const Radian& operator+() const { return *this; }
        Radian        operator+(const Radian& r) const { return Radian(mRad + r.mRad); }
        Radian        operator+(const Degree& d) const;
        Radian&       operator+=(const Radian& r)
        {
            mRad += r.mRad;
            return *this;
        }
        Radian& operator+=(const Degree& d);
        Radian  operator-() const { return Radian(-mRad); }
        Radian  operator-(const Radian& r) const { return Radian(mRad - r.mRad); }
        Radian  operator-(const Degree& d) const;
        Radian& operator-=(const Radian& r)
        {
            mRad -= r.mRad;
            return *this;
        }
        Radian& operator-=(const Degree& d);
        Radian  operator*(float f) const { return Radian(mRad * f); }
        Radian  operator*(const Radian& f) const { return Radian(mRad * f.mRad); }
        Radian& operator*=(float f)
        {
            mRad *= f;
            return *this;
        }
        Radian  operator/(float f) const { return Radian(mRad / f); }
        Radian& operator/=(float f)
        {
            mRad /= f;
            return *this;
        }

        bool operator<(const Radian& r) const { return mRad < r.mRad; }
        bool operator<=(const Radian& r) const { return mRad <= r.mRad; }
        bool operator==(const Radian& r) const { return mRad == r.mRad; }
        bool operator!=(const Radian& r) const { return mRad != r.mRad; }
        bool operator>=(const Radian& r) const { return mRad >= r.mRad; }
        bool operator>(const Radian& r) const { return mRad > r.mRad; }

    private:
        float mRad;
    };

    /** Wrapper class which indicates a given angle value is in Degrees.
    @remarks
        Degree values are interchangeable with Radian values, and conversions
        will be done automatically between them.
    */
    class Degree
    {
    public:
        explicit Degree(float d = 0) : mDeg(d) {}
        explicit Degree(const Radian& r) : mDeg(r.ValueDegrees()) {}
        Degree& operator=(float f)
        {
            mDeg = f;
            return *this;
        }
        Degree& operator=(const Degree& d) = default;
        Degree& operator=(const Radian& r)
        {
            mDeg = r.ValueDegrees();
            return *this;
        }

        float ValueDegrees() const { return mDeg; }
        float ValueRadians() const; // see bottom of this file
        float ValueAngleUnits() const;

        const Degree& operator+() const { return *this; }
        Degree        operator+(const Degree& d) const { return Degree(mDeg + d.mDeg); }
        Degree        operator+(const Radian& r) const { return Degree(mDeg + r.ValueDegrees()); }
        Degree&       operator+=(const Degree& d)
        {
            mDeg += d.mDeg;
            return *this;
        }
        Degree& operator+=(const Radian& r)
        {
            mDeg += r.ValueDegrees();
            return *this;
        }
        Degree  operator-() const { return Degree(-mDeg); }
        Degree  operator-(const Degree& d) const { return Degree(mDeg - d.mDeg); }
        Degree  operator-(const Radian& r) const { return Degree(mDeg - r.ValueDegrees()); }
        Degree& operator-=(const Degree& d)
        {
            mDeg -= d.mDeg;
            return *this;
        }
        Degree& operator-=(const Radian& r)
        {
            mDeg -= r.ValueDegrees();
            return *this;
        }
        Degree  operator*(float f) const { return Degree(mDeg * f); }
        Degree  operator*(const Degree& f) const { return Degree(mDeg * f.mDeg); }
        Degree& operator*=(float f)
        {
            mDeg *= f;
            return *this;
        }
        Degree  operator/(float f) const { return Degree(mDeg / f); }
        Degree& operator/=(float f)
        {
            mDeg /= f;
            return *this;
        }

        bool operator<(const Degree& d) const { return mDeg < d.mDeg; }
        bool operator<=(const Degree& d) const { return mDeg <= d.mDeg; }
        bool operator==(const Degree& d) const { return mDeg == d.mDeg; }
        bool operator!=(const Degree& d) const { return mDeg != d.mDeg; }
        bool operator>=(const Degree& d) const { return mDeg >= d.mDeg; }
        bool operator>(const Degree& d) const { return mDeg > d.mDeg; }
    
    private:
        float mDeg; // if you get an error here - make sure to define/typedef 'float' first
    };

    /** Wrapper class which identifies a value as the currently default angle
        type, as defined by Math::setAngleUnit.
    @remarks
        Angle values will be automatically converted between radians and degrees,
        as appropriate.
    */
    class Angle
    {
    public:
        explicit Angle(float angle) : mAngle(angle) {}
        Angle() { mAngle = 0; }

        explicit operator Radian() const;
        explicit operator Degree() const;
    
    private:
        float mAngle;
    };

    class Math
    {
    public:
        Math();

        static float Abs(float value) { return std::fabs(value); }
        static bool  IsNaN(float f) { return std::isnan(f); }
        static float Sqr(float value) { return value * value; }
        static float Sqrt(float fValue) { return std::sqrt(fValue); }
        static float InvSqrt(float value) { return 1.f / sqrt(value); }
        static bool  RealEqual(float a, float b, float tolerance = std::numeric_limits<float>::epsilon());
        static float Clamp(float v, float min, float max) { return std::clamp(v, min, max); }
        static float GetMaxElement(float x, float y, float z) { return std::max({x, y, z}); }

        static float DegreesToRadians(float degrees);
        static float RadiansToDegrees(float radians);
        static float AngleUnitsToRadians(float units);
        static float RadiansToAngleUnits(float radians);
        static float AngleUnitsToDegrees(float units);
        static float DegreesToAngleUnits(float degrees);

        static float  Sin(const Radian& rad) { return std::sin(rad.ValueRadians()); }
        static float  Sin(float value) { return std::sin(value); }
        static float  Cos(const Radian& rad) { return std::cos(rad.ValueRadians()); }
        static float  Cos(float value) { return std::cos(value); }
        static float  Tan(const Radian& rad) { return std::tan(rad.ValueRadians()); }
        static float  Tan(float value) { return std::tan(value); }
        static Radian Acos(float value);
        static Radian Asin(float value);
        static Radian Atan(float value) { return Radian(std::atan(value)); }
        static Radian Atan2(float y_v, float x_v) { return Radian(std::atan2(y_v, x_v)); }

        template<class T>
        static constexpr T Max(const T A, const T B)
        {
            return std::max(A, B);
        }

        template<class T>
        static constexpr T Min(const T A, const T B)
        {
            return std::min(A, B);
        }

        template<class T>
        static constexpr T Max3(const T A, const T B, const T C)
        {
            return std::max({A, B, C});
        }

        template<class T>
        static constexpr T Min3(const T A, const T B, const T C)
        {
            return std::min({A, B, C});
        }

        static Matrix4x4
        MakeViewMatrix(const Vector3& position, const Quaternion& orientation, const Matrix4x4* reflect_matrix = nullptr);

        static Matrix4x4
        MakeLookAtMatrix(const Vector3& eye_position, const Vector3& target_position, const Vector3& up_dir);

        static Matrix4x4 MakePerspectiveMatrix(Radian fovy, float aspect, float znear, float zfar);

        static Matrix4x4
        MakeOrthographicProjectionMatrix(float left, float right, float bottom, float top, float znear, float zfar);
        
        static Matrix4x4
        MakeOrthographicProjectionMatrix01(float left, float right, float bottom, float top, float znear, float zfar);

    private:
        enum class AngleUnit
        {
            AU_DEGREE,
            AU_RADIAN
        };

        // angle units used by the api
        static AngleUnit mkAngleUnit;
    };

    // these functions could not be defined within the class definition of class
    // Radian because they required class Degree to be defined
    inline Radian::Radian(const Degree& d) : mRad(d.ValueRadians()) {}
    inline Radian& Radian::operator=(const Degree& d)
    {
        mRad = d.ValueRadians();
        return *this;
    }
    inline Radian Radian::operator+(const Degree& d) const { return Radian(mRad + d.ValueRadians()); }
    inline Radian& Radian::operator+=(const Degree& d)
    {
        mRad += d.ValueRadians();
        return *this;
    }
    inline Radian Radian::operator-(const Degree& d) const { return Radian(mRad - d.ValueRadians()); }
    inline Radian& Radian::operator-=(const Degree& d)
    {
        mRad -= d.ValueRadians();
        return *this;
    }

    inline float Radian::ValueDegrees() const { return Math::RadiansToDegrees(mRad); }

    inline float Radian::ValueAngleUnits() const { return Math::RadiansToAngleUnits(mRad); }

    inline float Degree::ValueRadians() const { return Math::DegreesToRadians(mDeg); }

    inline float Degree::ValueAngleUnits() const { return Math::DegreesToAngleUnits(mDeg); }

    inline Angle::operator Radian() const { return Radian(Math::AngleUnitsToRadians(mAngle)); }

    inline Angle::operator Degree() const { return Degree(Math::AngleUnitsToDegrees(mAngle)); }

    inline Radian operator*(float a, const Radian& b) { return Radian(a * b.ValueRadians()); }

    inline Radian operator/(float a, const Radian& b) { return Radian(a / b.ValueRadians()); }

    inline Degree operator*(float a, const Degree& b) { return Degree(a * b.ValueDegrees()); }

    inline Degree operator/(float a, const Degree& b) { return Degree(a / b.ValueDegrees()); }
} // namespace MiniEngine
