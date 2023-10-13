#include "DebugDrawGroup.hpp"

#include "MRuntime/Function/Global/GlobalContext.hpp"
#include "MRuntime/Function/Render/RenderSystem.hpp"

#include <vector>

namespace MiniEngine
{
    DebugDrawGroup::~DebugDrawGroup()
    {
        Clear();
    }

    void DebugDrawGroup::Initialize()
    {
    }

    void DebugDrawGroup::Clear()
    {
        std::lock_guard<std::mutex> guard(mMutex);
        ClearData();
    }

    void DebugDrawGroup::ClearData()
    {
        mPoints.clear();
        mLines.clear();
        mTriangles.clear();
        mQuads.clear();
        mBoxes.clear();
        mCylinders.clear();
        mSpheres.clear();
        mCapsules.clear();
        mTexts.clear();
    }

    void DebugDrawGroup::SetName(const std::string &name)
    {
        mName = name;
    }

    const std::string &DebugDrawGroup::GetName() const
    {
        return mName;
    }

    void DebugDrawGroup::AddPoint(const Vector3 &position, const Vector4 &color, const float life_time, const bool no_depth_test)
    {
        std::lock_guard<std::mutex> guard(mMutex);
        DebugDrawPoint point;
        point.mVertex.mColor = color;
        point.SetTime(life_time);
        point.mFillMode = FillMode::Wireframe;
        point.mVertex.mPos = position;
        point.mbNoDepthTest = no_depth_test;
        mPoints.push_back(point);
    }

    void DebugDrawGroup::AddLine(const Vector3 &point0, const Vector3 &point1, const Vector4 &color0, const Vector4 &color1, const float life_time, const bool no_depth_test)
    {
        std::lock_guard<std::mutex> guard(mMutex);
        DebugDrawLine line;
        line.SetTime(life_time);
        line.mFillMode = FillMode::Wireframe;
        line.mbNoDepthTest = no_depth_test;

        line.mVertex[0].mPos   = point0;
        line.mVertex[0].mColor = color0;

        line.mVertex[1].mPos   = point1;
        line.mVertex[1].mColor = color1;

        mLines.push_back(line);
    }

    void DebugDrawGroup::AddTriangle(const Vector3 &point0, const Vector3 &point1, const Vector3 &point2, const Vector4 &color0, const Vector4 &color1, const Vector4 &color2, const float life_time, const bool no_depth_test, const FillMode fillmod)
    {
        std::lock_guard<std::mutex> guard(mMutex);
        DebugDrawTriangle triangle;
        triangle.SetTime(life_time);
        triangle.mFillMode = fillmod;
        triangle.mbNoDepthTest = no_depth_test;

        triangle.mVertex[0].mPos   = point0;
        triangle.mVertex[0].mColor = color0;

        triangle.mVertex[1].mPos   = point1;
        triangle.mVertex[1].mColor = color1;

        triangle.mVertex[2].mPos   = point2;
        triangle.mVertex[2].mColor = color2;
        
        mTriangles.push_back(triangle);
    }
    
    void DebugDrawGroup::AddQuad(const Vector3 &point0, const Vector3 &point1, const Vector3 &point2, const Vector3 &point3, const Vector4 &color0, const Vector4 &color1, const Vector4 &color2, const Vector4 &color3, const float life_time, const bool no_depth_test, const FillMode fillmode)
    {
        std::lock_guard<std::mutex> guard(mMutex);
        if (fillmode == FillMode::Wireframe)
        {
            DebugDrawQuad quad;

            quad.mVertex[0].mPos   = point0;
            quad.mVertex[0].mColor = color0;

            quad.mVertex[1].mPos   = point1;
            quad.mVertex[1].mColor = color1;
            
            quad.mVertex[2].mPos   = point2;
            quad.mVertex[2].mColor = color2;
            
            quad.mVertex[3].mPos   = point3;
            quad.mVertex[3].mColor = color3;

            quad.SetTime(life_time);
            quad.mbNoDepthTest = no_depth_test;

            mQuads.push_back(quad);
        }
        else
        {
            DebugDrawTriangle triangle;
            triangle.SetTime(life_time);
            triangle.mFillMode         = FillMode::Solid;
            triangle.mbNoDepthTest   = no_depth_test;

            triangle.mVertex[0].mPos     = point0;
            triangle.mVertex[0].mColor   = color0;
            triangle.mVertex[1].mPos     = point1;
            triangle.mVertex[1].mColor   = color1;
            triangle.mVertex[2].mPos     = point2;
            triangle.mVertex[2].mColor   = color2;
            mTriangles.push_back(triangle);

            triangle.mVertex[0].mPos     = point0;
            triangle.mVertex[0].mColor = color0;
            triangle.mVertex[1].mPos     = point2;
            triangle.mVertex[1].mColor = color2;
            triangle.mVertex[2].mPos     = point3;
            triangle.mVertex[2].mColor = color3;
            mTriangles.push_back(triangle);
        }
    }

    void DebugDrawGroup::AddBox(const Vector3 &center_point, const Vector3 &half_extends, const Vector4 &rotate, const Vector4 &color, const float life_time, const bool no_depth_test)
    {
        std::lock_guard<std::mutex> guard(mMutex);
        DebugDrawBox box;
        box.mCenterPoint = center_point;
        box.mHalfExtents = half_extends;
        box.mRotation = rotate;
        box.mColor = color;
        box.mbNoDepthTest = no_depth_test;
        box.SetTime(life_time);

        mBoxes.push_back(box);
    }

    void DebugDrawGroup::AddSphere(const Vector3 &center, const float radius, const Vector4 &color, const float life_time, const bool no_depth_test)
    {
        std::lock_guard<std::mutex> guard(mMutex);
        DebugDrawSphere sphere;
        sphere.mCenter = center;
        sphere.mRadius = radius;
        sphere.mColor = color;
        sphere.mbNoDepthTest = no_depth_test;
        sphere.SetTime(life_time);

        mSpheres.push_back(sphere);
    }

    void DebugDrawGroup::AddCylinder(const Vector3 &center, const float radius, const float height, const Vector4 &rotate, const Vector4 &color, const float life_time, const bool no_depth_test)
    {
        std::lock_guard<std::mutex> guard(mMutex);
        DebugDrawCylinder cylinder;
        cylinder.mRadius = radius;
        cylinder.mCenter = center;
        cylinder.mHeight = height;
        cylinder.mRotation = rotate;
        cylinder.mColor = color;
        cylinder.mbNoDepthTest = no_depth_test;
        cylinder.SetTime(life_time);

        mCylinders.push_back(cylinder);
    }

    void DebugDrawGroup::AddCapsule(const Vector3 &center, const Vector4 &rotation, const Vector3 &scale, const float radius, const float height, const Vector4 &color, const float life_time, const bool no_depth_test)
    {
        std::lock_guard<std::mutex> guard(mMutex);
        DebugDrawCapsule capsule;
        capsule.mCenter = center;
        capsule.mRotation = rotation;
        capsule.mScale = scale;
        capsule.mRadius = radius;
        capsule.mHeight = height;
        capsule.mColor = color;
        capsule.mbNoDepthTest = no_depth_test;
        capsule.SetTime(life_time);

        mCapsules.push_back(capsule);
    }

    void DebugDrawGroup::AddText(const std::string &content, const Vector4 &color, const Vector3 &coordinate, const int size, const bool is_screen_text, const float life_time)
    {
        std::lock_guard<std::mutex> guard(mMutex);
        DebugDrawText text;
        text.mContent = content;
        text.mColor = color;
        text.mCoordinate = coordinate;
        text.mSize = size;
        text.mbIsScreenText = is_screen_text;
        text.SetTime(life_time);
        mTexts.push_back(text);
    }

    void DebugDrawGroup::RemoveDeadPrimitives(float delta_time)
    {
        for (std::list<DebugDrawPoint>::iterator point = mPoints.begin(); point != mPoints.end();)
        {
            if (point->IsTimeOut(delta_time)) {
                point = mPoints.erase(point);
            }
            else {
                point++;
            }
        }
        for (std::list<DebugDrawLine>::iterator line = mLines.begin(); line != mLines.end();)
        {
            if (line->IsTimeOut(delta_time)) {
                line = mLines.erase(line);
            }
            else {
                line++;
            }
        }
        for (std::list<DebugDrawTriangle>::iterator triangle = mTriangles.begin(); triangle != mTriangles.end();)
        {
            if (triangle->IsTimeOut(delta_time)) {
                triangle = mTriangles.erase(triangle);
            }
            else {
                triangle++;
            }
        }
        for (std::list<DebugDrawQuad>::iterator quad = mQuads.begin(); quad != mQuads.end();)
        {
            if (quad->IsTimeOut(delta_time)) {
                quad = mQuads.erase(quad);
            }
            else {
                quad++;
            }
        }
        for (std::list<DebugDrawBox>::iterator box = mBoxes.begin(); box != mBoxes.end();)
        {
            if (box->IsTimeOut(delta_time)) {
                box = mBoxes.erase(box);
            }
            else {
                box++;
            }
        }
        for (std::list<DebugDrawCylinder>::iterator cylinder = mCylinders.begin(); cylinder != mCylinders.end();)
        {
            if (cylinder->IsTimeOut(delta_time)) {
                cylinder = mCylinders.erase(cylinder);
            }
            else {
                cylinder++;
            }
        }
        for (std::list<DebugDrawSphere>::iterator sphere = mSpheres.begin(); sphere != mSpheres.end();)
        {
            if (sphere->IsTimeOut(delta_time)) {
                sphere = mSpheres.erase(sphere);
            }
            else {
                sphere++;
            }
        }
        for (std::list<DebugDrawCapsule>::iterator capsule = mCapsules.begin(); capsule != mCapsules.end();)
        {
            if (capsule->IsTimeOut(delta_time)) {
                capsule = mCapsules.erase(capsule);
            }
            else {
                capsule++;
            }
        }
        for (std::list<DebugDrawText>::iterator text = mTexts.begin(); text != mTexts.end();)
        {
            if (text->IsTimeOut(delta_time)) {
                text = mTexts.erase(text);
            }
            else {
                text++;
            }
        }
    }

    void DebugDrawGroup::MergeFrom(DebugDrawGroup *group)
    {
        std::lock_guard<std::mutex> guard(mMutex);
        std::lock_guard<std::mutex> guard_2(group->mMutex);
        mPoints.insert(mPoints.end(), group->mPoints.begin(), group->mPoints.end());
        mLines.insert(mLines.end(), group->mLines.begin(), group->mLines.end());
        mTriangles.insert(mTriangles.end(), group->mTriangles.begin(), group->mTriangles.end());
        mQuads.insert(mQuads.end(), group->mQuads.begin(), group->mQuads.end());
        mBoxes.insert(mBoxes.end(), group->mBoxes.begin(), group->mBoxes.end());
        mCylinders.insert(mCylinders.end(), group->mCylinders.begin(), group->mCylinders.end());
        mSpheres.insert(mSpheres.end(), group->mSpheres.begin(), group->mSpheres.end());
        mCapsules.insert(mCapsules.end(), group->mCapsules.begin(), group->mCapsules.end());
        mTexts.insert(mTexts.end(), group->mTexts.begin(), group->mTexts.end());
    }

    size_t DebugDrawGroup::GetPointCount(bool no_depth_test) const
    {
        size_t count = 0;
        for (const DebugDrawPoint point : mPoints)
        {
            if (point.mbNoDepthTest == no_depth_test)count++;
        }
        return count;
    }

    size_t DebugDrawGroup::GetLineCount(bool no_depth_test) const
    {
        size_t line_count = 0;
        for (const DebugDrawLine line : mLines)
        {
            if (line.mbNoDepthTest == no_depth_test)line_count++;
        }
        for (const DebugDrawTriangle triangle : mTriangles)
        {
            if (triangle.mFillMode == FillMode::Wireframe && triangle.mbNoDepthTest == no_depth_test)
            {
                line_count += 3;
            }
        }
        for (const DebugDrawQuad quad : mQuads)
        {
            if (quad.mFillMode == FillMode::Wireframe && quad.mbNoDepthTest == no_depth_test)
            {
                line_count += 4;
            }
        }
        for (const DebugDrawBox box : mBoxes)
        {
            if (box.mbNoDepthTest == no_depth_test)line_count += 12;
        }
        return line_count;
    }

    size_t DebugDrawGroup::GetTriangleCount(bool no_depth_test) const
    {
        size_t triangle_count = 0;
        for (const DebugDrawTriangle triangle : mTriangles)
        {
            if (triangle.mFillMode == FillMode::Solid && triangle.mbNoDepthTest == no_depth_test)
            {
                triangle_count++;
            }
        }
        return triangle_count;
    }

    size_t DebugDrawGroup::GetUniformDynamicDataCount() const
    {
        return mSpheres.size() + mCylinders.size() + mCapsules.size();
    }

    size_t DebugDrawGroup::GetSphereCount(bool no_depth_test) const
    {
        size_t count = 0;
        for (const DebugDrawSphere sphere : mSpheres)
        {
            if (sphere.mbNoDepthTest == no_depth_test)count++;
        }
        return count; 
    }

    size_t DebugDrawGroup::GetCylinderCount(bool no_depth_test) const
    {
        size_t count = 0;
        for (const DebugDrawCylinder cylinder : mCylinders)
        {
            if (cylinder.mbNoDepthTest == no_depth_test)count++;
        }
        return count;
    }

    size_t DebugDrawGroup::GetCapsuleCount(bool no_depth_test) const
    {
        size_t count = 0;
        for (const DebugDrawCapsule capsule : mCapsules)
        {
            if (capsule.mbNoDepthTest == no_depth_test)count++;
        }
        return count;
    }

    size_t DebugDrawGroup::GetTextCharacterCount() const
    {
        size_t count = 0;
        for (const DebugDrawText text : mTexts)
        {
            for (unsigned char character : text.mContent)
            {
                if (character != '\n')count++;
            }
        }
        return count;
    }

    void DebugDrawGroup::WritePointData(std::vector<DebugDrawVertex> &vertexs, bool no_depth_test)
    {
        size_t vertexs_count = GetPointCount(no_depth_test);
        vertexs.resize(vertexs_count);

        size_t current_index = 0;
        for (DebugDrawPoint point : mPoints)
        {
            if (point.mbNoDepthTest == no_depth_test)vertexs[current_index++] = point.mVertex;
        }
    }

    void DebugDrawGroup::WriteLineData(std::vector<DebugDrawVertex> &vertexs, bool no_depth_test)
    {
        size_t vertexs_count = GetLineCount(no_depth_test) * 2;
        vertexs.resize(vertexs_count);

        size_t current_index = 0;
        for (DebugDrawLine line : mLines)
        {
            if (line.mFillMode == FillMode::Wireframe && line.mbNoDepthTest == no_depth_test)
            {
                vertexs[current_index++] = line.mVertex[0];
                vertexs[current_index++] = line.mVertex[1];
            }
        }
        for (DebugDrawTriangle triangle : mTriangles)
        {
            if (triangle.mFillMode == FillMode::Wireframe && triangle.mbNoDepthTest == no_depth_test)
            {
                std::vector<size_t> indies = { 0,1, 1,2, 2,0 };
                for (size_t i : indies)
                {
                    vertexs[current_index++] = triangle.mVertex[i];
                }
            }
        }
        for (DebugDrawQuad quad : mQuads)
        {
            if (quad.mFillMode == FillMode::Wireframe && quad.mbNoDepthTest == no_depth_test)
            {
                std::vector<size_t> indies = { 0,1, 1,2, 2,3, 3,0 };
                for (size_t i : indies)
                {
                    vertexs[current_index++] = quad.mVertex[i];
                }
            }
        }
        for (DebugDrawBox box : mBoxes)
        {
            if (box.mbNoDepthTest == no_depth_test)
            {
                std::vector<DebugDrawVertex> verts_4d(8);
                float f[2] = { -1.0f,1.0f };
                for (size_t i = 0; i < 8; i++)
                {
                    Vector3 v(f[i & 1] * box.mHalfExtents.x, f[(i >> 1) & 1] * box.mHalfExtents.y, f[(i >> 2) & 1] * box.mHalfExtents.z);
                    Vector3 uv, uuv;
                    Vector3 qvec(box.mRotation.x, box.mRotation.y, box.mRotation.z);
                    uv = qvec.CrossProduct(v);
                    uuv = qvec.CrossProduct(uv);
                    uv *= (2.0f * box.mRotation.w);
                    uuv *= 2.0f;
                    verts_4d[i].mPos = v + uv + uuv + box.mCenterPoint;
                    verts_4d[i].mColor = box.mColor;
                }
                std::vector<size_t> indies = { 0,1, 1,3, 3,2, 2,0, 4,5, 5,7, 7,6, 6,4, 0,4, 1,5, 3,7, 2,6 };
                for (size_t i : indies)
                {
                    vertexs[current_index++] = verts_4d[i];
                }
            }
        }
    }

    void DebugDrawGroup::WriteTriangleData(std::vector<DebugDrawVertex> &vertexs, bool no_depth_test)
    {
        size_t vertexs_count = GetTriangleCount(no_depth_test) * 3;
        vertexs.resize(vertexs_count);

        size_t current_index = 0;
        for (DebugDrawTriangle triangle : mTriangles)
        {
            if (triangle.mFillMode == FillMode::Solid && triangle.mbNoDepthTest == no_depth_test)
            {
                vertexs[current_index++] = triangle.mVertex[0];
                vertexs[current_index++] = triangle.mVertex[1];
                vertexs[current_index++] = triangle.mVertex[2];
            }
        }
    }

    void DebugDrawGroup::WriteUniformDynamicDataToCache(std::vector<std::pair<Matrix4x4, Vector4>> &datas)
    {
        // cache uniformDynamic data ,first has_depth_test ,second no_depth_test
        size_t data_count = GetUniformDynamicDataCount() * 3;
        datas.resize(data_count);

        bool no_depth_tests[] = { false,true };
        for (int32_t i = 0; i < 2; i++)
        {
            bool no_depth_test = no_depth_tests[i];

            size_t current_index = 0;
            for (DebugDrawSphere sphere : mSpheres)
            {
                if (sphere.mbNoDepthTest == no_depth_test)
                {
                    Matrix4x4 model = Matrix4x4::IDENTITY;

                    Matrix4x4 tmp = Matrix4x4::IDENTITY;
                    tmp.MakeTrans(sphere.mCenter);
                    model = model * tmp;
                    tmp = Matrix4x4::BuildScaleMatrix(sphere.mRadius, sphere.mRadius, sphere.mRadius);
                    model = model * tmp;
                    datas[current_index++] = std::make_pair(model, sphere.mColor);
                }
            }
            for (DebugDrawCylinder cylinder : mCylinders)
            {
                if (cylinder.mbNoDepthTest == no_depth_test)
                {
                    Matrix4x4 model = Matrix4x4::IDENTITY;

                    //rolate
                    float w = cylinder.mRotation.x;
                    float x = cylinder.mRotation.y;
                    float y = cylinder.mRotation.z;
                    float z = cylinder.mRotation.w;
                    Matrix4x4 tmp = Matrix4x4::IDENTITY;
                    tmp.MakeTrans(cylinder.mCenter);
                    model = model * tmp;
                    
                    tmp = Matrix4x4::BuildScaleMatrix(cylinder.mRadius, cylinder.mRadius, cylinder.mHeight / 2.0f);
                    model = model * tmp;
            
                    Matrix4x4 ro = Matrix4x4::IDENTITY;
                    ro[0][0] = 1.0f - 2.0f * y * y - 2.0f * z * z; ro[0][1] = 2.0f * x * y + 2.0f * w * z;        ro[0][2] = 2.0f * x * z - 2.0f * w * y;
                    ro[1][0] = 2.0f * x * y - 2.0f * w * z;        ro[1][1] = 1.0f - 2.0f * x * x - 2.0f * z * z; ro[1][2] = 2.0f * y * z + 2.0f * w * x;
                    ro[2][0] = 2.0f * x * z + 2.0f * w * y;        ro[2][1] = 2.0f * y * z - 2.0f * w * x;        ro[2][2] = 1.0f - 2.0f * x * x - 2.0f * y * y;
                    model = model * ro;

                    datas[current_index++] = std::make_pair(model, cylinder.mColor);
                }
            }
            for (DebugDrawCapsule capsule : mCapsules)
            {
                if (capsule.mbNoDepthTest == no_depth_test)
                {
                    Matrix4x4 model1 = Matrix4x4::IDENTITY;
                    Matrix4x4 model2 = Matrix4x4::IDENTITY;
                    Matrix4x4 model3 = Matrix4x4::IDENTITY;

                    Matrix4x4 tmp = Matrix4x4::IDENTITY;
                    tmp.MakeTrans(capsule.mCenter);
                    model1 = model1 * tmp;
                    model2 = model2 * tmp;
                    model3 = model3 * tmp;

                    tmp = Matrix4x4::BuildScaleMatrix(capsule.mScale.x, capsule.mScale.y, capsule.mScale.z);
                    model1 = model1 * tmp;
                    model2 = model2 * tmp;
                    model3 = model3 * tmp;

                    //rolate
                    float w = capsule.mRotation.x;
                    float x = capsule.mRotation.y;
                    float y = capsule.mRotation.z;
                    float z = capsule.mRotation.w;
                    Matrix4x4 ro = Matrix4x4::IDENTITY;
                    ro[0][0] = 1.0f - 2.0f * y * y - 2.0f * z * z; ro[0][1] = 2.0f * x * y + 2.0f * w * z;        ro[0][2] = 2.0f * x * z - 2.0f * w * y;
                    ro[1][0] = 2.0f * x * y - 2.0f * w * z;        ro[1][1] = 1.0f - 2.0f * x * x - 2.0f * z * z; ro[1][2] = 2.0f * y * z + 2.0f * w * x;
                    ro[2][0] = 2.0f * x * z + 2.0f * w * y;        ro[2][1] = 2.0f * y * z - 2.0f * w * x;        ro[2][2] = 1.0f - 2.0f * x * x - 2.0f * y * y;
                    model1 = model1 * ro;
                    model2 = model2 * ro;
                    model3 = model3 * ro;

                    tmp.MakeTrans(Vector3(0.0f, 0.0f, capsule.mHeight / 2.0f - capsule.mRadius));
                    model1 = model1 * tmp;

                    tmp = Matrix4x4::BuildScaleMatrix(1.0f, 1.0f, capsule.mHeight / (capsule.mRadius * 2.0f));
                    model2 = model2 * tmp;

                    tmp.MakeTrans(Vector3(0.0f, 0.0f, -(capsule.mHeight / 2.0f - capsule.mRadius)));
                    model3 = model3 * tmp;

                    tmp = Matrix4x4::BuildScaleMatrix(capsule.mRadius, capsule.mRadius, capsule.mRadius);
                    model1 = model1 * tmp;
                    model2 = model2 * tmp;
                    model3 = model3 * tmp;

                    datas[current_index++] = std::make_pair(model1, capsule.mColor);
                    datas[current_index++] = std::make_pair(model2, capsule.mColor);
                    datas[current_index++] = std::make_pair(model3, capsule.mColor);
                }
            }
        }
    }

    void DebugDrawGroup::WriteTextData(std::vector<DebugDrawVertex> &vertexs, DebugDrawFont *font, Matrix4x4 m_proj_view_matrix)
    {
        RHISwapChainDesc swapChainDesc = gRuntimeGlobalContext.mRenderSystem->GetRHI()->GetSwapChainInfo();
        uint32_t screenWidth = swapChainDesc.viewport->width;
        uint32_t screenHeight = swapChainDesc.viewport->height;

        size_t vertexs_count = GetTextCharacterCount() * 6;
        vertexs.resize(vertexs_count);

        size_t current_index = 0;
        for(DebugDrawText text : mTexts)
        {
            float absoluteW = text.mSize, absoluteH = text.mSize * 2;
            float w = absoluteW / (1.0f * screenWidth / 2.0f), h = absoluteH / (1.0f * screenHeight / 2.0f);
            Vector3 coordinate = text.mCoordinate;
            if (!text.mbIsScreenText)
            {
                Vector4 tempCoord(coordinate.x, coordinate.y, coordinate.z, 1.0f);
                tempCoord = m_proj_view_matrix * tempCoord;
                coordinate = Vector3(tempCoord.x / tempCoord.w, tempCoord.y / tempCoord.w, 0.0f);
            }
            float x = coordinate.x, y = coordinate.y;
            for (unsigned char character : text.mContent)
            {
                if (character == '\n')
                {
                    y += h;
                    x = coordinate.x;
                }
                else
                {

                    float x1, x2, y1, y2;
                    font->GetCharacterTextureRect(character, x1, y1, x2, y2);

                    float cx1, cx2, cy1, cy2;
                    cx1 = 0 + x; cx2 = w + x;
                    cy1 = 0 + y; cy2 = h + y;

                    vertexs[current_index].mPos = Vector3(cx1, cy1, 0.0f);
                    vertexs[current_index].mColor = text.mColor;
                    vertexs[current_index++].mTexCoord = Vector2(x1, y1);

                    vertexs[current_index].mPos = Vector3(cx1, cy2, 0.0f);
                    vertexs[current_index].mColor = text.mColor;
                    vertexs[current_index++].mTexCoord = Vector2(x1, y2);

                    vertexs[current_index].mPos = Vector3(cx2, cy2, 0.0f);
                    vertexs[current_index].mColor = text.mColor;
                    vertexs[current_index++].mTexCoord = Vector2(x2, y2);

                    vertexs[current_index].mPos = Vector3(cx1, cy1, 0.0f);
                    vertexs[current_index].mColor = text.mColor;
                    vertexs[current_index++].mTexCoord = Vector2(x1, y1);

                    vertexs[current_index].mPos = Vector3(cx2, cy2, 0.0f);
                    vertexs[current_index].mColor = text.mColor;
                    vertexs[current_index++].mTexCoord = Vector2(x2, y2);

                    vertexs[current_index].mPos = Vector3(cx2, cy1, 0.0f);
                    vertexs[current_index].mColor = text.mColor;
                    vertexs[current_index++].mTexCoord = Vector2(x2, y1);

                    x += w;
                }
            }
        }
    }
}
