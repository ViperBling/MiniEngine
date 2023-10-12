#include "RenderHelper.hpp"
#include "MRuntime/Function/Render/RenderCamera.hpp"
#include "MRuntime/Function/Render/RenderScene.hpp"
#include "MRuntime/Core/Math/Matrix4.hpp"

namespace MiniEngine
{
    ClusterFrustum CreateClusterFrustumFromMatrix(
        Matrix4x4 mat,
        float     x_left,
        float     x_right,
        float     y_top,
        float     y_bottom,
        float     z_near,
        float     z_far)
    {
        ClusterFrustum f;

        // the following is in the vulkan space
        // note that the Y axis is flipped in Vulkan
        assert(y_top < y_bottom);

        assert(x_left < x_right);
        assert(z_near < z_far);

        // calculate the tiled frustum
        // [Fast Extraction of Viewing Frustum Planes from the WorldView - Projection
        // Matrix](http://gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf)
        
        Matrix4x4 mat_column = mat;

        // [vec.xyz 1][mat.col0] / [vec.xyz 1][mat.col3] > x_right
        // [vec.xyz 1][mat.col0 - mat.col3*x_right] > 0
        f.mPlaneRight = Vector4(mat_column[0]) - (Vector4(mat_column[3]) * x_right);
        // normalize
        f.mPlaneRight *= (1.0 / Vector3(f.mPlaneRight.x, f.mPlaneRight.y, f.mPlaneRight.z).Length());

        // for example, we try to calculate the "plane_left" of the tile frustum
        // note that we use the row vector to be consistent with the DirectXMath
        // [vec.xyz 1][mat.col0] / [vec.xyz 1][mat.col3] < x_left
        //
        // evidently, the value "[vec.xyz 1][mat.col3]" is expected to be greater than
        // 0 (w of clip space) and we multiply both sides by "[vec.xyz 1][mat.col3]",
        // the inequality symbol remains the same [vec.xyz 1][mat.col0] < [vec.xyz
        // 1][mat.col3]*x_left
        //
        // since "x_left" is a scalar, the "scalar multiplication" is applied
        // [vec.xyz 1][mat.col0] < [vec.xyz 1][mat.col3*x_left]
        // [vec.xyz 1][mat.col0 - mat.col3*x_left] < 0
        //
        // we follow the "DirectX::BoundingFrustum::Intersects", the normal of the
        // plane is pointing ourward [vec.xyz 1][mat.col3*x_left - mat.col0] > 0
        //
        // the plane can be defined as [x y z 1][A B C D] = 0 and the [A B C D] is
        // exactly [mat.col0 - mat.col3*x_left] and we need to normalize the normal[A
        // B C] of the plane let [A B C D] = [mat.col3*x_left - mat.col0] [A B C D] /=
        // length([A B C].xyz)
        f.mPlaneLeft = (Vector4(mat_column[3]) * x_left) - Vector4(mat_column[0]);
        // normalize
        f.mPlaneLeft *= (1.0 / Vector3(f.mPlaneLeft.x, f.mPlaneLeft.y, f.mPlaneLeft.z).Length());

        // [vec.xyz 1][mat.col1] / [vec.xyz 1][mat.col3] < y_top
        // [vec.xyz 1][mat.col3*y_top - mat.col1] > 0
        f.mPlaneTop = (Vector4(mat_column[3]) * y_top) - Vector4(mat_column[1]);
        // normalize
        f.mPlaneTop *= (1.0 / Vector3(f.mPlaneTop.x, f.mPlaneTop.y, f.mPlaneTop.z).Length());

        // [vec.xyz 1][mat.col1] / [vec.xyz 1][mat.col3] > y_bottom
        // [vec.xyz 1][mat.col1 - mat.col3*y_bottom] > 0
        f.mPlaneBottom = Vector4(mat_column[1]) - (Vector4(mat_column[3]) * y_bottom);
        // normalize
        f.mPlaneBottom *= (1.0 / Vector3(f.mPlaneBottom.x, f.mPlaneBottom.y, f.mPlaneBottom.z).Length());

        // [vec.xyz 1][mat.col2] / [vec.xyz 1][mat.col3] < z_near
        // [vec.xyz 1][mat.col3*z_near - mat.col2] > 0
        f.mPlaneNear = (Vector4(mat_column[3]) * z_near) - Vector4(mat_column[2]);
        f.mPlaneNear *= (1.0 / Vector3(f.mPlaneNear.x, f.mPlaneNear.y, f.mPlaneNear.z).Length());

        // [vec.xyz 1][mat.col2] / [vec.xyz 1][mat.col3] > z_far
        // [vec.xyz 1][mat.col2 - mat.col3*z_far] > 0
        f.mPlaneFar = Vector4(mat_column[2]) - (Vector4(mat_column[3]) * z_far);
        f.mPlaneFar *= (1.0 / Vector3(f.mPlaneFar.x, f.mPlaneFar.y, f.mPlaneFar.z).Length());

        return f;
    }

    bool TiledFrustumIntersectBox(ClusterFrustum const& f, BoundingBox const& b)
    {
        // we follow the "DirectX::BoundingFrustum::Intersects"

        // Center of the box.
        Vector4 box_center((b.mMaxBound.x + b.mMinBound.x) * 0.5,
                             (b.mMaxBound.y + b.mMinBound.y) * 0.5,
                             (b.mMaxBound.z + b.mMinBound.z) * 0.5,
                             1.0);

        // Distance from the center to each side.
        // half extent //more exactly
        Vector3 box_extents((b.mMaxBound.x - b.mMinBound.x) * 0.5,
                              (b.mMaxBound.y - b.mMinBound.y) * 0.5,
                              (b.mMaxBound.z - b.mMinBound.z) * 0.5);

        // plane_right
        {
            float signed_distance_fromPlaneRight = f.mPlaneRight.DotProduct(box_center);
            float radius_project_plane_right =
                Vector3(fabs(f.mPlaneRight.x), fabs(f.mPlaneRight.y), fabs(f.mPlaneRight.z)).DotProduct(box_extents);

            bool intersecting_or_inside_right = signed_distance_fromPlaneRight < radius_project_plane_right;
            if (!intersecting_or_inside_right)
            {
                return false;
            }
        }

        // plane_left
        {
            float signed_distance_fromPlaneLeft = f.mPlaneLeft.DotProduct(box_center);
            float radius_project_plane_left =
                Vector3(fabs(f.mPlaneLeft.x), fabs(f.mPlaneLeft.y), fabs(f.mPlaneLeft.z)).DotProduct(box_extents);

            bool intersecting_or_inside_left = signed_distance_fromPlaneLeft < radius_project_plane_left;
            if (!intersecting_or_inside_left)
            {
                return false;
            }
        }

        // plane_top
        {
            float signed_distance_fromPlaneTop = f.mPlaneTop.DotProduct(box_center);
            float radius_project_plane_top =
                Vector3(fabs(f.mPlaneTop.x), fabs(f.mPlaneTop.y), fabs(f.mPlaneTop.z)).DotProduct(box_extents);

            bool intersecting_or_inside_top = signed_distance_fromPlaneTop < radius_project_plane_top;
            if (!intersecting_or_inside_top)
            {
                return false;
            }
        }

        // plane_bottom
        {
            float signed_distance_fromPlaneBottom = f.mPlaneBottom.DotProduct(box_center);
            float radius_project_plane_bottom =
                Vector3(fabs(f.mPlaneBottom.x), fabs(f.mPlaneBottom.y), fabs(f.mPlaneBottom.z)).DotProduct(box_extents);

            bool intersecting_or_inside_bottom = signed_distance_fromPlaneBottom < radius_project_plane_bottom;
            if (!intersecting_or_inside_bottom)
            {
                return false;
            }
        }

        // plane_near
        {
            float signed_distance_fromPlaneNear = f.mPlaneNear.DotProduct(box_center);
            float radius_project_plane_near =
                Vector3(fabs(f.mPlaneNear.x), fabs(f.mPlaneNear.y), fabs(f.mPlaneNear.z)).DotProduct(box_extents);

            bool intersecting_or_inside_near = signed_distance_fromPlaneNear < radius_project_plane_near;
            if (!intersecting_or_inside_near)
            {
                return false;
            }
        }

        // plane_far
        {
            float signed_distance_fromPlaneFar = f.mPlaneFar.DotProduct(box_center);
            float radius_project_plane_far =
                Vector3(fabs(f.mPlaneFar.x), fabs(f.mPlaneFar.y), fabs(f.mPlaneFar.z)).DotProduct(box_extents);

            bool intersecting_or_inside_far = signed_distance_fromPlaneFar < radius_project_plane_far;
            if (!intersecting_or_inside_far)
            {
                return false;
            }
        }

        return true;
    }

    BoundingBox BoundingBoxTransform(BoundingBox const& b, Matrix4x4 const& m)
    {
        // we follow the "BoundingBox::Transform"

        Vector3 const g_BoxOffset[8] = {Vector3(-1.0f, -1.0f, 1.0f),
                                          Vector3(1.0f, -1.0f, 1.0f),
                                          Vector3(1.0f, 1.0f, 1.0f),
                                          Vector3(-1.0f, 1.0f, 1.0f),
                                          Vector3(-1.0f, -1.0f, -1.0f),
                                          Vector3(1.0f, -1.0f, -1.0f),
                                          Vector3(1.0f, 1.0f, -1.0f),
                                          Vector3(-1.0f, 1.0f, -1.0f)};

        size_t const CORNER_COUNT = 8;

        // Load center and extents.
        // Center of the box.
        Vector3 center((b.mMaxBound.x + b.mMinBound.x) * 0.5,
                         (b.mMaxBound.y + b.mMinBound.y) * 0.5,
                         (b.mMaxBound.z + b.mMinBound.z) * 0.5);

        // Distance from the center to each side.
        // half extent //more exactly
        Vector3 extents((b.mMaxBound.x - b.mMinBound.x) * 0.5,
                          (b.mMaxBound.y - b.mMinBound.y) * 0.5,
                          (b.mMaxBound.z - b.mMinBound.z) * 0.5);

        Vector3 min;
        Vector3 max;

        // Compute and transform the corners and find new min/max bounds.
        for (size_t i = 0; i < CORNER_COUNT; ++i)
        {
            Vector3 corner_before = extents * g_BoxOffset[i] + center;
            Vector4 corner_with_w = m * Vector4(corner_before.x, corner_before.y, corner_before.z, 1.0);
            Vector3 corner        = Vector3(corner_with_w.x / corner_with_w.w,
                                         corner_with_w.y / corner_with_w.w,
                                         corner_with_w.z / corner_with_w.w);

            if (0 == i)
            {
                min = corner;
                max = corner;
            }
            else
            {
                min = Vector3(Math::Min(min[0], corner[0]), Math::Min(min[1], corner[1]), Math::Min(min[2], corner[2]));
                max = Vector3(Math::Max(max[0], corner[0]), Math::Max(max[1], corner[1]), Math::Max(max[2], corner[2]));
            }
        }

        BoundingBox b_out;
        b_out.mMaxBound = max;
        b_out.mMinBound = min;
        return b_out;
    }

    bool BoxIntersectsWithSphere(BoundingBox const& b, BoundingSphere const& s)
    {
        for (size_t i = 0; i < 3; ++i)
        {
            if (s.mCenter[i] < b.mMinBound[i])
            {
                if ((b.mMinBound[i] - s.mCenter[i]) > s.mRadius)
                {
                    return false;
                }
            }
            else if (s.mCenter[i] > b.mMaxBound[i])
            {
                if ((s.mCenter[i] - b.mMaxBound[i]) > s.mRadius)
                {
                    return false;
                }
            }
        }

        return true;
    }

    Matrix4x4 CalculateDirectionalLightCamera(RenderScene& scene, RenderCamera& camera)
    {
        Matrix4x4 proj_view_matrix;
        {
            Matrix4x4 view_matrix = camera.GetViewMatrix();
            Matrix4x4 proj_matrix = camera.GetPersProjMatrix();
            proj_view_matrix      = proj_matrix * view_matrix;
        }

        BoundingBox frustum_bounding_box;
        // CascadedShadowMaps11 / CreateFrustumPointsFromCascadeInterval
        {
            Vector3 const g_frustum_points_ndc_space[8] = {Vector3(-1.0f, -1.0f, 1.0f),
                                                             Vector3(1.0f, -1.0f, 1.0f),
                                                             Vector3(1.0f, 1.0f, 1.0f),
                                                             Vector3(-1.0f, 1.0f, 1.0f),
                                                             Vector3(-1.0f, -1.0f, 0.0f),
                                                             Vector3(1.0f, -1.0f, 0.0f),
                                                             Vector3(1.0f, 1.0f, 0.0f),
                                                             Vector3(-1.0f, 1.0f, 0.0f)};

            Matrix4x4 inverse_proj_view_matrix = proj_view_matrix.Inverse();

            frustum_bounding_box.mMinBound = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
            frustum_bounding_box.mMaxBound = Vector3(FLT_MIN, FLT_MIN, FLT_MIN);

            size_t const CORNER_COUNT = 8;
            for (size_t i = 0; i < CORNER_COUNT; ++i)
            {
                Vector4 frustum_point_with_w = inverse_proj_view_matrix * Vector4(g_frustum_points_ndc_space[i].x,
                                                                                      g_frustum_points_ndc_space[i].y,
                                                                                      g_frustum_points_ndc_space[i].z,
                                                                                      1.0);
                Vector3   frustum_point        = Vector3(frustum_point_with_w.x / frustum_point_with_w.w,
                                                frustum_point_with_w.y / frustum_point_with_w.w,
                                                frustum_point_with_w.z / frustum_point_with_w.w);

                frustum_bounding_box.Merge(frustum_point);
            }
        }

        BoundingBox scene_bounding_box;
        {
            scene_bounding_box.mMinBound = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
            scene_bounding_box.mMaxBound = Vector3(FLT_MIN, FLT_MIN, FLT_MIN);

            for (const RenderEntity& entity : scene.mRenderEntities)
            {
                BoundingBox mesh_asset_bounding_box {entity.mBoundingBox.GetMinCorner(),
                                                     entity.mBoundingBox.GetMaxCorner()};

                BoundingBox mesh_bounding_box_world =
                    BoundingBoxTransform(mesh_asset_bounding_box, entity.mModelMatrix);
                scene_bounding_box.Merge(mesh_bounding_box_world);
            }
        }

        // CascadedShadowMaps11 / ComputeNearAndFar
        Matrix4x4 light_view;
        Matrix4x4 light_proj;
        {
            Vector3 box_center((frustum_bounding_box.mMaxBound.x + frustum_bounding_box.mMinBound.x) * 0.5,
                                 (frustum_bounding_box.mMaxBound.y + frustum_bounding_box.mMinBound.y) * 0.5,
                                 (frustum_bounding_box.mMaxBound.z + frustum_bounding_box.mMinBound.z));
            Vector3 box_extents((frustum_bounding_box.mMaxBound.x - frustum_bounding_box.mMinBound.x) * 0.5,
                                  (frustum_bounding_box.mMaxBound.y - frustum_bounding_box.mMinBound.y) * 0.5,
                                  (frustum_bounding_box.mMaxBound.z - frustum_bounding_box.mMinBound.z) * 0.5);

            Vector3 eye =
                box_center + scene.mDirectionalLight.mDirection * box_extents.Length();
            Vector3 center = box_center;
            light_view       = Math::MakeLookAtMatrix(eye, center, Vector3(0.0, 0.0, 1.0));

            BoundingBox frustum_bounding_box_light_view = BoundingBoxTransform(frustum_bounding_box, light_view);
            BoundingBox scene_bounding_box_light_view   = BoundingBoxTransform(scene_bounding_box, light_view);
            light_proj = Math::MakeOrthographicProjectionMatrix01(
                std::max(frustum_bounding_box_light_view.mMinBound.x, scene_bounding_box_light_view.mMinBound.x),
                std::min(frustum_bounding_box_light_view.mMaxBound.x, scene_bounding_box_light_view.mMaxBound.x),
                std::max(frustum_bounding_box_light_view.mMinBound.y, scene_bounding_box_light_view.mMinBound.y),
                std::min(frustum_bounding_box_light_view.mMaxBound.y, scene_bounding_box_light_view.mMaxBound.y),
                -scene_bounding_box_light_view.mMaxBound
                     .z, // the objects which are nearer than the frustum bounding box may caster shadow as well
                -std::max(frustum_bounding_box_light_view.mMinBound.z, scene_bounding_box_light_view.mMinBound.z));
        }

        Matrix4x4 light_proj_view = (light_proj * light_view);
        return light_proj_view;
    }
} // namespace MiniEngine
