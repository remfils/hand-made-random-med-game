internal void
GameOutputSound(game_sound_output_buffer *soundBuffer, int toneHz)
{
    local_persist real32 tSine;
    int16 toneVolume = 8000;
    int wavePeriod = soundBuffer->SamplesPerSecond / toneHz;
    
    int16 *sampleOut = soundBuffer->Samples;
    for (int sampleIndex = 0; sampleIndex < soundBuffer->SampleCount; ++sampleIndex)
    {
        // real32 t = 2.0 * Pi32 * (real32)soundOutput->runningSampleIndex / (real32)soundOutput->wavePeriod;
        real32 sineValue = sinf(tSine);
        int16 sampleValue = (int16)(sineValue * toneVolume);
        *sampleOut++ = sampleValue;
        *sampleOut++ = sampleValue;
        
        tSine += (real32)(2.0f * Pi32 * 1.0f) / (real32)wavePeriod;
    }
}

internal void
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

internal void
GameUpdateAndRender(game_memory *memory, game_offscreen_buffer *buffer, game_sound_output_buffer *soundBuffer, game_input *input)
{
    // local_persist int xOffset = 0;
    // local_persist int yOffset = 0;
    // local_persist int toneHz = 512;

    Assert(sizeof(game_state) <= memory->PermanentStorageSize);
    
    game_state *gameState = (game_state *)memory->PermanentStorage;

    if (!memory->IsInitialized)
    {
        gameState->XOffset = 0;
        gameState->YOffset = 0;
        gameState->ToneHz = 256;

        char *fileName = __FILE__;
        debug_read_file_result fileResult = Debug_PlatformReadEntireFile(fileName);
        if (fileResult.Content)
        {
            Debug_PlatformWriteEntireFile("test.txt", fileResult.ContentSize, fileResult.Content);
            Debug_PlatformFreeFileMemory(fileResult.Content);
        }
        
        memory->IsInitialized = 1;
    }

    for (int controllerIndex=0; controllerIndex<ArrayCount(input->Controllers); controllerIndex++)
    {
        game_controller_input *inputController = &input->Controllers[controllerIndex];

        if (inputController->IsAnalog)
        {
            // gameState->ToneHz = 256 + (int)(128.0f * inputController->EndX);
            // gameState->XOffset -= (int)(4.0f * inputController->StickAverageX);
            // gameState->YOffset += (int)(4.0f * inputController->StickAverageY);
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
    
    // todo: allow sample offsets for more platform options
    GameOutputSound(soundBuffer, gameState->ToneHz);
    
    RenderGradient(buffer, gameState->XOffset, gameState->YOffset);
}

