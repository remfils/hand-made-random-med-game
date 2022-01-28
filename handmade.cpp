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


internal high_entity *
MakeEntityHighFrequency(game_state *gameState, uint32 lowIndex)
{
    high_entity *result = 0;
    low_entity *entityLow = &gameState->LowEntities[lowIndex];

    if (entityLow->HighEntityIndex)
    {
        result = gameState->HighEntities_ + entityLow->HighEntityIndex;
    }
    else
    {
        // TODO: why low not setting
        if (gameState->HighEntityCount < ArrayCount(gameState->HighEntities_))
        {
            uint32 highIndex = gameState->HighEntityCount++;
            result = &gameState->HighEntities_[highIndex];

            tile_map_diff diff = Subtract(gameState->World->TileMap, &entityLow->Position, &gameState->CameraPosition);

            result->Position = diff.dXY;
            result->AbsTileZ = entityLow->Position.AbsTileZ;
            result->FacingDirection = 0;
            result->LowEntityIndex = lowIndex;

            entityLow->HighEntityIndex = highIndex;
        }
        else
        {
            InvalidCodePath;
        }
    }
    
    return result;
}

internal void
MakeEntityLowFrequency(game_state *gameState, uint32 lowIndex)
{
    low_entity *entityLow = &gameState->LowEntities[lowIndex];
    uint32 highIndex = entityLow->HighEntityIndex;

    if (highIndex)
    {
        uint32 lastHighIndex = gameState->HighEntityCount - 1;
        if (highIndex != lastHighIndex)
        {
            high_entity *lastEntity = gameState->HighEntities_ + lastHighIndex;
            high_entity *deletedEntity = gameState->HighEntities_ + highIndex;

            *deletedEntity = *lastEntity;
            gameState->LowEntities[lastEntity->LowEntityIndex].HighEntityIndex = highIndex;
        }

        --gameState->HighEntityCount;

        entityLow->HighEntityIndex = 0;

        // uint23 highIndex = gameState->HighEntityCount++;
        // high_entity *entityHigh = &gameState->HighEntities[highIndex];p

        // tile_map_diff diff = Subtract(gameState->World->TileMap, &entityLow->Position, &gameState->CameraPosition);

        // entityHigh->Position = diff.dXY;
        // entityHigh->AbsTileZ = entityLow->Position.AbsTileZ;
        // entityHigh->FacingDirection = 0;

        // entityLow->HighEntityIndex = highIndex;
    }
}

inline void
OffsetEntitiesAndMakeLowFrequencyOutsideArea(game_state *gameState, v2 entityOffsetForFrame, rectangle2 areaBounds)
{
    for (uint32 entityIndex = 0;
         entityIndex < gameState->HighEntityCount;
        )
    {
        high_entity *high = gameState->HighEntities_ + entityIndex;
        high->Position += entityOffsetForFrame;

        if (IsInRectangle(areaBounds, high->Position))
        {
            ++entityIndex;
            
        }
        else
        {
            MakeEntityLowFrequency(gameState, high->LowEntityIndex);
        }
    }
}

internal low_entity *
GetLowEntity(game_state *gameState, uint32 lowIndex)
{
    low_entity *result = 0;
    if (lowIndex > 0 && lowIndex < gameState->LowEntityCount)
    {
        result = gameState->LowEntities + lowIndex;
    }

    return result;
}

internal entity
GetHighEntity(game_state *gameState, uint32 lowIndex)
{
    entity result = {};

    result.High = MakeEntityHighFrequency(gameState, lowIndex);
    result.Low = gameState->LowEntities + lowIndex;
    result.LowIndex = lowIndex;

    return result;
}

internal uint32
AddLowEntity(game_state *gameState, entity_type type)
{
    Assert(gameState->LowEntityCount < ArrayCount(gameState->LowEntities));

    uint32 ei = gameState->LowEntityCount++;

    gameState->LowEntities[ei] = {};
    
    gameState->LowEntities[ei].Type = type;

    return ei;
}

internal uint32
AddPlayer(game_state *gameState)
{
    uint32 entityIndex = AddLowEntity(gameState, EntityType_Hero);
    low_entity *entity = GetLowEntity(gameState, entityIndex);

    entity->Collides = true;

    entity->Position.AbsTileX = 2;
    entity->Position.AbsTileY = 2;
    entity->Position._Offset.X = 0;
    entity->Position._Offset.Y = 0;

    //entity->dPosition = {0, 0};

    entity->Width = (real32)0.75;
    entity->Height = (real32)0.4;

    // ChangeEntityResidence(gameState, entityIndex, EntityResidence_High);

    if (gameState->CameraFollowingEntityIndex == 0) {
        gameState->CameraFollowingEntityIndex = entityIndex;
    }

    return entityIndex;
}

internal uint32
AddWall(game_state *gameState, uint32 absTileX, uint32 absTileY, uint32 absTileZ)
{
    uint32 entityIndex = AddLowEntity(gameState, EntityType_Wall);
    low_entity *entity = GetLowEntity(gameState, entityIndex);

    entity->Collides = true;

    entity->Position.AbsTileX = absTileX;
    entity->Position.AbsTileY = absTileY;
    entity->Position.AbsTileZ = absTileZ;
    entity->Position._Offset.X = 0;
    entity->Position._Offset.Y = 0;

    //entity->dPosition = {0, 0};

    entity->Width = gameState->World->TileMap->TileSideInMeters;
    entity->Height = gameState->World->TileMap->TileSideInMeters;

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
        uint32 hitHighEntityIndex = 0;

        v2 desiredPosition = player.High->Position + playerPositionDelta;
    
        for (uint32 testHighIndex = 1;
             testHighIndex < gameState->HighEntityCount;
             ++testHighIndex)
        {
            if (testHighIndex != player.Low->HighEntityIndex)
            {
                entity testEntity = {};
                testEntity.High = gameState->HighEntities_ + testHighIndex;
                testEntity.Low = gameState->LowEntities + testEntity.High->LowEntityIndex;
                testEntity.LowIndex = testEntity.High->LowEntityIndex;

                if (testEntity.Low->Collides)
                {
                    real32 diameterWidth = testEntity.Low->Width + player.Low->Width;
                    real32 diameterHeight = testEntity.Low->Height + player.Low->Height;

                    v2 minCorner = -0.5f * v2{diameterWidth, diameterHeight};
                    v2 maxCorner = 0.5f * v2{diameterWidth, diameterHeight};

                    v2 rel = player.High->Position - testEntity.High->Position;

                    if (TestWall(minCorner.X, rel.X, rel.Y, playerPositionDelta.X, playerPositionDelta.Y,
                            &tMin, minCorner.Y, maxCorner.Y))
                    {
                        wallNormal = v2{-1,0};
                        hitHighEntityIndex = testHighIndex;
                    }
                    if (TestWall(maxCorner.X, rel.X, rel.Y, playerPositionDelta.X, playerPositionDelta.Y,
                            &tMin, minCorner.Y, maxCorner.Y))
                    {
                        wallNormal = v2{1,0};
                        hitHighEntityIndex = testHighIndex;
                    }
                    if (TestWall(minCorner.Y, rel.Y, rel.X, playerPositionDelta.Y, playerPositionDelta.X,
                            &tMin, minCorner.Y, maxCorner.Y))
                    {
                        wallNormal = v2{0,-1};
                        hitHighEntityIndex = testHighIndex;
                    }
                    if (TestWall(maxCorner.Y, rel.Y, rel.X, playerPositionDelta.Y, playerPositionDelta.X,
                            &tMin, minCorner.Y, maxCorner.Y))
                    {
                        wallNormal = v2{0,1};
                        hitHighEntityIndex = testHighIndex;
                    }   
                }
            }
            
        }

        player.High->Position += tMin * playerPositionDelta;
        if (hitHighEntityIndex)
        {
            player.High->dPosition = player.High->dPosition - 1 * Inner(player.High->dPosition, wallNormal) * wallNormal;

            playerPositionDelta = desiredPosition - player.High->Position;

            playerPositionDelta = playerPositionDelta - 1 * Inner(playerPositionDelta, wallNormal) * wallNormal;

            high_entity *hitHighEntity = gameState->HighEntities_ + hitHighEntityIndex;
            low_entity *hitLowEntity = gameState->LowEntities + hitHighEntity->LowEntityIndex;

            player.High->AbsTileZ += hitLowEntity->dAbsTileZ;
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
    player.Low->Position = MapIntoTileSpace(tileMap, gameState->CameraPosition, player.High->Position);
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

    OffsetEntitiesAndMakeLowFrequencyOutsideArea(gameState, entityOffsetForFrame, cameraBounds);

    uint32 minTileX = newCameraPosition.AbsTileX - tileSpanX / 2;
    uint32 minTileY = newCameraPosition.AbsTileY - tileSpanY / 2;
    uint32 maxTileX = newCameraPosition.AbsTileX + tileSpanX / 2;
    uint32 maxTileY = newCameraPosition.AbsTileY + tileSpanY / 2;

    for (uint32 entityIndex = 0;
         entityIndex < gameState->LowEntityCount;
         ++entityIndex)
    {
        low_entity *low = gameState->LowEntities + entityIndex;

        if (low->HighEntityIndex == 0)
        {
            if ((low->Position.AbsTileZ == newCameraPosition.AbsTileZ)&&
                (low->Position.AbsTileX >= minTileX)&&
                (low->Position.AbsTileX <= maxTileX)&&
                (low->Position.AbsTileY >= minTileY)&&
                (low->Position.AbsTileY <= maxTileY))
            {

                MakeEntityHighFrequency(gameState, entityIndex);
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
        AddLowEntity(gameState, EntityType_NULL);
        gameState->HighEntityCount = 1;

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

        uint32 lowIndex = gameState->PlayerIndexControllerMap[controllerIndex];

        if (lowIndex == 0)
        {
            if (inputController->Start.EndedDown) {
                uint32 entityIndex = AddPlayer(gameState);
                gameState->PlayerIndexControllerMap[controllerIndex] = entityIndex;
                
            }
        }
        else
        {
            entity controllingEntity = GetHighEntity(gameState, gameState->PlayerIndexControllerMap[controllerIndex]);

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
    }
    
    ClearRenderBuffer(buffer);

    tile_map_position newCameraPosition = gameState->CameraPosition;
    entity cameraFollowingEntity = GetHighEntity(gameState, gameState->CameraFollowingEntityIndex);
    if (cameraFollowingEntity.High)
    {
        newCameraPosition.AbsTileZ = cameraFollowingEntity.Low->Position.AbsTileZ;


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


    for (uint32 highEntityIndex = 0;
         highEntityIndex < gameState->HighEntityCount;
         ++highEntityIndex)
    {
        // TODO: draw entities on one Z-plane

        high_entity *entity = gameState->HighEntities_ + highEntityIndex;
        low_entity *lowEntity = gameState->LowEntities + entity->LowEntityIndex;

        // entity->Position += entityOffsetForFrame;

        color playerColor;
        playerColor.Red = 1.0f;
        playerColor.Green = 1.0f;
        playerColor.Blue = 0;
    
        real32 playerLeft = screenCenterX - 0.5f * MetersToPixels * lowEntity->Width + entity->Position.X * MetersToPixels;
        real32 playerTop = screenCenterY - MetersToPixels * lowEntity->Height * 0.5f - entity->Position.Y * MetersToPixels;
    
    
        hero_bitmaps *heroBitmaps = &gameState->HeroBitmaps[entity->FacingDirection];
        real32 bitmapLeft = playerLeft - (real32) heroBitmaps->AlignX + (lowEntity->Width * MetersToPixels / 2);
        real32 bitmapTop = playerTop - (real32) heroBitmaps->AlignY + lowEntity->Height * MetersToPixels;

        if (lowEntity->Type == EntityType_Wall)
        {
            color wallColor;
            wallColor.Red = 0.2f;
            wallColor.Green = 0.2f;
            wallColor.Blue = 0.2f;

            RenderRectangle(buffer, playerLeft, playerTop, playerLeft + lowEntity->Width * MetersToPixels, playerTop + lowEntity->Height * MetersToPixels, wallColor);
        }
        else if (lowEntity->Type == EntityType_Hero)
        {
            RenderRectangle(buffer, playerLeft, playerTop, playerLeft + lowEntity->Width * MetersToPixels, playerTop + lowEntity->Height * MetersToPixels, playerColor);
            RenderBitmap(buffer, &heroBitmaps->Character, bitmapLeft, bitmapTop, 0, 0, 1);
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
