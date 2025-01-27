#define MM(a, i) ((float *)&a)[i]
#define MMi(a, i) ((int *)&a)[i]



void
RenderRectangle(loaded_bitmap *drawBuffer, real32 realMinX, real32 realMinY, real32 realMaxX, real32 realMaxY, v4 color, rectangle2i clipRect, bool32 even, bool32 renderFringes=false);

inline v4
UnscaleAndBiasNormal(v4 normal)
{
    v4 result;

    real32 inv255 = 1.0f / 255.0f;

    result.x = -1.0f + 2.0f * normal.x * inv255;
    result.y = -1.0f + 2.0f * normal.y * inv255;
    result.z = -1.0f + 2.0f * normal.z * inv255;
    result.w = normal.w * inv255;

    return result;
}

struct bilinear_sample
{
    uint32 A, B, C, D;
};

inline bilinear_sample
BilinearSample(loaded_bitmap *texture, int32 imgX, int32 imgY)
{
    uint8 *texelPtr = (uint8 *)texture->Memory + imgY * texture->Pitch + imgX * sizeof(uint32);

    bilinear_sample result = {
        *(uint32 *)texelPtr,
        *(uint32 *)(texelPtr + sizeof(uint32)),
        *(uint32 *)(texelPtr + texture->Pitch),
        *(uint32 *)(texelPtr + texture->Pitch + sizeof(uint32))
    };
    
    return result;
}

inline v4
SRGBBilinearBlend(bilinear_sample texelSamples, real32 fX, real32 fY)
{
    v4 texelA = Unpack4x8(texelSamples.A);
    v4 texelB = Unpack4x8(texelSamples.B);
    v4 texelC = Unpack4x8(texelSamples.C);
    v4 texelD = Unpack4x8(texelSamples.D);

    texelA = SRGB255_ToLinear1(texelA);
    texelB = SRGB255_ToLinear1(texelB);
    texelC = SRGB255_ToLinear1(texelC);
    texelD = SRGB255_ToLinear1(texelD);

    v4 result = Lerp(Lerp(texelA, fX, texelB), fY, Lerp(texelC, fX, texelD));
    return result;
}

/**
 * @param screenSpaceUV - from where ray is cast in normalized
 * @param sampleDirection - what direction is cast is going - should be normalized
 * @param roughness - what level of detail to used
 * @param map - from where to sample
 * @param heightOfSkyBox - how far away from ground skybox texture is
 */
internal v4
SampleEnvironmentMap(v2 screenSpaceUV, v3 sampleDirection, real32 roughness, render_environment_map *map, real32 distanceToMapInZ)
{
    uint32 roughIndex = (uint32)(roughness * ((real32)ArrayCount(map->LevelsOfDetails) - 1.0f) + 0.5f);
    Assert(roughIndex < ArrayCount(map->LevelsOfDetails));
    
    loaded_bitmap *level = map->LevelsOfDetails + roughIndex;
    
    real32 uvsPerMeter = 0.1f;
    real32 coef = (uvsPerMeter * distanceToMapInZ) / sampleDirection.y;
    // TODO: make sure we know what direction Z should go and why
    v2 offset = coef * ToV2(sampleDirection.x, sampleDirection.z);

    // find the intersection point
    v2 uv = screenSpaceUV + offset;
    uv = Clamp01(uv);

    real32 tX = (uv.x * ((real32)(level->Width - 2)));
    real32 tY = (uv.y * ((real32)(level->Height - 2)));

    uint32 mapX = (int32)tX;
    uint32 mapY = (int32)tY;
    
    real32 fX = tX - (real32)mapX;
    real32 fY = tY - (real32)mapY;


#if 0
    uint8 *texelPtr = (uint8 *)level->Memory + mapY * level->Pitch + mapX * sizeof(uint32);
    *(uint32 *)texelPtr = 0xffffffff;
#endif

    bilinear_sample sample = BilinearSample(level, mapX, mapY);

    v4 result = SRGBBilinearBlend(sample, fX, fY);
    
    return result;
}

internal void
RenderRectangleSlowly(loaded_bitmap *drawBuffer,
                      v2 origin, v2 xAxis, v2 yAxis, v4 color,
                      loaded_bitmap *texture, loaded_bitmap *normalMap,
                      render_environment_map *top, render_environment_map *middle, render_environment_map *bottom,
                      real32 pixelsToMeters)
{
    BEGIN_TIMED_BLOCK(RenderRectangleSlowly);
    
    // color.rgb *= color.a;

    int32 widthMax = drawBuffer->Width - 1;
    int32 heightMax = drawBuffer->Height - 1;

    real32 invWidthMax = 1.0f / (real32)widthMax;
    real32 invHeightMax = 1.0f / (real32)heightMax;

    real32 xAxisLen = Length(xAxis);
    real32 yAxisLen = Length(yAxis);
    
    real32 invXLengthSq = 1.0f / LengthSq(xAxis);
    real32 invYLengthSq = 1.0f / LengthSq(yAxis);

    v2 normalXAxis = (yAxisLen / xAxisLen)  * xAxis;
    v2 normalYAxis = (xAxisLen / yAxisLen) * yAxis;

    real32 zScale = (xAxisLen + yAxisLen) / 2;

    real32 originZ = 0.0f;
    real32 originY = (origin + 0.5f *xAxis + 0.5f * yAxis).y;
    real32 fixedCastY = invHeightMax * originY; // TODO: specify this as param separatly
    
    int32 minX = widthMax;
    int32 maxX = 0;
    int32 minY = heightMax;
    int32 maxY = 0;

    v2 p[4] = {origin, origin + xAxis, origin + xAxis + yAxis, origin + yAxis};
    for (int pIndex=0; pIndex < ArrayCount(p); pIndex++)
    {
        v2 testP = p[pIndex];
        int32 floorX = FloorReal32ToInt32(testP.x);
        int32 ceilX = CeilReal32ToInt32(testP.x);
        int32 floorY = FloorReal32ToInt32(testP.y);
        int32 ceilY = CeilReal32ToInt32(testP.y);

        if (minY > floorY) { minY = floorY; }
        if (minX > floorX) { minX = floorX; }
        if (maxX < ceilX) { maxX = ceilX; }
        if (maxY < ceilY) { maxY = ceilY; }
    }

    if (minX < 0) minX = 0;
    if (minY < 0) minY = 0;
    if (maxX > widthMax) maxX = widthMax;
    if (maxY > heightMax) maxY = heightMax;
    uint8 *row = (uint8 *)drawBuffer->Memory + drawBuffer->Pitch * minY + minX * BITMAP_BYTES_PER_PIXEL;
    for (int32 y = minY; y <= maxY; ++y)
    {
        uint32 *pixel = (uint32 *)row;
        
        for (int32 x = minX; x <= maxX; ++x)
        {
            BEGIN_TIMED_BLOCK(Slowly_TestPixel);
            
            v2 pixelP = V2i(x, y);
            v2 d = pixelP - origin;

            // TODO: perpinner
            // TODO: simpler origin calcs
            real32 edge0 = Inner(d, -Perp(xAxis));
            real32 edge1 = Inner(d - xAxis, -Perp(yAxis));
            real32 edge2 = Inner(d - xAxis - yAxis, Perp(xAxis));
            real32 edge3 = Inner(d -  yAxis, Perp(yAxis));
            
            if (
                edge0 < 0
                && edge1 < 0
                && edge2 < 0
                && edge3 < 0
                )
            {
                BEGIN_TIMED_BLOCK(Slowly_FillPixel);
                
                v2 screenSpaceUV = {
                    (real32)x * invWidthMax, fixedCastY
                };

                real32 zDiff = ((real32)y - originY);
                
                real32 u = invXLengthSq * Inner(d, xAxis);
                real32 v = invYLengthSq * Inner(d, yAxis);

                real32 tX = (u * ((real32)(texture->Width - 2)));
                real32 tY = (v * ((real32)(texture->Height - 2)));

                int32 imgX = (int32)tX;
                int32 imgY = (int32)tY;

                real32 fX = tX - (real32)imgX;
                real32 fY = tY - (real32)imgY;

                bilinear_sample texelSamples = BilinearSample(texture, imgX, imgY);
                v4 texel = SRGBBilinearBlend(texelSamples, fX, fY);
                
                texel.a = texel.a * color.a;
                texel.rgb = texel.rgb * texel.a;

                if (normalMap)
                {
                    bilinear_sample normalSamples = BilinearSample(normalMap, imgX, imgY);
                    
                    v4 normalA = Unpack4x8(normalSamples.A);
                    v4 normalB = Unpack4x8(normalSamples.B);
                    v4 normalC = Unpack4x8(normalSamples.C);
                    v4 normalD = Unpack4x8(normalSamples.D);
                
                    v4 normal = Lerp(Lerp(normalA, fX, normalB), fY, Lerp(normalC, fX, normalD));

                    normal = UnscaleAndBiasNormal(normal);

                    // rotate normals on x/y axes with object

                    normal.xy = normal.x * normalXAxis + normal.y * normalYAxis;
                    normal.z *= zScale;
                    normal.xyz = Normalize(normal.xyz);

                    // NOTE: eye vector is always assumed to be from top
                    // v3 eyeVector = {0,0,1};

                    v3 bounceDirection = 2.f * normal.z * normal.xyz;
                    bounceDirection.z -= 1.0f;

                    real32 tEnvMap = bounceDirection.y;
                    real32 tFarMap = 0.0f;
                    render_environment_map *farMap = 0;
                    if (tEnvMap < -0.5f)
                    {
                        farMap = bottom;
                        tFarMap = -1.0f - 2.0f*tEnvMap;
                    }
                    else if (tEnvMap > 0.5f)
                    {
                        farMap = top;
                        tFarMap = 2.0f * (tEnvMap - 0.5f);
                    }

                    tFarMap *= tFarMap;
                    tFarMap *= tFarMap;

                    v4 lightColor = {0,0,0,1}; // SampleEnvironmentMap(screenSpaceUV, normal.xyz, normal.w, middle);

                    if (farMap) {
                        real32 distanceToMap = farMap->PositionZ - originZ - zDiff;
                        
                        v4 farMapColor = SampleEnvironmentMap(screenSpaceUV, bounceDirection, normal.w, farMap, distanceToMap);
                        farMapColor.a = 1.0f;
                        lightColor = Lerp(lightColor, tFarMap, farMapColor);
                    }
                    texel.rgb = texel.rgb + lightColor.rgb * texel.a;

#if 0
                    texel.rgb = (V3(0.5f,0.5f,0.5f) + 0.5*bounceDirection)*texel.a;
#endif
                }

                texel.rgb = Hadamard(texel.rgb, color.rgb);
                texel.r = Clamp01(texel.r);
                texel.b = Clamp01(texel.b);
                texel.g = Clamp01(texel.g);

                v4 destPixel = Unpack4x8(*pixel);
                destPixel = SRGB255_ToLinear1(destPixel);

                real32 inv_sa = (1.0f - texel.a);

                v4 blended = {
                    (inv_sa * destPixel.r + texel.r),
                    (inv_sa * destPixel.g + texel.g),
                    (inv_sa * destPixel.b + texel.b),
                    (texel.a + destPixel.a - destPixel.a * texel.a)
                };

                blended = Linear_1_ToSRGB255(blended);

                *pixel = (
                        ((uint32)(blended.a + 0.5f) << 24)
                        | ((uint32)(blended.r + 0.5f) << 16)
                        | ((uint32)(blended.g + 0.5f) << 8)
                        | ((uint32)(blended.b + 0.5f) << 0)
                        );

                END_TIMED_BLOCK(Slowly_FillPixel);
            }
            pixel++;

            END_TIMED_BLOCK(Slowly_TestPixel);
        }
        row += drawBuffer->Pitch;
    }

    END_TIMED_BLOCK(RenderRectangleSlowly);
}

internal void
RenderRectangleQuickly(loaded_bitmap *drawBuffer,v2 origin, v2 xAxis, v2 yAxis, v4 color, loaded_bitmap *texture, rectangle2i clipRect, bool32 even)
{
    BEGIN_TIMED_BLOCK(RenderRectangleHopefullyQuickly);
    
    // color.rgb *= color.a;

    real32 xAxisLen = Length(xAxis);
    real32 yAxisLen = Length(yAxis);
    
    real32 invXLengthSq = 1.0f / LengthSq(xAxis);
    real32 invYLengthSq = 1.0f / LengthSq(yAxis);

    v2 normalXAxis = (yAxisLen / xAxisLen)  * xAxis;
    v2 normalYAxis = (xAxisLen / yAxisLen) * yAxis;

    real32 zScale = (xAxisLen + yAxisLen) / 2;

    real32 originZ = 0.0f;
    real32 originY = (origin + 0.5f *xAxis + 0.5f * yAxis).y;
    
    rectangle2i fillRect = InvertedInfinityRectangle();

    v2 p[4] = {origin, origin + xAxis, origin + xAxis + yAxis, origin + yAxis};
    for (int pIndex=0; pIndex < ArrayCount(p); pIndex++)
    {
        v2 testP = p[pIndex];
        int32 floorX = FloorReal32ToInt32(testP.x);
        int32 ceilX = CeilReal32ToInt32(testP.x) + 1;
        int32 floorY = FloorReal32ToInt32(testP.y);
        int32 ceilY = CeilReal32ToInt32(testP.y) + 1;

        if (fillRect.MinX > floorX) { fillRect.MinX = floorX; }
        if (fillRect.MaxX < ceilX) { fillRect.MaxX = ceilX; }
        if (fillRect.MinY > floorY) { fillRect.MinY = floorY; }
        if (fillRect.MaxY < ceilY) { fillRect.MaxY = ceilY; }
    }

    fillRect = Intersect(fillRect, clipRect);

    if (!even == (fillRect.MinY % 2)) {
        fillRect.MinY += 1;
    }

    if (!HasArea(fillRect))
    {
        return;
    }

    __m128i startClipMask = _mm_set1_epi8(-1);
    __m128i endClipMask = _mm_set1_epi8(-1);

    int32 minRemainder = fillRect.MinX & 3;
    if (minRemainder)
    {
        fillRect.MinX = fillRect.MinX & ~3;
        switch (minRemainder) {
        case 1: startClipMask = _mm_slli_si128(startClipMask, 1 * 4); break; // shift is in bytes 
        case 2: startClipMask = _mm_slli_si128(startClipMask, 2 * 4); break; // shift is in bytes 
        case 3: startClipMask = _mm_slli_si128(startClipMask, 3 * 4); break; // shift is in bytes 
        }
    }

    int32 maxRemainder = fillRect.MaxX & 3;
    if (maxRemainder)
    {
        fillRect.MaxX = (fillRect.MaxX & ~3) + 4;
        switch (maxRemainder) {
        case 1: endClipMask = _mm_slli_si128(endClipMask, 3 * 4); break; // shift is in bytes 
        case 2: endClipMask = _mm_slli_si128(endClipMask, 2 * 4); break; // shift is in bytes 
        case 3: endClipMask = _mm_slli_si128(endClipMask, 1 * 4); break; // shift is in bytes 
        }
    }

    uint32 rowPitch = 2 * drawBuffer->Pitch;

    v2 nXAxis = invXLengthSq * xAxis;
    v2 nYAxis = invYLengthSq * yAxis;
    real32 inv255 = 1.0f / 255.0f;
    __m128 inv255_4x = _mm_set1_ps(inv255);

    __m128 inv255255_4x = _mm_set1_ps(inv255 * inv255);
    
    real32 val255 = 255.0f;
    __m128 val255_4x = _mm_set1_ps(val255);
    __m128 val255255_4x  = _mm_set1_ps(val255 * val255);

    __m128 zero_4x = _mm_set1_ps(0.0f);
    __m128 half_4x = _mm_set1_ps(0.5f);
    __m128 pixelMarginOffset_4x = _mm_set1_ps(0.5f);
    __m128 one_4x = _mm_set1_ps(1.0f);
    __m128 four_4x = _mm_set1_ps(4.0f);

    // color is multiplied by inversion to normalize after bilinear blend
    __m128 colorR_4x =  _mm_set1_ps(color.r);
    __m128 colorG_4x =  _mm_set1_ps(color.g);
    __m128 colorB_4x =  _mm_set1_ps(color.b);
    __m128 colorA_4x =  _mm_set1_ps(color.a);

    __m128 nXAxisx_4x = _mm_set1_ps(nXAxis.x);
    __m128 nXAxisy_4x = _mm_set1_ps(nXAxis.y);
    __m128 nYAxisx_4x = _mm_set1_ps(nYAxis.x);
    __m128 nYAxisy_4x = _mm_set1_ps(nYAxis.y);

    __m128 width_4x = _mm_set1_ps((real32(texture->Width)) - 2.0f);
    __m128 height_4x = _mm_set1_ps((real32(texture->Height)) - 2.0f);

    __m128i maskFF_4x = _mm_set1_epi32(0xff);

    void* textureMemory = texture->Memory;
    uint32 texturePitch = texture->Pitch;
    
    __m128i sizeuint32_x4  = _mm_set1_epi32(sizeof(uint32));
    __m128i texturePitch_x4 = _mm_set1_epi32(texturePitch);

    uint8 *row = (uint8 *)drawBuffer->Memory + drawBuffer->Pitch * fillRect.MinY + fillRect.MinX * BITMAP_BYTES_PER_PIXEL;

    __m128 pixelPxRow = _mm_setr_ps(
                                     (real32)(fillRect.MinX + 0 - origin.x),
                                     (real32)(fillRect.MinX + 1 - origin.x),
                                     (real32)(fillRect.MinX + 2 - origin.x),
                                     (real32)(fillRect.MinX + 3 - origin.x)
                                    );

    int32 minX = fillRect.MinX;
    int32 minY = fillRect.MinY;
    int32 maxX = fillRect.MaxX;
    int32 maxY = fillRect.MaxY;

    BEGIN_TIMED_BLOCK(Slowly_TestPixel);
    for (int32 y = minY; y < maxY; y+=2)
    {
        uint32 *pixel = (uint32 *)row;
        
        __m128 pixelPy = _mm_set1_ps((real32)y - origin.y);
        __m128 pixelPx = pixelPxRow;

        __m128i clipMask = startClipMask;
        
        for (int32 xI = minX; xI < maxX; xI+=4)
        {
            __m128i originalDest = _mm_load_si128((__m128i *)pixel);

            // NOTE: ORDER IS CORRECT
    
            __m128 u_4x = _mm_add_ps(_mm_mul_ps(pixelPx, nXAxisx_4x), _mm_mul_ps(pixelPy, nXAxisy_4x));
            __m128 v_4x = _mm_add_ps(_mm_mul_ps(pixelPx, nYAxisx_4x), _mm_mul_ps(pixelPy, nYAxisy_4x));

            __m128i writeMask =_mm_castps_si128(_mm_and_ps(
                _mm_and_ps(_mm_cmpge_ps(u_4x, zero_4x), _mm_cmple_ps(u_4x, one_4x))
                ,
                _mm_and_ps(_mm_cmpge_ps(v_4x, zero_4x), _mm_cmple_ps(v_4x, one_4x))
            ));

            writeMask = _mm_and_si128(writeMask, clipMask);

            if (_mm_movemask_epi8(writeMask))
            {
                u_4x = _mm_min_ps(_mm_max_ps(u_4x, zero_4x), one_4x);
                v_4x = _mm_min_ps(_mm_max_ps(v_4x, zero_4x), one_4x);

                // NOTE: images should have 1 pixel margin so that
                // bilinear blend is ok
                __m128 tx_4x = _mm_add_ps(pixelMarginOffset_4x, _mm_mul_ps(u_4x, width_4x));
                __m128 ty_4x = _mm_add_ps(pixelMarginOffset_4x, _mm_mul_ps(v_4x, height_4x));

                // truncating
                __m128i imgX_4x = _mm_cvttps_epi32(tx_4x);
                __m128i imgY_4x = _mm_cvttps_epi32(ty_4x);

                __m128 fX = _mm_sub_ps(tx_4x, _mm_cvtepi32_ps(imgX_4x));
                __m128 fY = _mm_sub_ps(ty_4x, _mm_cvtepi32_ps(imgY_4x));

                imgX_4x = _mm_slli_epi32(imgX_4x, 2); // NOTE: add sizeof(uint32)
                imgY_4x = _mm_mullo_epi32(imgY_4x, texturePitch_x4); // NOTE: we ignore high bits
                __m128i fetch_4x = _mm_add_epi32(imgX_4x, imgY_4x);

                // int32 imgX = MMi(imgX_4x, pIndex);
                // int32 imgY = MMi(imgY_4x, pIndex);
                int32 fetch0 = MMi(fetch_4x, 0);
                int32 fetch1 = MMi(fetch_4x, 1);
                int32 fetch2 = MMi(fetch_4x, 2);
                int32 fetch3 = MMi(fetch_4x, 3);

                uint8 *texelPtr0 = (uint8 *)textureMemory + fetch0;
                uint8 *texelPtr1 = (uint8 *)textureMemory + fetch1;
                uint8 *texelPtr2 = (uint8 *)textureMemory + fetch2;
                uint8 *texelPtr3 = (uint8 *)textureMemory + fetch3;

                __m128i sampleA = _mm_setr_epi32(*(uint32 *)texelPtr0,
                                                 *(uint32 *)texelPtr1,
                                                 *(uint32 *)texelPtr2,
                                                 *(uint32 *)texelPtr3);
                
                __m128i sampleB = _mm_setr_epi32(*(uint32 *)(texelPtr0 + sizeof(uint32)),
                                                 *(uint32 *)(texelPtr1 + sizeof(uint32)),
                                                 *(uint32 *)(texelPtr2 + sizeof(uint32)),
                                                 *(uint32 *)(texelPtr3 + sizeof(uint32)));
                
                __m128i sampleC = _mm_setr_epi32(*(uint32 *)(texelPtr0 + texturePitch),
                                                 *(uint32 *)(texelPtr1 + texturePitch),
                                                 *(uint32 *)(texelPtr2 + texturePitch),
                                                 *(uint32 *)(texelPtr3 + texturePitch));
                
                __m128i sampleD = _mm_setr_epi32(*(uint32 *)(texelPtr0 + texturePitch + sizeof(uint32)),
                                                 *(uint32 *)(texelPtr1 + texturePitch + sizeof(uint32)),
                                                 *(uint32 *)(texelPtr2 + texturePitch + sizeof(uint32)),
                                                 *(uint32 *)(texelPtr3 + texturePitch + sizeof(uint32)));


                __m128 texelAr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleA, 16), maskFF_4x));
                __m128 texelAg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleA, 8), maskFF_4x));
                __m128 texelAb = _mm_cvtepi32_ps(_mm_and_si128(sampleA, maskFF_4x));
                __m128 texelAa = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleA, 24), maskFF_4x));

                // v4 texelB = Unpack4x8(sampleB);
                __m128 texelBr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleB, 16), maskFF_4x));
                __m128 texelBg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleB, 8), maskFF_4x));
                __m128 texelBb = _mm_cvtepi32_ps(_mm_and_si128(sampleB, maskFF_4x));
                __m128 texelBa = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleB, 24), maskFF_4x));
                
                // v4 texelC = Unpack4x8(sampleC);
                __m128 texelCr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleC, 16), maskFF_4x));
                __m128 texelCg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleC, 8), maskFF_4x));
                __m128 texelCb = _mm_cvtepi32_ps(_mm_and_si128(sampleC, maskFF_4x));
                __m128 texelCa = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleC, 24), maskFF_4x));
            
                // v4 texelD = Unpack4x8(sampleD);
                __m128 texelDr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleD, 16), maskFF_4x));
                __m128 texelDg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleD, 8), maskFF_4x));
                __m128 texelDb = _mm_cvtepi32_ps(_mm_and_si128(sampleD, maskFF_4x));
                __m128 texelDa = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleD, 24), maskFF_4x));

                // v4 destPixel = Unpack4x8(*pixel);
                __m128 destr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(originalDest, 16), maskFF_4x));
                __m128 destg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(originalDest, 8), maskFF_4x));
                __m128 destb = _mm_cvtepi32_ps(_mm_and_si128(originalDest, maskFF_4x));
                __m128 desta = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(originalDest, 24), maskFF_4x));

                texelAr = _mm_mul_ps(texelAr, texelAr);
                texelAg = _mm_mul_ps(texelAg, texelAg);
                texelAb = _mm_mul_ps(texelAb, texelAb);
            
                texelBr = _mm_mul_ps(texelBr, texelBr);
                texelBg = _mm_mul_ps(texelBg, texelBg);
                texelBb = _mm_mul_ps(texelBb, texelBb);
            
                texelCr = _mm_mul_ps(texelCr, texelCr);
                texelCg = _mm_mul_ps(texelCg, texelCg);
                texelCb = _mm_mul_ps(texelCb, texelCb);
            
                texelDr = _mm_mul_ps(texelDr, texelDr);
                texelDg = _mm_mul_ps(texelDg, texelDg);
                texelDb = _mm_mul_ps(texelDb, texelDb);

                __m128 ifX = _mm_sub_ps(one_4x, fX);
                __m128 ifY = _mm_sub_ps(one_4x, fY);
                __m128 l_0 = _mm_mul_ps(ifX, ifY);
                __m128 l_1 = _mm_mul_ps(ifY, fX);
                __m128 l_2 = _mm_mul_ps(fY, ifX);
                __m128 l_3 = _mm_mul_ps(fY, fX);

                // real32 texelr = l_0 * MM(texelAr, pIndex) + l_1 * MM(texelBr, pIndex) + l_2 * MM(texelCr, pIndex) + l_3 * MM(texelDr, pIndex);
                __m128 texelr = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(l_0, texelAr), _mm_mul_ps(l_1, texelBr)), _mm_mul_ps(l_2, texelCr)), _mm_mul_ps(l_3, texelDr));
                __m128 texelg = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(l_0, texelAg), _mm_mul_ps(l_1, texelBg)), _mm_mul_ps(l_2, texelCg)), _mm_mul_ps(l_3, texelDg));
                __m128 texelb = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(l_0, texelAb), _mm_mul_ps(l_1, texelBb)), _mm_mul_ps(l_2, texelCb)), _mm_mul_ps(l_3, texelDb));
                __m128 texela = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(l_0, texelAa), _mm_mul_ps(l_1, texelBa)), _mm_mul_ps(l_2, texelCa)), _mm_mul_ps(l_3, texelDa));

                // NOTE(vlad): colorA_4x was multiplied by texel alpha
                texela = _mm_mul_ps(_mm_mul_ps(texela, colorA_4x), inv255_4x);
                texelr = _mm_mul_ps(_mm_mul_ps(texelr, colorR_4x), colorA_4x);
                texelg = _mm_mul_ps(_mm_mul_ps(texelg, colorG_4x), colorA_4x);
                texelb = _mm_mul_ps(_mm_mul_ps(texelb, colorB_4x), colorA_4x);

                texela = _mm_min_ps(_mm_max_ps(texela, zero_4x), val255255_4x);
                texelg = _mm_min_ps(_mm_max_ps(texelg, zero_4x), val255255_4x);
                texelb = _mm_min_ps(_mm_max_ps(texelb, zero_4x), val255255_4x);

                destr = _mm_mul_ps(destr, destr);
                destg = _mm_mul_ps(destg, destg);
                destb = _mm_mul_ps(destb, destb);
                desta = _mm_mul_ps(desta, inv255_4x);
            
                __m128 inv_sa = _mm_sub_ps(one_4x, texela);

                destr = _mm_add_ps(_mm_mul_ps(inv_sa, destr), texelr);
                destg = _mm_add_ps(_mm_mul_ps(inv_sa, destg), texelg);
                destb = _mm_add_ps(_mm_mul_ps(inv_sa, destb), texelb);
                desta = _mm_add_ps(_mm_mul_ps(inv_sa, desta), texela);

                destr = _mm_mul_ps(destr, _mm_rsqrt_ps(destr));
                destg = _mm_mul_ps(destg, _mm_rsqrt_ps(destg));
                destb = _mm_mul_ps(destb, _mm_rsqrt_ps(destb));
                desta = _mm_mul_ps(desta, val255_4x);

                // output to frame buffer
                __m128i intB = _mm_cvtps_epi32(destb);
                __m128i intR = _mm_cvtps_epi32(destr);
                __m128i intG = _mm_cvtps_epi32(destg);
                __m128i intA = _mm_cvtps_epi32(desta);
            
                intR = _mm_slli_epi32(intR, 16);
                intG = _mm_slli_epi32(intG, 8);
                intA = _mm_slli_epi32(intA, 24);
                __m128i argb = _mm_or_epi32(_mm_or_epi32(_mm_or_epi32(_mm_or_epi32(intB, intR), intB), intG), intA);

                __m128i maskedOut = _mm_or_si128(
                                                 _mm_and_si128(writeMask, argb),
                                                 _mm_andnot_si128(writeMask, originalDest)
                                                 );

                _mm_store_si128((__m128i *)pixel, maskedOut);
            }

            pixel += 4;
            pixelPx = _mm_add_ps(pixelPx, four_4x);

            if ((xI + 8) < maxX)
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
    END_TIMED_BLOCK_COUNTED(Slowly_TestPixel, (maxX - minX) * (maxY - minY) / 2);

    END_TIMED_BLOCK(RenderRectangleHopefullyQuickly);
}

internal void
ApplySaturation(loaded_bitmap *outputTarget, real32 level)
{
    uint8 *dstrow = (uint8 *)outputTarget->Memory;
    
    for (int y = 0;
         y < outputTarget->Height;
         ++y)
    {
        uint32 *dst = (uint32 *)dstrow;
        
        for (int x = 0;
             x < outputTarget->Width;
             ++x)
        {
            v4 destTexel = {
                (real32)((*dst >> 16) & 0xff),
                (real32)((*dst >> 8) & 0xff),
                (real32)((*dst >> 0) & 0xff),
                (real32)((*dst >> 24) & 0xff)
            };

            real32 avg = (destTexel.r + destTexel.g + destTexel.b) / 3.0f;
            v4 delta = {destTexel.r - avg, destTexel.g - avg, destTexel.b - avg, 0};
            v4 result = ToV4(avg, avg, avg, destTexel.a) + level * delta;

            *dst = (
                   ((uint32)(result.a + 0.5f) << 24)
                   | ((uint32)(result.r + 0.5f) << 16)
                   | ((uint32)(result.g + 0.5f) << 8)
                   | ((uint32)(result.b + 0.5f) << 0)
                    );
            
            *dst++;
        }
        
        dstrow += outputTarget->Pitch;
    }
}

internal void
RenderBitmap(loaded_bitmap *drawBuffer, loaded_bitmap *bitmap,
    real32 realX, real32 realY,
    v4 color)
{
    real32 cAlpha = color.a;
    
    int32 minX = RoundReal32ToInt32(realX);
    int32 minY = RoundReal32ToInt32(realY);
    int32 maxX = minX + bitmap->Width;
    int32 maxY = minY + bitmap->Height;
    
    int32 srcOffsetX = 0;
    if (minX < 0)
    {
        srcOffsetX = - minX;
        minX = 0;
    }

    int32 srcOffsetY = 0;
    if (minY < 0)
    {
        srcOffsetY = - minY;
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
    
    // TODO: sourcerow change to adjust for cliping
    // bitmap->Pitch * (bitmap->Height - 1)

    uint8 *srcrow = (uint8 *)bitmap->Memory + bitmap->Pitch * srcOffsetY + BITMAP_BYTES_PER_PIXEL * srcOffsetX;
    
    uint8 *dstrow = (uint8 *)drawBuffer->Memory + minX * BITMAP_BYTES_PER_PIXEL + drawBuffer->Pitch * minY;
    
    for (int y = minY;
         y < maxY;
         ++y)
    {
        uint32 *dst = (uint32 *)dstrow;
        uint32 * src = (uint32 *)srcrow;
        
        for (int x = minX;
             x < maxX;
             ++x)
        {
            v4 texel = {
                (real32)((*src >> 16) & 0xff),
                (real32)((*src >> 8) & 0xff),
                (real32)((*src >> 0) & 0xff),
                (real32)((*src >> 24) & 0xff)
            };

            texel = SRGB255_ToLinear1(texel);
            texel.rgb *= cAlpha;


            v4 destTexel = {
                (real32)((*dst >> 16) & 0xff),
                (real32)((*dst >> 8) & 0xff),
                (real32)((*dst >> 0) & 0xff),
                (real32)((*dst >> 24) & 0xff)
            };
            destTexel = SRGB255_ToLinear1(destTexel);

            real32 rsa = texel.a * cAlpha;
            real32 rda = destTexel.a;
            
            real32 inv_sa = (1.0f - rsa);

            v4 result = {
                inv_sa * destTexel.r + texel.r,
                inv_sa * destTexel.g + texel.g,
                inv_sa * destTexel.b + texel.b,
                (rsa + rda - rsa * rda)
            };

            result = Linear_1_ToSRGB255(result);
            
            *dst = (
                   ((uint32)(result.a + 0.5f) << 24)
                   | ((uint32)(result.r + 0.5f) << 16)
                   | ((uint32)(result.g + 0.5f) << 8)
                   | ((uint32)(result.b + 0.5f) << 0)
                    );
            
            *dst++;
            *src++;
        }
        
        dstrow += drawBuffer->Pitch;
        srcrow += bitmap->Pitch;
    }
}

internal void
RenderGradient(game_offscreen_buffer *buffer, int xOffset, int yOffset)
{
    uint8 *row = (uint8 *)buffer->Memory;
    for (int y=0; y < buffer->Height; ++y)
    {
        uint32 *pixel = (uint32 *)row;
        
        for (int x=0; x < buffer->Width; ++x)
        {
            uint8 b = (uint8)(x + xOffset);
            uint8 g = (uint8)(y + yOffset);
            
            *pixel++ = ((g << 8) | b);
        }
        
        row += buffer->Pitch;
    }
}

internal void
RenderSquareDot(loaded_bitmap *drawBuffer, real32 dotPositionX, real32 dotPositionY, real32 squareHalfWidth, v4 color)
{
    //RenderRectangle(drawBuffer, dotPositionX - squareHalfWidth, dotPositionY - squareHalfWidth, dotPositionX + squareHalfWidth, dotPositionY + squareHalfWidth, color);
}


internal render_group*
AllocateRenderGroup(memory_arena *arena, game_assets * assets, uint32 maxPushBufferSize, uint32 resolutionPixelX, uint32 resolutionPixelY)
{
    render_group *result = PushStruct(arena, render_group);

    result->Assets = assets;
    result->Transform = {};
    
    result->Transform.OffsetP = ToV3(0,0,0);
    result->Transform.Scale = 1.0f;

    if (maxPushBufferSize == 0) {
        maxPushBufferSize = (uint32)GetArenaSizeRemaining(arena);
    }
    
    result->PushBufferBase = (uint8 *)PushSize(arena, maxPushBufferSize);
    result->MaxPushBufferSize = maxPushBufferSize;
    result->PushBufferSize = 0;
    result->MissingResourceCount = 0;

    
    result->GlobalAlpha = 1.0f;

    return result;
}

internal void
MakePerspective(render_group *renderGroup, uint32 width, uint32 height, real32 focalLength, real32 distanceAboveTarget)
{
    renderGroup->Transform.Perspective = true;
    
    renderGroup->Transform.FocalLength = focalLength;
    renderGroup->Transform.DistanceToTarget = distanceAboveTarget;
    real32 widthOfMonitorInMeters = 0.635f;
    renderGroup->Transform.MetersToPixels = width / widthOfMonitorInMeters;

    real32 pixelsToMeters = 0.5f / renderGroup->Transform.MetersToPixels;
    renderGroup->Transform.ScreenCenter = 0.5f * ToV2i(width, height);
    renderGroup->MonitorHalfDimInMeters = pixelsToMeters * ToV2i(width, height);
}

internal void
MakeOrthographic(render_group *renderGroup, uint32 pixelWidth, uint32 pixelHeight, real32 metersToPixels)
{
    real32 pixelsToMeters = 1.0f / metersToPixels;

    renderGroup->Transform.Perspective = false;
    
    renderGroup->Transform.FocalLength = 1.0f;
    renderGroup->Transform.DistanceToTarget = 1.0f;
    renderGroup->Transform.MetersToPixels = metersToPixels;

    renderGroup->Transform.ScreenCenter = 0.5f * ToV2i(pixelWidth, pixelHeight);
    renderGroup->MonitorHalfDimInMeters = 0.5f * pixelsToMeters * ToV2i(pixelWidth, pixelHeight);
}

struct entity_basis_p_result
{
    v2 P;
    real32 Scale;
    bool32 IsValid;
};
inline entity_basis_p_result
GetTopLeftPointForEntityBasis(render_transform *transform, v3 p)
{
    entity_basis_p_result result = {};

    v3 entityBaseP = p + transform->OffsetP;
    
    if (transform->Perspective)
    {
        if (0) { transform->DistanceToTarget = 100.0f; }; // DEBUG: HIGH CAMERA VIEW
        
        real32 distanceToPointZ = transform->DistanceToTarget - entityBaseP.z;
        real32 nearClipPlane = 0.2f;

        if (distanceToPointZ > nearClipPlane) {
            v3 projectedXY = ToV3(entityBaseP.xy, 1.0f) * (transform->FocalLength / distanceToPointZ) * transform->MetersToPixels;
            result.P = transform->ScreenCenter + projectedXY.xy;
            result.Scale = projectedXY.z;
            result.IsValid = true;
        }
    }
    else
    {
        result.P = transform->ScreenCenter + entityBaseP.xy * transform->MetersToPixels;
        result.IsValid = true;
        result.Scale = transform->MetersToPixels;
    }
    
    return result;
}

#define PushRenderElement(group, type) (type *)PushRenderElement_(group, sizeof(type), RenderEntryType_##type)

inline void*
PushRenderElement_(render_group *grp, uint32 size, render_entry_type type)
{
    size += sizeof(render_entry_header);
    
    void *result = 0;
    if (grp->PushBufferSize + size < grp->MaxPushBufferSize)
    {
        render_entry_header* header = (render_entry_header *)(grp->PushBufferBase + grp->PushBufferSize);
        header->Type = type;

        result = ((uint8 *)header + sizeof(*header));
        grp->PushBufferSize += size;
    }
    else
    {
        InvalidCodePath;
    }
    return result;
}

inline void
PushBitmap(render_group *grp, loaded_bitmap *bmp, real32 height, v3 offset, v4 color = {1,1,1,1})
{
    v2 size = ToV2(height * bmp->WidthOverHeight, height);
    v2 align = Hadamard(bmp->AlignPercent, size);
    v3 p = offset - ToV3(align);
    
    entity_basis_p_result basisResult = GetTopLeftPointForEntityBasis(&grp->Transform, p);

    if (basisResult.IsValid)
    {
        render_entry_bitmap *renderEntry = PushRenderElement(grp, render_entry_bitmap);
        if (renderEntry)
        {
            renderEntry->P = basisResult.P;
            renderEntry->Size = basisResult.Scale * size;
            renderEntry->Bitmap = bmp;
            color.a = grp->GlobalAlpha;
            renderEntry->Color = color;
        }
    }
}

inline void
PushBitmap(render_group *group, bitmap_id id, real32 height, v3 offset, v4 color = {1,1,1,1})
{
    loaded_bitmap *bitmap = GetBitmap(group->Assets, id);
    if (bitmap)
    {
        PushBitmap(group, bitmap, height, offset, color);
    }
    else
    {
        LoadBitmap(group->Assets, id);
        group->MissingResourceCount++;
    }
}


inline void
PushPieceRect(render_group *grp, v3 offset, v2 dim, v4 color, bool32 renderFringes=false)
{
    v3 p = (offset - 0.5f * ToV3(dim));
    
    entity_basis_p_result basisResult = GetTopLeftPointForEntityBasis(&grp->Transform, p);

    if (basisResult.IsValid)
    {
        render_entry_rectangle *renderEntry = PushRenderElement(grp, render_entry_rectangle);

        if (renderEntry) {
            renderEntry->P = basisResult.P;
            color.a = grp->GlobalAlpha;
            renderEntry->Color = color;
            renderEntry->Dim = basisResult.Scale * dim;
            renderEntry->renderFringes = renderFringes;
        }
    }
}

inline void
PushPieceRectOutline(render_group *grp, v3 offset, v2 dim, v4 color)
{
    // TODO: redo to offset is left corner + dim
    
    real32 lineWidth = 0.05f;
    real32 halfW = 0.5f * lineWidth;
    v2 halfD = 0.5f * dim;
    // top bottom
    PushPieceRect(grp, ToV3(offset.x, offset.y+halfD.y, offset.z), ToV2(dim.x, lineWidth), color);
    PushPieceRect(grp, ToV3(offset.x, offset.y-halfD.y, offset.z), ToV2(dim.x, lineWidth), color);

    // left/right
    PushPieceRect(grp, ToV3(offset.x-halfD.x, offset.y, offset.z), ToV2(lineWidth, dim.y), color);
    PushPieceRect(grp, ToV3(offset.x+halfD.x, offset.y, offset.z), ToV2(lineWidth, dim.y), color);
}

inline void
PushSaturationFilter(render_group *grp, real32 level)
{
    render_entry_saturation *renderEntry = PushRenderElement(grp, render_entry_saturation);

    if (renderEntry) {
        renderEntry->Level = level;
    }
}

inline void
PushDefaultColorFill(render_group *grp, v4 color)
{
    render_entry_clear *renderEntry = PushRenderElement(grp, render_entry_clear);

    if (renderEntry) {
        renderEntry->Color = color;
    }
}

inline void
PushDefaultRenderClear(render_group *grp)
{
    PushDefaultColorFill(grp, ToV4(143.0f / 255.0f, 118.0f / 255.0f, 74.0f / 255.0f, 1.0f));
}

inline void
PushScreenSquareDot(render_group *grp, v2 screenPosition, real32 width, v4 color)
{
    entity_basis_p_result basisResult = GetTopLeftPointForEntityBasis(&grp->Transform, ToV3(screenPosition));

    if (basisResult.IsValid)
    {
        render_entry_screen_dot *renderEntry = PushRenderElement(grp, render_entry_screen_dot);
        if (renderEntry)
        {
            renderEntry->P = basisResult.P;
            renderEntry->Width = basisResult.Scale * width;
            renderEntry->Color = color;
        }
    }
}


inline void
PushCoordinateSystem(render_group *grp, v2 origin, v2 x, v2 y, v4 color, loaded_bitmap *texture, loaded_bitmap *normalMap,
                     render_environment_map *top, render_environment_map *middle, render_environment_map *bottom)
{

    #if 0
    entity_basis_p_result basisResult = GetTopLeftPointForEntityBasis(grp, screenDim, &entry->EntityBasis);

    if (basisResult.IsValid)
    {
        render_entry_coordinate_system *renderEntry = PushRenderElement(grp, render_entry_coordinate_system);
        if (renderEntry)
        {
            renderEntry->Origin = origin;
            renderEntry->XAxis = x;
            renderEntry->YAxis = y;
            renderEntry->Color = color;
            renderEntry->Texture = texture;
            renderEntry->NormalMap = normalMap;
            renderEntry->Top = top;
            renderEntry->Middle = middle;
            renderEntry->Bottom = bottom;

            uint32 pIndex = 0;
            for (real32 pX=0; pX <= 1; pX += 0.25f)
            {
                for (real32 pY=0; pY <= 1; pY += 0.25f)
                {
                    // renderEntry->Points[pIndex] = V2(pX, pY);
                    pIndex++;
                }
            }
        }
    }

#endif
}

inline void
PushHitpoints(render_group *renderGroup, sim_entity *simEntity)
{
    // health bars
    if (simEntity->HitPointMax >= 1) {
        v2 healthDim = ToV2(0.1f, 0.1f);
        real32 spacingX = healthDim.x;
        real32 firstY = 0.3f;
        real32 firstX = -1 * (real32)(simEntity->HitPointMax - 1) * spacingX;
        for (uint32 idx=0;
             idx < simEntity->HitPointMax;
             idx++)
        {
            hit_point *hp = simEntity->HitPoints + idx;
            v4 color = ToV4(1.0f, 0.0f, 0.0f, 1);

            if (hp->FilledAmount == 0)
            {
                color.r = 0.2f;
                color.g = 0.2f;
                color.b = 0.2f;
            }
                    
            PushPieceRect(renderGroup, ToV3(firstX, firstY , 0), healthDim, color);
            firstX += spacingX + healthDim.x;
        }
    }
}

internal void
RenderGroup(loaded_bitmap *outputTarget, render_group *renderGroup, rectangle2i clipRect, bool32 even)
{
    BEGIN_TIMED_BLOCK(RenderGroupToOutput);

    real32 NullPixelsToMeters = 1.0f;
    
    for (uint32 baseAddress=0;
         baseAddress < renderGroup->PushBufferSize;
         )
    {
        render_entry_header *entryHeader = (render_entry_header *)(renderGroup->PushBufferBase + baseAddress);
        baseAddress += sizeof(*entryHeader);
        void *voidEntry = (render_entry_header *)(renderGroup->PushBufferBase + baseAddress);

        switch (entryHeader->Type)
        {
        case RenderEntryType_render_entry_clear: {
            render_entry_clear *entry = (render_entry_clear *) voidEntry;

            RenderRectangle(outputTarget, 0, 0, (real32)outputTarget->Width, (real32)outputTarget->Height, entry->Color, clipRect, even);
            
            baseAddress += sizeof(*entry);
        } break;
        case RenderEntryType_render_entry_bitmap: {
            render_entry_bitmap *entry = (render_entry_bitmap *) voidEntry;

            Assert(entry->Bitmap);

#if 0
            RenderBitmap(outputTarget, entry->Bitmap, entry->P.x, entry->P.y, entry->Color);
#else

#if 0
            RenderRectangleSlowly(outputTarget,
                                  basisResult.P, ToV2(entry->Size.x,0), ToV2(0, entry->Size.y), entry->Color,
                                  entry->Bitmap, 0,
                                  0, 0, 0,
                                  NullPixelsToMeters
                                  );

#else
            RenderRectangleQuickly(outputTarget,
                                   entry->P, ToV2(entry->Size.x,0), ToV2(0, entry->Size.y), entry->Color,
                                   entry->Bitmap, clipRect, even);
#endif
            #endif
                
            baseAddress += sizeof(*entry);
        } break;
        case RenderEntryType_render_entry_rectangle: {
            render_entry_rectangle *entry = (render_entry_rectangle *) voidEntry;
            RenderRectangle(outputTarget, entry->P.x, entry->P.y, entry->P.x + entry->Dim.x, entry->P.y + entry->Dim.y, entry->Color, clipRect, even, entry->renderFringes);
            
            baseAddress += sizeof(*entry);
        } break;
        case RenderEntryType_render_entry_screen_dot: {
            render_entry_screen_dot *entry = (render_entry_screen_dot *) voidEntry;

            RenderSquareDot(outputTarget, entry->P.x, entry->P.y, 0.5f * entry->Width, entry->Color);

            baseAddress += sizeof(*entry);
        } break;
        case RenderEntryType_render_entry_saturation: {
            render_entry_saturation *entry = (render_entry_saturation *) voidEntry;

            ApplySaturation(outputTarget, entry->Level);

            baseAddress += sizeof(*entry);
        } break;
        case RenderEntryType_render_entry_coordinate_system: {
            render_entry_coordinate_system *entry = (render_entry_coordinate_system *) voidEntry;

            v2 p = entry->Origin;
            v2 dim = {5, 5};
            RenderRectangleSlowly(outputTarget,
                                  entry->Origin, entry->XAxis, entry->YAxis, entry->Color,
                                  entry->Texture, entry->NormalMap,
                                  entry->Top, entry->Middle, entry->Bottom,
                                  NullPixelsToMeters
                                  );

            baseAddress += sizeof(*entry);
        } break;
            
        InvalidDefaultCase

        }
    }

    END_TIMED_BLOCK(RenderGroupToOutput);
}

struct tile_render_work
{
    render_group *RenderGroup;
    loaded_bitmap *OutputTarget;
    rectangle2i ClipRect;
};

PLATFORM_WORK_QUEUE_CALLBACK(DoTiledRenderWork)
{
    tile_render_work *work = (tile_render_work *)data;

    RenderGroup(work->OutputTarget, work->RenderGroup, work->ClipRect, true);
    RenderGroup(work->OutputTarget, work->RenderGroup, work->ClipRect, false);
}

internal void
TiledRenderGroup(platform_work_queue *renderQueue, loaded_bitmap *outputTarget, render_group *renderGroup)
{
    // TODO: fix this
    int32 padd = 0;

    int32 const tileCountX = 3;
    int32 const tileCountY = 2;
    tile_render_work workArray[tileCountX*tileCountY];

    // ensure that memory is aligned for 4 pixels
    Assert(((uintptr)outputTarget->Memory & 15) == 0);

    // TODO: width rounding
    // TODO: round to 4
    int tileWidth = outputTarget->Width  / tileCountX;
    int tileHeight = outputTarget->Height / tileCountY;

    tileWidth = ((tileWidth + 3) / 4) * 4;
    int finalTileWidth = outputTarget->Width - (tileWidth * (tileCountX  - 1));

    int32 workIndex = 0;
    for (int tileY =0;
         tileY < tileCountY;
         tileY++)
    {
        for (int tileX =0;
             tileX < tileCountX;
             tileX++)
        {
            rectangle2i clipRect;
            clipRect.MinX = tileX * tileWidth;
            clipRect.MinY = tileY * tileHeight;
            clipRect.MaxX = tileX * tileWidth + tileWidth;
            clipRect.MaxY = tileY * tileHeight + tileHeight;

            if (tileX == tileCountX - 1) {
                clipRect.MaxX = outputTarget->Width;
            }
            if (tileY == tileCountY - 1) {
                clipRect.MaxY = outputTarget->Height;
            }
            
            tile_render_work *workItem = workArray + workIndex++;
            workItem->ClipRect = clipRect;
            workItem->RenderGroup = renderGroup;
            workItem->OutputTarget = outputTarget;


            PlatformAPI.AddEntry(renderQueue, DoTiledRenderWork , workItem);
        }
    }

    PlatformAPI.CompleteAllWork(renderQueue);
}

internal void
SingleThreadedRenderGroup(loaded_bitmap *outputTarget, render_group *renderGroup)
{
    Assert(((uintptr)outputTarget->Memory & 15) == 0);

    rectangle2i clipRect;
    clipRect.MinX = 0;
    clipRect.MinY = 0;
    clipRect.MaxX = outputTarget->Width;
    clipRect.MaxY = outputTarget->Height;

    tile_render_work work;
    work.ClipRect = clipRect;
    work.RenderGroup = renderGroup;
    work.OutputTarget = outputTarget;
    
    DoTiledRenderWork(0, &work);
}

inline v2
Unproject(render_group * group, v2 projectedXY, real32 atDistanceFromCamera)
{
    v2 rawXY = (atDistanceFromCamera / group->Transform.FocalLength) * projectedXY;
    return rawXY;
}

inline rectangle2
GetCameraRectangleAtDistance(render_group *group, real32 distanceFromCamera)
{
    v2 rawXY = Unproject(group, group->MonitorHalfDimInMeters, distanceFromCamera);
    rectangle2 result = RectCenterHalfDim(ToV2(0,0), rawXY);
    return result;
}

inline rectangle2
GetCameraRectangleAtTarget(render_group *group)
{
    rectangle2 result = GetCameraRectangleAtDistance(group, group->Transform.DistanceToTarget);
    return result;
}

inline bool32
AllBitmapsAreValid(render_group *renderGroup)
{
    bool32 result = renderGroup->MissingResourceCount == 0;
    return result;
}
