#include "RenderCamera.hpp"

namespace MiniEngine
{
    void RenderCamera::SetCurrentCameraType(RenderCameraType type)
    {
        std::lock_guard<std::mutex> lock_guard(mViewMatrixMutex);
        mCurrentCameraType = type;
    }

    void RenderCamera::SetMainViewMatrix(const Matrix4x4 &view_matrix, RenderCameraType type)
    {
        std::lock_guard<std::mutex> lock_guard(mViewMatrixMutex);
        mCurrentCameraType                   = type;
        mViewMatrices[MAIN_VIEW_MATRIX_INDEX] = view_matrix;

        Vector3 s  = Vector3(view_matrix[0][0], view_matrix[0][1], view_matrix[0][2]);
        Vector3 u  = Vector3(view_matrix[1][0], view_matrix[1][1], view_matrix[1][2]);
        Vector3 f  = Vector3(-view_matrix[2][0], -view_matrix[2][1], -view_matrix[2][2]);
        mPosition = s * (-view_matrix[0][3]) + u * (-view_matrix[1][3]) + f * view_matrix[2][3];
    }

    void RenderCamera::Move(Vector3 delta)
    {
        mPosition += delta;
    }

    void RenderCamera::Rotate(Vector2 delta)
    {
        // rotation around x, y axis
        delta = Vector2(Radian(Degree(delta.x)).ValueRadians(), Radian(Degree(delta.y)).ValueRadians());

        // limit pitch
        float dot = mUpAxis.DotProduct(Forward());
        if ((dot < -0.99f && delta.x > 0.0f) || // angle nearing 180 degrees
            (dot > 0.99f && delta.x < 0.0f))    // angle nearing 0 degrees
            delta.x = 0.0f;

        // pitch is relative to current sideways rotation
        // yaw happens independently
        // this prevents roll
        Quaternion pitch, yaw;
        pitch.FromAngleAxis(Radian(delta.x), X);
        yaw.FromAngleAxis(Radian(delta.y), Z);

        mRotation = pitch * mRotation * yaw;

        mInvRotation = mRotation.Conjugate();
    }

    void RenderCamera::Zoom(float offset)
    {
        // > 0 = zoom in (decrease FOV by <offset> angles)
        mFovX = Math::Clamp(mFovX - offset, MIN_FOV, MAX_FOV);
    }

    void RenderCamera::LookAt(const Vector3 &position, const Vector3 &target, const Vector3 &up)
    {
        mPosition = position;

        // model rotation
        // maps vectors to camera space (x, y, z)
        Vector3 Forward = (target - position).NormalizedCopy();
        mRotation      = Forward.GetRotationTo(Y);

        // correct the up vector
        // the cross product of non-orthogonal vectors is not normalized
        Vector3 right  = Forward.CrossProduct(up.NormalizedCopy()).NormalizedCopy();
        Vector3 orthUp = right.CrossProduct(Forward);

        Quaternion upRotation = (mRotation * orthUp).GetRotationTo(Z);

        mRotation = Quaternion(upRotation) * mRotation;

        // inverse of the model rotation
        // maps camera space vectors to model vectors
        mInvRotation = mRotation.Conjugate();
    }

    void RenderCamera::SetAspect(float aspect)
    {
        mAspect = aspect;

        // 1 / tan(fovy * 0.5) / aspect = 1 / tan(fovx * 0.5)
        // 1 / tan(fovy * 0.5) = aspect / tan(fovx * 0.5)
        // tan(fovy * 0.5) = tan(fovx * 0.5) / aspect

        mFovY = Radian(Math::Atan(Math::Tan(Radian(Degree(mFovX) * 0.5f)) / mAspect) * 2.0f).ValueDegrees();
    }

    Matrix4x4 RenderCamera::GetViewMatrix()
    {
        std::lock_guard<std::mutex> lock_guard(mViewMatrixMutex);
        auto                        view_matrix = Matrix4x4::IDENTITY;
        switch (mCurrentCameraType)
        {
            case RenderCameraType::Editor:
                view_matrix = Math::MakeLookAtMatrix(Position(), Position() + Forward(), Up());
                break;
            case RenderCameraType::Motor:
                view_matrix = mViewMatrices[MAIN_VIEW_MATRIX_INDEX];
                break;
            default:
                break;
        }
        return view_matrix;
    }

    Matrix4x4 RenderCamera::GetPersProjMatrix() const
    {
        Matrix4x4 fix_mat(1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
        Matrix4x4 proj_mat = fix_mat * Math::MakePerspectiveMatrix(Radian(Degree(mFovY)), mAspect, mZNear, mZFar);

        return proj_mat;
    }
}