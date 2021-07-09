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


inline uint32
GetTileValueUnchecked(world *world, tile_map *tileMap, int32 tileX, int32 tileY)
{
    uint32 tileValue = tileMap->Tiles[tileY * world->TileCountX + tileX];
    return tileValue;
}

inline void
RecanonicalizeCoord(world *world, int32 tileCount, int32 *tileMapCoord, int32* tileCoord, real32 *relCoord)
{
    // todo: player can go back
    int32 offset = FloorReal32ToInt32(*relCoord / world->TileSideInMeters);
    *tileCoord += offset;
    *relCoord = *relCoord - offset * world->TileSideInMeters;

    if (*tileCoord <= 0)
    {
        *tileCoord = tileCount + *tileCoord;
        --*tileMapCoord;
    }

    if (*tileCoord >= tileCount)
    {
        *tileCoord = *tileCoord - tileCount;
        ++*tileMapCoord;
    }
}

inline canonical_position
RecanonicalizePosition(world *world, canonical_position pos)
{
    canonical_position res = pos;
    
    RecanonicalizeCoord(world, world->TileCountX, &res.TileMapX, &res.TileX, &res.RelX);
    RecanonicalizeCoord(world, world->TileCountY, &res.TileMapY, &res.TileY, &res.RelY);

    return res;
}

internal tile_map*
GetTileMap(world * world, int32 tileMapX, int32 tileMapY)
{
    tile_map *tileMap = 0;

    if ((tileMapX >= 0) && (tileMapX < world->TileMapCountX) &&
        (tileMapY >= 0) && (tileMapY < world->TileMapCountY))
    {
        tileMap = &world->TileMaps[tileMapY * world->TileMapCountX + tileMapX];
    }

    return tileMap;
}

internal bool32
IsTileMapPointEmpty(world * world, tile_map *tileMap, int32 testX, int32 testY)
{
    bool32 isValid = false;

    if (tileMap)
    {
        if ((testX >= 0) && (testX < world->TileCountX)
            && (testY >= 0) && (testY < world->TileCountY))
        {
            uint32 tileMapValue = GetTileValueUnchecked(world, tileMap, testX, testY);

            isValid = (tileMapValue != 1);
        }
    }
    
    return isValid;
}

internal bool32
IsWorldPointEmpty(world *world, canonical_position wp)
{
    tile_map* tm = GetTileMap(world, wp.TileMapX, wp.TileMapY);

    bool32 isEmpty = IsTileMapPointEmpty(world, tm, wp.TileX, wp.TileY);

    return isEmpty;
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

        gameState->PlayerPosition.TileMapX = 0;
        gameState->PlayerPosition.TileMapY = 0;
        gameState->PlayerPosition.TileX = 2;
        gameState->PlayerPosition.TileY = 2;
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

#define TILE_MAP_COUNT_X 17
#define TILE_MAP_COUNT_Y 9


    uint32 map00[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
    {
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 1, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1}
    };

    uint32 map01[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
    {
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0}
    };

    uint32 map10[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
    {
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1}
    };

    uint32 map11[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
    {
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {9, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 9},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1}
    };

    tile_map tileMaps[2][2];
    tileMaps[0][0].Tiles = (uint32 *)map00;
    tileMaps[0][1].Tiles = (uint32 *)map01;
    tileMaps[1][0].Tiles = (uint32 *)map10;
    tileMaps[1][1].Tiles = (uint32 *)map11;

    world world;
    
    world.TileMapCountX = 2;
    world.TileMapCountY = 2;
    world.TileCountX = 17;
    world.TileCountY = 9;
    
    world.UpperLeftX = 10;
    world.UpperLeftY = 10;
        
    world.TileSideInMeters = 1.4f;
    world.TileSideInPixels = 50;
    world.MetersToPixels = (real32)world.TileSideInPixels / (real32)world.TileSideInMeters;
    

    world.TileMaps = (tile_map*)tileMaps;

    tile_map *currentTileMap = GetTileMap(&world, gameState->PlayerPosition.TileMapX, gameState->PlayerPosition.TileMapY);
    Assert(currentTileMap);

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
                playerDy = -1.0f;
            }
            if (inputController->MoveDown.EndedDown)
            {
                playerDy = 1.0f;
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

            canonical_position newPlayerPosition = gameState->PlayerPosition;
            newPlayerPosition.RelX += playerDx * speed;
            newPlayerPosition.RelY += playerDy * speed;
            newPlayerPosition = RecanonicalizePosition(&world, newPlayerPosition);
            
            canonical_position playerLeft = newPlayerPosition;
            playerLeft.RelX -= playerWidth / 2;
            playerLeft = RecanonicalizePosition(&world, playerLeft);

            canonical_position playerRight = newPlayerPosition;
            playerRight.RelX += playerWidth / 2;
            playerRight = RecanonicalizePosition(&world, playerRight);
            
            if (IsWorldPointEmpty(&world, playerLeft)
                && IsWorldPointEmpty(&world, playerRight))
            {
                canonical_position canPos = newPlayerPosition;

                gameState->PlayerPosition = canPos;
            }
        }
    }


    ClearRenderBuffer(buffer);
    
    for (int row = 0;
         row < world.TileCountY;
        ++row)
    {
        for (int col=0;
             col<world.TileCountX;
             ++col)
        {
            uint32 tileId = GetTileValueUnchecked(&world, currentTileMap, col, row);
            
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
            if (row == gameState->PlayerPosition.TileY && col == gameState->PlayerPosition.TileX)
            {
                color.Red += 0.2f;
                color.Green += 0.2f;
                color.Blue += 0.2f;
            }

            real32 x = ((real32)col) * world.TileSideInPixels + world.UpperLeftX;
            real32 y = ((real32)row) * world.TileSideInPixels + world.UpperLeftY;
            RenderRectangle(buffer, x, y, x + world.TileSideInPixels, y + world.TileSideInPixels, color);
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

    real32 playerLeft = world.UpperLeftX
        + gameState->PlayerPosition.TileX * world.TileSideInPixels
        + gameState->PlayerPosition.RelX * world.MetersToPixels
        - 0.5f * playerWidth * world.MetersToPixels;
    real32 playerTop = world.UpperLeftY
        + gameState->PlayerPosition.TileY * world.TileSideInPixels
        + gameState->PlayerPosition.RelY * world.MetersToPixels
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
