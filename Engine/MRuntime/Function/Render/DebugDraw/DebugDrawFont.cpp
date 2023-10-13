#include "DebugDrawFont.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace MiniEngine
{
    void DebugDrawFont::GetCharacterTextureRect(const unsigned char character, float &x1, float &y1, float &x2, float &y2)
    {
        if (character >= mRangeL && character <= mRangeR)
        {
            x1 = (character - mRangeL) % mNumOfCharInOneLine * mSingleCharWidth * 1.0f / mBitmapW;
            x2 = ((character - mRangeL) % mNumOfCharInOneLine * mSingleCharWidth + mSingleCharWidth)* 1.0f / mBitmapW;
            y1 = (character - mRangeL) / mNumOfCharInOneLine * mSingleCharHeight * 1.0f / mBitmapH;
            y2 = ((character - mRangeL) / mNumOfCharInOneLine * mSingleCharHeight + mSingleCharHeight)* 1.0f / mBitmapH;
        }
        else
        {
            x1 = x2 = y1 = y2 = 0;
        }
    }

    RHIImageView *DebugDrawFont::GetImageView() const
    {
        return mFontImageView;
    }

    void DebugDrawFont::Initialize()
    {
        loadFont();
    }

    void DebugDrawFont::Destroy()
    {
        std::shared_ptr<RHI> rhi = gRuntimeGlobalContext.mRenderSystem->GetRHI();
        rhi->FreeMemory(mFontImageMemory);
        rhi->DestroyImageView(mFontImageView);
        rhi->DestroyImage(mFontImage);
    }

    void DebugDrawFont::loadFont()
    {
        std::string str = gRuntimeGlobalContext.mConfigManager->GetEditorFontPath().string();
        const char* fontFilePath = str.c_str();
        FILE* fontFile = fopen(fontFilePath, "rb");
        if (fontFile == NULL)
        {
            std::runtime_error("debug draw cannot open font.ttf");
        }
        fseek(fontFile, 0, SEEK_END);
        uint64_t size = ftell(fontFile);
        fseek(fontFile, 0, SEEK_SET);

        stbtt_fontinfo fontInfo;
        unsigned char* fontBuffer = (unsigned char*)calloc(size, sizeof(unsigned char));
        fread(fontBuffer, size, 1, fontFile);
        fclose(fontFile);
        
        if (!stbtt_InitFont(&fontInfo, fontBuffer, 0))
        {
            std::runtime_error("debug draw stb init font failed\n");
        }

        unsigned char* bitmap = (unsigned char*)calloc(mBitmapW * mBitmapH, sizeof(unsigned char));

        float pixels = mSingleCharHeight - 2;
        float scale = stbtt_ScaleForPixelHeight(&fontInfo, pixels);

        int c_x1, c_y1, c_x2, c_y2;
        int ascent = 0;
        int descent = 0;
        int lineGap = 0;
        stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);
        ascent = roundf(ascent * scale);
        descent = roundf(descent * scale);
        

        int x = 0;
        for (unsigned char character = mRangeL; character <= mRangeR; character++)
        {
            int advanceWidth = 0;
            int leftSideBearing = 0;
            stbtt_GetCodepointHMetrics(&fontInfo, (unsigned char)character, &advanceWidth, &leftSideBearing);

            int c_x1, c_y1, c_x2, c_y2;
            stbtt_GetCodepointBitmapBox(&fontInfo, character, scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);
            
            int y = ascent + c_y1 - 2;
            int byteOffset = roundf(leftSideBearing * scale) + (character - mRangeL) % mNumOfCharInOneLine * mSingleCharWidth + ((character - mRangeL)/ mNumOfCharInOneLine * mSingleCharHeight + y) * mBitmapW;
            
            stbtt_MakeCodepointBitmap(&fontInfo, bitmap + byteOffset, c_x2 - c_x1, c_y2 - c_y1, mBitmapW, scale, scale, character);

            x += roundf(advanceWidth * scale);

            int kern;
            kern = stbtt_GetCodepointKernAdvance(&fontInfo, character, (unsigned char)(character + 1));
            x += roundf(kern * scale);

        }
        std::vector<float> imageData(mBitmapW * mBitmapH);
        for (int i = 0; i < mBitmapW * mBitmapH; i++)
        {
            imageData[i] = static_cast<float>(*(bitmap + i))/255.0f;
        }
        
        std::shared_ptr<RHI> rhi = gRuntimeGlobalContext.mRenderSystem->GetRHI();
        rhi->CreateGlobalImage(mFontImage, mFontImageView, mAllocation, mBitmapW, mBitmapH, imageData.data(), RHIFormat::RHI_FORMAT_R32_SFLOAT);

        free(fontBuffer);
        free(bitmap);
    }

} // namespace MiniEngine
