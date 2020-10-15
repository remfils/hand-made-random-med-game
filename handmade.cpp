#include "handmade.h"

static void
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
GameUpdateAndRender(game_offscreen_buffer *buffer)
{
    int xOffset = 0;
    int yOffset = 0;
    RenderGradient(buffer, xOffset, yOffset);
}

