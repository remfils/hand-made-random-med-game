struct game_offscreen_buffer
{
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel = 4;
};

struct game_sound_output_buffer
{
    int16 *Samples;
    int SampleCount;
    int SamplesPerSecond;
};

void GameUpdateAndRender(game_offscreen_buffer *buffer, game_sound_output_buffer *soundBuffer);
