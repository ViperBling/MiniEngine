#pragma once

#include "MRuntime/Function/Global/GlobalContext.hpp"
#include "MRuntime/Function/Render/RenderSystem.hpp"
#include "MRuntime/Function/Render/Interface/RHI.hpp"
#include "MRuntime/Resource/ConfigManager/ConfigManager.hpp"

#include <stdexcept>
#include <vector>

namespace MiniEngine
{
    class DebugDrawFont
    {
    public:
        void GetCharacterTextureRect(const unsigned char character, float &x1, float &y1, float &x2, float &y2);
        RHIImageView* GetImageView() const;
        void Initialize();
        void Destroy();
    
    private:
        void loadFont();
        
    private:
        const unsigned char mRangeL = 32, mRangeR = 126;
        const int mSingleCharWidth = 32;
        const int mSingleCharHeight = 64;
        const int mNumOfCharInOneLine = 16;
        const int mNumOfChar = (mRangeR - mRangeL + 1);
        const int mBitmapW = mSingleCharWidth * mNumOfCharInOneLine;
        const int mBitmapH = mSingleCharHeight * ((mNumOfChar + mNumOfCharInOneLine - 1) / mNumOfCharInOneLine);

        RHIImage*        mFontImage = nullptr;
        RHIImageView*    mFontImageView = nullptr;
        RHIDeviceMemory* mFontImageMemory = nullptr;
        VmaAllocation    mAllocation;
    };
}