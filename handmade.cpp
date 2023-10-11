#include "handmade.h"

#define PushSize(arena, type) (type *)PushSize_(arena, sizeof(type))
#define PushArray(arena, count,  type) (type *)PushSize_(arena, count * sizeof(type))

inline void*
PushSize_(memory_arena *arena, memory_index size)
{
    Assert((arena->Used + size) <= arena->Size);

    void *result = arena->Base + arena->Used;
    arena->Used += size;
    return result;
}

#define ZeroStruct(instance) ZeroSize(sizeof(instance), &(instance))
inline void
ZeroSize(memory_index size, void *ptr)
{
    // TODO(casey): performance
    uint8 *byte = (uint8*)ptr;
    while (size--)
    {
        *byte++ = 0;
    }
}

inline temporary_memory
BeginTemporaryMemory(memory_arena *arena)
{
    temporary_memory result;

    result.Arena = arena;
    result.Used = arena->Used;
    result.Arena->TempCount++;

    return result;
}

inline void
EndTemporaryMemory(temporary_memory tempMem)
{
    memory_arena *arena = tempMem.Arena;
    Assert(arena->Used >= tempMem.Used)
    arena->Used = tempMem.Used;
    Assert(arena->TempCount > 0);
    arena->TempCount--;
}

inline void
CheckArena(memory_arena *arena)
{
    Assert(arena->TempCount == 0);
}

#include "handmade_world.cpp"
#include "random.h"
#include "handmade_sim_region.cpp"
#include "handmade_sim_entity.cpp"



global_variable real32 tSine = 0;

#pragma pack(push, 1)
struct bitmap_header {
    uint16 FileType;
    uint32 FileSize;
    uint16 Reserved1;
    uint16 Reserved2;
    uint32 BitmapOffset;
    uint32 Size;
    int32 Width;
    int32 Height;
    uint16 Planes;
    uint16 BitsPerPixel;
    uint32 Compression; // wtf
    uint32 ImageSize; // wtf
    uint32 XPixelsPerMeter; // wtf
    uint32 YPixelsPerMeter; // wtf
    uint32 ColorsInColorTable; // wtf
    uint32 ImportantColorCount; // wtf
    uint32 RedMask;
    uint32 GreenMask;
    uint32 BlueMask;
    uint32 AlphaMask;
};
#pragma pack(pop)


internal loaded_bitmap DEBUGLoadBMP(thread_context *thread, debug_platform_read_entire_file ReadEntireFile, char *filename)
{
    // NOTE: bitmap order is AABBGGRR, bottom row is first
    
    debug_read_file_result readResult = ReadEntireFile(thread, filename);
    
    loaded_bitmap result = {};
    
    if (readResult.Content)
    {
        void *content = readResult.Content;
        
        bitmap_header *header = (bitmap_header *) content;
        
        result.Width = header->Width;
        result.Height = header->Height;
        
        uint8 *pixels = (uint8 *)((uint8 *)content + header->BitmapOffset);
        result.Memory = pixels;

        uint32 redMask = header->RedMask;
        uint32 greenMask = header->GreenMask;
        uint32 blueMask = header->BlueMask;
        uint32 alphaMask = ~(redMask | greenMask | blueMask);

        bit_scan_result redScan = FindLeastSignificantSetBit(redMask);
        bit_scan_result greenScan = FindLeastSignificantSetBit(greenMask);
        bit_scan_result blueScan = FindLeastSignificantSetBit(blueMask);
        bit_scan_result alphaScan = FindLeastSignificantSetBit(alphaMask);

        uint32 alphaShiftUp = 24;
        int32 alphaShiftDown = (int32)alphaScan.Index;
        
        uint32 redShiftUp = 16;
        int32 redShiftDown = (int32)redScan.Index;

        uint32 greenShiftUp = 8;
        int32 greenShiftDown = (int32)greenScan.Index;

        uint32 blueShiftUp = 0;
        int32 blueShiftDown = (int32)blueScan.Index;


        // NOTE: why this is not needed?
        uint32 *srcDest = (uint32 *)pixels;
        for (int32 y =0;
             y < header->Height;
             ++y)
        {
            for (int32 x =0;
                 x < header->Width;
                 ++x)
            {
                uint32 color = *srcDest;

                real32 r = (real32)((color & redMask) >> redShiftDown);
                real32 g = (real32)((color & greenMask) >> greenShiftDown);
                real32 b = (real32)((color & blueMask) >> blueShiftDown);
                real32 a = (real32)((color & alphaMask) >> alphaShiftDown);

                real32 ra = a / 255.0f;

                r *= ra;
                g *= ra;
                b *= ra;

                *srcDest++ = ((uint32)(a + 0.5f) << alphaShiftUp)
                    | ((uint32)(r + 0.5f) << redShiftUp)
                    | ((uint32)(g + 0.5f) << greenShiftUp)
                    | ((uint32)(b + 0.5f) << blueShiftUp);
            }
        }
    }
    else
    {
        // todo: error
    }

    result.Pitch = -result.Width * BITMAP_BYTES_PER_PIXEL;
    result.Memory = (uint8 *)result.Memory - result.Pitch * (result.Height - 1);
    
    return result;
}


struct add_low_entity_result
{
    low_entity *Low;
    uint32 Index;
};

internal add_low_entity_result
AddLowEntity(game_state *gameState, entity_type type, world_position pos)
{
    Assert(gameState->LowEntityCount < ArrayCount(gameState->LowEntities));

    add_low_entity_result result;

    uint32 ei = gameState->LowEntityCount++;

    low_entity *entityLow = gameState->LowEntities + ei;
    *entityLow = {};
    
    entityLow->Sim.Type = type;
    entityLow->WorldP = NullPosition();

    entityLow->Sim.Collision = gameState->NullCollision;

    ChangeEntityLocation(&gameState->WorldArena, gameState->World, ei, entityLow, pos);

    result.Low = entityLow;
    result.Index = ei;
    return result;
}

internal add_low_entity_result
AddGroundedEntity(game_state *gameState, entity_type type, world_position pos, sim_entity_collision_volume_group *collision)
{
    add_low_entity_result result = AddLowEntity(gameState, type, pos);
    
    result.Low->Sim.Collision = collision;
    
    return result;
}

inline void
InitHitpoints(low_entity *lowEntity, uint32 hitpointCount, uint32 filledAmount = HIT_POINT_SUB_COUNT)
{
    lowEntity->Sim.HitPointMax = hitpointCount;
    
    lowEntity->Sim.HitPoints[2] = lowEntity->Sim.HitPoints[1] = lowEntity->Sim.HitPoints[0];

    for (uint32 i=0; i<hitpointCount;++i)
    {
        lowEntity->Sim.HitPoints[i].FilledAmount = filledAmount;
    }
}

inline world_position
ChunkPositionFromTilePosition(world * world, int32 absTileX, int32 absTileY, int32 absTileZ, v3 additionalOffset=V3(0,0,0))
{
    world_position basePosition = {};

    real32 tileSideInMeters = 2.0f;
    real32 tileDepthInMeters = 3.0f;

    v3 tileDim = V3(tileSideInMeters, tileSideInMeters, tileDepthInMeters);
    v3 offset = Hadamard(tileDim, V3((real32)absTileX, (real32)absTileY, (real32)absTileZ));

    world_position result = MapIntoChunkSpace(world, basePosition, offset + additionalOffset);

    return result;
}

internal add_low_entity_result
AddSword(game_state *gameState)
{
    // v3 dim = {25 / gameState->MetersToPixels, 63 / gameState->MetersToPixels, 0.5f};
    add_low_entity_result lowEntityResult = AddLowEntity(gameState, EntityType_Sword, NullPosition());
    
    lowEntityResult.Low->Sim.Collision = gameState->SwordCollision;

    AddFlag(&lowEntityResult.Low->Sim, EntityFlag_Nonspacial);

    return lowEntityResult;
}

internal add_low_entity_result
AddMonster(game_state *gameState, uint32 absX, uint32 absY, uint32 absZ)
{   
    world_position pos = ChunkPositionFromTilePosition(gameState->World, absX, absY, absZ);
    add_low_entity_result lowEntityResult = AddGroundedEntity(gameState, EntityType_Monster, pos, gameState->MonsterCollision);

    AddFlag(&lowEntityResult.Low->Sim, EntityFlag_Collides);

    InitHitpoints(lowEntityResult.Low, 8, 1);

    return lowEntityResult;
}

internal add_low_entity_result
AddStairway(game_state *gameState, uint32 absX, uint32 absY, uint32 absZ)
{
    world_position pos = ChunkPositionFromTilePosition(gameState->World, absX, absY, absZ);
    add_low_entity_result lowEntityResult = AddGroundedEntity(gameState, EntityType_Stairwell, pos, gameState->StairCollision);

    AddFlag(&lowEntityResult.Low->Sim, EntityFlag_Collides);

    lowEntityResult.Low->Sim.WalkableHeight = gameState->TypicalFloorHeight;
    lowEntityResult.Low->Sim.WalkableDim = lowEntityResult.Low->Sim.Collision->TotalVolume.Dim.XY;
    
    return lowEntityResult;
}

internal add_low_entity_result
AddStandartRoom(game_state *gameState, uint32 absX, uint32 absY, uint32 absZ)
{
    world_position pos = ChunkPositionFromTilePosition(gameState->World, absX, absY, absZ);
    add_low_entity_result lowEntityResult = AddGroundedEntity(gameState, EntityType_Space, pos, gameState->StandardRoomCollision);
    
    AddFlag(&lowEntityResult.Low->Sim, EntityFlag_Traversable);
    
    return lowEntityResult;
}

internal add_low_entity_result
AddFamiliar(game_state *gameState, uint32 absX, uint32 absY, uint32 absZ)
{
    world_position pos = ChunkPositionFromTilePosition(gameState->World, absX, absY, absZ);
    add_low_entity_result lowEntityResult = AddGroundedEntity(gameState, EntityType_Familiar, pos, gameState->FamiliarCollision);

    lowEntityResult.Low->WorldP._Offset.X += 8;
   
    AddFlag(&lowEntityResult.Low->Sim, EntityFlag_Collides);
    AddFlag(&lowEntityResult.Low->Sim, EntityFlag_Movable);

    return lowEntityResult;
}

internal add_low_entity_result
AddPlayer(game_state *gameState)
{
    add_low_entity_result lowEntityResult = AddGroundedEntity(gameState, EntityType_Hero, gameState->CameraPosition, gameState->PlayerCollision);

    AddFlag(&lowEntityResult.Low->Sim, EntityFlag_Collides);
    AddFlag(&lowEntityResult.Low->Sim, EntityFlag_Movable);

    InitHitpoints(lowEntityResult.Low, 3);

    if (gameState->CameraFollowingEntityIndex == 0) {
        gameState->CameraFollowingEntityIndex = lowEntityResult.Index;
    }

    add_low_entity_result swordEntity = AddSword(gameState);
    entity_reference swordRef;
    swordRef.Index = swordEntity.Index;
    lowEntityResult.Low->Sim.Sword = swordRef;

    return lowEntityResult;
}

internal add_low_entity_result
AddWall(game_state *gameState, uint32 absX, uint32 absY, uint32 absZ)
{
    world_position pos = ChunkPositionFromTilePosition(gameState->World, absX, absY, absZ);
    add_low_entity_result lowEntityResult = AddGroundedEntity(gameState, EntityType_Wall, pos, gameState->WallCollision);

    AddFlag(&lowEntityResult.Low->Sim, EntityFlag_Collides);

    return lowEntityResult;
}

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
GameOutputSound(game_sound_output_buffer *soundBuffer, game_state *gameState)
{
    int16 *sampleOut = soundBuffer->Samples;

    real32 toneHz = 256;
    real32 toneVolume = 1000;
    real32 wavePeriod = (real32)soundBuffer->SamplesPerSecond / toneHz;
    
    for (int sampleIndex = 0; sampleIndex < soundBuffer->SampleCount; ++sampleIndex)
    {
        // TODO: bugs in sound, if want to hear broken sound loop unkomment
        // 
        // real32 sineValue = sinf(tSine);
        // int16 sampleValue = (int16)(sineValue * toneVolume);

        int16 sampleValue = 0;
        
        *sampleOut++ = sampleValue;
        *sampleOut++ = sampleValue;

        tSine += (2.0f * Pi32 * 1.0f) / wavePeriod;
    }
}

internal
void
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

// TODO: rewrite use v2!!!
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

void
RenderPlayer(loaded_bitmap *drawBuffer, int playerX, int playerY)
{
    real32 squareHalfWidth = 10;
    color playerColor;
    playerColor.Red = 1;
    playerColor.Green = 0;
    playerColor.Blue = 1;
    playerColor.Alpha = 1.0f;
    
    RenderRectangle(drawBuffer, (real32)playerX - squareHalfWidth, (real32)playerY - squareHalfWidth, (real32)playerX + squareHalfWidth, (real32)playerY + squareHalfWidth, playerColor);
}

internal void
InitializeArena(memory_arena *arena, memory_index size, void *base)
{
    arena->Size = size;
    arena->Base = (uint8 *)base;
    arena->Used = 0;
    arena->TempCount = 0;
}


inline void
PushPiece(entity_visible_piece_group *grp, loaded_bitmap *bmp, v3 offset, v2 align, real32 alpha = 1.0f, real32 EntitiyZC=1.0f)
{
    Assert(grp->PieceCount < ArrayCount(grp->Pieces));
    entity_visible_piece *renderPice = grp->Pieces + grp->PieceCount;

    renderPice->Bitmap = bmp;
    renderPice->Offset = grp->GameState->MetersToPixels * offset - V3(align.X, align.Y, 0);
    renderPice->Alpha = alpha;
    renderPice->EntitiyZC = EntitiyZC;
        
    grp->PieceCount++;
}

inline void
PushPieceRect(entity_visible_piece_group *grp, v3 offset, v2 dim, v4 color)
{
    Assert(grp->PieceCount < ArrayCount(grp->Pieces));
    entity_visible_piece *renderPice = grp->Pieces + grp->PieceCount;

    renderPice->Offset = grp->GameState->MetersToPixels * offset;
    renderPice->Dim = grp->GameState->MetersToPixels * dim;
    renderPice->R = color.R;
    renderPice->G = color.G;
    renderPice->B = color.B;
        
    grp->PieceCount++;
}

inline void
PushPieceRectOutline(entity_visible_piece_group *grp, v3 offset, v2 dim, v4 color)
{
    real32 lineWidth = 0.05f;
    // top bottom
    PushPieceRect(grp, V3(offset.X, offset.Y-dim.Y/2, offset.Z), V2(dim.X, lineWidth), color);
    PushPieceRect(grp, V3(offset.X, offset.Y+dim.Y/2, offset.Z), V2(dim.X, lineWidth), color);

    // left/right
    PushPieceRect(grp, V3(offset.X-dim.X/2, offset.Y, offset.Z), V2(lineWidth, dim.Y), color);
    PushPieceRect(grp, V3(offset.X+dim.X/2, offset.Y, offset.Z), V2(lineWidth, dim.Y), color);
}

inline void
DrawHitpoints(entity_visible_piece_group *pieceGroup, sim_entity *simEntity)
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
                    
            PushPieceRect(pieceGroup, V3(firstX, firstY, 0), healthDim, color);
            firstX += spacingX + healthDim.X;
        }
    }
}

sim_entity_collision_volume_group *
MakeNullCollision(game_state *gameState)
{
    // TODO: fundamental types arena
    // gameState->WorldArena;

    sim_entity_collision_volume_group *group = PushSize(&gameState->WorldArena, sim_entity_collision_volume_group);
    group->VolumeCount = 0;
    group->Volumes = 0;
    group->TotalVolume.Offset = V3(0,0, 0);
    // TODO: make negative ?
    group->TotalVolume.Dim = V3(0,0, 0);
    return group;
}

sim_entity_collision_volume_group *
MakeSimpleGroundedCollision(game_state *gameState, real32 x, real32 y, real32 z)
{
    // TODO: fundamental types arena
    // gameState->WorldArena;

    sim_entity_collision_volume_group *group = PushSize(&gameState->WorldArena, sim_entity_collision_volume_group);
    group->VolumeCount = 1;
    group->Volumes = PushArray(&gameState->WorldArena, group->VolumeCount, sim_entity_collision_volume);
    group->TotalVolume.Offset = V3(0,0, 0.5f * z);
    group->TotalVolume.Dim = V3(x, y, z);
    group->Volumes[0] = group->TotalVolume;

    return group;
}

internal void
FillGroundChunk(transient_state *tranState, game_state *gameState, ground_buffer *groundBuffer, world_position *chunkP)
{
    loaded_bitmap drawBuffer = tranState->GroundBitmapTemplate;
    drawBuffer.Memory = groundBuffer->Memory;

    groundBuffer->P = *chunkP;
    
    // TODO: look into wang hashing here
    random_series series = CreateRandomSeed(139 * chunkP->ChunkX + 25*chunkP->ChunkY -3*chunkP->ChunkZ);
    
    uint32 randomIndex = 0;

    v2 center = 0.5f * V2u(drawBuffer.Width, drawBuffer.Height);

    for (uint32 grassIndex=0; grassIndex < 100; grassIndex++)
    {
        v2 p = {
            RandomUnilateral(&series), RandomUnilateral(&series)
        };

        loaded_bitmap *bitmap;
        if (RandomChoice(&series, 2)) {
            bitmap = gameState->Grass + RandomChoice(&series, ArrayCount(gameState->Grass));
        } else {
            bitmap = gameState->Ground + RandomChoice(&series, ArrayCount(gameState->Ground));
        }
        
        v2 grassCenter = 0.5f * V2u(bitmap->Width, bitmap->Height);
        
        RenderBitmap(&drawBuffer, bitmap, center.X + center.X * p.X - grassCenter.X, center.X + center.X * p.Y - grassCenter.Y);
    }
}

internal void
ClearBitmap(loaded_bitmap *bmp)
{
    if (bmp->Memory) {
        int32 totalBitmapSize = bmp->Pitch * bmp->Height;
        ZeroSize(totalBitmapSize, bmp->Memory);
    }
}

internal loaded_bitmap
MakeEmptyBitmap(memory_arena *arena, int32 width, int32 height, bool32 clearToZero=true)
{
    loaded_bitmap result;

    result.Width = width;
    result.Height = height;
    result.Pitch = width * BITMAP_BYTES_PER_PIXEL;

    int32 totalBitmapSize = result.Pitch * result.Height;
    result.Memory = PushSize_(arena, totalBitmapSize);

    if (clearToZero) {
        ClearBitmap(&result);
    }

    return result;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(game_state) <= memory->PermanentStorageSize);
    
    game_state *gameState = (game_state *)memory->PermanentStorage;

    real32 screenWidthInMeters = buffer->Width * gameState->PixelsToMeters;
    real32 screenHeightInMeters = buffer->Height * gameState->PixelsToMeters;
    
    // real32 metersToPixels = (real32)TileSideInPixels / (real32)TileSideInMeters;
    
    if (!memory->IsInitialized)
    {
        uint32 groundBufferWidth = 256;
        uint32 groundBufferHeight = 256;
        
        real32 metersToPixels = 32.0f;
        gameState->MetersToPixels = metersToPixels;
        gameState->PixelsToMeters = 1.0f / metersToPixels;
        gameState->TypicalFloorHeight = 3.0f;

        real32 tileSideInMeters = 2.0f;
        real32 tileDepthInMeters = 3.0f;
        
        uint32 tilesPerScreenWidth = 17;
        uint32 tilesPerScreenHeight = 9;

        v3 chunkDimInMeters = V3(groundBufferWidth * gameState->PixelsToMeters, groundBufferHeight * gameState->PixelsToMeters, gameState->TypicalFloorHeight);

        
        // TODO: sub arena start partitioning memory space
        InitializeArena(&gameState->WorldArena, memory->PermanentStorageSize - sizeof(game_state), (uint8 *)memory->PermanentStorage + sizeof(game_state));

        AddLowEntity(gameState, EntityType_NULL, NullPosition());


        gameState->World = PushSize(&gameState->WorldArena, world);
        InitializeWorld(gameState->World, chunkDimInMeters);

        gameState->NullCollision = MakeNullCollision(gameState);
        gameState->SwordCollision = MakeSimpleGroundedCollision(gameState, 01.f, 0.1f, 0.1f);
        gameState->StairCollision = MakeSimpleGroundedCollision(gameState, tileSideInMeters, 2.0f * tileSideInMeters, 1.05f * tileDepthInMeters);
        gameState->PlayerCollision = MakeSimpleGroundedCollision(gameState, 0.75f, 0.4f, 0.5f);
        gameState->MonsterCollision = MakeSimpleGroundedCollision(gameState, 0.75f, 0.4f, 0.5f);
        gameState->FamiliarCollision = MakeSimpleGroundedCollision(gameState, 0.75f, 0.4f, 0.1f);
        gameState->WallCollision = MakeSimpleGroundedCollision(gameState, tileSideInMeters, tileSideInMeters, tileDepthInMeters);
        gameState->StandardRoomCollision = MakeSimpleGroundedCollision(gameState, tilesPerScreenWidth * tileSideInMeters, tilesPerScreenHeight * tileSideInMeters, 0.9f * tileDepthInMeters);


        gameState->LoadedBitmap = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/new-bg-code.bmp");

        gameState->EnemyDemoBitmap = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/test2/enemy_demo.bmp");
        gameState->FamiliarDemoBitmap = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/test2/familiar_demo.bmp");
        gameState->WallDemoBitmap = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/test2/wall_demo.bmp");
        gameState->SwordDemoBitmap = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/test2/sword_demo.bmp");
        gameState->StairwayBitmap = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/old_random_med_stuff/Stairway2.bmp");

        gameState->Grass[0] = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/grass001.bmp");
        gameState->Grass[1] = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/grass002.bmp");
        gameState->Ground[0] = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/ground001.bmp");
        gameState->Ground[1] = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/ground002.bmp");
        
        gameState->HeroBitmaps[0].Align = V2(60, 195);
        gameState->HeroBitmaps[0].Character = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/old_random_med_stuff/stand_right.bmp");
        
        gameState->HeroBitmaps[1].Align = V2(60, 185);
        gameState->HeroBitmaps[1].Character = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/old_random_med_stuff/stand_up.bmp");
        
        gameState->HeroBitmaps[2].Align = V2(60, 195);
        gameState->HeroBitmaps[2].Character = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/old_random_med_stuff/stand_left.bmp");
        
        gameState->HeroBitmaps[3].Align = V2(60, 185);
        gameState->HeroBitmaps[3].Character = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/old_random_med_stuff/stand_down.bmp");
        
        gameState->XOffset = 0;
        gameState->YOffset = 0;
        
        // generate tiles
        uint32 screenBaseX = 0;
        uint32 screenX = screenBaseX;

        uint32 screenBaseY = 0;
        uint32 screenY = screenBaseY;

        uint32 screenBaseZ = 0;
        uint32 absTileZ = screenBaseZ;

        uint32 screenIndex = 10;
        
        bool isDoorLeft = false;
        bool isDoorRight = false;
        bool isDoorTop = false;
        bool isDoorBottom = false;
        bool isDoorUp = false;
        bool isDoorDown = false;
        
        uint32 drawZDoorCounter = 0;
        uint32 famimiliarMaxCount = 1;

        random_series generatorSeries = CreateRandomSeed(0);
        
        while (screenIndex--) {
            uint32 rand;
            if (drawZDoorCounter > 0) {
                rand = RandomChoice(&generatorSeries, 2);
            } else {
                rand = RandomChoice(&generatorSeries, 3);
            }

            // DEBUG, no up and down movement currently
            // rand = RandomChoice(&generatorSeries, 3);
            rand = RandomChoice(&generatorSeries, 2);
            
            if (rand == 0) {
                isDoorRight = true;
            }
            else if (rand == 1) {
                isDoorTop = true;
            } else {
                drawZDoorCounter = 2;
                if (absTileZ == screenBaseZ) {
                    isDoorUp = true;
                } else {
                    isDoorDown = true;
                }
            }

            AddStandartRoom(gameState, screenX * tilesPerScreenWidth + tilesPerScreenWidth/2, screenY * tilesPerScreenHeight + tilesPerScreenHeight/2, absTileZ);

            for (uint32 tileY = 0;
                 tileY < tilesPerScreenHeight;
                 ++tileY)
            {
                for (uint32 tileX = 0;
                     tileX < tilesPerScreenWidth;
                     ++tileX)
                {
                    uint32 absTileX = screenX * tilesPerScreenWidth + tileX;
                    uint32 absTileY = screenY * tilesPerScreenHeight + tileY;

                    if (famimiliarMaxCount > 0 && tileX == 4 && tileY == 4)
                    {
                        // AddFamiliar(gameState, absTileX, absTileY, absTileZ);
                        famimiliarMaxCount--;
                    }

                    bool isWallTile = false;

                    if (tileX == 0) {
                        if (tileY == (tilesPerScreenHeight / 2) && isDoorLeft) {
                            isWallTile = false;
                        } else {
                            isWallTile = true;
                        }
                    }
                    if (tileX == (tilesPerScreenWidth -1)) {
                        if (tileY == (tilesPerScreenHeight / 2) && isDoorRight) {
                            isWallTile = false;
                        } else {
                            isWallTile = true;
                        }
                    }
                    if (tileY == 0) {
                        if (tileX == (tilesPerScreenWidth / 2) && isDoorBottom) {
                            isWallTile = false;
                        } else {
                            isWallTile = true;
                        }
                    }
                    if (tileY == (tilesPerScreenHeight -1)) {
                        if (tileX == (tilesPerScreenWidth / 2) && isDoorTop) {
                            isWallTile = false;
                        } else {
                            isWallTile = true;
                        }
                    }

                    // TODO: one door span for two chunks?
                    if (drawZDoorCounter != 0 && isDoorUp) {
                        if (tileY == 4 && ((tileX - 1) == tilesPerScreenWidth / 2)) {
                            AddStairway(gameState, absTileX, absTileY, absTileZ);
                        }
                    }
                    
                    if (drawZDoorCounter != 0 && isDoorDown) {
                        if (tileY == (tilesPerScreenHeight / 2) - 2 && (tileX + 1 == tilesPerScreenWidth / 2)) {
                            AddStairway(gameState, absTileX, absTileY, absTileZ);
                        }
                    }
                    
                    if (isWallTile)
                    {
                        if (screenIndex > 6) {
                            AddWall(gameState, absTileX, absTileY, absTileZ);
                        }
                    }
                }   
            }
            
            if (rand == 2) {
                if (isDoorUp) {
                    ++absTileZ;
                }
                else if (isDoorDown) {
                    --absTileZ;
                }
            }
            
            if (drawZDoorCounter > 0) {
                --drawZDoorCounter;
                
                if (drawZDoorCounter == 0) {
                    isDoorUp = false;
                    isDoorDown = false;
                } else {
                    if (isDoorDown) {
                        isDoorDown = false;
                        isDoorUp = true;
                    } else if (isDoorUp) {
                        isDoorDown = true;
                        isDoorUp = false;
                    }
                }
            }
            
            isDoorLeft = isDoorRight;
            isDoorBottom = isDoorTop;
            isDoorRight = false;
            isDoorTop = false;
            
            if (rand == 0) {
                screenX += 1;
            } else if (rand == 1) {
                screenY += 1;
            }
        }
        
#if HANDMADE_INERNAL
        char *fileName = __FILE__;
        debug_read_file_result fileResult = memory->DEBUG_PlatformReadEntireFile(thread, fileName);
        if (fileResult.Content)
        {
            memory->DEBUG_PlatformWriteEntireFile(thread, "test.txt", fileResult.ContentSize, fileResult.Content);
            memory->DEBUG_PlatformFreeFileMemory(thread, fileResult.Content);
        }
#endif

        world_position newCameraP = ChunkPositionFromTilePosition(gameState->World, screenBaseX*17 + 17 / 2, screenBaseY*9 + 9 / 2, screenBaseZ);
        gameState->CameraPosition = newCameraP;


        real32 screenWidth = (real32)buffer->Width;
        real32 screenHeight = (real32)buffer->Height;
        real32 maxZScale = 0.5f;
        real32 groundOverscan = 1.5f;

        // uint32 groundBufferWidth = RoundReal32ToInt32(groundOverscan * screenWidth);
        // uint32 groundBufferHeight = RoundReal32ToInt32(groundOverscan * screenHeight);
        
        gameState->GroundP = gameState->CameraPosition;
        
        AddMonster(gameState, newCameraP.ChunkX+4, newCameraP.ChunkY+4, newCameraP.ChunkZ);

        memory->IsInitialized = true;
    }

    Assert(sizeof(transient_state) <= memory->TransientStorageSize);
    transient_state *tranState = (transient_state *)memory->TransientStorage;
    if (!tranState->IsInitialized) {
        InitializeArena(&tranState->TransientArena, memory->TransientStorageSize-sizeof(transient_state), (uint8 *)memory->TransientStorage + sizeof(transient_state));

        uint32 groundBufferWidth = 256;
        uint32 groundBufferHeight = 256;
        tranState->GroundBufferCount = 128;
        tranState->GroundBuffers = PushArray(&tranState->TransientArena, tranState->GroundBufferCount, ground_buffer);
        
        for (uint32 groundBufferIndex=0;
             groundBufferIndex < tranState->GroundBufferCount;
             groundBufferIndex++)
        {
            ground_buffer *buf = tranState->GroundBuffers + groundBufferIndex;
            tranState->GroundBitmapTemplate = MakeEmptyBitmap(&tranState->TransientArena, groundBufferWidth, groundBufferHeight);
            buf->Memory = tranState->GroundBitmapTemplate.Memory;
            buf->P = NullPosition();
        }
        
        tranState->IsInitialized = true;

        // TODO: test fill
        // FillGroundChunk(tranState, gameState, tranState->GroundBuffers, &gameState->CameraPosition);
    }
    
    world *world = gameState->World;
    for (int controllerIndex=0; controllerIndex<ArrayCount(input->Controllers); controllerIndex++)
    {
        game_controller_input *inputController = &input->Controllers[controllerIndex];

        if (!inputController->IsConnected)
        {
            continue;
        }

        controlled_hero *controlledHero = gameState->ControlledHeroes + controllerIndex;
        controlledHero->ddPRequest = {0,0};
        controlledHero->dSwordRequest = {0,0};

        if (controlledHero->StoreIndex == 0)
        {
            if (inputController->Start.EndedDown) {
                controlledHero->StoreIndex = AddPlayer(gameState).Index;
            }
        }
        else
        {
            if (inputController->IsAnalog)
            {
                controlledHero->ddPRequest = 4.0f * v2{inputController->StickAverageX, inputController->StickAverageY};

                // TODO: restore and test analog input

                // gameState->XOffset -= (int)
                // gameState->YOffset += (int)(4.0f * inputController->StickAverageY);
            
                // todo: analog movement is out of date. Should be updated to new pos
                // gameState->PlayerPosition.TileX += (int)(10.0f * inputController->StickAverageX);
                // gameState->PlayerY -= (int)(10.0f * inputController->StickAverageY);
            }
            else
            {
                if (inputController->MoveUp.EndedDown)
                {
                    controlledHero->ddPRequest.Y = 1.0f;
                }
                if (inputController->MoveDown.EndedDown)
                {
                    controlledHero->ddPRequest.Y = -1.0f;
                }
                if (inputController->MoveLeft.EndedDown)
                {
                    controlledHero->ddPRequest.X = -1.0f;
                }
                if (inputController->MoveRight.EndedDown)
                {
                    controlledHero->ddPRequest.X = 1.0f;
                }
            }

            if (inputController->ActionUp.EndedDown)
            {
                controlledHero->dSwordRequest.Y = 1;
            }
            if (inputController->ActionDown.EndedDown)
            {
                controlledHero->dSwordRequest.Y = -1;
            }
            if (inputController->ActionLeft.EndedDown)
            {
                controlledHero->dSwordRequest.X = -1;
            }
            if (inputController->ActionRight.EndedDown)
            {
                controlledHero->dSwordRequest.X = 1;
            }
        }
    }

    loaded_bitmap drawBuffer_ = {};
    loaded_bitmap *drawBuffer = &drawBuffer_;
    drawBuffer->Memory = buffer->Memory;
    drawBuffer->Width = buffer->Width;
    drawBuffer->Height = buffer->Height;
    drawBuffer->Pitch = buffer->Pitch;
        
    DefaultColorRenderBuffer(drawBuffer);


    v2 screenCenter = 0.5f * V2((real32) drawBuffer->Width, (real32)drawBuffer->Height);
    rectangle3 cameraBoundsInMeters = RectCenterDim(V3(0,0,0), V3(screenWidthInMeters, screenHeightInMeters, 0));
    
    {
        world_position minChunkP = MapIntoChunkSpace(gameState->World, gameState->CameraPosition, GetMinCorner(cameraBoundsInMeters));
        world_position maxChunkP = MapIntoChunkSpace(gameState->World, gameState->CameraPosition, GetMaxCorner(cameraBoundsInMeters));

        color chunkColor = {1.0f,1.0f,0.0f,0.0f};
        v2 chunkDimPixels = 0.5f * gameState->MetersToPixels * gameState->World->ChunkDimInMeters.XY;

        for (int32 chunkZ=minChunkP.ChunkZ;
             chunkZ <= maxChunkP.ChunkZ;
             chunkZ++)
        {
            for (int32 chunkY=minChunkP.ChunkY;
                 chunkY <= maxChunkP.ChunkY;
                 chunkY++)
            {
                for (int32 chunkX=minChunkP.ChunkX;
                     chunkX <= maxChunkP.ChunkX;
                     chunkX++)
                {
                    world_position chunkCenter = CenteredTilePoint(chunkX, chunkY, chunkZ);
                    v3 relP = gameState->MetersToPixels *Subtract(gameState->World, &chunkCenter, &gameState->CameraPosition);

                    v2 screenP = V2(screenCenter.X + relP.X, screenCenter.Y - relP.Y);

                    bool32 found = false;
                    ground_buffer *emptyBuffer = 0;
                    for (uint32 groundBufferIndex=0; groundBufferIndex<tranState->GroundBufferCount; groundBufferIndex++)
                    {
                        ground_buffer *groundBuffer = tranState->GroundBuffers + groundBufferIndex;
                        if (AreInSameChunk(&groundBuffer->P, &chunkCenter)) {
                            found = true;
                            break;
                        } else if (!IsValid(groundBuffer->P)) {
                            emptyBuffer = groundBuffer;
                        }
                    }

                    if (!found && emptyBuffer) {
                        FillGroundChunk(tranState, gameState, emptyBuffer, &chunkCenter);
                    }
                        
                    RenderRectangleOutline(drawBuffer, screenP.X - chunkDimPixels.X , screenP.Y - chunkDimPixels.Y, screenP.X + chunkDimPixels.X, screenP.Y + chunkDimPixels.Y, chunkColor);
                }
            }
        }
    }
    

    // TODO: fix meters
    v3 simBoundsExpansion = V3(15.0f, 15.0f, 15.0f);
    rectangle3 simBounds = AddRadiusTo(cameraBoundsInMeters, simBoundsExpansion);

    temporary_memory simMemory = BeginTemporaryMemory(&tranState->TransientArena);
    sim_region *simRegion = BeginSim(&tranState->TransientArena, gameState, gameState->World, gameState->CameraPosition, simBounds, input->DtForFrame);
    
    
    ///RenderBitmap(drawBuffer, &gameState->LoadedBitmap, 0, 0);

    // RenderBitmap(drawBuffer, &gameState->GroundCachedBitmap, 0, 0);

    for (uint32 groundBufferIndex=0;
         groundBufferIndex < tranState->GroundBufferCount;
         groundBufferIndex++)
    {
        ground_buffer *buf = tranState->GroundBuffers + groundBufferIndex;
        if (IsValid(buf->P)) {
            loaded_bitmap bmp = tranState->GroundBitmapTemplate;
            bmp.Memory = buf->Memory;
            
            v2 groundP = {screenCenter.X - 0.5f * (real32) bmp.Width,
                          screenCenter.Y - 0.5f * (real32) bmp.Height};

            v3 groundDelta = gameState->MetersToPixels * Subtract(gameState->World, &buf->P, &gameState->CameraPosition);

            groundP = V2(groundP.X + groundDelta.X , groundP.Y - groundDelta.Y);

            RenderBitmap(drawBuffer, &bmp, groundP.X, groundP.Y, 1.0);
        }
    }
    
    sim_entity *simEntity = simRegion->Entities;
    for (uint32 entityIndex = 0;
         entityIndex < simRegion->EntityCount;
         ++entityIndex, ++simEntity)
    {
        if (simEntity->Updatable)
        {
            entity_visible_piece_group pieceGroup = {};
            low_entity *lowEntity = gameState->LowEntities + simEntity->StorageIndex;

            // TODO: draw entities on one Z-plane

            hero_bitmaps *heroBitmaps = &gameState->HeroBitmaps[simEntity->FacingDirection];

            move_spec moveSpec = DefaultMoveSpec();
            v3 ddp = {};

            pieceGroup.GameState = gameState;
            switch (simEntity->Type) {
            case EntityType_Wall:
            {
                real32 wallAlpha = 0.5f;
                real32 halfDepth = simEntity->Collision->TotalVolume.Dim.Z * 0.5f;
                if (simEntity->P.Z < halfDepth && simEntity->P.Z > -0.5f*halfDepth) {
                    wallAlpha = 1.0f;
                }
                
                PushPiece(&pieceGroup, &gameState->WallDemoBitmap, V3(0,0, 0), V2(64.0f * 0.5f, 64.0f * 0.5f), wallAlpha);
            } break;
            case EntityType_Sword:
            {
                moveSpec.UnitMaxAccelVector = false;
                moveSpec.Speed = 0;
                moveSpec.Drag = 0;

                if (simEntity->DistanceLimit == 0.0f) {
                    MakeEntityNonSpacial(simEntity);
                    ClearCollisionRulesFor(gameState, simEntity->StorageIndex);
                }

                PushPiece(&pieceGroup, &gameState->SwordDemoBitmap, V3(0,0,0), V2(12,60), 1);
            } break;
            case EntityType_Hero:
            {
                for (uint32 idx=0; idx<ArrayCount(gameState->ControlledHeroes); idx++)
                {
                    controlled_hero *conHero = gameState->ControlledHeroes+idx;
                    if (conHero->StoreIndex == simEntity->StorageIndex)
                    {
                        moveSpec.UnitMaxAccelVector = true;
                        moveSpec.Speed = 50.0f;
                        moveSpec.Drag = 8.0f;

                        ddp = V3(conHero->ddPRequest, 0.0f);

                        if (conHero->dSwordRequest.X != 0 || conHero->dSwordRequest.Y != 0)
                        {
                            sim_entity *swordEntity = simEntity->Sword.Ptr;
                            if (swordEntity && IsSet(swordEntity, EntityFlag_Nonspacial))
                            {
                                AddCollisionRule(gameState, simEntity->StorageIndex, swordEntity->StorageIndex, false);
                                MakeEntitySpacial(swordEntity, simEntity->P, 7.0f * V3(conHero->dSwordRequest, 0));
                                swordEntity->DistanceLimit = 15.0f;
                            }
                        }
                    }
                }

                PushPiece(&pieceGroup, &heroBitmaps->Character, V3(0,0,0), heroBitmaps->Align, 1);

                sim_entity_collision_volume *volume = &simEntity->Collision->TotalVolume;
                PushPieceRectOutline(&pieceGroup, V3(volume->Offset.X, volume->Offset.Y, 0), volume->Dim.XY, V4(0, 0.3f, 0.3f, 0));

                DrawHitpoints(&pieceGroup, simEntity);
            } break;
            case EntityType_Monster:
            {
                PushPiece(&pieceGroup, &gameState->EnemyDemoBitmap, V3(0, 0, 0), V2(61,197), 1);

                DrawHitpoints(&pieceGroup, simEntity);
            } break;
            case EntityType_Familiar:
            {
                sim_entity *closestHero = 0;
                real32 closestHeroRadSq = Square(90.0f);

                // TODO(casey): make spacial queries
                for (uint32 entityCount = 0;
                     entityCount < simRegion->EntityCount;
                     ++entityCount)
                {
                    sim_entity *testEntity = simRegion->Entities + entityCount;
                    if (testEntity->Type == EntityType_Hero)
                    {
                        real32 testD = LengthSq(testEntity->P - simEntity->P);
                        if (testD < closestHeroRadSq)
                        {
                            closestHero = testEntity;
                            closestHeroRadSq = testD;
                        }
                    }
                }
    
                if (closestHero && closestHeroRadSq > 4.0f)
                {
                    real32 acceleration = 0.5f;
                    real32 oneOverLength = acceleration / SquareRoot(closestHeroRadSq);
        
                    ddp = oneOverLength * (closestHero->P - simEntity->P);
                }

                moveSpec.UnitMaxAccelVector = true;
                moveSpec.Speed = 100.0f;
                moveSpec.Drag = 9.0f;

                simEntity->TBobing += input->DtForFrame;
                if (simEntity->TBobing > 2.0f * Pi32)
                {
                    simEntity->TBobing -= 2.0f * Pi32;
                }

                PushPiece(&pieceGroup, &gameState->FamiliarDemoBitmap, V3(0, 0.2f * Sin(13 * simEntity->TBobing), 0), V2(58, 203), 1);
            } break;
            case EntityType_Stairwell:
            {
                real32 wallAlpha = 1.0f;
                // if (simEntity->P.Z != 0) {
                //     wallAlpha = 0.1f;
                // }
                
                PushPieceRect(&pieceGroup, V3(0, 0, 0), simEntity->WalkableDim, V4(1,1,0,1));
                PushPieceRect(&pieceGroup, V3(0, 0, simEntity->WalkableHeight), simEntity->WalkableDim, V4(1,1,0,0));
            } break;
            case EntityType_Space:
            {
                for (uint32 volumeIndex=0; volumeIndex < simEntity->Collision->VolumeCount; volumeIndex++)
                {
                    sim_entity_collision_volume *volume = simEntity->Collision->Volumes + volumeIndex;
                    PushPieceRectOutline(&pieceGroup, V3(volume->Offset.X, volume->Offset.Y, 0), volume->Dim.XY, V4(0,1,1,0));
                }
            } break;
            }

            if (!IsSet(simEntity, EntityFlag_Nonspacial) && IsSet(simEntity, EntityFlag_Movable))
            {
                MoveEntity(gameState, simRegion, simEntity, input->DtForFrame, &moveSpec, ddp);
            }

            v3 entityBaseP = GetEntityGroundPoint(simEntity);

            real32 zFugde = 1.0f + 0.1f * entityBaseP.Z;

            real32 entityX = screenCenter.X + entityBaseP.X * gameState->MetersToPixels * zFugde;
            real32 entityY = screenCenter.Y - entityBaseP.Y * gameState->MetersToPixels * zFugde;
            real32 entityZ = -entityBaseP.Z * gameState->MetersToPixels;

            for (uint32 idx =0; idx < pieceGroup.PieceCount; ++idx)
            {
                entity_visible_piece piece = pieceGroup.Pieces[idx];

                v2 center = {entityX + piece.Offset.X, entityY + piece.Offset.Y};
                
                if (piece.Bitmap)
                {
                    RenderBitmap(drawBuffer, piece.Bitmap, entityX + piece.Offset.X, entityY + piece.Offset.Y + piece.EntitiyZC*entityZ, piece.Alpha);
                }
                else
                {
                    color c;
                    c.Red = piece.R;
                    c.Green = piece.G;
                    c.Blue = piece.B;
                    c.Alpha = 1.0f;

                    real32 realMinX = center.X - piece.Dim.X / 2;
                    real32 realMinY = center.Y - piece.Dim.Y / 2;
                    real32 realMaxX = center.X + piece.Dim.X / 2;
                    real32 realMaxY = center.Y + piece.Dim.Y / 2;
                    RenderRectangle(drawBuffer, realMinX, realMinY, realMaxX, realMaxY, c);
                }
            }

            if (false) // draw entity bounds
            {
                /*
                  NOTE: does NOT support any z logic
                 */
                
                color c;

                switch (simEntity->Type) {
                case EntityType_Wall: {
                    if (simEntity->P.Z == 0) {
                        c.Red = 0.3f;
                        c.Green = 0.3f;
                        c.Blue = 0.3f;
                    } else if (simEntity->P.Z == 1) {
                        c.Red = 0.5f;
                        c.Green = 0.5f;
                        c.Blue = 0.5f;
                    } else if (simEntity->P.Z == -1) {
                        c.Red = 0.8f;
                        c.Green = 0.8f;
                        c.Blue = 0.8f;
                    } else {
                        c.Red = 0.1f;
                        c.Green = 0.1f;
                        c.Blue = 0.1f;
                    }
                } break;
                case EntityType_Stairwell: {
                    c.Red = 0.0f;
                    c.Green = (real32)simEntity->P.Z / 5;
                    c.Blue = 0.0f;
                } break;
                case EntityType_Hero: {
                    c.Red = 0.0f;
                    c.Green = 0.5f;
                    c.Blue = 0.5f;
                } break;
                default: {
                    c.Red = 1.0f;
                    c.Green = 0.0f;
                    c.Blue = 0.0f;
                } break;
                }

                c.Alpha = 1.0f;

                // real32 halfW = simEntity->Dim.X * MetersToPixels * 0.5f;
                // real32 halfH = simEntity->Dim.Y * MetersToPixels * 0.5f;

                // NOTE: wtf rects?
                real32 halfW = 1;
                real32 halfH = 2;

                real32 realMinX = entityX - halfW;
                real32 realMinY = entityY - halfH;
                real32 realMaxX = entityX + halfW;
                real32 realMaxY = entityY + halfH;

                if (simEntity->Type == EntityType_Hero) {
                    realMinX = realMinX - simEntity->P.Z * 10.0f;
                    realMaxX = realMaxX + simEntity->P.Z * 10.0f;
                }
                
                RenderRectangle(drawBuffer, realMinX, realMinY, realMaxX, realMaxY, c);
            }
        }   
    }
    
    if (input->MouseButtons[0].EndedDown)
    {
        int testSpace = 50;
        RenderPlayer(drawBuffer, input->MouseX + testSpace, input->MouseY + testSpace);
        
        RenderPlayer(drawBuffer, input->MouseX - testSpace, input->MouseY - testSpace);
        
        RenderPlayer(drawBuffer, input->MouseX + testSpace, input->MouseY - testSpace);
        
        RenderPlayer(drawBuffer, input->MouseX - testSpace, input->MouseY + testSpace);
    }
    
    if (input->MouseButtons[1].EndedDown)
    {
        int testSpace = 50;
        RenderPlayer(drawBuffer, input->MouseX + testSpace, input->MouseY);
        
        RenderPlayer(drawBuffer, input->MouseX - testSpace, input->MouseY);
        
        RenderPlayer(drawBuffer, input->MouseX, input->MouseY - testSpace);
        
        RenderPlayer(drawBuffer, input->MouseX, input->MouseY + testSpace);
    }

    EndSim(simRegion, gameState);

    EndTemporaryMemory(simMemory);
    CheckArena(&gameState->WorldArena);
    CheckArena(&tranState->TransientArena);
}


extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *gameState = (game_state*)memory->PermanentStorage;
    GameOutputSound(soundBuffer, gameState);
}
