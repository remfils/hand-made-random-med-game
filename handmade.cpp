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


internal void
ChangeEntityResidence(game_state *gameState, uint32 entityIndex, entity_residence residence)
{
    if (residence == EntityResidence_High)
    {
        if (gameState->EntityResidence[entityIndex] != EntityResidence_High)
        {
            high_entity *entityHigh = &gameState->HighEntities[entityIndex];
            dormant_entity *entityDormant = &gameState->DormantEntities[entityIndex];


            tile_map_diff diff = Subtract(gameState->World->TileMap, &entityDormant->Position, &gameState->CameraPosition);

            entityHigh->Position = diff.dXY;
            entityHigh->AbsTileZ = entityDormant->Position.AbsTileZ;
            entityHigh->FacingDirection = 0;
        }
    }

    gameState->EntityResidence[entityIndex] = residence;
}

internal entity
GetEntity(game_state *gameState, entity_residence residence, uint32 entityIndex)
{
    entity entity = {};
    if ((entityIndex > 0) && (entityIndex < gameState->EntityCount))
    {
        if (gameState->EntityResidence[entityIndex] < residence)
        {
            ChangeEntityResidence(gameState, entityIndex, residence);
        }

        entity.Residence = residence;
        entity.Dormant = &gameState->DormantEntities[entityIndex];
        entity.Low = &gameState->LowEntities[entityIndex];
        entity.High = &gameState->HighEntities[entityIndex];
    }

    return entity;
}

internal uint32
AddEntity(game_state *gameState, entity_type type)
{
    Assert(gameState->EntityCount < ArrayCount(gameState->LowEntities));
    Assert(gameState->EntityCount < ArrayCount(gameState->DormantEntities));
    Assert(gameState->EntityCount < ArrayCount(gameState->HighEntities));

    uint32 ei = gameState->EntityCount++;

    gameState->EntityResidence[ei] = EntityResidence_Dormant;
    gameState->DormantEntities[ei] = {};
    gameState->LowEntities[ei] = {};
    gameState->HighEntities[ei] = {};

    gameState->DormantEntities[ei].Type = type;

    return ei;
}

internal uint32
AddPlayer(game_state *gameState)
{
    uint32 entityIndex = AddEntity(gameState, EntityType_Hero);
    entity entity = GetEntity(gameState, EntityResidence_Dormant, entityIndex);

    entity.Dormant->Collides = true;

    entity.Dormant->Position.AbsTileX = 2;
    entity.Dormant->Position.AbsTileY = 2;
    entity.Dormant->Position._Offset.X = 0;
    entity.Dormant->Position._Offset.Y = 0;

    //entity->dPosition = {0, 0};

    entity.Dormant->Width = (real32)0.75;
    entity.Dormant->Height = (real32)0.4;

    ChangeEntityResidence(gameState, entityIndex, EntityResidence_High);

    if (GetEntity(gameState, EntityResidence_Dormant, gameState->CameraFollowingEntityIndex).Residence == EntityResidence_Nonexistant) {
        gameState->CameraFollowingEntityIndex = entityIndex;
    }

    return entityIndex;
}

internal uint32
AddWall(game_state *gameState, uint32 absTileX, uint32 absTileY, uint32 absTileZ)
{
    uint32 entityIndex = AddEntity(gameState, EntityType_Wall);
    entity entity = GetEntity(gameState, EntityResidence_Dormant, entityIndex);

    entity.Dormant->Collides = true;

    entity.Dormant->Position.AbsTileX = absTileX;
    entity.Dormant->Position.AbsTileY = absTileY;
    entity.Dormant->Position.AbsTileZ = absTileZ;
    entity.Dormant->Position._Offset.X = 0;
    entity.Dormant->Position._Offset.Y = 0;

    //entity->dPosition = {0, 0};

    entity.Dormant->Width = gameState->World->TileMap->TileSideInMeters;
    entity.Dormant->Height = gameState->World->TileMap->TileSideInMeters;

    ChangeEntityResidence(gameState, entityIndex, EntityResidence_High);

    return entityIndex;
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
RenderBitmap(game_offscreen_buffer *buffer, loaded_bitmap *bitmap,
    real32 realX, real32 realY,
    int32 alignX = 0, int32 alignY = 0,
    real32 cAlpha = 0.5f)
{
    realX -= (real32)alignX;
    realY -= (real32)alignY;

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


internal bool32
TestWall(real32 wallX, real32 relX, real32 relY, real32 playerDeltaX, real32 playerDeltaY, real32 *tMin, real32 minY, real32 maxY)
{
    bool32 result = false;
    real32 tEpsilon = 0.0001f;

    if (playerDeltaX != 0.0f)
    {
        real32 tResult = (wallX - relX) / playerDeltaX;
        real32 y = relY + tResult * playerDeltaY;
        if ((tResult >= 0.0f) && (*tMin > tResult))
        {
            if ((y >= minY) && (y <= maxY))
            {
                *tMin = Maximum(0.0f, tResult - tEpsilon);
                result = true;
            }
        }
    }
    return result;
}

internal void
MovePlayer(game_state *gameState, entity player, real32 dt, v2 ddPosition)
{
    // normlize ddPlayer
    real32 ddLen = SquareRoot(LengthSq(ddPosition));
    if (ddLen > 1)
    {
        ddPosition = (1.0f / ddLen) * ddPosition;
    }

    tile_map *tileMap = gameState->World->TileMap;
    
    real32 speed = 50.0f;
    ddPosition *= speed;

    ddPosition += -8 * player.High->dPosition;

    // NOTE: here you can add super speed fn
    v2 oldPlayerPosition = player.High->Position;
    v2 playerPositionDelta = 0.5f * dt * dt * ddPosition + player.High->dPosition * dt;
    player.High->dPosition = ddPosition * dt + player.High->dPosition;
    v2 newPlayerPosition = oldPlayerPosition + playerPositionDelta;

    


/*
    uint32 minTileX = Minimum(player.Dormant->Position.AbsTileX, newPlayerPosition.AbsTileX);
    uint32 minTileY = Minimum(player.Dormant->Position.AbsTileY, newPlayerPosition.AbsTileY);
    uint32 maxTileX = Maximum(entity.Dormant->Position.AbsTileX, newPlayerPosition.AbsTileX);
    uint32 maxTileY = Maximum(player.Dormant->Position.AbsTileY, newPlayerPosition.AbsTileY);

    uint32 entityWidthInTiles = CeilReal32ToInt32(player.Dormant->Width / tileMap->TileSideInMeters);
    uint32 entityHeightInTiles = CeilReal32ToInt32(player.Dormant->Height / tileMap->TileSideInMeters);

    minTileX -= entityWidthInTiles;    
    minTileY -= entityHeightInTiles;
    maxTileX += entityWidthInTiles;
    maxTileY += entityHeightInTiles;

    uint32 absTileZ = player.Dormant->Position.AbsTileZ;
*/
    for (uint32 iteration = 0;
         iteration < 4;
        iteration++)
    {

        // get lower if collision detected
        real32 tMin = 1.0f;
        v2 wallNormal = {0,0};
        uint32 hitEntityIndex = 0;

        v2 desiredPosition = player.High->Position + playerPositionDelta;
    
        for (uint32 ei = 1;
             ei < gameState->EntityCount;
             ++ei)
        {
            entity testEntity = GetEntity(gameState, EntityResidence_High, ei);

            if (testEntity.High != player.High)
            {
                if (testEntity.Dormant->Collides)
                {
                    real32 diameterWidth = testEntity.Dormant->Width + player.Dormant->Width;
                    real32 diameterHeight = testEntity.Dormant->Height + player.Dormant->Height;

                    v2 minCorner = -0.5f * v2{diameterWidth, diameterHeight};
                    v2 maxCorner = 0.5f * v2{diameterWidth, diameterHeight};

                    v2 rel = player.High->Position - testEntity.High->Position;

                    if (TestWall(minCorner.X, rel.X, rel.Y, playerPositionDelta.X, playerPositionDelta.Y,
                            &tMin, minCorner.Y, maxCorner.Y))
                    {
                        wallNormal = v2{-1,0};
                        hitEntityIndex = ei;
                    }
                    if (TestWall(maxCorner.X, rel.X, rel.Y, playerPositionDelta.X, playerPositionDelta.Y,
                            &tMin, minCorner.Y, maxCorner.Y))
                    {
                        wallNormal = v2{1,0};
                        hitEntityIndex = ei;
                    }
                    if (TestWall(minCorner.Y, rel.Y, rel.X, playerPositionDelta.Y, playerPositionDelta.X,
                            &tMin, minCorner.Y, maxCorner.Y))
                    {
                        wallNormal = v2{0,-1};
                        hitEntityIndex = ei;
                    }
                    if (TestWall(maxCorner.Y, rel.Y, rel.X, playerPositionDelta.Y, playerPositionDelta.X,
                            &tMin, minCorner.Y, maxCorner.Y))
                    {
                        wallNormal = v2{0,1};
                        hitEntityIndex = ei;
                    }   
                }
            }
            
        }

        player.High->Position += tMin * playerPositionDelta;
        if (hitEntityIndex)
        {
            player.High->dPosition = player.High->dPosition - 1 * Inner(player.High->dPosition, wallNormal) * wallNormal;

            playerPositionDelta = desiredPosition - player.High->Position;

            playerPositionDelta = playerPositionDelta - 1 * Inner(playerPositionDelta, wallNormal) * wallNormal;

            entity hitEntity = GetEntity(gameState, EntityResidence_Dormant, hitEntityIndex);

            player.High->AbsTileZ += hitEntity.Dormant->dAbsTileZ;
        }
        else
        {
            break;
        }
    }
    
    if (player.High->dPosition.X == 0 && player.High->dPosition.Y == 0)
    {
        // dont change direction if both zero
    }
    else if (AbsoluteValue(player.High->dPosition.X) > AbsoluteValue(player.High->dPosition.Y))
    {
        if (player.High->dPosition.X > 0)
        {
            player.High->FacingDirection = 0;
        }
        else
        {
            player.High->FacingDirection = 2;
        }
    }
    else
    {
        if (player.High->dPosition.Y > 0)
        {
            player.High->FacingDirection = 1;
        }
        else
        {
            player.High->FacingDirection = 3;
        }
    }
    player.Dormant->Position = MapIntoTileSpace(tileMap, gameState->CameraPosition, player.High->Position);
}




internal void
SetCamera(game_state *gameState, tile_map_position newCameraPosition)
{
    tile_map *tileMap = gameState->World->TileMap;
    tile_map_diff dCameraPosition = Subtract(tileMap, &newCameraPosition, &gameState->CameraPosition);
    
    v2 entityOffsetForFrame = -1 * dCameraPosition.dXY;

    gameState->CameraPosition = newCameraPosition;

    // TODO: fix numbers for tiles
    uint32 tileSpanX = 17 * 3;
    uint32 tileSpanY = 9*3;
    rectangle2 cameraBounds = RectCenterDim(V2(0,0), tileMap->TileSideInMeters * V2((real32)tileSpanX, (real32)tileSpanY));

    for (uint32 entityIndex = 0;
         entityIndex < gameState->EntityCount;
        ++entityIndex)
    {
        if (gameState->EntityResidence[entityIndex] == EntityResidence_High) {
            high_entity *high = gameState->HighEntities + entityIndex;
            high->Position += entityOffsetForFrame;

            if (IsInRectangle(cameraBounds, high->Position))
            {
            
            }
            else
            {
                ChangeEntityResidence(gameState, entityIndex, EntityResidence_Dormant);
            }
        }
    }

    uint32 minTileX = newCameraPosition.AbsTileX - tileSpanX / 2;
    uint32 minTileY = newCameraPosition.AbsTileY - tileSpanY / 2;
    uint32 maxTileX = newCameraPosition.AbsTileX + tileSpanX / 2;
    uint32 maxTileY = newCameraPosition.AbsTileY + tileSpanY / 2;

    for (uint32 entityIndex = 0;
         entityIndex < gameState->EntityCount;
         ++entityIndex)
    {
        if (gameState->EntityResidence[entityIndex] == EntityResidence_Dormant) {
            dormant_entity *dormant = gameState->DormantEntities+entityIndex;

            if ((dormant->Position.AbsTileZ == newCameraPosition.AbsTileZ)&&
                (dormant->Position.AbsTileX >= minTileX)&&
                (dormant->Position.AbsTileX <= maxTileX)&&
                (dormant->Position.AbsTileY >= minTileY)&&
                (dormant->Position.AbsTileY <= maxTileY))
            {
                ChangeEntityResidence(gameState, entityIndex, EntityResidence_High);
            }
        }
    }


    // TODO: move entities into high set here
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
        uint32 nullEntity = AddEntity(gameState, EntityType_NULL);
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
        
#if 0
        // TODO: when sparsness use this
        uint32 screenX = INT32_MAX / 2;
        uint32 screenY = INT32_MAX / 2;
#else
        uint32 screenX = 0;
        uint32 screenY = 0;
#endif
        uint32 absTileZ = 0;
        uint32 screenIndex = 2;
        
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

        tile_map_position newCameraP = {};
        newCameraP.AbsTileX = 17 / 2;
        newCameraP.AbsTileY = 9 / 2;
        SetCamera(gameState, newCameraP);
    }
    
    tile_map *tileMap = gameState->World->TileMap;

    for (int controllerIndex=0; controllerIndex<ArrayCount(input->Controllers); controllerIndex++)
    {
        game_controller_input *inputController = &input->Controllers[controllerIndex];

        if (!inputController->IsConnected)
        {
            continue;
        }

        entity controllingEntity = GetEntity(gameState, EntityResidence_High, gameState->PlayerIndexControllerMap[controllerIndex]);

        if (controllingEntity.Residence != EntityResidence_Nonexistant)
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

                // TODO: remove this
                // if (ddPlayer.X != 0 && ddPlayer.Y != 0) {
                //     ddPlayer *= 0.707106781188f;
                // }
            }

            MovePlayer(gameState, controllingEntity, input->DtForFrame, ddPlayer);
        }
        else
        {
            if (inputController->Start.EndedDown) {
                uint32 entityIndex = AddPlayer(gameState);
                gameState->PlayerIndexControllerMap[controllerIndex] = entityIndex;
                
            }
        }
    }
    
    ClearRenderBuffer(buffer);

    tile_map_position newCameraPosition = gameState->CameraPosition;
    entity cameraFollowingEntity = GetEntity(gameState, EntityResidence_High, gameState->CameraFollowingEntityIndex);
    if (cameraFollowingEntity.Residence != EntityResidence_Nonexistant)
    {
        newCameraPosition.AbsTileZ = cameraFollowingEntity.Dormant->Position.AbsTileZ;


#if 1
        if (cameraFollowingEntity.High->Position.Y > (5 * tileMap->TileSideInMeters))
        {
            newCameraPosition.AbsTileY += 9;
        }
        if (cameraFollowingEntity.High->Position.Y < -(5 * tileMap->TileSideInMeters))
        {
            newCameraPosition.AbsTileY -= 9;
        }
            
        if (cameraFollowingEntity.High->Position.X > (9 * tileMap->TileSideInMeters))
        {
            newCameraPosition.AbsTileX += 17;
        }
        if (cameraFollowingEntity.High->Position.X < -(9 * tileMap->TileSideInMeters))
        {
            newCameraPosition.AbsTileX -= 17;
        }
#else

        if (cameraFollowingEntity.High->Position.Y > (1 * tileMap->TileSideInMeters))
        {
            newCameraPosition.AbsTileY += 1;
        }
        if (cameraFollowingEntity.High->Position.Y < -(1 * tileMap->TileSideInMeters))
        {
            newCameraPosition.AbsTileY -= 1;
        }
            
        if (cameraFollowingEntity.High->Position.X > (1 * tileMap->TileSideInMeters))
        {
            newCameraPosition.AbsTileX += 1;
        }
        if (cameraFollowingEntity.High->Position.X < -(1 * tileMap->TileSideInMeters))
        {
            newCameraPosition.AbsTileX -= 1;
        }
#endif

        SetCamera(gameState, newCameraPosition);

        // TODO: move entities in and out. dormant <-> high
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
            
            real32 cenX = screenCenterX - MetersToPixels * gameState->CameraPosition._Offset.X + ((real32) relCol) * TileSideInPixels;
            real32 cenY = screenCenterY + MetersToPixels * gameState->CameraPosition._Offset.Y - ((real32) relRow ) * TileSideInPixels;
            
            real32 minX = cenX - 0.4f * TileSideInPixels;
            real32 minY = cenY - 0.4f * TileSideInPixels;
            // real32 minY = screenCenterY + MetersToPixels * gameState->CameraPosition.TileRelY - ((real32) relRow ) * TileSideInPixels;
            real32 maxX = cenX + 0.4f * TileSideInPixels;
            real32 maxY = cenY + 0.4f * TileSideInPixels;
            
            RenderRectangle(buffer, minX, minY, maxX, maxY, color);
        }
    }


    for (uint32 ei = 0;
         ei < gameState->EntityCount;
         ++ei)
    {
        // TODO: draw entities on one Z-plane

        if (gameState->EntityResidence[ei] == EntityResidence_High)
        {
            high_entity *entity = &gameState->HighEntities[ei];
            dormant_entity *dormantEntity = &gameState->DormantEntities[ei];

            // entity->Position += entityOffsetForFrame;

            color playerColor;
            playerColor.Red = 1.0f;
            playerColor.Green = 1.0f;
            playerColor.Blue = 0;
    
            real32 playerLeft = screenCenterX - 0.5f * MetersToPixels * dormantEntity->Width + entity->Position.X * MetersToPixels;
            real32 playerTop = screenCenterY - MetersToPixels * dormantEntity->Height * 0.5f - entity->Position.Y * MetersToPixels;
    
    
            hero_bitmaps *heroBitmaps = &gameState->HeroBitmaps[entity->FacingDirection];
            real32 bitmapLeft = playerLeft - (real32) heroBitmaps->AlignX + (dormantEntity->Width * MetersToPixels / 2);
            real32 bitmapTop = playerTop - (real32) heroBitmaps->AlignY + dormantEntity->Height * MetersToPixels;

            if (dormantEntity->Type == EntityType_Wall)
            {
                color wallColor;
                wallColor.Red = 0.2f;
                wallColor.Green = 0.2f;
                wallColor.Blue = 0.2f;

                RenderRectangle(buffer, playerLeft, playerTop, playerLeft + dormantEntity->Width * MetersToPixels, playerTop + dormantEntity->Height * MetersToPixels, wallColor);
            }
            else if (dormantEntity->Type == EntityType_Hero)
            {
                RenderRectangle(buffer, playerLeft, playerTop, playerLeft + dormantEntity->Width * MetersToPixels, playerTop + dormantEntity->Height * MetersToPixels, playerColor);
                RenderBitmap(buffer, &heroBitmaps->Character, bitmapLeft, bitmapTop, 0, 0, 1);
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
