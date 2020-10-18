
/*
  HANDMADE_INTERNAL:
  	0 - build for public release
        1 - build for developer ony

  HANDMADE_SLOW:
  	0 - no slow code is allowed
        1 - can be less performant
 */

#define Kilobytes(val) ((val)*1024LL)
#define Megabytes(val) (Kilobytes((val)*1024LL))
#define Gigabytes(val) (Megabytes((val)*1024LL))
#define Terabytes(val) (Gigabytes((val)*1024LL))
#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))

#if HANDMADE_SLOW
#define Assert(expression) \
    if (!(expression)) {*(int *)0 = 0;}
#else
#define Assert(expression)
#endif

inline uint32
SafeTruncateUInt64(uint64 value)
{
    Assert(value <= 0xFFFFFFFF);
    uint32 result = (uint32)value;
    return (result);
}

#if HANDMADE_INTERNAL
struct debug_read_file_result
{
    void *Content;
    uint32 ContentSize;
};
void Debug_PlatformFreeFileMemory(void *memory);
debug_read_file_result Debug_PlatformReadEntireFile(char *filename);
bool32 Debug_PlatformWriteEntireFile(char *filename, uint32 memorySize, void *memory);
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
    bool32 IsConnected;
    bool32 IsAnalog;
    real32 StickAverageX;
    real32 StickAverageY;
    
    union
    {
        game_button_state Buttons[12];
        struct
        {
            game_button_state MoveUp;
            game_button_state MoveDown;
            game_button_state MoveLeft;
            game_button_state MoveRight;

            game_button_state ActionUp;
            game_button_state ActionDown;
            game_button_state ActionLeft;
            game_button_state ActionRight;

            game_button_state LeftShoulder;
            game_button_state RightShoulder;

            game_button_state Start;
            game_button_state Back;
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
    game_controller_input Controllers[5];
};

void GameUpdateAndRender(game_memory *memory, game_offscreen_buffer *buffer, game_sound_output_buffer *soundBuffer, game_input *input);
