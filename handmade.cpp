#include "handmade.h"

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
        
        tSine += 2.0 * Pi32 * 1.0f / (real32)wavePeriod;
    }
}

internal void
RenderGradient(game_offscreen_buffer *buffer, int xOffset, int yOffset)
{
    int bytesPerPixel = 4;
    
    uint8 *row = (uint8 *)buffer->Memory;
    for (int y=0; y < buffer->Height; ++y)
    {
         uint32 *pixel = (uint32 *)row;

        for (int x=0; x < buffer->Width; ++x)
        {
            uint8 b = x + xOffset;
            uint8 g = y + yOffset;

            *pixel++ = ((g << 8) | b);
        }

        row += buffer->Pitch;
    }
}

internal void
GameUpdateAndRender(game_offscreen_buffer *buffer, game_sound_output_buffer *soundBuffer, game_input *input)
{
    local_persist int xOffset = 0;
    local_persist int yOffset = 0;
    local_persist int toneHz = 512;

    game_controller_input *input0 = &input->Controllers[0];

    if (input0->IsAnalog)
    {
        toneHz = 256 + (int) (128.0f * input0->EndX);
        xOffset -= (int)(4.0f * input0->EndX);
        yOffset += (int)(4.0f * input0->EndY);
    }
    else
    {
        if (input0->Up.EndedDown)
        {
            yOffset += 1;
        }
        if (input0->Down.EndedDown)
        {
            yOffset -= 1;
        }
        if (input0->Left.EndedDown)
        {
            xOffset += 1;
        }
        if (input0->Right.EndedDown)
        {
            xOffset -= 1;
        }
        
    }
    
    // todo: allow sample offsets for more platform options
    GameOutputSound(soundBuffer, toneHz);
    
    RenderGradient(buffer, xOffset, yOffset);
}

