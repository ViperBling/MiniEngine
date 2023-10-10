#pragma once

#include "MRuntime/Core/Math/Math.hpp"
#include "MRuntime/Core/Math/Quaternion.hpp"
#include "MRuntime/Core/Math/Vector3.hpp"

#include <cassert>
#include <string>

namespace MiniEngine
{
    class Matrix3x3
    {
    public:

        Matrix3x3() { operator=(IDENTITY); }

        explicit Matrix3x3(float arr[3][3])
        {
            memcpy(mMat[0], arr[0], 3 * sizeof(float));
            memcpy(mMat[1], arr[1], 3 * sizeof(float));
            memcpy(mMat[2], arr[2], 3 * sizeof(float));
        }

        Matrix3x3(float (&float_array)[9])
        {
            mMat[0][0] = float_array[0];
            mMat[0][1] = float_array[1];
            mMat[0][2] = float_array[2];
            mMat[1][0] = float_array[3];
            mMat[1][1] = float_array[4];
            mMat[1][2] = float_array[5];
            mMat[2][0] = float_array[6];
            mMat[2][1] = float_array[7];
            mMat[2][2] = float_array[8];
        }

        Matrix3x3(float entry00,
                  float entry01,
                  float entry02,
                  float entry10,
                  float entry11,
                  float entry12,
                  float entry20,
                  float entry21,
                  float entry22)
        {
            mMat[0][0] = entry00;
            mMat[0][1] = entry01;
            mMat[0][2] = entry02;
            mMat[1][0] = entry10;
            mMat[1][1] = entry11;
            mMat[1][2] = entry12;
            mMat[2][0] = entry20;
            mMat[2][1] = entry21;
            mMat[2][2] = entry22;
        }

        Matrix3x3(const Vector3& row0, const Vector3& row1, const Vector3& row2)
        {
            mMat[0][0] = row0.x;
            mMat[0][1] = row0.y;
            mMat[0][2] = row0.z;
            mMat[1][0] = row1.x;
            mMat[1][1] = row1.y;
            mMat[1][2] = row1.z;
            mMat[2][0] = row2.x;
            mMat[2][1] = row2.y;
            mMat[2][2] = row2.z;
        }

        Matrix3x3(const Quaternion& q)
        {
            float yy = q.y * q.y;
            float zz = q.z * q.z;
            float xy = q.x * q.y;
            float zw = q.z * q.w;
            float xz = q.x * q.z;
            float yw = q.y * q.w;
            float xx = q.x * q.x;
            float yz = q.y * q.z;
            float xw = q.x * q.w;

            mMat[0][0] = 1 - 2 * yy - 2 * zz;
            mMat[0][1] = 2 * xy + 2 * zw;
            mMat[0][2] = 2 * xz - 2 * yw;

            mMat[1][0] = 2 * xy - 2 * zw;
            mMat[1][1] = 1 - 2 * xx - 2 * zz;
            mMat[1][2] = 2 * yz + 2 * xw;

            mMat[2][0] = 2 * xz + 2 * yw;
            mMat[2][1] = 2 * yz - 2 * xw;
            mMat[2][2] = 1 - 2 * xx - 2 * yy;
        }

        void FromData(float (&float_array)[9])
        {
            mMat[0][0] = float_array[0];
            mMat[0][1] = float_array[1];
            mMat[0][2] = float_array[2];
            mMat[1][0] = float_array[3];
            mMat[1][1] = float_array[4];
            mMat[1][2] = float_array[5];
            mMat[2][0] = float_array[6];
            mMat[2][1] = float_array[7];
            mMat[2][2] = float_array[8];
        }

        void ToData(float (&float_array)[9]) const
        {
            float_array[0] = mMat[0][0];
            float_array[1] = mMat[0][1];
            float_array[2] = mMat[0][2];
            float_array[3] = mMat[1][0];
            float_array[4] = mMat[1][1];
            float_array[5] = mMat[1][2];
            float_array[6] = mMat[2][0];
            float_array[7] = mMat[2][1];
            float_array[8] = mMat[2][2];
        }

        // member access, allows use of construct mat[r][c]
        float* operator[](size_t row_index) const { return (float*)mMat[row_index]; }

        // assignment and comparison
        bool operator==(const Matrix3x3& rhs) const
        {
            for (size_t row_index = 0; row_index < 3; row_index++)
            {
                for (size_t col_index = 0; col_index < 3; col_index++)
                {
                    if (mMat[row_index][col_index] != rhs.mMat[row_index][col_index])
                        return false;
                }
            }

            return true;
        }
        bool operator!=(const Matrix3x3& rhs) const { return !operator==(rhs); }

        // arithmetic operations
        Matrix3x3 operator+(const Matrix3x3& rhs) const
        {
            Matrix3x3 sum;
            for (size_t row_index = 0; row_index < 3; row_index++)
            {
                for (size_t col_index = 0; col_index < 3; col_index++)
                {
                    sum.mMat[row_index][col_index] = mMat[row_index][col_index] + rhs.mMat[row_index][col_index];
                }
            }
            return sum;
        }
        Matrix3x3 operator-(const Matrix3x3& rhs) const
        {
            Matrix3x3 diff;
            for (size_t row_index = 0; row_index < 3; row_index++)
            {
                for (size_t col_index = 0; col_index < 3; col_index++)
                {
                    diff.mMat[row_index][col_index] = mMat[row_index][col_index] - rhs.mMat[row_index][col_index];
                }
            }
            return diff;
        }
        Matrix3x3 operator*(const Matrix3x3& rhs) const
        {
            Matrix3x3 prod;
            for (size_t row_index = 0; row_index < 3; row_index++)
            {
                for (size_t col_index = 0; col_index < 3; col_index++)
                {
                    prod.mMat[row_index][col_index] = mMat[row_index][0] * rhs.mMat[0][col_index] +
                                                       mMat[row_index][1] * rhs.mMat[1][col_index] +
                                                       mMat[row_index][2] * rhs.mMat[2][col_index];
                }
            }
            return prod;
        }

        // matrix * vector [3x3 * 3x1 = 3x1]
        Vector3 operator*(const Vector3& rhs) const
        {
            Vector3 prod;
            for (size_t row_index = 0; row_index < 3; row_index++)
            {
                prod[row_index] =
                    mMat[row_index][0] * rhs.x + mMat[row_index][1] * rhs.y + mMat[row_index][2] * rhs.z;
            }
            return prod;
        }

        // vector * matrix [1x3 * 3x3 = 1x3]
        friend Vector3 operator*(const Vector3& point, const Matrix3x3& rhs)
        {
            Vector3 prod;
            for (size_t row_index = 0; row_index < 3; row_index++)
            {
                prod[row_index] = point.x * rhs.mMat[0][row_index] + point.y * rhs.mMat[1][row_index] +
                                  point.z * rhs.mMat[2][row_index];
            }
            return prod;
        }

        Matrix3x3 operator-() const
        {
            Matrix3x3 neg;
            for (size_t row_index = 0; row_index < 3; row_index++)
            {
                for (size_t col_index = 0; col_index < 3; col_index++)
                    neg[row_index][col_index] = -mMat[row_index][col_index];
            }
            return neg;
        }

        // matrix * scalar
        Matrix3x3 operator*(float scalar) const
        {
            Matrix3x3 prod;
            for (size_t row_index = 0; row_index < 3; row_index++)
            {
                for (size_t col_index = 0; col_index < 3; col_index++)
                    prod[row_index][col_index] = scalar * mMat[row_index][col_index];
            }
            return prod;
        }

        // scalar * matrix
        friend Matrix3x3 operator*(float scalar, const Matrix3x3& rhs)
        {
            Matrix3x3 prod;
            for (size_t row_index = 0; row_index < 3; row_index++)
            {
                for (size_t col_index = 0; col_index < 3; col_index++)
                    prod[row_index][col_index] = scalar * rhs.mMat[row_index][col_index];
            }
            return prod;
        }
        
        Vector3 GetColumn(size_t col_index) const
        {
            assert(0 <= col_index && col_index < 3);
            return Vector3(mMat[0][col_index], mMat[1][col_index], mMat[2][col_index]);
        }

        void SetColumn(size_t iCol, const Vector3& vec);
        void FromAxes(const Vector3& x_axis, const Vector3& y_axis, const Vector3& z_axis);

        // utilities
        Matrix3x3 Transpose() const
        {
            Matrix3x3 transpose_v;
            for (size_t row_index = 0; row_index < 3; row_index++)
            {
                for (size_t col_index = 0; col_index < 3; col_index++)
                    transpose_v[row_index][col_index] = mMat[col_index][row_index];
            }
            return transpose_v;
        }

        bool Inverse(Matrix3x3& inv_mat, float fTolerance = 1e-06) const
        {
            // Invert a 3x3 using cofactors.  This is about 8 times faster than
            // the Numerical Recipes code which uses Gaussian elimination.

            float det = Determinant();
            if (std::fabs(det) <= fTolerance)
                return false;

            inv_mat[0][0] = mMat[1][1] * mMat[2][2] - mMat[1][2] * mMat[2][1];
            inv_mat[0][1] = mMat[0][2] * mMat[2][1] - mMat[0][1] * mMat[2][2];
            inv_mat[0][2] = mMat[0][1] * mMat[1][2] - mMat[0][2] * mMat[1][1];
            inv_mat[1][0] = mMat[1][2] * mMat[2][0] - mMat[1][0] * mMat[2][2];
            inv_mat[1][1] = mMat[0][0] * mMat[2][2] - mMat[0][2] * mMat[2][0];
            inv_mat[1][2] = mMat[0][2] * mMat[1][0] - mMat[0][0] * mMat[1][2];
            inv_mat[2][0] = mMat[1][0] * mMat[2][1] - mMat[1][1] * mMat[2][0];
            inv_mat[2][1] = mMat[0][1] * mMat[2][0] - mMat[0][0] * mMat[2][1];
            inv_mat[2][2] = mMat[0][0] * mMat[1][1] - mMat[0][1] * mMat[1][0];

            float inv_det = 1.0f / det;
            for (size_t row_index = 0; row_index < 3; row_index++)
            {
                for (size_t col_index = 0; col_index < 3; col_index++)
                    inv_mat[row_index][col_index] *= inv_det;
            }

            return true;
        }

        Matrix3x3 Inverse(float tolerance = 1e-06) const
        {
            Matrix3x3 inv = ZERO;
            Inverse(inv, tolerance);
            return inv;
        }

        float Determinant() const
        {
            float cofactor00 = mMat[1][1] * mMat[2][2] - mMat[1][2] * mMat[2][1];
            float cofactor10 = mMat[1][2] * mMat[2][0] - mMat[1][0] * mMat[2][2];
            float cofactor20 = mMat[1][0] * mMat[2][1] - mMat[1][1] * mMat[2][0];

            float det = mMat[0][0] * cofactor00 + mMat[0][1] * cofactor10 + mMat[0][2] * cofactor20;

            return det;
        }

        void CalculateQDUDecomposition(Matrix3x3& out_Q, Vector3& out_D, Vector3& out_U) const;

        // matrix must be orthonormal
        void ToAngleAxis(Vector3& axis, Radian& angle) const;
        void ToAngleAxis(Vector3& axis, Degree& angle) const
        {
            Radian r;
            ToAngleAxis(axis, r);
            angle = r;
        }
        void FromAngleAxis(const Vector3& axis, const Radian& radian);

        static Matrix3x3 Scale(const Vector3& scale)
        {
            Matrix3x3 mat = ZERO;

            mat.mMat[0][0] = scale.x;
            mat.mMat[1][1] = scale.y;
            mat.mMat[2][2] = scale.z;

            return mat;
        }

        static const Matrix3x3 ZERO;
        static const Matrix3x3 IDENTITY;

    public:
        float mMat[3][3];
    };
}