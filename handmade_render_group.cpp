inline uint32
GetUintColor(v4 color)
{
    uint32 uColor = (uint32)(
                             (RoundReal32ToInt32(color.a * 255.0f) << 24) |
                             (RoundReal32ToInt32(color.r * 255.0f) << 16) |
                             (RoundReal32ToInt32(color.g * 255.0f) << 8) |
                             (RoundReal32ToInt32(color.b * 255.0f) << 0)
                             );
    
    return uColor;
}

internal void
RenderRectangle(loaded_bitmap *drawBuffer, real32 realMinX, real32 realMinY, real32 realMaxX, real32 realMaxY, v4 color)
{
    uint32 uColor = GetUintColor(color);
    
    int32 minX = RoundReal32ToInt32(realMinX);
    int32 minY = RoundReal32ToInt32(realMinY);
    int32 maxX = RoundReal32ToInt32(realMaxX);
    int32 maxY = RoundReal32ToInt32(realMaxY);
    
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
    
    uint8 *row = (uint8 *)drawBuffer->Memory + drawBuffer->Pitch * minY + minX * BITMAP_BYTES_PER_PIXEL;
    for (int y = minY; y < maxY; ++y)
    {
        uint32 *pixel = (uint32 *)row;
        
        for (int x = minX; x < maxX; ++x)
        {
            *(uint32 *)pixel = uColor;
            pixel++;
        }
        row += drawBuffer->Pitch;
    }
}

inline v4
SRGB255_ToLinear1(v4 color)
{
    v4 result;

    real32 inv255 = 1.0f / 255.0f;

    result.r = Square(color.r * inv255);
    result.g = Square(color.g * inv255);
    result.b = Square(color.b * inv255);
    result.a = color.a * inv255;
    
    return result;
}

inline v4
Linear_1_ToSRGB255(v4 color)
{
    v4 result;

    real32 val255 = 255.0f;
    
    result.r = SquareRoot(color.r) * val255;
    result.g = SquareRoot(color.g) * val255;
    result.b = SquareRoot(color.b) * val255;
    result.a = color.a * val255;

    return result;
}

inline v4
Unpack4x8(uint32 packed)
{
    v4 result = {
        (real32)((packed >> 16) & 0xff),
        (real32)((packed >> 8) & 0xff),
        (real32)((packed >> 0) & 0xff),
        (real32)((packed >> 24) & 0xff)
    };
    return result;
}

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
SampleEnvironmentMap(v2 screenSpaceUV, v3 sampleDirection, real32 roughness, render_environment_map *map, real32 heightOfSkyBox)
{
    uint32 roughIndex = (uint32)(roughness * ((real32)ArrayCount(map->LevelsOfDetails) - 1.0f) + 0.5f);
    Assert(roughIndex < ArrayCount(map->LevelsOfDetails));
    
    loaded_bitmap *level = map->LevelsOfDetails + roughIndex;
    
    real32 uvsPerMeter = 0.01f;
    real32 coef = (uvsPerMeter * heightOfSkyBox) / sampleDirection.y;
    // TODO: make sure we know what direction Z should go and why
    v2 offset = coef * V2(sampleDirection.x, sampleDirection.z);

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
    color.rgb *= color.a;

    int32 widthMax = drawBuffer->Width - 1;
    int32 heightMax = drawBuffer->Height - 1;

    real32 invWidthMax = 1.0f / (real32)widthMax;
    real32 invHeightMax = 1.0f / (real32)heightMax;

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

    real32 invXLengthSq = 1.0f / LengthSq(xAxis);
    real32 invYLengthSq = 1.0f / LengthSq(yAxis);;

    real32 xAxisLen = Length(xAxis);
    real32 yAxisLen = Length(yAxis);

    v2 normalXAxis = (yAxisLen / xAxisLen)  * xAxis;
    v2 normalYAxis = (xAxisLen / yAxisLen) * yAxis;

    real32 zScale = (xAxisLen + yAxisLen) / 2;

    real32 originZ = 0.0f;
    real32 originY = (0.5 *xAxis + (0.5 * yAxis)).y;
    real32 fixedCastY = invHeightMax * (origin.y + originY); // TODO: specify this as param separatly
    
    
    for (int32 y = minY; y <= maxY; ++y)
    {
        uint32 *pixel = (uint32 *)row;
        
        for (int32 x = minX; x <= maxX; ++x)
        {
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
                v2 screenSpaceUV = {
                    (real32)x * invWidthMax, fixedCastY
                };

                real32 zDiff = pixelsToMeters * ((real32)y - originY);
                
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
                    normal.xyz = Normalize(normal.xyz); // TODO: is this needed?

                    // NOTE: eye vector is always assumed to be from top
                    // v3 eyeVector = {0,0,1};

                    // TODO: compute a bounce based on normals

                    v3 bounceDirection = 2.f * normal.z * normal.xyz;
                    bounceDirection.z -= 1.0f;
                    bounceDirection.z = -bounceDirection.z;

                    real32 tEnvMap = bounceDirection.y;
                    real32 tFarMap = 0.0f;
                    render_environment_map *farMap = 0;
                    if (tEnvMap < -0.5)
                    {
                        farMap = bottom;
                        tFarMap = -1.0f - 2.0f*tEnvMap;
                    }
                    else if (tEnvMap > 0.5)
                    {
                        farMap = top;
                        tFarMap = 2.0f * (tEnvMap - 0.5f);
                    }

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

                texel = Hadamard(texel, color);
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
            }
            pixel++;
        }
        row += drawBuffer->Pitch;
    }
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
            v4 result = V4(avg, avg, avg, destTexel.a) + level * delta;

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
    real32 cAlpha = 1.0f)
{
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

// TODO: rewrite use v2!!!
internal void
RenderRectangleOutline(loaded_bitmap *drawBuffer, real32 minX, real32 minY, real32 maxX, real32 maxY, v4 color)
{
    real32 lineWidth = 2.0f;
    real32 lineHalfWidth = lineWidth * 0.5f;
    // top bottom
    RenderRectangle(drawBuffer, minX, minY - lineHalfWidth, maxX, minY + lineHalfWidth, color);
    RenderRectangle(drawBuffer, minX, maxY - lineHalfWidth, maxX, maxY + lineHalfWidth, color);

    // left/right
    RenderRectangle(drawBuffer, minX-lineHalfWidth, minY, minX+lineHalfWidth, maxY, color);
    RenderRectangle(drawBuffer, maxX-lineHalfWidth, minY, maxX+lineHalfWidth, maxY, color);
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
    RenderRectangle(drawBuffer, dotPositionX - squareHalfWidth, dotPositionY - squareHalfWidth, dotPositionX + squareHalfWidth, dotPositionY + squareHalfWidth, color);
}

void
ClearRenderBuffer(loaded_bitmap *drawBuffer)
{
    v4 bgColor = {0.0f, 0.0f, 0.0f, 0.0f};
    RenderRectangle(drawBuffer, 0, 0, (real32)drawBuffer->Width, (real32)drawBuffer->Height, bgColor);
}



internal render_group *
AllocateRenderGroup(memory_arena *arena, real32 metersToPixels, uint32 maxPushBufferSize)
{
    render_group *result = PushStruct(arena, render_group);
    result->MetersToPixels = metersToPixels;
    result->PushBufferBase = (uint8 *)PushSize(arena, maxPushBufferSize);
    result->DefaultBasis = PushStruct(arena, render_basis);
    result->DefaultBasis->P = V3(0,0,0);

    result->MaxPushBufferSize = maxPushBufferSize;
    result->PushBufferSize = 0;

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
PushSaturationFilter(render_group *grp, real32 level)
{
    render_entry_saturation *renderEntry = PushRenderElement(grp, render_entry_saturation);

    if (renderEntry) {
        renderEntry->Level = level;
    }
}

inline void
PushPiece(render_group *grp, loaded_bitmap *bmp, v2 offset, real32 offsetZ, v2 align, real32 alpha = 1.0f, real32 EntitiyZC=1.0f)
{
    render_entry_bitmap *renderEntry = PushRenderElement(grp, render_entry_bitmap);

    if (renderEntry) {
        renderEntry->EntityBasis.Basis = grp->DefaultBasis;
        renderEntry->EntityBasis.Offset = grp->MetersToPixels * V2(offset.x, -offset.y) - align;
        renderEntry->EntityBasis.OffsetZ = offsetZ * grp->MetersToPixels;
        renderEntry->EntityBasis.EntitiyZC = EntitiyZC;
        renderEntry->Bitmap = bmp;
        renderEntry->Alpha = alpha;
    }
}


inline void
PushPieceRect(render_group *grp, v2 offset, real32 offsetZ, v2 dim, v4 color)
{
    render_entry_rectangle *renderEntry = PushRenderElement(grp, render_entry_rectangle);

    if (renderEntry) {
        renderEntry->Dim = grp->MetersToPixels * dim;
        
        v2 halfDim = 0.5f * renderEntry->Dim;
        
        renderEntry->EntityBasis.Basis = grp->DefaultBasis;
        renderEntry->EntityBasis.Offset = V2(grp->MetersToPixels * offset.x - halfDim.x, grp->MetersToPixels * offset.y - halfDim.y);
        renderEntry->EntityBasis.OffsetZ = offsetZ * grp->MetersToPixels;
        renderEntry->Color = color;
    }
}

inline void
PushPieceRectOutline(render_group *grp, v2 offset, real32 offsetZ, v2 dim, v4 color)
{
    real32 lineWidth = 0.05f;
    // top bottom
    PushPieceRect(grp, V2(offset.x, -offset.y-dim.y/2), offsetZ, V2(dim.x, lineWidth), color);
    PushPieceRect(grp, V2(offset.x, -offset.y+dim.y/2), offsetZ, V2(dim.x, lineWidth), color);

    // left/right
    PushPieceRect(grp, V2(offset.x-dim.x/2, -offset.y), offsetZ, V2(lineWidth, dim.y), color);
    PushPieceRect(grp, V2(offset.x+dim.x/2, -offset.y), offsetZ, V2(lineWidth, dim.y), color);
}

inline void
PushDefaultRenderClear(render_group *grp)
{
    render_entry_clear *renderEntry = PushRenderElement(grp, render_entry_clear);

    if (renderEntry) {
        renderEntry->Color.r = (143.0f / 255.0f);
        renderEntry->Color.g = (118.0f / 255.0f);
        renderEntry->Color.b = (74.0f / 255.0f);
        renderEntry->Color.a = 1.0f;
    }
}

inline void
PushScreenSquareDot(render_group *grp, v2 screenPosition, real32 width, v4 color)
{
    render_entry_screen_dot *renderEntry = PushRenderElement(grp, render_entry_screen_dot);
    if (renderEntry)
    {
        renderEntry->P = screenPosition;
        renderEntry->Width = width;
        renderEntry->Color = color;
    }
}

inline void
PushCoordinateSystem(render_group *grp, v2 origin, v2 x, v2 y, v4 color, loaded_bitmap *texture, loaded_bitmap *normalMap,
                     render_environment_map *top, render_environment_map *middle, render_environment_map *bottom)
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

inline void
DrawHitpoints(render_group *renderGroup, sim_entity *simEntity)
{
    // health bars
    if (simEntity->HitPointMax >= 1) {
        v2 healthDim = V2(0.2f, 0.2f);
        real32 spacingX = healthDim.x;
        real32 firstY = 0.3f;
        real32 firstX = -1 * (real32)(simEntity->HitPointMax - 1) * spacingX;
        for (uint32 idx=0;
             idx < simEntity->HitPointMax;
             idx++)
        {
            hit_point *hp = simEntity->HitPoints + idx;
            v4 color = V4(1.0f, 0.0f, 0.0f, 1);

            if (hp->FilledAmount == 0)
            {
                color.r = 0.2f;
                color.g = 0.2f;
                color.b = 0.2f;
            }
                    
            PushPieceRect(renderGroup, V2(firstX, firstY), 0, healthDim, color);
            firstX += spacingX + healthDim.x;
        }
    }
}

inline v2
GetTopLeftPointForEntityBasis(render_group *renderGroup, v2 screenCenter, render_entity_basis *entityBasis)
{
    v3 entityBaseP = entityBasis->Basis->P;
    real32 zFugde = 1.0f + 0.1f * entityBaseP.z;

    real32 entityX = screenCenter.x + entityBaseP.x * renderGroup->MetersToPixels * zFugde;
    real32 entityY = screenCenter.y - entityBaseP.y * renderGroup->MetersToPixels * zFugde;
    real32 entityZ = -entityBaseP.z * renderGroup->MetersToPixels - entityBasis->OffsetZ;

    v2 result = {entityX + entityBasis->Offset.x, entityY + entityBasis->Offset.y + entityZ};

    return result;
}

internal void
RenderGroup(loaded_bitmap *outputTarget, render_group *renderGroup)
{
    v2 screenCenter = 0.5f * V2i(outputTarget->Width, outputTarget->Height);
    
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

            RenderRectangle(outputTarget, 0, 0, (real32)outputTarget->Width, (real32)outputTarget->Height, entry->Color);
            
            baseAddress += sizeof(*entry);
        } break;
        case RenderEntryType_render_entry_bitmap: {
            render_entry_bitmap *entry = (render_entry_bitmap *) voidEntry;
#if 0

            v2 center = GetTopLeftPointForEntityBasis(renderGroup, screenCenter, &entry->EntityBasis);

            Assert(entry->Bitmap);
            RenderBitmap(outputTarget, entry->Bitmap, center.x, center.y + entry->EntityBasis.EntitiyZC, entry->Alpha);
#endif
                
            baseAddress += sizeof(*entry);
        } break;
        case RenderEntryType_render_entry_rectangle: {
            render_entry_rectangle *entry = (render_entry_rectangle *) voidEntry;

            v2 center = GetTopLeftPointForEntityBasis(renderGroup, screenCenter, &entry->EntityBasis);
            RenderRectangle(outputTarget, center.x, center.y, center.x + entry->Dim.x, center.y + entry->Dim.y, entry->Color);
            
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
                                  1.0f / renderGroup->MetersToPixels
                                  );

            baseAddress += sizeof(*entry);
        } break;
            
        InvalidDefaultCase

        }
    }
}
