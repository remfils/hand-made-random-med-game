
/*
  HANDMADE_INTERNAL:
  	0 - build for public release
        1 - build for developer ony

  HANDMADE_SLOW:
  	0 - no slow code is allowed
        1 - can be less performant
 */

#define Kilobytes(val) ((val)*1024)
#define Megabytes(val) (Kilobytes((val)*1024))
#define Gigabytes(val) (Megabytes((val)*1024))
#define Terabytes(val) (Gigabytes((val)*1024))
#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))

#if HANDMADE_SLOW
#define Assert(expression) \
    if (!(expression)) {*(int *)0 = 0;}
#else
#define Assert(expression)
#endif

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

struct game_state
{
    int ToneHz;
    int XOffset;
    int YOffset;
};

struct game_memory
{
    bool32 IsInitialized;
    uint64 PermanentStorageSize;
    void *PermanentStorage; // should be initialized to zero

    uint64 TransientStorageSize;
    void *TransientStorage; // should be initialized to zero
};

struct game_input
{
    game_controller_input Controllers[4];
};

void GameUpdateAndRender(game_memory *memory, game_offscreen_buffer *buffer, game_sound_output_buffer *soundBuffer, game_input *input);
