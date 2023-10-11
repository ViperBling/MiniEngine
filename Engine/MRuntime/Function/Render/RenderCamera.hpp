#pragma once

#include "MRuntime/Core/Math/MathHeaders.hpp"

#include <mutex>

namespace MiniEngine
{
    enum class RenderCameraType : int
    {
        Editor,
        Motor
    };

    class RenderCamera
    {
    public:
        void SetCurrentCameraType(RenderCameraType type);
        void SetMainViewMatrix(const Matrix4x4& view_matrix, RenderCameraType type = RenderCameraType::Editor);

        void Move(Vector3 delta);
        void Rotate(Vector2 delta);
        void Zoom(float offset);
        void LookAt(const Vector3& position, const Vector3& target, const Vector3& up);

        void SetAspect(float aspect);
        void SetFovX(float fovx) { mFovX = fovx; }

        Vector3    Position() const { return mPosition; }
        Quaternion Rotation() const { return mRotation; }

        Vector3   Forward() const { return (mInvRotation * Y); }
        Vector3   Up() const { return (mInvRotation * Z); }
        Vector3   Right() const { return (mInvRotation * X); }
        Vector2   GetFOV() const { return {mFovX, mFovY}; }
        Matrix4x4 GetViewMatrix();
        Matrix4x4 GetPersProjMatrix() const;
        Matrix4x4 GetLookAtMatrix() const { return Math::MakeLookAtMatrix(Position(), Position() + Forward(), Up()); }
        float     GetFovYDeprecated() const { return mFovY; }

    public:
        RenderCameraType mCurrentCameraType {RenderCameraType::Editor};

        static const Vector3 X, Y, Z;

        Vector3    mPosition {0.0f, 0.0f, 0.0f};
        Quaternion mRotation {Quaternion::IDENTITY};
        Quaternion mInvRotation {Quaternion::IDENTITY};
        float      mZNear {1000.0f};
        float      mZFar {0.1f};
        Vector3    mUpAxis {Z};

        static constexpr float MIN_FOV {10.0f};
        static constexpr float MAX_FOV {89.0f};
        static constexpr int   MAIN_VIEW_MATRIX_INDEX {0};

        std::vector<Matrix4x4> mViewMatrices {Matrix4x4::IDENTITY};

    protected:
        float mAspect {0.f};
        float mFovX {Degree(89.f).ValueDegrees()};
        float mFovY {0.f};

        std::mutex mViewMatrixMutex;
    };

    inline const Vector3 RenderCamera::X = {1.0f, 0.0f, 0.0f};
    inline const Vector3 RenderCamera::Y = {0.0f, 1.0f, 0.0f};
    inline const Vector3 RenderCamera::Z = {0.0f, 0.0f, 1.0f};

} // namespace Piccolo
