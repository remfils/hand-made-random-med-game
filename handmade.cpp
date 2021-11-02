#include "handmade.h"

#define PushSize(arena, type) (type *)PushSize_(arena, sizeof(type))
#define PushArray(arena, count,  type) (type *)PushSize_(arena, count * sizeof(type))
void *
PushSize_(memory_arena *arena, memory_index size)
{
    Assert((arena->Used + size) <= arena->Size);

    void *result = arena->Base + arena->Used;
    arena->Used += size;
    return result;
}

#include "handmade_tile.cpp"
#include "random.h"



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



internal entity*
GetEntity(game_state *gameState, uint32 entityIndex)
{
    entity *entity = 0;
    if ((entityIndex > 0) && (entityIndex < ArrayCount(gameState->Entities)))
    {
        entity = &gameState->Entities[entityIndex];
    }

    return entity;
}

internal uint32
AddEntity(game_state *gameState)
{
    Assert(gameState->EntityCount < ArrayCount(gameState->Entities));

    uint32 entityIndex = gameState->EntityCount;
    entity *result = &gameState->Entities[gameState->EntityCount++];
    *result = {};
    return entityIndex;
}

internal void
InitializePlayer(game_state *gameState, uint32 entityIndex)
{
    entity *entity = GetEntity(gameState, entityIndex);
    entity->Exists = true;
    entity->Position.AbsTileX = 2;
    entity->Position.AbsTileY = 2;
    entity->Position.Offset.X = 5.f;
    entity->Position.Offset.Y = 5.f;
    entity->dPosition = {0, 0};

    entity->Width = (real32)0.75;
    entity->Height = (real32)0.3;

    if (!GetEntity(gameState, gameState->CameraFollowingEntityIndex)) {
        gameState->CameraFollowingEntityIndex = entityIndex;
    }
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
    int16 toneVolume = 2000;
    int wavePeriod = soundBuffer->SamplesPerSecond / gameState->ToneHz;
    
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
RenderBitmap(game_offscreen_buffer *buffer, loaded_bitmap *bitmap, real32 realX, real32 realY)
{
    int32 minX = RoundReal32ToInt32(realX);
    int32 minY = RoundReal32ToInt32(realY);
    int32 maxX = RoundReal32ToInt32(realX + ((real32) bitmap->Width));
    int32 maxY = RoundReal32ToInt32(realY + ((real32) bitmap->Height));
    
    int32 srcOffsetX = 0;
    int32 srcOffsetY = 0;
    
    if (minX < 0)
    {
        srcOffsetX = - minX;
        minX = 0;
    }
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
    
    uint8 *dstrow = (uint8 *)buffer->Memory + buffer->Pitch * minY + minX * buffer->BytesPerPixel;
    uint32 *srcrow = bitmap->Pixels + bitmap->Width * (bitmap->Height - 1);
    
    srcrow -= bitmap->Width * srcOffsetY - srcOffsetX;
    
    for (int y = minY; y < maxY; ++y)
    {
        uint32 *dst = (uint32 *)dstrow;
        uint32 * src = (uint32 *)srcrow;
        
        for (int x = minX; x < maxX; ++x)
        {
            real32 a = (real32)((*src >> 24) & 0xff) / 255.0f;
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
    bgColor.Red = 0.3f;
    bgColor.Green = 0.3f;
    bgColor.Blue = 0.3f;
    
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
InitializeArena(memory_arena *arena, memory_index size, uint8 *base)
{
    arena->Size = size;
    arena->Base = base;
    arena->Used = 0;
}


internal void
MovePlayer(game_state *gameState, entity *entity, real32 dt, v2 ddPlayer)
{
    tile_map *tileMap = gameState->World->TileMap;
    
    real32 speed = 50.0f;
    ddPlayer *= speed;

    ddPlayer += -10 * entity->dPosition;

    // NOTE: here you can add super speed fn
    tile_map_position oldPlayerPosition = entity->Position;
    tile_map_position newPlayerPosition = entity->Position;
    v2 playerPositionDelta = 0.5f * dt * dt * ddPlayer + entity->dPosition * dt;
    newPlayerPosition.Offset += playerPositionDelta;

    entity->dPosition = ddPlayer * dt + entity->dPosition;
    newPlayerPosition = RecanonicalizePosition(tileMap, newPlayerPosition);

    // NOTE: old code

    tile_map_position playerLeft = newPlayerPosition;
    playerLeft.Offset.X -= entity->Width / 2;
    playerLeft = RecanonicalizePosition(tileMap, playerLeft);
            
    tile_map_position playerRight = newPlayerPosition;
    playerRight.Offset.X += entity->Width / 2;
    playerRight = RecanonicalizePosition(tileMap, playerRight);


    bool32 isCollided = false;
    tile_map_position collidedPos = {};
    if (!IsTileMapPointEmpty(tileMap, playerLeft))
    {
        isCollided = true;
        collidedPos = playerLeft;
    }
    if (!IsTileMapPointEmpty(tileMap, playerRight))
    {
        isCollided = true;
        collidedPos = playerRight;
    }

    if (isCollided)
    {
        v2 r = {};
        if (collidedPos.AbsTileX < entity->Position.AbsTileX) {
            r = {1,0};
        } 
        if (collidedPos.AbsTileX > entity->Position.AbsTileX) {
            r = {-1,0};
        }
        if (collidedPos.AbsTileY < entity->Position.AbsTileY) {
            r = {0,1};
        }
        if (collidedPos.AbsTileY > entity->Position.AbsTileY) {
            r = {0,-1};
        } 
        entity->dPosition = entity->dPosition - 2 * Inner(entity->dPosition, r) * r;
    }
    else
    {
        if (!AreOnSameTile(entity->Position, newPlayerPosition)) {
            uint32 newTileValue = GetTileValue(tileMap, newPlayerPosition);
            if (newTileValue == 3) {
                ++newPlayerPosition.AbsTileZ;
            } else if (newTileValue == 4) {
                --newPlayerPosition.AbsTileZ;
            }
        }
                
        tile_map_position canPos = newPlayerPosition;
                
        entity->Position = canPos;
    }


    // NOTE: new collision code

    // uint32 minTileX = 0;
    // uint32 minTileY = 0;
    // uint32 onePastmaxTileY = 0;
    // uint32 onePastmaxTileX = 0;
    // uint32 absTileZ = entity->Position.AbsTileZ;
    // tile_map_position bestPlayerPosition = entity->Position;
    // real32 bestDistanceSq = LengthSq(playerPositionDelta);
    // for (uint32 absTileY = minTileY;
    //      absTileY != onePastmaxTileY;
    //      ++absTileY)
    // {
    //     for (uint32 absTileX = minTileX;
    //          absTileX != onePastmaxTileX;
    //          ++absTileX)
    //     {
    //         tile_map_position testTilePosition = CenteredTilePoint(absTileX, absTileY, absTileZ);

    //         uint32 tileValue = GetTileValue(tileMap, absTileX, absTileY, absTileZ);

    //         if (IsTileValueEmpty(tileValue))
    //         {
    //             v2 minCorner = -0.5f * v2{tileMap->TileSideInMeters, tileMap->TileSideInMeters};
    //             v2 maxCorner = 0.5f * v2{tileMap->TileSideInMeters, tileMap->TileSideInMeters};

    //             tile_map_diff relNewPlayerPoint = Subtract(tileMap, &testTilePosition, &newPlayerPosition);

    //             v2 testP = ClosestPointInRectangle(minCorner, maxCorner, relNewPlayerPoint);
    //         }
    //     }
    // }



    if (entity->dPosition.X == 0 && entity->dPosition.Y == 0)
    {
        
    }
    else if (AbsoluteValue(entity->dPosition.X) > AbsoluteValue(entity->dPosition.Y))
    {
        if (entity->dPosition.X > 0)
        {
            entity->FacingDirection = 0;
        }
        else
        {
            entity->FacingDirection = 2;
        }
    }
    else
    {
        if (entity->dPosition.Y > 0)
        {
            entity->FacingDirection = 1;
        }
        else
        {
            entity->FacingDirection = 3;
        }
    }


    if (!AreOnSameTile(oldPlayerPosition, entity->Position)) {
        uint32 newTileValue = GetTileValue(tileMap, entity->Position);
        if (newTileValue == 3) {
            ++entity->Position.AbsTileZ;
        } else if (newTileValue == 4) {
            --entity->Position.AbsTileZ;
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
    
    if (!memory->IsInitialized)
    {
        uint32 nullEntity = AddEntity(gameState);
        gameState->LoadedBitmap = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/new-bg-code.bmp");
        
        
        gameState->HeroBitmaps[0].AlignX = 60;
        gameState->HeroBitmaps[0].AlignY = 195;
        gameState->HeroBitmaps[0].Character = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/player/stand_right.bmp");
        
        gameState->HeroBitmaps[1].AlignX = 60;
        gameState->HeroBitmaps[1].AlignY = 185;
        gameState->HeroBitmaps[1].Character = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/player/stand_up.bmp");
        
        gameState->HeroBitmaps[2].AlignX = 60;
        gameState->HeroBitmaps[2].AlignY = 195;
        gameState->HeroBitmaps[2].Character = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/player/stand_left.bmp");
        
        gameState->HeroBitmaps[3].AlignX = 60;
        gameState->HeroBitmaps[3].AlignY = 185;
        gameState->HeroBitmaps[3].Character = DEBUGLoadBMP(thread, memory->DEBUG_PlatformReadEntireFile, "../data/player/stand_down.bmp");
        
        InitializeArena(&gameState->WorldArena, memory->PermanentStorageSize - sizeof(game_state), (uint8 *)memory->PermanentStorage + sizeof(game_state));
        
        gameState->XOffset = 0;
        gameState->YOffset = 0;
        gameState->ToneHz = 256;
        
        gameState->CameraPosition.AbsTileX = 17 / 2;
        gameState->CameraPosition.AbsTileY = 9 / 2;
        
        gameState->World = PushSize(&gameState->WorldArena, world);
        world *world = gameState->World;
        
        world->TileMap = PushSize(&gameState->WorldArena, tile_map);
        tile_map *tileMap = world->TileMap;
        
        tileMap->TileSideInMeters = TileSideInMeters;
        
        tileMap->TileChunkCountX = 64;
        tileMap->TileChunkCountY = 64;
        tileMap->TileChunkCountZ = 2;
        
        tileMap->LowerLeftX = 0;
        tileMap->LowerLeftY = (real32)buffer->Height;
        
        tileMap->ChunkShift = 4;
        tileMap->ChunkMask = (1 << tileMap->ChunkShift) - 1;
        tileMap->ChunkDim = (1 << tileMap->ChunkShift);
        
        tileMap->TileChunks = PushArray(&gameState->WorldArena, tileMap->TileChunkCountX * tileMap->TileChunkCountY * tileMap->TileChunkCountZ, tile_chunk);
        
        
        // generate tiles
        uint32 tilesPerScreenWidth = 17;
        uint32 tilesPerScreenHeight = 9;        
        
        uint32 screenX = 0;
        uint32 screenY = 0;
        uint32 absTileZ = 0;
        uint32 screenIndex = 100;
        
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
            
            if (rand == 0) {
                isDoorRight = true;
            }
            else if (rand == 1) {
                isDoorTop = true;
            } else {
                drawZDoorCounter = 2;
                if (absTileZ == 0) {
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
                    
                    SetTileValue(&gameState->WorldArena, world->TileMap, absTileX, absTileY, absTileZ, tileVal);
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
    }
    
    tile_map *tileMap = gameState->World->TileMap;

    for (int controllerIndex=0; controllerIndex<ArrayCount(input->Controllers); controllerIndex++)
    {
        game_controller_input *inputController = &input->Controllers[controllerIndex];

        if (!inputController->IsConnected)
        {
            continue;
        }

        entity *controllingEntity = GetEntity(gameState, gameState->PlayerIndexControllerMap[controllerIndex]);

        if (controllingEntity)
        {
            v2 ddPlayer = {};

            if (inputController->IsAnalog)
            {
                ddPlayer = 4.0f * v2{inputController->StickAverageX, inputController->StickAverageY};

                // todo: restore and test analog input

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
                    ddPlayer.Y = 1.0f;
                }
                if (inputController->MoveDown.EndedDown)
                {
                    ddPlayer.Y = -1.0f;
                }
                if (inputController->MoveLeft.EndedDown)
                {
                    ddPlayer.X = -1.0f;
                }
                if (inputController->MoveRight.EndedDown)
                {
                    ddPlayer.X = 1.0f;
                }

                if (ddPlayer.X != 0 && ddPlayer.Y != 0) {
                    ddPlayer *= 0.707106781188f;
                }
            }

            MovePlayer(gameState, controllingEntity, input->DtForFrame, ddPlayer);
        }
        else
        {
            if (inputController->Start.EndedDown) {
                uint32 entityIndex = AddEntity(gameState);
                gameState->PlayerIndexControllerMap[controllerIndex] = entityIndex;
                InitializePlayer(gameState, entityIndex);
            }
        }
        
        
    }
            
    

    
    ClearRenderBuffer(buffer);
    

    entity *cameraFollowingEntity = GetEntity(gameState, gameState->CameraFollowingEntityIndex);
    if (cameraFollowingEntity)
    {
        gameState->CameraPosition.AbsTileZ = cameraFollowingEntity->Position.AbsTileZ;

        tile_map_diff diff = Subtract(tileMap, &cameraFollowingEntity->Position, &gameState->CameraPosition);
        if (diff.dXY.Y > (5 * tileMap->TileSideInMeters))
        {
            gameState->CameraPosition.AbsTileY += 9;
        }
        if (diff.dXY.Y < -(5 * tileMap->TileSideInMeters))
        {
            gameState->CameraPosition.AbsTileY -= 9;
        }
            
        if (diff.dXY.X > (9 * tileMap->TileSideInMeters))
        {
            gameState->CameraPosition.AbsTileX += 17;
        }
        if (diff.dXY.X < -(9 * tileMap->TileSideInMeters))
        {
            gameState->CameraPosition.AbsTileX -= 17;
        }
    }

    
    
    real32 screenCenterX = 0.5f * (real32) buffer->Width;
    real32 screenCenterY = 0.5f * (real32) buffer->Height;
    
    
    
    RenderBitmap(buffer, &gameState->LoadedBitmap, 0, 0);
    
    for (int32 relCol = -10; relCol < 10; ++relCol)
    {
        for (int32 relRow = -10; relRow < 10; ++relRow)
        {
            uint32 col = gameState->CameraPosition.AbsTileX + relCol;
            uint32 row = gameState->CameraPosition.AbsTileY + relRow;
            uint32 z = gameState->CameraPosition.AbsTileZ;
            uint32 tileId = GetTileValue(tileMap, col, row, z);
            
            color color;
            
            if (tileId == 4) {
                color.Red = 0.2f;
                color.Green = 0.4f;
                color.Blue = 0.2f;
            } else if (tileId == 3) {
                color.Red = 0.0f;
                color.Green = 1.0f;
                color.Blue = 0;
            } else if (tileId == 2)
            {
                color.Red = 0.0f;
                color.Green = 0.3f;
                color.Blue = 1.0f;
            }
            else if (tileId == 1)
            {
                // color.Red = 0.2f;
                // color.Green = 0.2f;
                // color.Blue = 0.2f;
                continue;
            } else {
                color.Red = 0.2f;
                color.Green = 0.2f;
                color.Blue = 0.2f;
            }
            
            // debug stuff
            if (col == gameState->CameraPosition.AbsTileX && row == gameState->CameraPosition.AbsTileY)
            {
                color.Red += 0.2f;
                color.Green += 0.2f;
                color.Blue += 0.2f;
            }
            
            real32 cenX = screenCenterX - MetersToPixels * gameState->CameraPosition.Offset.X + ((real32) relCol) * TileSideInPixels;
            real32 cenY = screenCenterY + MetersToPixels * gameState->CameraPosition.Offset.Y - ((real32) relRow ) * TileSideInPixels;
            
            real32 minX = cenX - 0.5f * TileSideInPixels;
            real32 minY = cenY - 0.5f * TileSideInPixels;
            // real32 minY = screenCenterY + MetersToPixels * gameState->CameraPosition.TileRelY - ((real32) relRow ) * TileSideInPixels;
            real32 maxX = cenX + 0.5f * TileSideInPixels;
            real32 maxY = cenY + 0.5f * TileSideInPixels;
            
            RenderRectangle(buffer, minX, minY, maxX, maxY, color);
        }
    }


    entity *entity = gameState->Entities;
    for (uint32 ei = 0;
         ei < gameState->EntityCount;
         ++ei, ++entity)
    {
        // TODO: draw entities on one Z-plane

        if (entity->Exists) {
            tile_map_diff diff = Subtract(tileMap, &entity->Position, &gameState->CameraPosition);

            color playerColor;
            playerColor.Red = 1.0f;
            playerColor.Green = 1.0f;
            playerColor.Blue = 0;
    
            real32 playerLeft = screenCenterX - 0.5f * MetersToPixels * entity->Width + diff.dXY.X * MetersToPixels;
            real32 playerTop = screenCenterY - MetersToPixels * entity->Height - diff.dXY.Y * MetersToPixels;
    
    
            hero_bitmaps *heroBitmaps = &gameState->HeroBitmaps[entity->FacingDirection];
            real32 bitmapLeft = playerLeft - (real32) heroBitmaps->AlignX + (entity->Width * MetersToPixels / 2);
            real32 bitmapTop = playerTop - (real32) heroBitmaps->AlignY + entity->Height * MetersToPixels;
    
            if (entity->FacingDirection == 1)
            {
                RenderRectangle(buffer, playerLeft, playerTop, playerLeft + entity->Width * MetersToPixels, playerTop + entity->Height * MetersToPixels, playerColor);
                RenderBitmap(buffer, &heroBitmaps->Character, bitmapLeft, bitmapTop);
            }
            else
            {
                RenderRectangle(buffer, playerLeft, playerTop, playerLeft + entity->Width * MetersToPixels, playerTop + entity->Height * MetersToPixels, playerColor);
                RenderBitmap(buffer, &heroBitmaps->Character, bitmapLeft, bitmapTop);
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
}


extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *gameState = (game_state*)memory->PermanentStorage;
    GameOutputSound(soundBuffer, gameState);
}
