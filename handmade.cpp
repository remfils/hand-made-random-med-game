#include "handmade.h"
#include "handmade_intrinsics.h"

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
        /* real32 sineValue = sinf(gameState->TSine); */
        /* int16 sampleValue = (int16)(sineValue * toneVolume); */

        int16 sampleValue = 0;
        
        *sampleOut++ = sampleValue;
        *sampleOut++ = sampleValue;
        
        // real32 sineValue = sinf(gameState->TSine);
        // int16 sampleValue = (int16)(sineValue * toneVolume);
        // *sampleOut++ = sampleValue;
        // *sampleOut++ = sampleValue;
        
        gameState->TSine += (real32)(2.0f * Pi32 * 1.0f) / (real32)wavePeriod;
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


inline void
RecanonicalizeCoord(world *world, uint32* tileCoord, real32 *relCoord)
{
    // todo: player can go back
    int32 offset = FloorReal32ToInt32(*relCoord / world->TileSideInMeters);
    *tileCoord += offset;
    *relCoord = *relCoord - offset * world->TileSideInMeters;
}

inline world_position
RecanonicalizePosition(world *world, world_position pos)
{
    world_position res = pos;
    
    RecanonicalizeCoord(world, &res.AbsTileX, &res.RelX);
    RecanonicalizeCoord(world, &res.AbsTileY, &res.RelY);

    return res;
}

internal tile_chunk*
GetTileChunk(world * world, uint32 tileX, uint32 tileY)
{
    tile_chunk *tileChunk = 0;

    if ((tileX >= 0) && (tileX < world->TileMapCountX) &&
        (tileY >= 0) && (tileY < world->TileMapCountY))
    {
        tileChunk = &world->TileChunk[tileY * world->TileMapCountX + tileX];
    }

    return tileChunk;
}

inline tile_chunk_position
GetChunkPositionFor(world *world, uint32 absTileX, uint32 absTileY)
{
    tile_chunk_position result;

    result.TileChunkX = absTileX >> world->ChunkShift;
    result.TileChunkY = absTileY >> world->ChunkShift;
    result.RelTileX = absTileX & world->ChunkMask;
    result.RelTileY = absTileY & world->ChunkMask;

    return result;
}

inline uint32
GetTileValueUnchecked(world *world, tile_chunk *tileChunk, uint32 tileX, uint32 tileY)
{
    Assert(tileChunk);
    Assert(tileX < world->ChunkDim);
    Assert(tileY < world->ChunkDim);
    
    uint32 tileValue = tileChunk->Tiles[tileY * world->ChunkDim + tileX];
    return tileValue;
}

internal uint32
GetTileValue(world * world, tile_chunk *tileChunk, uint32 testX, uint32 testY)
{
    uint32 tileChunkValue = 0;
    
    if (tileChunk)
    {
        tileChunkValue = GetTileValueUnchecked(world, tileChunk, testX, testY);
    }
    
    return tileChunkValue;
}

internal uint32
GetTileValue(world * world, world_position wp)
{
    tile_chunk_position chunkPos = GetChunkPositionFor(world, wp.AbsTileX, wp.AbsTileY);
    tile_chunk *tileChunk = GetTileChunk(world, chunkPos.TileChunkX, chunkPos.TileChunkY);
    uint32 tileChunkValue = GetTileValue(world, tileChunk, chunkPos.RelTileX, chunkPos.RelTileY);
    
    return tileChunkValue;
}

internal bool32
IsWorldPointEmpty(world *world, world_position wp)
{
    uint32 tileChunkValue = GetTileValue(world, wp);

    return tileChunkValue == 0;
}


extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(game_state) <= memory->PermanentStorageSize);
    
    game_state *gameState = (game_state *)memory->PermanentStorage;

    if (!memory->IsInitialized)
    {
        gameState->XOffset = 0;
        gameState->YOffset = 0;
        gameState->ToneHz = 256;

        gameState->PlayerPosition.AbsTileX = 2;
        gameState->PlayerPosition.AbsTileY = 2;
        gameState->PlayerPosition.RelX = 5.f;
        gameState->PlayerPosition.RelY = 5.f;

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

    uint32 tileChunkMap[256][256] =
    {
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 1, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 1, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 1,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 1, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, },
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0,  9, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 9},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0}
    };

    tile_chunk tileChunk;
    tileChunk.Tiles = (uint32 *)tileChunkMap;
    
    world world;
    
    world.TileMapCountX = 2;
    world.TileMapCountY = 2;
    
    world.LowerLeftX = 0;
    world.LowerLeftY = (real32)buffer->Height;

    world.ChunkShift = 8;
    world.ChunkMask = 0xff;
    world.ChunkDim = 256;
        
    world.TileSideInMeters = 1.4f;
    world.TileSideInPixels = 56;
    world.MetersToPixels = (real32)world.TileSideInPixels / (real32)world.TileSideInMeters;
    

    world.TileChunk = &tileChunk;

    real32 playerWidth = (real32)0.75 * (real32)world.TileSideInMeters;
    real32 playerHeight = (real32)world.TileSideInMeters;

    for (int controllerIndex=0; controllerIndex<ArrayCount(input->Controllers); controllerIndex++)
    {
        game_controller_input *inputController = &input->Controllers[controllerIndex];

        if (!inputController->IsConnected)
        {
            continue;
        }

        if (inputController->IsAnalog)
        {
            // todo: restore and test analog input
            
            // gameState->XOffset -= (int)(4.0f * inputController->StickAverageX);
            // gameState->YOffset += (int)(4.0f * inputController->StickAverageY);

            // todo: analog movement is out of date. Should be updated to new pos
            // gameState->PlayerPosition.TileX += (int)(10.0f * inputController->StickAverageX);
            // gameState->PlayerY -= (int)(10.0f * inputController->StickAverageY);
        }
        else
        {
            real32 playerDx = 0.0f;
            real32 playerDy = 0.0f;
            if (inputController->MoveUp.EndedDown)
            {
                playerDy = 1.0f;
            }
            if (inputController->MoveDown.EndedDown)
            {
                playerDy = -1.0f;
            }
            if (inputController->MoveLeft.EndedDown)
            {
                playerDx = -1.0f;
                
            }
            if (inputController->MoveRight.EndedDown)
            {
                playerDx = 1.0f;
            }

            real32 speed = 4 * input->DtForFrame;

            world_position newPlayerPosition = gameState->PlayerPosition;
            newPlayerPosition.RelX += playerDx * speed;
            newPlayerPosition.RelY += playerDy * speed;
            newPlayerPosition = RecanonicalizePosition(&world, newPlayerPosition);
            
            world_position playerLeft = newPlayerPosition;
            playerLeft.RelX -= playerWidth / 2;
            playerLeft = RecanonicalizePosition(&world, playerLeft);

            world_position playerRight = newPlayerPosition;
            playerRight.RelX += playerWidth / 2;
            playerRight = RecanonicalizePosition(&world, playerRight);
            
            if (IsWorldPointEmpty(&world, playerLeft)
                && IsWorldPointEmpty(&world, playerRight))
            {
                world_position canPos = newPlayerPosition;

                gameState->PlayerPosition = canPos;
            }
        }
    }


    ClearRenderBuffer(buffer);

    tile_chunk_position chunkPos = GetChunkPositionFor(&world, gameState->PlayerPosition.AbsTileX, gameState->PlayerPosition.AbsTileY);
    tile_chunk *currentTileMap = GetTileChunk(&world, chunkPos.TileChunkX, chunkPos.TileChunkY);

    real32 centerX = 0.5f * (real32) buffer->Width;
    real32 centerY = 0.5f * (real32) buffer->Height;
    
    for (uint32 row = gameState->PlayerPosition.AbsTileX - 10;
         row < gameState->PlayerPosition.AbsTileX + 10;
        ++row)
    {
        for (uint32 col=gameState->PlayerPosition.AbsTileY - 20;
             col < gameState->PlayerPosition.AbsTileY - 20;
             ++col)
        {
            uint32 tileId = GetTileValue(&world, currentTileMap, col, row);
            
            color color;
            if (tileId == 1)
            {
                color.Red = 0.3f;
                color.Green = 0.3f;
                color.Blue = 1;                
            }
            else
            {
                color.Red = 0.2f;
                color.Green = 0.2f;
                color.Blue = 0.2f;
            }

            // debug stuff
            if (row == gameState->PlayerPosition.AbsTileX && col == gameState->PlayerPosition.AbsTileY)
            {
                color.Red += 0.2f;
                color.Green += 0.2f;
                color.Blue += 0.2f;
            }

            real32 x = centerX + (real32)(col * world.TileSideInPixels);
            real32 y = centerY - (real32)(row * world.TileSideInPixels);
            RenderRectangle(buffer, x, y - (real32)world.TileSideInPixels, x + (real32)world.TileSideInPixels, y, color);
        }
    }

    // color redColor;
    // redColor.Red = 1;
    // redColor.Green = 0;
    // redColor.Blue = 0;
    
    // RenderRectangle(buffer, (real32)gameState->XOffset, (real32)gameState->YOffset, (real32)input->MouseX, (real32)input->MouseY, redColor);

    // RenderPlayer(buffer, input->MouseX, input->MouseY);


    color playerColor;
    playerColor.Red = 1.0f;
    playerColor.Green = 1.0f;
    playerColor.Blue = 0;

    real32 playerLeft = centerX
        + gameState->PlayerPosition.AbsTileX * world.TileSideInPixels
        + gameState->PlayerPosition.RelX * world.MetersToPixels
        - 0.5f * playerWidth * world.MetersToPixels;
    real32 playerTop = centerY
        - gameState->PlayerPosition.AbsTileY * world.TileSideInPixels
        - gameState->PlayerPosition.RelY * world.MetersToPixels
        - playerHeight * world.MetersToPixels;

    RenderRectangle(buffer, playerLeft, playerTop, playerLeft + playerWidth * world.MetersToPixels, playerTop + playerHeight * world.MetersToPixels, playerColor);
    

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
