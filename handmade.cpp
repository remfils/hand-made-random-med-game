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

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(game_state) <= memory->PermanentStorageSize);
    
    game_state *gameState = (game_state *)memory->PermanentStorage;

    if (!memory->IsInitialized)
    {
        gameState->XOffset = 0;
        gameState->YOffset = 0;
        gameState->ToneHz = 256;

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

            gameState->PlayerX += playerDx * speed;
            gameState->PlayerY += playerDy * speed;
        }
    }


    ClearRenderBuffer(buffer);

    uint32 tileMap[9][16] =
        {
            {1, 0, 1, 0,  0, 0, 0, 0,  0, 1, 0, 0,  0, 0, 0, 1},
            {1, 0, 0, 1,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 1},
            {1, 0, 0, 0,  1, 1, 1, 1,  0, 0, 0, 0,  0, 0, 0, 1},
            {1, 0, 0, 0,  1, 0, 0, 1,  0, 0, 0, 0,  0, 0, 0, 1},
            {0, 0, 0, 0,  1, 0, 0, 1,  0, 0, 0, 0,  0, 0, 0, 0},
            {1, 0, 0, 0,  1, 0, 0, 1,  0, 0, 0, 0,  0, 0, 0, 1},
            {1, 0, 0, 0,  1, 1, 1, 1,  0, 0, 0, 0,  0, 0, 0, 1},
            {1, 0, 0, 1,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 1},
            {1, 0, 1, 0,  0, 0, 0, 0,  0, 1, 0, 0,  0, 0, 0, 1}
        };

    real32 topPadding = 10;
    real32 leftPadding = 10;
    real32 tileWidth = 50;
    for (int row = 0;
         row < 9;
        ++row)
    {
        for (int col=0;
             col<16;
             ++col)
        {
            uint32 tileId = tileMap[row][col];
            
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

            real32 x = ((real32)col) * tileWidth + leftPadding;
            real32 y = ((real32)row) * tileWidth + topPadding;
            RenderRectangle(buffer, x, y, x + tileWidth, y + tileWidth, color);
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

    real32 playerWidth = 0.75 * tileWidth;
    real32 playerHeight = tileWidth;

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
