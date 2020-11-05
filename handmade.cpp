#include "handmade.h"

void
GameOutputSound(game_sound_output_buffer *soundBuffer, game_state *gameState)
{
    int16 toneVolume = 2000;
    int wavePeriod = soundBuffer->SamplesPerSecond / gameState->ToneHz;
    
    int16 *sampleOut = soundBuffer->Samples;
    for (int sampleIndex = 0; sampleIndex < soundBuffer->SampleCount; ++sampleIndex)
    {
        real32 sineValue = sinf(gameState->TSine);
        int16 sampleValue = (int16)(sineValue * toneVolume);
        *sampleOut++ = sampleValue;
        *sampleOut++ = sampleValue;
        
        gameState->TSine += (real32)(2.0f * Pi32 * 1.0f) / (real32)wavePeriod;
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
RenderPlayer(game_offscreen_buffer *buffer, int playerX, int playerY)
{
    int squareSide = 20;
    int playerColor = 0xFF00FF;

    for (int x = playerX; x < playerX + squareSide; ++x)
    {
        uint8 *pixel = ((uint8 *)buffer->Memory + x * buffer->BytesPerPixel + playerY * buffer->Pitch);
                
        for (int y=playerY; y < playerY + squareSide; ++y)
        {
            *(uint32 *)pixel = playerColor;
            pixel += buffer->Pitch;
        }
    }
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
        debug_read_file_result fileResult = memory->DEBUG_PlatformReadEntireFile(fileName);
        if (fileResult.Content)
        {
            memory->DEBUG_PlatformWriteEntireFile("test.txt", fileResult.ContentSize, fileResult.Content);
            memory->DEBUG_PlatformFreeFileMemory(fileResult.Content);
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
    
    RenderGradient(buffer, gameState->XOffset, gameState->YOffset);

    RenderPlayer(buffer, gameState->PlayerX, gameState->PlayerY);
}


extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *gameState = (game_state*)memory->PermanentStorage;
    GameOutputSound(soundBuffer, gameState);
}
