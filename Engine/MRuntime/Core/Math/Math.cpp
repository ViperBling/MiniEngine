#include "Math.hpp"
#include "MRuntime/Core/Math/Matrix4.hpp"

namespace MiniEngine
{
    Math::AngleUnit Math::mkAngleUnit;

    Math::Math() { mkAngleUnit = AngleUnit::AU_DEGREE; }

    bool Math::RealEqual(float a, float b, float tolerance /* = std::numeric_limits<float>::epsilon() */)
    {
        return std::fabs(b - a) <= tolerance;
    }

    float Math::DegreesToRadians(float degrees) { return degrees * MATH_fDeg2Rad; }

    float Math::RadiansToDegrees(float radians) { return radians * MATH_fRad2Deg; }

    float Math::AngleUnitsToRadians(float angleunits)
    {
        if (mkAngleUnit == AngleUnit::AU_DEGREE)
            return angleunits * MATH_fDeg2Rad;

        return angleunits;
    }

    float Math::RadiansToAngleUnits(float radians)
    {
        if (mkAngleUnit == AngleUnit::AU_DEGREE)
            return radians * MATH_fRad2Deg;

        return radians;
    }

    float Math::AngleUnitsToDegrees(float angleunits)
    {
        if (mkAngleUnit == AngleUnit::AU_RADIAN)
            return angleunits * MATH_fRad2Deg;

        return angleunits;
    }

    float Math::DegreesToAngleUnits(float degrees)
    {
        if (mkAngleUnit == AngleUnit::AU_RADIAN)
            return degrees * MATH_fDeg2Rad;

        return degrees;
    }

    Radian Math::Acos(float value)
    {
        if (-1.0 < value)
        {
            if (value < 1.0)
                return Radian(::acos(value));

            return Radian(0.0);
        }

        return Radian(MATH_PI);
    }
    //-----------------------------------------------------------------------
    Radian Math::Asin(float value)
    {
        if (-1.0 < value)
        {
            if (value < 1.0)
                return Radian(::asin(value));

            return Radian(MATH_HALF_PI);
        }

        return Radian(-MATH_HALF_PI);
    }

    Matrix4x4 Math::MakeViewMatrix(const Vector3& position, const Quaternion& orientation, const Matrix4x4* reflect_matrix)
    {
        Matrix4x4 viewMatrix;

        // View matrix is:
        //
        //  [ Lx  Uy  Dz  Tx  ]
        //  [ Lx  Uy  Dz  Ty  ]
        //  [ Lx  Uy  Dz  Tz  ]
        //  [ 0   0   0   1   ]
        //
        // Where T = -(Transposed(Rot) * Pos)

        // This is most efficiently done using 3x3 Matrices
        Matrix3x3 rot;
        orientation.ToRotationMatrix(rot);

        // Make the translation relative to new axes
        Matrix3x3 rotT  = rot.Transpose();
        Vector3   trans = -rotT * position;

        // Make final matrix
        viewMatrix = Matrix4x4::IDENTITY;
        viewMatrix.SetMatrix3x3(rotT); // fills upper 3x3
        viewMatrix[0][3] = trans.x;
        viewMatrix[1][3] = trans.y;
        viewMatrix[2][3] = trans.z;

        // Deal with reflections
        if (reflect_matrix)
        {
            viewMatrix = viewMatrix * (*reflect_matrix);
        }

        return viewMatrix;
    }

    Matrix4x4 Math::MakeLookAtMatrix(const Vector3& eye_position, const Vector3& target_position, const Vector3& up_dir)
    {
        const Vector3& up = up_dir.NormalizedCopy();

        Vector3 f = (target_position - eye_position).NormalizedCopy();
        Vector3 s = f.CrossProduct(up).NormalizedCopy();
        Vector3 u = s.CrossProduct(f);

        Matrix4x4 view_mat = Matrix4x4::IDENTITY;

        view_mat[0][0] = s.x;
        view_mat[0][1] = s.y;
        view_mat[0][2] = s.z;
        view_mat[0][3] = -s.DotProduct(eye_position);
        view_mat[1][0] = u.x;
        view_mat[1][1] = u.y;
        view_mat[1][2] = u.z;
        view_mat[1][3] = -u.DotProduct(eye_position);
        view_mat[2][0] = -f.x;
        view_mat[2][1] = -f.y;
        view_mat[2][2] = -f.z;
        view_mat[2][3] = f.DotProduct(eye_position);
        return view_mat;
    }

    Matrix4x4 Math::MakePerspectiveMatrix(Radian fovy, float aspect, float znear, float zfar)
    {
        float tan_half_fovy = Math::Tan(fovy / 2.f);

        Matrix4x4 ret = Matrix4x4::ZERO;
        ret[0][0]     = 1.f / (aspect * tan_half_fovy);
        ret[1][1]     = 1.f / tan_half_fovy;
        ret[2][2]     = zfar / (znear - zfar);
        ret[3][2]     = -1.f;
        ret[2][3]     = -(zfar * znear) / (zfar - znear);

        return ret;
    }

    Matrix4x4 Math::MakeOrthographicProjectionMatrix(float left, float right, float bottom, float top, float znear, float zfar)
    {
        float inv_width    = 1.0f / (right - left);
        float inv_height   = 1.0f / (top - bottom);
        float inv_distance = 1.0f / (zfar - znear);

        float A  = 2 * inv_width;
        float B  = 2 * inv_height;
        float C  = -(right + left) * inv_width;
        float D  = -(top + bottom) * inv_height;
        float q  = -2 * inv_distance;
        float qn = -(zfar + znear) * inv_distance;

        // NB: This creates 'uniform' orthographic projection matrix,
        // which depth range [-1,1], right-handed rules
        //
        // [ A   0   0   C  ]
        // [ 0   B   0   D  ]
        // [ 0   0   q   qn ]
        // [ 0   0   0   1  ]
        //
        // A = 2 * / (right - left)
        // B = 2 * / (top - bottom)
        // C = - (right + left) / (right - left)
        // D = - (top + bottom) / (top - bottom)
        // q = - 2 / (far - near)
        // qn = - (far + near) / (far - near)

        Matrix4x4 proj_matrix = Matrix4x4::ZERO;
        proj_matrix[0][0]     = A;
        proj_matrix[0][3]     = C;
        proj_matrix[1][1]     = B;
        proj_matrix[1][3]     = D;
        proj_matrix[2][2]     = q;
        proj_matrix[2][3]     = qn;
        proj_matrix[3][3]     = 1;

        return proj_matrix;
    }

    Matrix4x4 Math::MakeOrthographicProjectionMatrix01(float left, float right, float bottom, float top, float znear, float zfar)
    {
        float inv_width    = 1.0f / (right - left);
        float inv_height   = 1.0f / (top - bottom);
        float inv_distance = 1.0f / (zfar - znear);

        float A  = 2 * inv_width;
        float B  = 2 * inv_height;
        float C  = -(right + left) * inv_width;
        float D  = -(top + bottom) * inv_height;
        float q  = -1 * inv_distance;
        float qn = -znear * inv_distance;

        // NB: This creates 'uniform' orthographic projection matrix,
        // which depth range [-1,1], right-handed rules
        //
        // [ A   0   0   C  ]
        // [ 0   B   0   D  ]
        // [ 0   0   q   qn ]
        // [ 0   0   0   1  ]
        //
        // A = 2 * / (right - left)
        // B = 2 * / (top - bottom)
        // C = - (right + left) / (right - left)
        // D = - (top + bottom) / (top - bottom)
        // q = - 1 / (far - near)
        // qn = - near / (far - near)

        Matrix4x4 proj_matrix = Matrix4x4::ZERO;
        proj_matrix[0][0]     = A;
        proj_matrix[0][3]     = C;
        proj_matrix[1][1]     = B;
        proj_matrix[1][3]     = D;
        proj_matrix[2][2]     = q;
        proj_matrix[2][3]     = qn;
        proj_matrix[3][3]     = 1;

        return proj_matrix;
    }
}