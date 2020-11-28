#include "handmade.h"

int32
RoundRealToInt(real32 value)
{
    int32 result = (int32)(value + 0.5f);
    return(result);
}

uint32 GetUintColor(color color)
{
    uint32 uColor = (uint32)(
        (RoundRealToInt(color.Red * 255.0f) << 16)
        | (RoundRealToInt(color.Green * 255.0f) << 8)
        | (RoundRealToInt(color.Blue * 255.0f) << 0)
        );

    return uColor;
}

void
GameOutputSound(game_sound_output_buffer *soundBuffer, game_state *gameState)
{
    int16 toneVolume = 2000;
    int wavePeriod = soundBuffer->SamplesPerSecond / gameState->ToneHz;
    
    int16 *sampleOut = soundBuffer->Samples;

    for (int sampleIndex = 0; sampleIndex < soundBuffer->SampleCount; ++sampleIndex)
    {
        *sampleOut++ = 0;
        *sampleOut++ = 0;
        
        // real32 sineValue = sinf(gameState->TSine);
        // int16 sampleValue = (int16)(sineValue * toneVolume);
        // *sampleOut++ = sampleValue;
        // *sampleOut++ = sampleValue;
        
        // gameState->TSine += (real32)(2.0f * Pi32 * 1.0f) / (real32)wavePeriod;
    }
}

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

void
RenderRectangle(game_offscreen_buffer *buffer, real32 realMinX, real32 realMinY, real32 realMaxX, real32 realMaxY, color color)
{
    uint32 uColor = GetUintColor(color);
    
    int32 minX = RoundRealToInt(realMinX);
    int32 minY = RoundRealToInt(realMinY);
    int32 maxX = RoundRealToInt(realMaxX);
    int32 maxY = RoundRealToInt(realMaxY);

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


struct tile_map
{
    int CountX;
    int CountY;
    real32 PaddingLeft;
    real32 PaddingTop;
    real32 TileWidth;
    real32 TileHeight;

    uint32 *Tiles;
};

inline uint32
GetTileValueUnchecked(tile_map *tileMap, int32 tileX, int32 tileY)
{
    uint32 tileValue = tileMap->Tiles[tileY * tileMap->CountX + tileX];
    return tileValue;
}

internal uint32
GetTileValueUnchecked(tile_map *tileMap, real32 testX, real32 testY)
{
    int32 testTileX = (int32)((testX - tileMap->PaddingLeft) / tileMap->TileWidth);
    int32 testTileY = (int32)((testY - tileMap->PaddingTop) / tileMap->TileHeight);
    
    bool32 isValid = false;
    if ((testTileX >= 0) && (testTileX < tileMap->CountX)
        && (testTileY >0) && (testTileY < tileMap->CountY))
    {
        uint32 tileMapValue = GetTileValueUnchecked(tileMap, testTileX, testTileY);

        return tileMapValue;
    }

    return -1;
}

internal bool32
IsPlayerInExit(tile_map *tileMap, real32 testX, real32 testY)
{
    uint32 value = GetTileValueUnchecked(tileMap, testX, testY);
    if (value != 9)
    {
        return false;
    }
    else
    {
        return true;
    }
}


internal bool32
IsTileMapPointEmpty(tile_map *tileMap, real32 testX, real32 testY)
{
    int32 testTileX = (int32)((testX - tileMap->PaddingLeft) / tileMap->TileWidth);
    int32 testTileY = (int32)((testY - tileMap->PaddingTop) / tileMap->TileHeight);
    
    bool32 isValid = false;
    if ((testTileX >= 0) && (testTileX < tileMap->CountX)
        && (testTileY >0) && (testTileY < tileMap->CountY))
    {
        uint32 tileMapValue = GetTileValueUnchecked(tileMap, testTileX, testTileY);

        isValid = (tileMapValue != 1);
    }

    return isValid;
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

        gameState->CurrentTileIndex = 1;

        gameState->PlayerX = 100;
        gameState->PlayerY = 100;

        char *fileName = __FILE__;
        debug_read_file_result fileResult = memory->DEBUG_PlatformReadEntireFile(thread, fileName);
        if (fileResult.Content)
        {
            memory->DEBUG_PlatformWriteEntireFile(thread, "test.txt", fileResult.ContentSize, fileResult.Content);
            memory->DEBUG_PlatformFreeFileMemory(thread, fileResult.Content);
        }
        
        memory->IsInitialized = 1;
    }

#define TILE_MAP_COUNT_X 17
#define TILE_MAP_COUNT_Y 9


    uint32 map0[9][17] =
    {
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 1,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  1, 1, 1, 1,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  1, 0, 0, 1,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {9, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 9},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  1, 1, 1, 1,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 1,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 1, 0,  0, 0, 0, 0,  0, 1, 0, 0,  0, 0, 0, 0, 1}
    };

    uint32 map1[9][17] =
    {
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  1, 1, 1, 1,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 1, 1, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {9, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 9},
        {1, 1, 1, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  1, 1, 1, 1,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1}
    };

    


    tile_map tileMaps[2];

    tileMaps[0].CountX = 17;
    tileMaps[0].CountY = 9;
    tileMaps[0].PaddingTop = 10;
    tileMaps[0].PaddingLeft = 10;
    tileMaps[0].TileWidth = 50;
    tileMaps[0].TileHeight = 50;
    tileMaps[0].Tiles = (uint32 *)map0;

    tileMaps[1] = tileMaps[0];

    tileMaps[1].Tiles = (uint32 *)map1;


    tile_map tileMap = tileMaps[gameState->CurrentTileIndex];
    

    real32 playerWidth = 0.75 * tileMap.TileWidth;
    real32 playerHeight = tileMap.TileHeight;

    

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

            gameState->PlayerX += (int)(10.0f * inputController->StickAverageX);
            gameState->PlayerY -= (int)(10.0f * inputController->StickAverageY);
        }
        else
        {
            real32 speed = 128.0f * input->DtForFrame;
            
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

            real32 newPlayerX = gameState->PlayerX + playerDx * speed;
            real32 newPlayerY = gameState->PlayerY + playerDy * speed;

            bool32 isValid = IsTileMapPointEmpty(&tileMap, newPlayerX + playerWidth / 2, newPlayerY)
                && IsTileMapPointEmpty(&tileMap, newPlayerX - playerWidth / 2, newPlayerY);

            if (isValid)
            {

                uint32 tileValue = GetTileValueUnchecked(&tileMap, newPlayerX, newPlayerY);

                if (IsPlayerInExit(&tileMap, newPlayerX, newPlayerY))
                {
                    if (gameState->CurrentTileIndex == 1)
                    {
                        gameState->CurrentTileIndex = 0;
                    }
                    else
                    {
                        gameState->CurrentTileIndex = 1;
                    }

                    gameState->PlayerX = 100;
                    gameState->PlayerY = 100;
                }
                else
                {
                    gameState->PlayerX = newPlayerX;
                    gameState->PlayerY = newPlayerY;
                }
            }
        }
    }


    ClearRenderBuffer(buffer);
    
    for (int row = 0;
         row < 9;
        ++row)
    {
        for (int col=0;
             col<17;
             ++col)
        {
            uint32 tileId = GetTileValueUnchecked(&tileMap, col, row);
            
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

            real32 x = ((real32)col) * tileMap.TileWidth + tileMap.PaddingLeft;
            real32 y = ((real32)row) * tileMap.TileHeight + tileMap.PaddingTop;
            RenderRectangle(buffer, x, y, x + tileMap.TileWidth, y + tileMap.TileHeight, color);
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

    real32 playerLeft = gameState->PlayerX - 0.5f * playerWidth;
    real32 playerTop = gameState->PlayerY - playerHeight;

    RenderRectangle(buffer, playerLeft, playerTop, playerLeft + playerWidth, playerTop + playerHeight, playerColor);
    

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
