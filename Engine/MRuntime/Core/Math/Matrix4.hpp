#pragma once

#include "MRuntime/Core/Math/Math.hpp"
#include "MRuntime/Core/Math/Matrix3.hpp"
#include "MRuntime/Core/Math/Quaternion.hpp"
#include "MRuntime/Core/Math/Vector3.hpp"
#include "MRuntime/Core/Math/Vector4.hpp"

namespace MiniEngine
{
    REFLECTION_TYPE(Matrix4x4_)
    CLASS(Matrix4x4_, Fields)
    {
        REFLECTION_BODY(Matrix4x4_);

    public:
        Matrix4x4_() {}
        float v0 {1.f};
        float v1 {0};
        float v2 {0};
        float v3 {0};
        float v4 {0};
        float v5 {1.f};
        float v6 {0};
        float v7 {0};
        float v8 {0};
        float v9 {0};
        float v10 {1.f};
        float v11 {0};
        float v12 {0};
        float v13 {0};
        float v14 {0};
        float v15 {1.f};
    };
    class Matrix4x4
    {
    public:

        Matrix4x4(const Matrix4x4_& mat)
        {
            mMat[0][0] = mat.v0;
            mMat[0][1] = mat.v1;
            mMat[0][2] = mat.v2;
            mMat[0][3] = mat.v3;
            mMat[1][0] = mat.v4;
            mMat[1][1] = mat.v5;
            mMat[1][2] = mat.v6;
            mMat[1][3] = mat.v7;
            mMat[2][0] = mat.v8;
            mMat[2][1] = mat.v9;
            mMat[2][2] = mat.v10;
            mMat[2][3] = mat.v11;
            mMat[3][0] = mat.v12;
            mMat[3][1] = mat.v13;
            mMat[3][2] = mat.v14;
            mMat[3][3] = mat.v15;
        }

        Matrix4x4_ ToMatrix4x4_()
        {
            Matrix4x4_ res;

            res.v0  = mMat[0][0];
            res.v1  = mMat[0][1];
            res.v2  = mMat[0][2];
            res.v3  = mMat[0][3];
            res.v4  = mMat[1][0];
            res.v5  = mMat[1][1];
            res.v6  = mMat[1][2];
            res.v7  = mMat[1][3];
            res.v8  = mMat[2][0];
            res.v9  = mMat[2][1];
            res.v10 = mMat[2][2];
            res.v11 = mMat[2][3];
            res.v12 = mMat[3][0];
            res.v13 = mMat[3][1];
            res.v14 = mMat[3][2];
            res.v15 = mMat[3][3];
            return res;
        }

        Matrix4x4() { operator=(IDENTITY); }

        Matrix4x4(const float (&float_array)[16])
        {
            mMat[0][0] = float_array[0];
            mMat[0][1] = float_array[1];
            mMat[0][2] = float_array[2];
            mMat[0][3] = float_array[3];
            mMat[1][0] = float_array[4];
            mMat[1][1] = float_array[5];
            mMat[1][2] = float_array[6];
            mMat[1][3] = float_array[7];
            mMat[2][0] = float_array[8];
            mMat[2][1] = float_array[9];
            mMat[2][2] = float_array[10];
            mMat[2][3] = float_array[11];
            mMat[3][0] = float_array[12];
            mMat[3][1] = float_array[13];
            mMat[3][2] = float_array[14];
            mMat[3][3] = float_array[15];
        }

        Matrix4x4(
            float m00,
            float m01,
            float m02,
            float m03,
            float m10,
            float m11,
            float m12,
            float m13,
            float m20,
            float m21,
            float m22,
            float m23,
            float m30,
            float m31,
            float m32,
            float m33)
        {
            mMat[0][0] = m00;
            mMat[0][1] = m01;
            mMat[0][2] = m02;
            mMat[0][3] = m03;
            mMat[1][0] = m10;
            mMat[1][1] = m11;
            mMat[1][2] = m12;
            mMat[1][3] = m13;
            mMat[2][0] = m20;
            mMat[2][1] = m21;
            mMat[2][2] = m22;
            mMat[2][3] = m23;
            mMat[3][0] = m30;
            mMat[3][1] = m31;
            mMat[3][2] = m32;
            mMat[3][3] = m33;
        }

        Matrix4x4(const Vector4& row0, const Vector4& row1, const Vector4& row2, const Vector4& row3)
        {
            mMat[0][0] = row0.x;
            mMat[0][1] = row0.y;
            mMat[0][2] = row0.z;
            mMat[0][3] = row0.w;
            mMat[1][0] = row1.x;
            mMat[1][1] = row1.y;
            mMat[1][2] = row1.z;
            mMat[1][3] = row1.w;
            mMat[2][0] = row2.x;
            mMat[2][1] = row2.y;
            mMat[2][2] = row2.z;
            mMat[2][3] = row2.w;
            mMat[3][0] = row3.x;
            mMat[3][1] = row3.y;
            mMat[3][2] = row3.z;
            mMat[3][3] = row3.w;
        }

        Matrix4x4(const Vector3& position, const Vector3& scale, const Quaternion& rotation)
        {
            MakeTransform(position, scale, rotation);
        }

        void FromData(const float (&float_array)[16])
        {
            mMat[0][0] = float_array[0];
            mMat[0][1] = float_array[1];
            mMat[0][2] = float_array[2];
            mMat[0][3] = float_array[3];
            mMat[1][0] = float_array[4];
            mMat[1][1] = float_array[5];
            mMat[1][2] = float_array[6];
            mMat[1][3] = float_array[7];
            mMat[2][0] = float_array[8];
            mMat[2][1] = float_array[9];
            mMat[2][2] = float_array[10];
            mMat[2][3] = float_array[11];
            mMat[3][0] = float_array[12];
            mMat[3][1] = float_array[13];
            mMat[3][2] = float_array[14];
            mMat[3][3] = float_array[15];
        }

        void ToData(float (&float_array)[16]) const
        {
            float_array[0]  = mMat[0][0];
            float_array[1]  = mMat[0][1];
            float_array[2]  = mMat[0][2];
            float_array[3]  = mMat[0][3];
            float_array[4]  = mMat[1][0];
            float_array[5]  = mMat[1][1];
            float_array[6]  = mMat[1][2];
            float_array[7]  = mMat[1][3];
            float_array[8]  = mMat[2][0];
            float_array[9]  = mMat[2][1];
            float_array[10] = mMat[2][2];
            float_array[11] = mMat[2][3];
            float_array[12] = mMat[3][0];
            float_array[13] = mMat[3][1];
            float_array[14] = mMat[3][2];
            float_array[15] = mMat[3][3];
        }

        /** Creates a standard 4x4 transformation matrix with a zero translation part from a rotation/scaling 3x3
         * matrix.
         */
        void SetMatrix3x3(const Matrix3x3& mat3)
        {
            mMat[0][0] = mat3.mMat[0][0];
            mMat[0][1] = mat3.mMat[0][1];
            mMat[0][2] = mat3.mMat[0][2];
            mMat[0][3] = 0;
            mMat[1][0] = mat3.mMat[1][0];
            mMat[1][1] = mat3.mMat[1][1];
            mMat[1][2] = mat3.mMat[1][2];
            mMat[1][3] = 0;
            mMat[2][0] = mat3.mMat[2][0];
            mMat[2][1] = mat3.mMat[2][1];
            mMat[2][2] = mat3.mMat[2][2];
            mMat[2][3] = 0;
            mMat[3][0] = 0;
            mMat[3][1] = 0;
            mMat[3][2] = 0;
            mMat[3][3] = 1;
        }

        /** Creates a standard 4x4 transformation matrix with a zero translation part from a rotation/scaling
         * Quaternion.
         */
        Matrix4x4(const Quaternion& rot)
        {
            Matrix3x3 m3x3;
            rot.ToRotationMatrix(m3x3);
            operator=(IDENTITY);
            SetMatrix3x3(m3x3);
        }

        float* operator[](size_t row_index)
        {
            assert(row_index < 4);
            return mMat[row_index];
        }

        const float* operator[](size_t row_index) const
        {
            assert(row_index < 4);
            return mMat[row_index];
        }

        Matrix4x4 Concatenate(const Matrix4x4& m2) const
        {
            Matrix4x4 r;
            r.mMat[0][0] = mMat[0][0] * m2.mMat[0][0] + mMat[0][1] * m2.mMat[1][0] + mMat[0][2] * m2.mMat[2][0] +
                            mMat[0][3] * m2.mMat[3][0];
            r.mMat[0][1] = mMat[0][0] * m2.mMat[0][1] + mMat[0][1] * m2.mMat[1][1] + mMat[0][2] * m2.mMat[2][1] +
                            mMat[0][3] * m2.mMat[3][1];
            r.mMat[0][2] = mMat[0][0] * m2.mMat[0][2] + mMat[0][1] * m2.mMat[1][2] + mMat[0][2] * m2.mMat[2][2] +
                            mMat[0][3] * m2.mMat[3][2];
            r.mMat[0][3] = mMat[0][0] * m2.mMat[0][3] + mMat[0][1] * m2.mMat[1][3] + mMat[0][2] * m2.mMat[2][3] +
                            mMat[0][3] * m2.mMat[3][3];

            r.mMat[1][0] = mMat[1][0] * m2.mMat[0][0] + mMat[1][1] * m2.mMat[1][0] + mMat[1][2] * m2.mMat[2][0] +
                            mMat[1][3] * m2.mMat[3][0];
            r.mMat[1][1] = mMat[1][0] * m2.mMat[0][1] + mMat[1][1] * m2.mMat[1][1] + mMat[1][2] * m2.mMat[2][1] +
                            mMat[1][3] * m2.mMat[3][1];
            r.mMat[1][2] = mMat[1][0] * m2.mMat[0][2] + mMat[1][1] * m2.mMat[1][2] + mMat[1][2] * m2.mMat[2][2] +
                            mMat[1][3] * m2.mMat[3][2];
            r.mMat[1][3] = mMat[1][0] * m2.mMat[0][3] + mMat[1][1] * m2.mMat[1][3] + mMat[1][2] * m2.mMat[2][3] +
                            mMat[1][3] * m2.mMat[3][3];

            r.mMat[2][0] = mMat[2][0] * m2.mMat[0][0] + mMat[2][1] * m2.mMat[1][0] + mMat[2][2] * m2.mMat[2][0] +
                            mMat[2][3] * m2.mMat[3][0];
            r.mMat[2][1] = mMat[2][0] * m2.mMat[0][1] + mMat[2][1] * m2.mMat[1][1] + mMat[2][2] * m2.mMat[2][1] +
                            mMat[2][3] * m2.mMat[3][1];
            r.mMat[2][2] = mMat[2][0] * m2.mMat[0][2] + mMat[2][1] * m2.mMat[1][2] + mMat[2][2] * m2.mMat[2][2] +
                            mMat[2][3] * m2.mMat[3][2];
            r.mMat[2][3] = mMat[2][0] * m2.mMat[0][3] + mMat[2][1] * m2.mMat[1][3] + mMat[2][2] * m2.mMat[2][3] +
                            mMat[2][3] * m2.mMat[3][3];

            r.mMat[3][0] = mMat[3][0] * m2.mMat[0][0] + mMat[3][1] * m2.mMat[1][0] + mMat[3][2] * m2.mMat[2][0] +
                            mMat[3][3] * m2.mMat[3][0];
            r.mMat[3][1] = mMat[3][0] * m2.mMat[0][1] + mMat[3][1] * m2.mMat[1][1] + mMat[3][2] * m2.mMat[2][1] +
                            mMat[3][3] * m2.mMat[3][1];
            r.mMat[3][2] = mMat[3][0] * m2.mMat[0][2] + mMat[3][1] * m2.mMat[1][2] + mMat[3][2] * m2.mMat[2][2] +
                            mMat[3][3] * m2.mMat[3][2];
            r.mMat[3][3] = mMat[3][0] * m2.mMat[0][3] + mMat[3][1] * m2.mMat[1][3] + mMat[3][2] * m2.mMat[2][3] +
                            mMat[3][3] * m2.mMat[3][3];

            return r;
        }

        /** Matrix concatenation using '*'.
         */
        Matrix4x4 operator*(const Matrix4x4& m2) const { return Concatenate(m2); }

        /** Vector transformation using '*'.
        @remarks
        Transforms the given 3-D vector by the matrix, projecting the
        result back into <i>w</i> = 1.
        @note
        This means that the initial <i>w</i> is considered to be 1.0,
        and then all the three elements of the resulting 3-D vector are
        divided by the resulting <i>w</i>.
        */
        Vector3 operator*(const Vector3& v) const
        {
            Vector3 r;

            float inv_w = 1.0f / (mMat[3][0] * v.x + mMat[3][1] * v.y + mMat[3][2] * v.z + mMat[3][3]);

            r.x = (mMat[0][0] * v.x + mMat[0][1] * v.y + mMat[0][2] * v.z + mMat[0][3]) * inv_w;
            r.y = (mMat[1][0] * v.x + mMat[1][1] * v.y + mMat[1][2] * v.z + mMat[1][3]) * inv_w;
            r.z = (mMat[2][0] * v.x + mMat[2][1] * v.y + mMat[2][2] * v.z + mMat[2][3]) * inv_w;

            return r;
        }

        Vector4 operator*(const Vector4& v) const
        {
            return Vector4(mMat[0][0] * v.x + mMat[0][1] * v.y + mMat[0][2] * v.z + mMat[0][3] * v.w,
                           mMat[1][0] * v.x + mMat[1][1] * v.y + mMat[1][2] * v.z + mMat[1][3] * v.w,
                           mMat[2][0] * v.x + mMat[2][1] * v.y + mMat[2][2] * v.z + mMat[2][3] * v.w,
                           mMat[3][0] * v.x + mMat[3][1] * v.y + mMat[3][2] * v.z + mMat[3][3] * v.w);
        }

        /** Matrix addition.
         */
        Matrix4x4 operator+(const Matrix4x4& m2) const
        {
            Matrix4x4 r;

            r.mMat[0][0] = mMat[0][0] + m2.mMat[0][0];
            r.mMat[0][1] = mMat[0][1] + m2.mMat[0][1];
            r.mMat[0][2] = mMat[0][2] + m2.mMat[0][2];
            r.mMat[0][3] = mMat[0][3] + m2.mMat[0][3];

            r.mMat[1][0] = mMat[1][0] + m2.mMat[1][0];
            r.mMat[1][1] = mMat[1][1] + m2.mMat[1][1];
            r.mMat[1][2] = mMat[1][2] + m2.mMat[1][2];
            r.mMat[1][3] = mMat[1][3] + m2.mMat[1][3];

            r.mMat[2][0] = mMat[2][0] + m2.mMat[2][0];
            r.mMat[2][1] = mMat[2][1] + m2.mMat[2][1];
            r.mMat[2][2] = mMat[2][2] + m2.mMat[2][2];
            r.mMat[2][3] = mMat[2][3] + m2.mMat[2][3];

            r.mMat[3][0] = mMat[3][0] + m2.mMat[3][0];
            r.mMat[3][1] = mMat[3][1] + m2.mMat[3][1];
            r.mMat[3][2] = mMat[3][2] + m2.mMat[3][2];
            r.mMat[3][3] = mMat[3][3] + m2.mMat[3][3];

            return r;
        }

        /** Matrix subtraction.
         */
        Matrix4x4 operator-(const Matrix4x4& m2) const
        {
            Matrix4x4 r;
            r.mMat[0][0] = mMat[0][0] - m2.mMat[0][0];
            r.mMat[0][1] = mMat[0][1] - m2.mMat[0][1];
            r.mMat[0][2] = mMat[0][2] - m2.mMat[0][2];
            r.mMat[0][3] = mMat[0][3] - m2.mMat[0][3];

            r.mMat[1][0] = mMat[1][0] - m2.mMat[1][0];
            r.mMat[1][1] = mMat[1][1] - m2.mMat[1][1];
            r.mMat[1][2] = mMat[1][2] - m2.mMat[1][2];
            r.mMat[1][3] = mMat[1][3] - m2.mMat[1][3];

            r.mMat[2][0] = mMat[2][0] - m2.mMat[2][0];
            r.mMat[2][1] = mMat[2][1] - m2.mMat[2][1];
            r.mMat[2][2] = mMat[2][2] - m2.mMat[2][2];
            r.mMat[2][3] = mMat[2][3] - m2.mMat[2][3];

            r.mMat[3][0] = mMat[3][0] - m2.mMat[3][0];
            r.mMat[3][1] = mMat[3][1] - m2.mMat[3][1];
            r.mMat[3][2] = mMat[3][2] - m2.mMat[3][2];
            r.mMat[3][3] = mMat[3][3] - m2.mMat[3][3];

            return r;
        }

        Matrix4x4 operator*(float scalar) const
        {
            return Matrix4x4(scalar * mMat[0][0],
                             scalar * mMat[0][1],
                             scalar * mMat[0][2],
                             scalar * mMat[0][3],
                             scalar * mMat[1][0],
                             scalar * mMat[1][1],
                             scalar * mMat[1][2],
                             scalar * mMat[1][3],
                             scalar * mMat[2][0],
                             scalar * mMat[2][1],
                             scalar * mMat[2][2],
                             scalar * mMat[2][3],
                             scalar * mMat[3][0],
                             scalar * mMat[3][1],
                             scalar * mMat[3][2],
                             scalar * mMat[3][3]);
        }

        /** Tests 2 matrices for equality.
         */
        bool operator==(const Matrix4x4& m2) const
        {
            return !(mMat[0][0] != m2.mMat[0][0] || mMat[0][1] != m2.mMat[0][1] || mMat[0][2] != m2.mMat[0][2] ||
                mMat[0][3] != m2.mMat[0][3] || mMat[1][0] != m2.mMat[1][0] || mMat[1][1] != m2.mMat[1][1] ||
                mMat[1][2] != m2.mMat[1][2] || mMat[1][3] != m2.mMat[1][3] || mMat[2][0] != m2.mMat[2][0] ||
                mMat[2][1] != m2.mMat[2][1] || mMat[2][2] != m2.mMat[2][2] || mMat[2][3] != m2.mMat[2][3] ||
                mMat[3][0] != m2.mMat[3][0] || mMat[3][1] != m2.mMat[3][1] || mMat[3][2] != m2.mMat[3][2] ||
                mMat[3][3] != m2.mMat[3][3]);
        }

        /** Tests 2 matrices for inequality.
         */
        bool operator!=(const Matrix4x4& m2) const
        {
            return mMat[0][0] != m2.mMat[0][0] || mMat[0][1] != m2.mMat[0][1] || mMat[0][2] != m2.mMat[0][2] ||
                mMat[0][3] != m2.mMat[0][3] || mMat[1][0] != m2.mMat[1][0] || mMat[1][1] != m2.mMat[1][1] ||
                mMat[1][2] != m2.mMat[1][2] || mMat[1][3] != m2.mMat[1][3] || mMat[2][0] != m2.mMat[2][0] ||
                mMat[2][1] != m2.mMat[2][1] || mMat[2][2] != m2.mMat[2][2] || mMat[2][3] != m2.mMat[2][3] ||
                mMat[3][0] != m2.mMat[3][0] || mMat[3][1] != m2.mMat[3][1] || mMat[3][2] != m2.mMat[3][2] ||
                mMat[3][3] != m2.mMat[3][3];
        }

        Matrix4x4 Transpose() const
        {
            return Matrix4x4(mMat[0][0],
                             mMat[1][0],
                             mMat[2][0],
                             mMat[3][0],
                             mMat[0][1],
                             mMat[1][1],
                             mMat[2][1],
                             mMat[3][1],
                             mMat[0][2],
                             mMat[1][2],
                             mMat[2][2],
                             mMat[3][2],
                             mMat[0][3],
                             mMat[1][3],
                             mMat[2][3],
                             mMat[3][3]);
        }

        //-----------------------------------------------------------------------
        float GetMinor(size_t r0, size_t r1, size_t r2, size_t c0, size_t c1, size_t c2) const
        {
            return mMat[r0][c0] * (mMat[r1][c1] * mMat[r2][c2] - mMat[r2][c1] * mMat[r1][c2]) -
                   mMat[r0][c1] * (mMat[r1][c0] * mMat[r2][c2] - mMat[r2][c0] * mMat[r1][c2]) +
                   mMat[r0][c2] * (mMat[r1][c0] * mMat[r2][c1] - mMat[r2][c0] * mMat[r1][c1]);
        }

        /*
        -----------------------------------------------------------------------
        Translation Transformation
        -----------------------------------------------------------------------
        */
        /** Sets the translation transformation part of the matrix.
         */
        void SetTrans(const Vector3& v)
        {
            mMat[0][3] = v.x;
            mMat[1][3] = v.y;
            mMat[2][3] = v.z;
        }

        /** Extracts the translation transformation part of the matrix.
         */
        Vector3 GetTrans() const { return Vector3(mMat[0][3], mMat[1][3], mMat[2][3]); }

        Matrix4x4 BuildViewportMatrix(uint32_t width, uint32_t height)
        {
            return Matrix4x4(0.5f * (float)width,
                             0.0f,
                             0.0f,
                             0.5f * (float)width,
                             0.0f,
                             -0.5f * (float)height,
                             0.0f,
                             0.5f * (float)height,
                             0.0f,
                             0.0f,
                             -1.0f,
                             1.0f,
                             0.0f,
                             0.0f,
                             0.0f,
                             1.0f);
        }

        static Matrix4x4 MirrorMatrix(Vector4 mirror_plane)
        {
            Matrix4x4 result;
            result.mMat[0][0] = -2 * mirror_plane.x * mirror_plane.x + 1;
            result.mMat[1][0] = -2 * mirror_plane.x * mirror_plane.y;
            result.mMat[2][0] = -2 * mirror_plane.x * mirror_plane.z;
            result.mMat[3][0] = 0;

            result.mMat[0][1] = -2 * mirror_plane.y * mirror_plane.x;
            result.mMat[1][1] = -2 * mirror_plane.y * mirror_plane.y + 1;
            result.mMat[2][1] = -2 * mirror_plane.y * mirror_plane.z;
            result.mMat[3][1] = 0;

            result.mMat[0][2] = -2 * mirror_plane.z * mirror_plane.x;
            result.mMat[1][2] = -2 * mirror_plane.z * mirror_plane.y;
            result.mMat[2][2] = -2 * mirror_plane.z * mirror_plane.z + 1;
            result.mMat[3][2] = 0;

            result.mMat[0][3] = -2 * mirror_plane.w * mirror_plane.x;
            result.mMat[1][3] = -2 * mirror_plane.w * mirror_plane.y;
            result.mMat[2][3] = -2 * mirror_plane.w * mirror_plane.z;
            result.mMat[3][3] = 1;

            return result;
        }

        static Matrix4x4 RotationMatrix(Vector3 normal)
        {
            Vector3 up = Vector3(0, 0, 1);
            if (fabs(normal.z) > 0.999f)
            {
                up = Vector3(0, 1, 0);
            }

            Vector3 left = up.CrossProduct(normal);
            up           = normal.CrossProduct(left);

            left.Normalize();
            up.Normalize();

            Matrix4x4 result = Matrix4x4::IDENTITY;
            result.SetMatrix3x3(Matrix3x3(left, up, normal));

            return result.Transpose();
        }

        /** Builds a translation matrix
         */
        void MakeTrans(const Vector3& v)
        {
            mMat[0][0] = 1.0;
            mMat[0][1] = 0.0;
            mMat[0][2] = 0.0;
            mMat[0][3] = v.x;
            mMat[1][0] = 0.0;
            mMat[1][1] = 1.0;
            mMat[1][2] = 0.0;
            mMat[1][3] = v.y;
            mMat[2][0] = 0.0;
            mMat[2][1] = 0.0;
            mMat[2][2] = 1.0;
            mMat[2][3] = v.z;
            mMat[3][0] = 0.0;
            mMat[3][1] = 0.0;
            mMat[3][2] = 0.0;
            mMat[3][3] = 1.0;
        }

        void MakeTrans(float tx, float ty, float tz)
        {
            mMat[0][0] = 1.0;
            mMat[0][1] = 0.0;
            mMat[0][2] = 0.0;
            mMat[0][3] = tx;
            mMat[1][0] = 0.0;
            mMat[1][1] = 1.0;
            mMat[1][2] = 0.0;
            mMat[1][3] = ty;
            mMat[2][0] = 0.0;
            mMat[2][1] = 0.0;
            mMat[2][2] = 1.0;
            mMat[2][3] = tz;
            mMat[3][0] = 0.0;
            mMat[3][1] = 0.0;
            mMat[3][2] = 0.0;
            mMat[3][3] = 1.0;
        }

        /** Gets a translation matrix.
         */
        static Matrix4x4 GetTrans(const Vector3& v)
        {
            Matrix4x4 r;

            r.mMat[0][0] = 1.0;
            r.mMat[0][1] = 0.0;
            r.mMat[0][2] = 0.0;
            r.mMat[0][3] = v.x;
            r.mMat[1][0] = 0.0;
            r.mMat[1][1] = 1.0;
            r.mMat[1][2] = 0.0;
            r.mMat[1][3] = v.y;
            r.mMat[2][0] = 0.0;
            r.mMat[2][1] = 0.0;
            r.mMat[2][2] = 1.0;
            r.mMat[2][3] = v.z;
            r.mMat[3][0] = 0.0;
            r.mMat[3][1] = 0.0;
            r.mMat[3][2] = 0.0;
            r.mMat[3][3] = 1.0;

            return r;
        }

        /** Gets a translation matrix - variation for not using a vector.
         */
        static Matrix4x4 GetTrans(float t_x, float t_y, float t_z)
        {
            Matrix4x4 r;

            r.mMat[0][0] = 1.0;
            r.mMat[0][1] = 0.0;
            r.mMat[0][2] = 0.0;
            r.mMat[0][3] = t_x;
            r.mMat[1][0] = 0.0;
            r.mMat[1][1] = 1.0;
            r.mMat[1][2] = 0.0;
            r.mMat[1][3] = t_y;
            r.mMat[2][0] = 0.0;
            r.mMat[2][1] = 0.0;
            r.mMat[2][2] = 1.0;
            r.mMat[2][3] = t_z;
            r.mMat[3][0] = 0.0;
            r.mMat[3][1] = 0.0;
            r.mMat[3][2] = 0.0;
            r.mMat[3][3] = 1.0;

            return r;
        }

        /*
        -----------------------------------------------------------------------
        Scale Transformation
        -----------------------------------------------------------------------
        */
        /** Sets the scale part of the matrix.
         */
        void SetScale(const Vector3& v)
        {
            mMat[0][0] = v.x;
            mMat[1][1] = v.y;
            mMat[2][2] = v.z;
        }

        /** Gets a scale matrix.
         */
        static Matrix4x4 GetScale(const Vector3& v)
        {
            Matrix4x4 r;
            r.mMat[0][0] = v.x;
            r.mMat[0][1] = 0.0;
            r.mMat[0][2] = 0.0;
            r.mMat[0][3] = 0.0;
            r.mMat[1][0] = 0.0;
            r.mMat[1][1] = v.y;
            r.mMat[1][2] = 0.0;
            r.mMat[1][3] = 0.0;
            r.mMat[2][0] = 0.0;
            r.mMat[2][1] = 0.0;
            r.mMat[2][2] = v.z;
            r.mMat[2][3] = 0.0;
            r.mMat[3][0] = 0.0;
            r.mMat[3][1] = 0.0;
            r.mMat[3][2] = 0.0;
            r.mMat[3][3] = 1.0;

            return r;
        }

        /** Gets a scale matrix - variation for not using a vector.
         */
        static Matrix4x4 BuildScaleMatrix(float s_x, float s_y, float s_z)
        {
            Matrix4x4 r;
            r.mMat[0][0] = s_x;
            r.mMat[0][1] = 0.0;
            r.mMat[0][2] = 0.0;
            r.mMat[0][3] = 0.0;
            r.mMat[1][0] = 0.0;
            r.mMat[1][1] = s_y;
            r.mMat[1][2] = 0.0;
            r.mMat[1][3] = 0.0;
            r.mMat[2][0] = 0.0;
            r.mMat[2][1] = 0.0;
            r.mMat[2][2] = s_z;
            r.mMat[2][3] = 0.0;
            r.mMat[3][0] = 0.0;
            r.mMat[3][1] = 0.0;
            r.mMat[3][2] = 0.0;
            r.mMat[3][3] = 1.0;

            return r;
        }

        /** Extracts the rotation / scaling part of the Matrix as a 3x3 matrix.
        @param m3x3 Destination Matrix3
        */
        void Extract3x3Matrix(Matrix3x3& m3x3) const
        {
            m3x3.mMat[0][0] = mMat[0][0];
            m3x3.mMat[0][1] = mMat[0][1];
            m3x3.mMat[0][2] = mMat[0][2];
            m3x3.mMat[1][0] = mMat[1][0];
            m3x3.mMat[1][1] = mMat[1][1];
            m3x3.mMat[1][2] = mMat[1][2];
            m3x3.mMat[2][0] = mMat[2][0];
            m3x3.mMat[2][1] = mMat[2][1];
            m3x3.mMat[2][2] = mMat[2][2];
        }

        void ExtractAxes(Vector3& out_x, Vector3& out_y, Vector3& out_z) const
        {
            out_x = Vector3(mMat[0][0], mMat[1][0], mMat[2][0]);
            out_x.Normalize();
            out_y = Vector3(mMat[0][1], mMat[1][1], mMat[2][1]);
            out_y.Normalize();
            out_z = Vector3(mMat[0][2], mMat[1][2], mMat[2][2]);
            out_z.Normalize();
        }

        /** Determines if this matrix involves a scaling. */
        bool HasScale() const
        {
            // check magnitude of column vectors (==local axes)
            float t = mMat[0][0] * mMat[0][0] + mMat[1][0] * mMat[1][0] + mMat[2][0] * mMat[2][0];
            if (!Math::RealEqual(t, 1.0, (float)1e-04))
                return true;
            t = mMat[0][1] * mMat[0][1] + mMat[1][1] * mMat[1][1] + mMat[2][1] * mMat[2][1];
            if (!Math::RealEqual(t, 1.0, (float)1e-04))
                return true;
            t = mMat[0][2] * mMat[0][2] + mMat[1][2] * mMat[1][2] + mMat[2][2] * mMat[2][2];
            return !Math::RealEqual(t, 1.0, (float)1e-04);
        }

        /** Determines if this matrix involves a negative scaling. */
        bool HasNegativeScale() const { return Determinant() < 0; }

        /** Extracts the rotation / scaling part as a quaternion from the Matrix.
         */
        Quaternion ExtractQuaternion() const
        {
            Matrix3x3 m3x3;
            Extract3x3Matrix(m3x3);
            return Quaternion(m3x3);
        }

        Matrix4x4 Adjoint() const;

        float Determinant() const
        {
            return mMat[0][0] * GetMinor(1, 2, 3, 1, 2, 3) - mMat[0][1] * GetMinor(1, 2, 3, 0, 2, 3) +
                   mMat[0][2] * GetMinor(1, 2, 3, 0, 1, 3) - mMat[0][3] * GetMinor(1, 2, 3, 0, 1, 2);
        }

        /** Building a Matrix4 from orientation / scale / position.
        @remarks
        Transform is performed in the order scale, rotate, translation, i.e. translation is independent
        of orientation axes, scale does not affect size of translation, rotation and scaling are always
        centered on the origin.
        */
        void MakeTransform(const Vector3& position, const Vector3& scale, const Quaternion& orientation);

        /** Building an inverse Matrix4 from orientation / scale / position.
        @remarks
        As makeTransform except it build the inverse given the same data as makeTransform, so
        performing -translation, -rotate, 1/scale in that order.
        */
        void MakeInverseTransform(const Vector3& position, const Vector3& scale, const Quaternion& orientation);

        /** Decompose a Matrix4 to orientation / scale / position.
         */
        void Decomposition(Vector3& position, Vector3& scale, Quaternion& orientation) const;

        void DecompositionWithoutScale(Vector3& position, Quaternion& rotation) const;

        /** Check whether or not the matrix is affine matrix.
        @remarks
        An affine matrix is a 4x4 matrix with row 3 equal to (0, 0, 0, 1),
        e.g. no projective coefficients.
        */
        bool IsAffine(void) const
        {
            return mMat[3][0] == 0 && mMat[3][1] == 0 && mMat[3][2] == 0 && mMat[3][3] == 1;
        }

        /** Returns the inverse of the affine matrix.
        @note
        The matrix must be an affine matrix. @see Matrix4::isAffine.
        */
        Matrix4x4 InverseAffine() const;

        /** Concatenate two affine matrices.
        @note
        The matrices must be affine matrix. @see Matrix4::isAffine.
        */
        Matrix4x4 ConcatenateAffine(const Matrix4x4& m2) const
        {
            assert(IsAffine() && m2.IsAffine());

            return Matrix4x4(mMat[0][0] * m2.mMat[0][0] + mMat[0][1] * m2.mMat[1][0] + mMat[0][2] * m2.mMat[2][0],
                             mMat[0][0] * m2.mMat[0][1] + mMat[0][1] * m2.mMat[1][1] + mMat[0][2] * m2.mMat[2][1],
                             mMat[0][0] * m2.mMat[0][2] + mMat[0][1] * m2.mMat[1][2] + mMat[0][2] * m2.mMat[2][2],
                             mMat[0][0] * m2.mMat[0][3] + mMat[0][1] * m2.mMat[1][3] +
                                 mMat[0][2] * m2.mMat[2][3] + mMat[0][3],

                             mMat[1][0] * m2.mMat[0][0] + mMat[1][1] * m2.mMat[1][0] + mMat[1][2] * m2.mMat[2][0],
                             mMat[1][0] * m2.mMat[0][1] + mMat[1][1] * m2.mMat[1][1] + mMat[1][2] * m2.mMat[2][1],
                             mMat[1][0] * m2.mMat[0][2] + mMat[1][1] * m2.mMat[1][2] + mMat[1][2] * m2.mMat[2][2],
                             mMat[1][0] * m2.mMat[0][3] + mMat[1][1] * m2.mMat[1][3] +
                                 mMat[1][2] * m2.mMat[2][3] + mMat[1][3],

                             mMat[2][0] * m2.mMat[0][0] + mMat[2][1] * m2.mMat[1][0] + mMat[2][2] * m2.mMat[2][0],
                             mMat[2][0] * m2.mMat[0][1] + mMat[2][1] * m2.mMat[1][1] + mMat[2][2] * m2.mMat[2][1],
                             mMat[2][0] * m2.mMat[0][2] + mMat[2][1] * m2.mMat[1][2] + mMat[2][2] * m2.mMat[2][2],
                             mMat[2][0] * m2.mMat[0][3] + mMat[2][1] * m2.mMat[1][3] +
                                 mMat[2][2] * m2.mMat[2][3] + mMat[2][3],
                             0,
                             0,
                             0,
                             1);
        }

        /** 3-D Vector transformation specially for an affine matrix.
        @remarks
        Transforms the given 3-D vector by the matrix, projecting the
        result back into <i>w</i> = 1.
        @note
        The matrix must be an affine matrix. @see Matrix4::isAffine.
        */
        Vector3 TransformAffine(const Vector3& v) const
        {
            assert(IsAffine());

            return Vector3(mMat[0][0] * v.x + mMat[0][1] * v.y + mMat[0][2] * v.z + mMat[0][3],
                           mMat[1][0] * v.x + mMat[1][1] * v.y + mMat[1][2] * v.z + mMat[1][3],
                           mMat[2][0] * v.x + mMat[2][1] * v.y + mMat[2][2] * v.z + mMat[2][3]);
        }

        /** 4-D Vector transformation specially for an affine matrix.
        @note
        The matrix must be an affine matrix. @see Matrix4::isAffine.
        */
        Vector4 TransformAffine(const Vector4& v) const
        {
            assert(IsAffine());

            return Vector4(mMat[0][0] * v.x + mMat[0][1] * v.y + mMat[0][2] * v.z + mMat[0][3] * v.w,
                           mMat[1][0] * v.x + mMat[1][1] * v.y + mMat[1][2] * v.z + mMat[1][3] * v.w,
                           mMat[2][0] * v.x + mMat[2][1] * v.y + mMat[2][2] * v.z + mMat[2][3] * v.w,
                           v.w);
        }

        Matrix4x4 Inverse() const
        {
            float m00 = mMat[0][0], m01 = mMat[0][1], m02 = mMat[0][2], m03 = mMat[0][3];
            float m10 = mMat[1][0], m11 = mMat[1][1], m12 = mMat[1][2], m13 = mMat[1][3];
            float m20 = mMat[2][0], m21 = mMat[2][1], m22 = mMat[2][2], m23 = mMat[2][3];
            float m30 = mMat[3][0], m31 = mMat[3][1], m32 = mMat[3][2], m33 = mMat[3][3];

            float v0 = m20 * m31 - m21 * m30;
            float v1 = m20 * m32 - m22 * m30;
            float v2 = m20 * m33 - m23 * m30;
            float v3 = m21 * m32 - m22 * m31;
            float v4 = m21 * m33 - m23 * m31;
            float v5 = m22 * m33 - m23 * m32;

            float t00 = +(v5 * m11 - v4 * m12 + v3 * m13);
            float t10 = -(v5 * m10 - v2 * m12 + v1 * m13);
            float t20 = +(v4 * m10 - v2 * m11 + v0 * m13);
            float t30 = -(v3 * m10 - v1 * m11 + v0 * m12);

            float invDet = 1 / (t00 * m00 + t10 * m01 + t20 * m02 + t30 * m03);

            float d00 = t00 * invDet;
            float d10 = t10 * invDet;
            float d20 = t20 * invDet;
            float d30 = t30 * invDet;

            float d01 = -(v5 * m01 - v4 * m02 + v3 * m03) * invDet;
            float d11 = +(v5 * m00 - v2 * m02 + v1 * m03) * invDet;
            float d21 = -(v4 * m00 - v2 * m01 + v0 * m03) * invDet;
            float d31 = +(v3 * m00 - v1 * m01 + v0 * m02) * invDet;

            v0 = m10 * m31 - m11 * m30;
            v1 = m10 * m32 - m12 * m30;
            v2 = m10 * m33 - m13 * m30;
            v3 = m11 * m32 - m12 * m31;
            v4 = m11 * m33 - m13 * m31;
            v5 = m12 * m33 - m13 * m32;

            float d02 = +(v5 * m01 - v4 * m02 + v3 * m03) * invDet;
            float d12 = -(v5 * m00 - v2 * m02 + v1 * m03) * invDet;
            float d22 = +(v4 * m00 - v2 * m01 + v0 * m03) * invDet;
            float d32 = -(v3 * m00 - v1 * m01 + v0 * m02) * invDet;

            v0 = m21 * m10 - m20 * m11;
            v1 = m22 * m10 - m20 * m12;
            v2 = m23 * m10 - m20 * m13;
            v3 = m22 * m11 - m21 * m12;
            v4 = m23 * m11 - m21 * m13;
            v5 = m23 * m12 - m22 * m13;

            float d03 = -(v5 * m01 - v4 * m02 + v3 * m03) * invDet;
            float d13 = +(v5 * m00 - v2 * m02 + v1 * m03) * invDet;
            float d23 = -(v4 * m00 - v2 * m01 + v0 * m03) * invDet;
            float d33 = +(v3 * m00 - v1 * m01 + v0 * m02) * invDet;

            return Matrix4x4(d00, d01, d02, d03, d10, d11, d12, d13, d20, d21, d22, d23, d30, d31, d32, d33);
        }

        Vector3 TransformCoord(const Vector3& v)
        {
            Vector4 temp(v, 1.0f);
            Vector4 ret = (*this) * temp;
            if (ret.w == 0.0f)
            {
                return Vector3::ZERO;
            }
            else
            {
                ret /= ret.w;
                return Vector3(ret.x, ret.y, ret.z);
            }

            return Vector3::ZERO;
        }

        static const Matrix4x4 ZERO;
        static const Matrix4x4 ZEROAFFINE;
        static const Matrix4x4 IDENTITY;

    public:
        float mMat[4][4];
    };

    Vector4 operator*(const Vector4& v, const Matrix4x4& mat);
}