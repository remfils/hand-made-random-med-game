#define internal

#include "handmade.h"


void
RenderRectangle(loaded_bitmap *drawBuffer, real32 realMinX, real32 realMinY, real32 realMaxX, real32 realMaxY, v4 color, rectangle2i clipRect, bool32 even, bool32 renderFringes=false)
{
    int32 minX = RoundReal32ToInt32(realMinX);
    int32 minY = RoundReal32ToInt32(realMinY);
    int32 maxX = RoundReal32ToInt32(realMaxX);
    int32 maxY = RoundReal32ToInt32(realMaxY);

    if (renderFringes)
    {
        int32 debug = 0;
    }
    
    if (minX < 0)
    {
        minX = 0;
    }
    if (minY < 0)
    {
        minY = 0;
    }
    
    if (maxX > drawBuffer->Width)
    {
        maxX = drawBuffer->Width;
    }
    if (maxY > drawBuffer->Height)
    {
        maxY = drawBuffer->Height;
    }

    // TODO: this is horrible
    rectangle2i fillRect = {minX, minY, maxX, maxY};
    fillRect = Intersect(fillRect, clipRect);

    if (!HasArea(fillRect)) {
        return;
    }

    if (!even == (fillRect.MinY % 2)) {
        fillRect.MinY += 1;
    }

    __m128i startClipMask = _mm_set1_epi8(-1);
    __m128i endClipMask = _mm_set1_epi8(-1);

    int32 minXRemainder = fillRect.MinX & 3;
    if (minXRemainder)
    {
        fillRect.MinX = fillRect.MinX & ~3;
        switch (minXRemainder)
        {
        case 1: startClipMask = _mm_slli_si128(startClipMask, 1 * 4); break; // shift is in bytes 
        case 2: startClipMask = _mm_slli_si128(startClipMask, 2 * 4); break; // shift is in bytes 
        case 3: startClipMask = _mm_slli_si128(startClipMask, 3 * 4); break; // shift is in bytes 
        }
    }

    int32 maxXRemainder = fillRect.MaxX & 3;
    if (maxXRemainder)
    {
        fillRect.MaxX = (fillRect.MaxX & ~3) + 4;
        switch (maxXRemainder)
        {
        case 1: endClipMask = _mm_srli_si128(endClipMask, 3 * 4); break; // shift is in bytes 
        case 2: endClipMask = _mm_srli_si128(endClipMask, 2 * 4); break; // shift is in bytes 
        case 3: endClipMask = _mm_srli_si128(endClipMask, 1 * 4); break; // shift is in bytes 
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // fill base rectangle
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    minX = fillRect.MinX;
    minY = fillRect.MinY;
    maxX = fillRect.MaxX;
    maxY = fillRect.MaxY;

    v4 originalColor = color;
    color.rgb *= color.a;

    real32 val255 = 255.0f;
    real32 inv255 = 1.0f / val255;
    __m128 inv255_4x = _mm_set1_ps(inv255);
    __m128 val255_4x = _mm_set1_ps(val255);

    __m128 one_4x = _mm_set1_ps(1.0f);

    __m128 colorR_4x = _mm_set1_ps(color.r);
    __m128 colorG_4x = _mm_set1_ps(color.g);
    __m128 colorB_4x = _mm_set1_ps(color.b);
    __m128 colorA_4x = _mm_set1_ps(color.a);

    uint32 rowPitch = 2 * drawBuffer->Pitch;


    BEGIN_TIMED_BLOCK(RenderRectangle);

    __m128 zero_4x = _mm_set1_ps(0.0f);
    __m128i maskFF_4x = _mm_set1_epi32(0xff);

    uint8 *row = (uint8 *)drawBuffer->Memory + drawBuffer->Pitch * minY + minX * BITMAP_BYTES_PER_PIXEL;
    for (int y = minY; y < maxY; y+=2)
    {
        uint32 *pixel = (uint32 *)row;

        __m128i clipMask = startClipMask;
        
        for (int x = minX; x < maxX; x+=4)
        {
            __m128i originalDest = _mm_loadu_si128((__m128i *)pixel);
            __m128 destR = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(originalDest, 16), maskFF_4x));
            __m128 destG = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(originalDest, 8), maskFF_4x));
            __m128 destB = _mm_cvtepi32_ps(_mm_and_si128(originalDest, maskFF_4x));
            __m128 destA = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(originalDest, 24), maskFF_4x));

            // destPixel = SRGB255_ToLinear1(destPixel);
            
            destR = _mm_mul_ps(destR, inv255_4x);
            destR = _mm_mul_ps(destR, destR);
            
            destG = _mm_mul_ps(destG, inv255_4x);
            destG = _mm_mul_ps(destG, destG);
            
            destB = _mm_mul_ps(destB, inv255_4x);
            destB = _mm_mul_ps(destB, destB);
            
            destA = _mm_mul_ps(destA, inv255_4x);

            // real32 inv_sa = (1.0f - color.a);
            __m128 inv_sa = _mm_sub_ps(one_4x, colorA_4x);

            destR = _mm_add_ps(_mm_mul_ps(inv_sa, destR), colorR_4x);
            destG = _mm_add_ps(_mm_mul_ps(inv_sa, destG), colorG_4x);
            destB = _mm_add_ps(_mm_mul_ps(inv_sa, destB), colorB_4x);
            destA = _mm_add_ps(_mm_mul_ps(inv_sa, destA), colorA_4x);            

            // blended = Linear_1_ToSRGB255(blended);
            destR = _mm_mul_ps(_mm_sqrt_ps(destR), val255_4x);
            destG = _mm_mul_ps(_mm_sqrt_ps(destG), val255_4x);
            destB = _mm_mul_ps(_mm_sqrt_ps(destB), val255_4x);
            destA = _mm_mul_ps(destA, val255_4x);

            // rounding, cvt == convert
            __m128i intR = _mm_cvtps_epi32(destR);
            __m128i intB = _mm_cvtps_epi32(destB);
            __m128i intG = _mm_cvtps_epi32(destG);
            __m128i intA = _mm_cvtps_epi32(destA);

            intR = _mm_slli_epi32(intR, 16);
            intG = _mm_slli_epi32(intG, 8);
            intA = _mm_slli_epi32(intA, 24);

            __m128i argb = _mm_or_epi32(_mm_or_epi32(intB, intR), _mm_or_epi32(intG, intA));

            __m128i maskedOut = _mm_or_si128(
                                             _mm_and_si128(clipMask, argb),
                                             _mm_andnot_si128(clipMask, originalDest)
                                             );
            // is needed for alignment
            _mm_storeu_si128((__m128i *)pixel, maskedOut);
            pixel += 4;

            if ((x + 8) < maxX)
            {
                clipMask = _mm_set1_epi8(-1);
            }
            else
            {
                clipMask = endClipMask;
            }
        }
        row += rowPitch;
    }

    END_TIMED_BLOCK(RenderRectangle);
}
