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

#include "handmade_world.cpp"
#include "random.h"
#include "handmade_sim_region.cpp"
#include "handmade_sim_entity.cpp"



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
        
        uint32 *pixels = (uint32 *)((uint8 *)content + header->BitmapOffset);
        
        result.Pixels = pixels;
        
        // NOTE: why this is not needed?
        // uint32 *srcDest = pixels;
        // for (int32 y =0;
        //      y < header->Height;
        //      ++y)
        // {
        //     for (int32 x =0;
        //          x < header->Width;
        //          ++x)
        //     {
        //         *srcDest = (*srcDest >> 8) | (*srcDest << 24);
        //         ++srcDest;
        //     }
        // }
    }
    else
    {
        // todo: error
    }
    
    return result;
}

inline v2
GetCameraSpaceP(game_state *gameState, low_entity *entityLow)
{
    world_diff diff = Subtract(gameState->World, &entityLow->WorldP, &gameState->CameraPosition);
    v2 result = diff.dXY;
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

    ChangeEntityLocation(&gameState->WorldArena, gameState->World, ei, entityLow, pos);

    result.Low = entityLow;
    result.Index = ei;
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

internal add_low_entity_result
AddSword(game_state *gameState)
{
    add_low_entity_result lowEntityResult = AddLowEntity(gameState, EntityType_Sword, NullPosition());

    lowEntityResult.Low->Sim.Width = (real32)25 / gameState->MetersToPixels;
    lowEntityResult.Low->Sim.Height = (real32)63 / gameState->MetersToPixels;
    AddFlag(&lowEntityResult.Low->Sim, EntityFlag_Nonspacial);

    return lowEntityResult;
}

internal add_low_entity_result
AddMonster(game_state *gameState, uint32 absX, uint32 absY, uint32 absZ)
{
    world_position pos = ChunkPositionFromTilePosition(gameState->World, absX, absY, absZ);
    add_low_entity_result lowEntityResult = AddLowEntity(gameState, EntityType_Monster, pos);
    
    AddFlag(&lowEntityResult.Low->Sim, EntityFlag_Collides);

    InitHitpoints(lowEntityResult.Low, 8, 1);

    lowEntityResult.Low->Sim.Width = (real32)0.75;
    lowEntityResult.Low->Sim.Height = (real32)0.4;

    return lowEntityResult;
}

internal add_low_entity_result
AddFamiliar(game_state *gameState, uint32 absX, uint32 absY, uint32 absZ)
{
    world_position pos = ChunkPositionFromTilePosition(gameState->World, absX, absY, absZ);
    add_low_entity_result lowEntityResult = AddLowEntity(gameState, EntityType_Familiar, pos);

    lowEntityResult.Low->WorldP._Offset.X += 8;
   
    lowEntityResult.Low->Sim.Width = (real32)0.75;
    lowEntityResult.Low->Sim.Height = (real32)0.4;
    AddFlag(&lowEntityResult.Low->Sim, EntityFlag_Collides);

    return lowEntityResult;
}

internal add_low_entity_result
AddPlayer(game_state *gameState)
{
    add_low_entity_result lowEntityResult = AddLowEntity(gameState, EntityType_Hero, gameState->CameraPosition);

    //entity->dPosition = {0, 0};

    lowEntityResult.Low->Sim.Width = (real32)0.75;
    lowEntityResult.Low->Sim.Height = (real32)0.4;
    AddFlag(&lowEntityResult.Low->Sim, EntityFlag_Collides);

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
    add_low_entity_result lowEntityResult = AddLowEntity(gameState, EntityType_Wall, pos);

    AddFlag(&lowEntityResult.Low->Sim, EntityFlag_Collides);
    lowEntityResult.Low->Sim.Width = gameState->World->TileSideInMeters;
    lowEntityResult.Low->Sim.Height = gameState->World->TileSideInMeters;

    return lowEntityResult;
}

uint32 GetUintColor(color color)
{
    uint32 uColor = (uint32)(
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
    
    for (int sampleIndex = 0; sampleIndex < soundBuffer->SampleCount; ++sampleIndex)
    {
        int16 sampleValue = 0;
        
        *sampleOut++ = sampleValue;
        *sampleOut++ = sampleValue;
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

internal
void
RenderRectangle(game_offscreen_buffer *buffer, real32 realMinX, real32 realMinY, real32 realMaxX, real32 realMaxY, color color)
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
    
    if (maxX > buffer->Width)
    {
        maxX = buffer->Width;
    }
    if (maxY > buffer->Height)
    {
        maxY = buffer->Height;
    }
    
    uint8 *row = (uint8 *)buffer->Memory + buffer->Pitch * minY + minX * buffer->BytesPerPixel;
    for (int y = minY; y < maxY; ++y)
    {
        uint32 *pixel = (uint32 *)row;
        
        for (int x = minX; x < maxX; ++x)
        {
            *(uint32 *)pixel = uColor;
            pixel++;
        }
        row += buffer->Pitch;
    }
}


internal void
RenderBitmap(game_offscreen_buffer *buffer, loaded_bitmap *bitmap,
    real32 realX, real32 realY,
    real32 cAlpha = 0.5f)
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
    
    if (maxX > buffer->Width)
    {
        maxX = buffer->Width;
    }
    if (maxY > buffer->Height)
    {
        maxY = buffer->Height;
    }
    
    // TODO: sourcerow change to adjust for cliping
    
    uint32 *srcrow = bitmap->Pixels + bitmap->Width * (bitmap->Height - 1);
    srcrow += srcOffsetX - bitmap->Width * srcOffsetY;

    uint8 *dstrow = (uint8 *)buffer->Memory + minX * buffer->BytesPerPixel + buffer->Pitch * minY;
    
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
                real32 a = (real32)((*src >> 24) & 0xff) / 255.0f;
            a *= cAlpha;

            real32 sr = (real32)((*src >> 16) & 0xff);
            real32 sg = (real32)((*src >> 8) & 0xff);
            real32 sb = (real32)((*src >> 0) & 0xff);
            
            real32 dr = (real32)((*dst >> 16) & 0xff);
            real32 dg = (real32)((*dst >> 8) & 0xff);
            real32 db = (real32)((*dst >> 0) & 0xff);
            
            real32 r = (1.0f - a) * dr + a * sr;
            real32 g = (1.0f - a) * dg + a * sg;
            real32 b = (1.0f - a) * db + a * sb;
            
            
            *dst = ((uint32)(r + 0.5f) << 16) | ((uint32)(g + 0.5f) << 8) | ((uint32)(b + 0.5f) << 0);
            
            *dst++;
            *src++;
        }
        
        dstrow += buffer->Pitch;
        srcrow -= bitmap->Width;
    }
    
}

void
ClearRenderBuffer(game_offscreen_buffer *buffer)
{
    color bgColor;
    bgColor.Red = (143.0f / 255.0f);
    bgColor.Green = (118.0f / 255.0f);
    bgColor.Blue = (74.0f / 255.0f);
    
    RenderRectangle(buffer, 0, 0, (real32)buffer->Width, (real32)buffer->Height, bgColor);
}

void
RenderPlayer(game_offscreen_buffer *buffer, int playerX, int playerY)
{
    real32 squareHalfWidth = 10;
    color playerColor;
    playerColor.Red = 1;
    playerColor.Green = 0;
    playerColor.Blue = 1;
    
    RenderRectangle(buffer, (real32)playerX - squareHalfWidth, (real32)playerY - squareHalfWidth, (real32)playerX + squareHalfWidth, (real32)playerY + squareHalfWidth, playerColor);
}

internal void
InitializeArena(memory_arena *arena, memory_index size, void *base)
{
    arena->Size = size;
    arena->Base = (uint8 *)base;
    arena->Used = 0;
}


inline void
PushPiece(entity_visible_piece_group *grp, loaded_bitmap *bmp, v2 offset, real32 z, v2 align, real32 alpha = 1.0f)
{
    Assert(grp->PieceCount < ArrayCount(grp->Pieces));
    entity_visible_piece *renderPice = grp->Pieces + grp->PieceCount;

    renderPice->Bitmap = bmp;
    renderPice->Offset = grp->GameState->MetersToPixels * offset - align;
    renderPice->Z = grp->GameState->MetersToPixels * z;
    renderPice->Alpha = alpha;
        
    grp->PieceCount++;
}

inline void
PushPieceRect(entity_visible_piece_group *grp, v2 offset, real32 z, v2 dim, v4 color)
{
    Assert(grp->PieceCount < ArrayCount(grp->Pieces));
    entity_visible_piece *renderPice = grp->Pieces + grp->PieceCount;

    renderPice->Offset = grp->GameState->MetersToPixels * offset;
    renderPice->Z = grp->GameState->MetersToPixels * z;
    renderPice->Dim = grp->GameState->MetersToPixels * dim;
    renderPice->R = color.R;
    renderPice->G = color.G;
    renderPice->B = color.B;
        
    grp->PieceCount++;
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
                    
            PushPieceRect(pieceGroup, V2(firstX, firstY), 0, healthDim, color);
            firstX += spacingX + healthDim.X;
        }
    }
}


extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(game_state) <= memory->PermanentStorageSize);
    
    game_state *gameState = (game_state *)memory->PermanentStorage;
    
    real32 TileSideInMeters = 1.4f;
    int32 TileSideInPixels = 64;
    
    real32 MetersToPixels = (real32)TileSideInPixels / (real32)TileSideInMeters;
    gameState->MetersToPixels = MetersToPixels;
    
    if (!memory->IsInitialized)
    {
        AddLowEntity(gameState, EntityType_NULL, NullPosition());

        gameState->LoadedBitmap = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/new-bg-code.bmp");

        gameState->EnemyDemoBitmap = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/test2/enemy_demo.bmp");
        gameState->FamiliarDemoBitmap = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/test2/familiar_demo.bmp");
        gameState->WallDemoBitmap = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/test2/wall_demo.bmp");
        gameState->SwordDemoBitmap = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/test2/sword_demo.bmp");
        
        
        gameState->HeroBitmaps[0].Align = V2(60, 195);
        gameState->HeroBitmaps[0].Character = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/player/stand_right.bmp");
        
        gameState->HeroBitmaps[1].Align = V2(60, 185);
        gameState->HeroBitmaps[1].Character = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/player/stand_up.bmp");
        
        gameState->HeroBitmaps[2].Align = V2(60, 195);
        gameState->HeroBitmaps[2].Character = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/player/stand_left.bmp");
        
        gameState->HeroBitmaps[3].Align = V2(60, 185);
        gameState->HeroBitmaps[3].Character = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/player/stand_down.bmp");
        
        InitializeArena(&gameState->WorldArena, memory->PermanentStorageSize - sizeof(game_state), (uint8 *)memory->PermanentStorage + sizeof(game_state));
        
        gameState->XOffset = 0;
        gameState->YOffset = 0;
        
        gameState->World = PushSize(&gameState->WorldArena, world);
        InitializeWorld(gameState->World, 1.4f);
        
        // generate tiles
        uint32 tilesPerScreenWidth = 17;
        uint32 tilesPerScreenHeight = 9;
        
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
        
        
        while (screenIndex--) {
            uint32 rand;
            if (drawZDoorCounter > 0) {
                rand = RandomNumberTable[screenIndex] % 2;
            } else {
                rand = RandomNumberTable[screenIndex] % 3;
            }

            // DEBUG, no up and down movement currently
            rand = RandomNumberTable[screenIndex] % 2;
            
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

                    if (tileX == 4 && tileY == 4)
                    {
                        AddFamiliar(gameState, absTileX, absTileY, absTileY);
                    }
                    
                    uint32 tileVal = 1;
                    if (tileX == 0) {
                        if (tileY == (tilesPerScreenHeight / 2)) {
                            if (isDoorLeft) {
                                tileVal = 1;
                            }
                            else {
                                tileVal = 2;
                            }
                        } else {
                            tileVal = 2;
                        }
                    }
                    if (tileX == (tilesPerScreenWidth -1)) {
                        if (tileY == (tilesPerScreenHeight / 2)) {
                            if (isDoorRight) {
                                tileVal = 1;
                            }
                            else {
                                tileVal = 2;
                            }
                        } else {
                            tileVal = 2;
                        }
                    }
                    if (tileY == 0) {
                        if (tileX == (tilesPerScreenWidth / 2)) {
                            if (isDoorBottom) {
                                tileVal = 1;
                            } else {
                                tileVal = 2;
                            }
                            
                        } else {
                            tileVal = 2;
                        }
                    }
                    if (tileY == (tilesPerScreenHeight -1)) {
                        if (tileX == (tilesPerScreenWidth / 2)) {
                            if (isDoorTop) {
                                tileVal = 1;
                            } else {
                                tileVal = 2;
                            }
                            
                        } else {
                            tileVal = 2;
                        }
                    }
                    
                    if (drawZDoorCounter != 0 && isDoorUp) {
                        if (tileY == tilesPerScreenHeight / 2 && (tileX - 1 == tilesPerScreenWidth / 2)) {
                            tileVal = 3;
                        }
                    }
                    
                    if (drawZDoorCounter != 0 && isDoorDown) {
                        if (tileY == tilesPerScreenHeight / 2 && (tileX + 1 == tilesPerScreenWidth / 2)) {
                            tileVal = 4;
                        }
                    }
                    
                    if (tileVal == 2)
                    {
                        AddWall(gameState, absTileX, absTileY, absTileZ);
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
        
        memory->IsInitialized = 1;

        world_position newCameraP = ChunkPositionFromTilePosition(gameState->World, screenBaseX*17 + 17 / 2, screenBaseY*9 + 9 / 2, screenBaseZ);
        gameState->CameraPosition = newCameraP;

        
        AddMonster(gameState, newCameraP.ChunkX+4, newCameraP.ChunkY+4, newCameraP.ChunkZ);
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
    
    ClearRenderBuffer(buffer);

    // TODO: fix numbers for tiles
    int32 tileSpanX = 17 * 3;
    int32 tileSpanY = 9 * 3;
    rectangle2 cameraBounds = RectCenterDim(V2(0,0), world->TileSideInMeters * V2((real32)tileSpanX, (real32)tileSpanY));

    memory_arena simArena;
    InitializeArena(&simArena, memory->TransientStorageSize, memory->TransientStorage);
    sim_region *simRegion = BeginSim(&simArena, gameState, gameState->World, gameState->CameraPosition, cameraBounds);
    
    
    real32 screenCenterX = 0.5f * (real32) buffer->Width;
    real32 screenCenterY = 0.5f * (real32) buffer->Height;
    
    RenderBitmap(buffer, &gameState->LoadedBitmap, 0, 0);
    
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
            v2 ddp = {};

            pieceGroup.GameState = gameState;
            switch (simEntity->Type) {
            case EntityType_Wall:
            {
                PushPiece(&pieceGroup, &gameState->WallDemoBitmap, V2(0,0), 0, V2(64.0f * 0.5f, 64.0f * 0.5f), 1);
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

                PushPiece(&pieceGroup, &gameState->SwordDemoBitmap, V2(0,0), 0, V2(12,60), 1);
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

                        ddp = conHero->ddPRequest;

                        if (conHero->dSwordRequest.X != 0 || conHero->dSwordRequest.Y != 0)
                        {
                            sim_entity *swordEntity = simEntity->Sword.Ptr;
                            if (swordEntity && IsSet(swordEntity, EntityFlag_Nonspacial))
                            {
                                AddCollisionRule(gameState, simEntity->StorageIndex, swordEntity->StorageIndex, false);
                                MakeEntitySpacial(swordEntity, simEntity->P, 7.0f * conHero->dSwordRequest);
                                swordEntity->DistanceLimit = 15.0f;
                            }
                        }
                    }
                }

                PushPiece(&pieceGroup, &heroBitmaps->Character, V2(0,0), 0, heroBitmaps->Align, 1);

                DrawHitpoints(&pieceGroup, simEntity);
            } break;
            case EntityType_Monster:
            {
                PushPiece(&pieceGroup, &gameState->EnemyDemoBitmap, V2(0, 0), 0, V2(61,197), 1);

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
                moveSpec.Speed = 50.0f;
                moveSpec.Drag = 8.0f;

                simEntity->TBobing += input->DtForFrame;
                if (simEntity->TBobing > 2.0f * Pi32)
                {
                    simEntity->TBobing -= 2.0f * Pi32;
                }

                PushPiece(&pieceGroup, &gameState->FamiliarDemoBitmap, V2(0, 0.2f * Sin(13 * simEntity->TBobing)), 0, V2(58, 203), 1);
            }break;
            }

            if (!IsSet(simEntity, EntityFlag_Nonspacial))
            {
                MoveEntity(gameState, simRegion, simEntity, input->DtForFrame, &moveSpec, ddp);
            }

            real32 entityX = screenCenterX + simEntity->P.X * MetersToPixels;
            real32 entityY = screenCenterY - simEntity->P.Y * MetersToPixels;

            for (uint32 idx =0; idx < pieceGroup.PieceCount; ++idx)
            {
                entity_visible_piece piece = pieceGroup.Pieces[idx];
                if (piece.Bitmap)
                {
                    RenderBitmap(buffer, piece.Bitmap, entityX + piece.Offset.X, entityY + piece.Offset.Y, piece.Alpha);
                }
                else
                {
                    color c;
                    c.Red = piece.R;
                    c.Green = piece.G;
                    c.Blue = piece.B;

                    real32 realMinX = entityX + piece.Offset.X;
                    real32 realMinY = entityY + piece.Offset.Y;
                    real32 realMaxX = realMinX + piece.Dim.X;
                    real32 realMaxY = realMinY + piece.Dim.Y;
                    RenderRectangle(buffer, realMinX, realMinY, realMaxX, realMaxY, c);
                }
            }

            if (false) // draw entity bounds
            {
                color c;
                c.Red = 1;
                c.Green = 0;
                c.Blue = 0;

                real32 halfW = simEntity->Width * MetersToPixels * 0.5f;
                real32 halfH = simEntity->Height * MetersToPixels * 0.5f;

                real32 realMinX = entityX - halfW;
                real32 realMinY = entityY - halfH;
                real32 realMaxX = entityX + halfW;
                real32 realMaxY = entityY + halfH;
                RenderRectangle(buffer, realMinX, realMinY, realMaxX, realMaxY, c);
            }
        }   
    }
    
    if (input->MouseButtons[0].EndedDown)
    {
        int testSpace = 50;
        RenderPlayer(buffer, input->MouseX + testSpace, input->MouseY + testSpace);
        
        RenderPlayer(buffer, input->MouseX - testSpace, input->MouseY - testSpace);
        
        RenderPlayer(buffer, input->MouseX + testSpace, input->MouseY - testSpace);
        
        RenderPlayer(buffer, input->MouseX - testSpace, input->MouseY + testSpace);
    }
    
    if (input->MouseButtons[1].EndedDown)
    {
        int testSpace = 50;
        RenderPlayer(buffer, input->MouseX + testSpace, input->MouseY);
        
        RenderPlayer(buffer, input->MouseX - testSpace, input->MouseY);
        
        RenderPlayer(buffer, input->MouseX, input->MouseY - testSpace);
        
        RenderPlayer(buffer, input->MouseX, input->MouseY + testSpace);
    }

    EndSim(simRegion, gameState);
}


extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *gameState = (game_state*)memory->PermanentStorage;
    GameOutputSound(soundBuffer, gameState);
}
