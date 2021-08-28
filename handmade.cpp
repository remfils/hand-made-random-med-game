#include "handmade.h"
#include "handmade_tile.cpp"


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

internal void
InitializeArena(memory_arena *arena, memory_index size, uint8 *base)
{
    arena->Size = size;
    arena->Base = base;
    arena->Used = 0;
}

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


extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(game_state) <= memory->PermanentStorageSize);
    
    game_state *gameState = (game_state *)memory->PermanentStorage;

    if (!memory->IsInitialized)
    {
        InitializeArena(&gameState->WorldArena, memory->PermanentStorageSize - sizeof(game_state), (uint8 *)memory->PermanentStorage + sizeof(game_state));
        
        gameState->XOffset = 0;
        gameState->YOffset = 0;
        gameState->ToneHz = 256;

        gameState->PlayerPosition.AbsTileX = 2;
        gameState->PlayerPosition.AbsTileY = 2;
        gameState->PlayerPosition.TileRelX = 5.f;
        gameState->PlayerPosition.TileRelY = 5.f;

        gameState->World = PushSize(&gameState->WorldArena, world);
        world *world = gameState->World;

        world->TileMap = PushSize(&gameState->WorldArena, tile_map);
        tile_map *tileMap = world->TileMap;

        tileMap->TileMapCountX = 4;
        tileMap->TileMapCountY = 4;
    
        tileMap->LowerLeftX = 0;
        tileMap->LowerLeftY = (real32)buffer->Height;

        tileMap->ChunkShift = 8;
        tileMap->ChunkMask = 0xff;
        tileMap->ChunkDim = 256;
        
        tileMap->TileSideInMeters = 1.4f;
        tileMap->TileSideInPixels = 56;
        tileMap->MetersToPixels = (real32)tileMap->TileSideInPixels / (real32)tileMap->TileSideInMeters;

        tileMap->TileChunks = PushArray(&gameState->WorldArena, tileMap->TileMapCountX * tileMap->TileMapCountY, tile_chunk);
        for (uint32 y = 0;
             y < tileMap->TileMapCountY;
             ++y)
        {
            for (uint32 x = 0;
                 x < tileMap->TileMapCountX;
                 ++x)
            {
                tileMap->TileChunks[y * tileMap->TileMapCountX + x].Tiles = PushArray(&gameState->WorldArena, tileMap->ChunkDim, uint32);
            }   
        }


        // generate tiles
        uint32 tilesPerScreenHeight = 9;
        uint32 tilesPerScreenWidth = 17;
        for (uint32 screenX = 0;
             screenX < 25;
             ++screenX)
        {
            for (uint32 screenY = 0;
                 screenY < 25;
                 ++screenY)
            {
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
                        SetTileValue(&gameState->WorldArena, world->TileMap, absTileX, absTileY,
                            (tileX == tileY) && (tileY % 2) ? 1 : 0);
                    }   
                }   
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

    real32 playerWidth = (real32)0.75 * (real32)tileMap->TileSideInMeters;
    real32 playerHeight = (real32)tileMap->TileSideInMeters;

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

            tile_map_position newPlayerPosition = gameState->PlayerPosition;
            newPlayerPosition.TileRelX += playerDx * speed;
            newPlayerPosition.TileRelY += playerDy * speed;
            newPlayerPosition = RecanonicalizePosition(tileMap, newPlayerPosition);
            
            tile_map_position playerLeft = newPlayerPosition;
            playerLeft.TileRelX -= playerWidth / 2;
            playerLeft = RecanonicalizePosition(tileMap, playerLeft);

            tile_map_position playerRight = newPlayerPosition;
            playerRight.TileRelX += playerWidth / 2;
            playerRight = RecanonicalizePosition(tileMap, playerRight);
            
            if (IsTileMapPointEmpty(tileMap, playerLeft)
                && IsTileMapPointEmpty(tileMap, playerRight))
            {
                tile_map_position canPos = newPlayerPosition;

                gameState->PlayerPosition = canPos;
            }
        }
    }


    ClearRenderBuffer(buffer);

    tile_chunk_position chunkPos = GetChunkPositionFor(tileMap, gameState->PlayerPosition.AbsTileX, gameState->PlayerPosition.AbsTileY);
    tile_chunk *currentTileMap = GetTileChunk(tileMap, chunkPos.TileChunkX, chunkPos.TileChunkY);

    real32 screenCenterX = 0.5f * (real32) buffer->Width;
    real32 screenCenterY = 0.5f * (real32) buffer->Height;
    
    for (int32 relRow = -10; relRow < 10; ++relRow)
    {
        for (int32 relCol = -20; relCol < 20; ++relCol)
        {
            uint32 col = gameState->PlayerPosition.AbsTileX + relCol;
            uint32 row = gameState->PlayerPosition.AbsTileY + relRow;
            uint32 tileId = GetTileValue(tileMap, col, row);
            
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
            if (col == gameState->PlayerPosition.AbsTileX && row == gameState->PlayerPosition.AbsTileY)
            {
                color.Red += 0.2f;
                color.Green += 0.2f;
                color.Blue += 0.2f;
            }

            real32 cenX = screenCenterX - tileMap->MetersToPixels * gameState->PlayerPosition.TileRelX + ((real32) relCol) * tileMap->TileSideInPixels;
            real32 cenY = screenCenterY + tileMap->MetersToPixels * gameState->PlayerPosition.TileRelY - ((real32) relRow ) * tileMap->TileSideInPixels;

            real32 minX = cenX - 0.5f * tileMap->TileSideInPixels;
            real32 minY = cenY - 0.5f * tileMap->TileSideInPixels;
            // real32 minY = screenCenterY + tileMap->MetersToPixels * gameState->PlayerPosition.TileRelY - ((real32) relRow ) * tileMap->TileSideInPixels;
            real32 maxX = cenX + 0.5f * tileMap->TileSideInPixels;
            real32 maxY = cenY + 0.5f * tileMap->TileSideInPixels;

            RenderRectangle(buffer, minX, minY, maxX, maxY, color);
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

    real32 playerLeft = screenCenterX - 0.5f * tileMap->MetersToPixels * playerWidth;
    real32 playerTop = screenCenterY - tileMap->MetersToPixels * playerHeight;

    RenderRectangle(buffer, playerLeft, playerTop, playerLeft + playerWidth * tileMap->MetersToPixels, playerTop + playerHeight * tileMap->MetersToPixels, playerColor);
    

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
