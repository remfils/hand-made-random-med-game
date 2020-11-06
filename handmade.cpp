#include "handmade.h"

int32
RoundRealToInt(real32 value)
{
    int32 result = (int32)(value + 0.5f);
    return(result);
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
RenderRectangle(game_offscreen_buffer *buffer, real32 realMinX, real32 realMinY, real32 realMaxX, real32 realMaxY, uint32 color)
{
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
            *(uint32 *)pixel = color;
            pixel++;
        }
        row += buffer->Pitch;
    }
}

void
ClearRenderBuffer(game_offscreen_buffer *buffer)
{
    uint32 bgColor = 0x002200;    
    RenderRectangle(buffer, 0, 0, (real32)buffer->Width, (real32)buffer->Height, bgColor);
}

void
RenderPlayer(game_offscreen_buffer *buffer, int playerX, int playerY)
{
    real32 squareHalfWidth = 10;
    int playerColor = 0xFF00FF;

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
        
        }

        if (inputController->MoveUp.EndedDown)
        {
            gameState->YOffset += 1;
        }
        if (inputController->MoveDown.EndedDown)
        {
            gameState->YOffset -= 1;
        }
        if (inputController->MoveLeft.EndedDown)
        {
            gameState->XOffset += 1;
        }
        if (inputController->MoveRight.EndedDown)
        {
            gameState->XOffset -= 1;
        }
    }

    ClearRenderBuffer(buffer);

    RenderRectangle(buffer, (real32)gameState->XOffset, (real32)gameState->YOffset, (real32)input->MouseX, (real32)input->MouseY, 0xFF0000);

    RenderPlayer(buffer, input->MouseX, input->MouseY);

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
