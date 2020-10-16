#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))

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

struct game_button_state
{
    int HalfTransitionCount;
    bool32 EndedDown;
};

struct game_controller_input
{
    real32 StartX;
    real32 StartY;
    real32 EndX;
    real32 EndY;
    real32 MinX;
    real32 MinY;
    real32 MaxX;
    real32 MaxY;

    bool32 IsAnalog;
    
    union
    {
        game_button_state Buttons[0];
        struct
        {
            game_button_state Up;
            game_button_state Down;
            game_button_state Left;
            game_button_state Right;
            game_button_state LeftShoulder;
            game_button_state RightShoulder;
        };
    };
};

struct game_input
{
    game_controller_input Controllers[4];
};

void GameUpdateAndRender(game_offscreen_buffer *buffer, game_sound_output_buffer *soundBuffer, game_input *input);
