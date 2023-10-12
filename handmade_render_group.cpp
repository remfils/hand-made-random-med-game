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

    result->PieceCount = 0;

    return result;
}

inline void *
PushRenderElement(render_group *grp, uint32 size)
{
    void *result = 0;
    if (grp->PushBufferSize + size < grp->MaxPushBufferSize)
    {
        result = (grp->PushBufferBase + grp->PushBufferSize);
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
    entity_visible_piece *renderPice = (entity_visible_piece *)PushRenderElement(grp, sizeof(entity_visible_piece));

    renderPice->Basis = grp->DefaultBasis;

    renderPice->Bitmap = bmp;
    renderPice->Offset = grp->MetersToPixels * V2(offset.X, -offset.Y) - align;
    renderPice->Alpha = alpha;
    renderPice->EntitiyZC = EntitiyZC;
        
    grp->PieceCount++;
}


inline void
PushPieceRect(render_group *grp, v2 offset, real32 offsetZ, v2 dim, v4 color)
{
    entity_visible_piece *renderPice = (entity_visible_piece *)PushRenderElement(grp, sizeof(entity_visible_piece));

    renderPice->Basis = grp->DefaultBasis;

    renderPice->Bitmap = 0;
    renderPice->Offset = grp->MetersToPixels * offset;
    renderPice->OffsetZ = offsetZ;
    renderPice->Dim = grp->MetersToPixels * dim;
    renderPice->Color = color;
        
    grp->PieceCount++;
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
