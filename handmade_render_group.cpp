uint32 GetUintColor(color color)
{
    uint32 uColor = (uint32)(
                             (RoundReal32ToInt32(color.Alpha * 255.0f) << 24) |
                             (RoundReal32ToInt32(color.Red * 255.0f) << 16) |
                             (RoundReal32ToInt32(color.Green * 255.0f) << 8) |
                             (RoundReal32ToInt32(color.Blue * 255.0f) << 0)
                             );
    
    return uColor;
}

internal
void
RenderRectangle(loaded_bitmap *drawBuffer, real32 realMinX, real32 realMinY, real32 realMaxX, real32 realMaxY, color color)
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

// TODO: rewrite use v2!!!
internal
void
RenderRectangleOutline(loaded_bitmap *drawBuffer, real32 minX, real32 minY, real32 maxX, real32 maxY, color color)
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
            
            real32 sa = (real32)((*src >> 24) & 0xff) * cAlpha;
            real32 sr = (real32)((*src >> 16) & 0xff) * cAlpha;
            real32 sg = (real32)((*src >> 8) & 0xff) * cAlpha;
            real32 sb = (real32)((*src >> 0) & 0xff) * cAlpha;

            // TODO: put cAlpha back
            

            real32 da = (real32)((*dst >> 24) & 0xff);
            real32 dr = (real32)((*dst >> 16) & 0xff);
            real32 dg = (real32)((*dst >> 8) & 0xff);
            real32 db = (real32)((*dst >> 0) & 0xff);

            real32 rsa = (sa / 255.0f);
            real32 rda = (da / 255.0f);
            
            real32 inv_sa = (1.0f - rsa);

            /*
            real32 a = Clamp(0, inv_sa * da + sa, 255.0f); // TODO: why clamp?
            real32 r = Clamp(0, inv_sa * dr + sr, 255.0f); // TODO: why clamp?
            real32 g = Clamp(0, inv_sa * dg + sg, 255.0f); // TODO: why clamp?
            real32 b = Clamp(0, inv_sa * db + sb, 255.0f); // TODO: why clamp?
            */

            real32 a = 255.0f * (rsa + rda - rsa * rda);
            real32 r = inv_sa * dr + sr;
            real32 g = inv_sa * dg + sg;
            real32 b = inv_sa * db + sb;

            if (cAlpha < 1.0f) {
                int j = 0;
            }
            
            
            *dst = (
                   ((uint32)(a + 0.5f) << 24)
                   | ((uint32)(r + 0.5f) << 16)
                   | ((uint32)(g + 0.5f) << 8)
                   | ((uint32)(b + 0.5f) << 0)
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
RenderSquareDot(loaded_bitmap *drawBuffer, int32 dotPositionX, int32 dotPositionY)
{
    real32 squareHalfWidth = 10;
    color playerColor = {1.0f, 0.0f, 1.0f, 1.0f};
    
    RenderRectangle(drawBuffer, (real32)dotPositionX - squareHalfWidth, (real32)dotPositionY - squareHalfWidth, (real32)dotPositionX + squareHalfWidth, (real32)dotPositionY + squareHalfWidth, playerColor);
}

void
DefaultColorRenderBuffer(loaded_bitmap *drawBuffer)
{
    color bgColor;
    bgColor.Red = (143.0f / 255.0f);
    bgColor.Green = (118.0f / 255.0f);
    bgColor.Blue = (74.0f / 255.0f);
    bgColor.Alpha = 1.0f;
    
    RenderRectangle(drawBuffer, 0, 0, (real32)drawBuffer->Width, (real32)drawBuffer->Height, bgColor);
}

void
ClearRenderBuffer(loaded_bitmap *drawBuffer)
{
    color bgColor;
    bgColor.Red = 0.0f;
    bgColor.Green = 0.0f;
    bgColor.Blue = 0.0f;
    bgColor.Alpha = 0.0f;
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

inline render_entry_header *
PushRenderElement_(render_group *grp, uint32 size, render_entry_type type)
{
    render_entry_header *result = 0;
    if (grp->PushBufferSize + size < grp->MaxPushBufferSize)
    {
        result = (render_entry_header *)(grp->PushBufferBase + grp->PushBufferSize);
        result->Type = type;
        grp->PushBufferSize += size;
    }
    else
    {
        InvalidCodePath;
    }
    return result;
}

inline void
PushPiece(render_group *grp, loaded_bitmap *bmp, v2 offset, real32 offsetZ, v2 align, real32 alpha = 1.0f, real32 EntitiyZC=1.0f)
{
    render_entry_bitmap *renderPice = PushRenderElement(grp, render_entry_bitmap);

    if (renderPice) {
        renderPice->EntityBasis.Basis = grp->DefaultBasis;
        renderPice->EntityBasis.Offset = grp->MetersToPixels * V2(offset.X, -offset.Y) - align;
        renderPice->EntityBasis.OffsetZ = offsetZ * grp->MetersToPixels;
        renderPice->EntityBasis.EntitiyZC = EntitiyZC;
        renderPice->Bitmap = bmp;
        renderPice->Alpha = alpha;
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
        renderEntry->EntityBasis.Offset = V2(grp->MetersToPixels * offset.X - halfDim.X, grp->MetersToPixels * offset.Y - halfDim.Y);
        renderEntry->EntityBasis.OffsetZ = offsetZ * grp->MetersToPixels;
        renderEntry->Color = color;
    }
}

inline void
PushPieceRectOutline(render_group *grp, v3 offset, v2 dim, v4 color)
{
    real32 lineWidth = 0.05f;
    // top bottom
    PushPieceRect(grp, V2(offset.X, offset.Y-dim.Y/2), offset.Z, V2(dim.X, lineWidth), color);
    PushPieceRect(grp, V2(offset.X, offset.Y+dim.Y/2), offset.Z, V2(dim.X, lineWidth), color);

    // left/right
    PushPieceRect(grp, V2(offset.X-dim.X/2, offset.Y), offset.Z, V2(lineWidth, dim.Y), color);
    PushPieceRect(grp, V2(offset.X+dim.X/2, offset.Y), offset.Z, V2(lineWidth, dim.Y), color);
}

inline void
DrawHitpoints(render_group *renderGroup, sim_entity *simEntity)
{
    // health bars
    if (simEntity->HitPointMax >= 1) {
        v2 healthDim = V2(0.2f, 0.2f);
        real32 spacingX = healthDim.X;
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
                color.R = 0.2f;
                color.G = 0.2f;
                color.B = 0.2f;
            }
                    
            PushPieceRect(renderGroup, V2(firstX, firstY), 0, healthDim, color);
            firstX += spacingX + healthDim.X;
        }
    }
}

inline v2
GetTopLeftPointForEntityBasis(render_group *renderGroup, v2 screenCenter, render_entity_basis *entityBasis)
{
    v3 entityBaseP = entityBasis->Basis->P;
    real32 zFugde = 1.0f + 0.1f * entityBaseP.Z;

    real32 entityX = screenCenter.X + entityBaseP.X * renderGroup->MetersToPixels * zFugde;
    real32 entityY = screenCenter.Y - entityBaseP.Y * renderGroup->MetersToPixels * zFugde;
    real32 entityZ = -entityBaseP.Z * renderGroup->MetersToPixels - entityBasis->OffsetZ;

    v2 result = {entityX + entityBasis->Offset.X, entityY + entityBasis->Offset.Y + entityZ};

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

        switch (entryHeader->Type)
        {
        case RenderEntryType_render_entry_clear: {
            render_entry_clear *entry = (render_entry_clear *) entryHeader;
            
            
            baseAddress += sizeof(*entry);
        } break;
        case RenderEntryType_render_entry_bitmap: {
            render_entry_bitmap *entry = (render_entry_bitmap *) entryHeader;

            v2 center = GetTopLeftPointForEntityBasis(renderGroup, screenCenter, &entry->EntityBasis);

            Assert(entry->Bitmap);
            RenderBitmap(outputTarget, entry->Bitmap, center.X, center.Y + entry->EntityBasis.EntitiyZC, entry->Alpha);
                
            baseAddress += sizeof(*entry);
        } break;
        case RenderEntryType_render_entry_rectangle: {
            render_entry_rectangle *entry = (render_entry_rectangle *) entryHeader;

            // TODO: color
            color c;
            c.Red = entry->Color.R;
            c.Green = entry->Color.G;
            c.Blue = entry->Color.B;
            c.Alpha = 1.0f;

            v2 center = GetTopLeftPointForEntityBasis(renderGroup, screenCenter, &entry->EntityBasis);
            RenderRectangle(outputTarget, center.X, center.Y, center.X + entry->Dim.X, center.Y + entry->Dim.Y, c);
            
            baseAddress += sizeof(*entry);
        } break;
        // case RenderGroupType_Clear: {
            
        // } break;
            InvalidDefaultCase
        }
    }
}
