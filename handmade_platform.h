#include <stdint.h>
#include <limits.h>
#include <float.h>

#if _MSC_VER
#include <intrin.h>
#endif

#define internal static
#define local_persist static
#define global_variable static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef size_t memory_index;

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef float real32;
typedef double real64;

#define Pi32 3.1415926535f
#define Real32Maximum FLT_MAX

#define Kilobytes(val) ((val)*1024LL)
#define Megabytes(val) (Kilobytes((val)*1024LL))
#define Gigabytes(val) (Megabytes((val)*1024LL))
#define Terabytes(val) (Gigabytes((val)*1024LL))
#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))


#if HANDMADE_SLOW
#define Assert(expression) if (!(expression)) {*(int *)0 = 0;}

enum
{
    DebugCounter_GameUpdateAndRender,
    DebugCounter_RenderGroupToOutput,
    DebugCounter_RenderRectangleSlowly,
    DebugCounter_Slowly_TestPixel,
    DebugCounter_Slowly_FillPixel,
    DebugCounter_RenderRectangleHopefullyQuickly,
    DebugCounter_RenderRectangle,
    // NOTE: MUST BE LAST
    DebugCounter_Count
};

typedef struct debug_cycle_counter
{
    uint64 CycleCount;
    uint32 HitCount;
} debug_cycle_counter;

extern struct game_memory *DebugGlobalMemory;

#if _MSC_VER
#define BEGIN_TIMED_BLOCK(ID) uint64 startCycleCount##ID = __rdtsc();
#define END_TIMED_BLOCK(ID) DebugGlobalMemory->DebugCounters[DebugCounter_##ID].CycleCount += __rdtsc() - startCycleCount##ID; DebugGlobalMemory->DebugCounters[DebugCounter_##ID].HitCount++;
#define END_TIMED_BLOCK_COUNTED(ID, count) DebugGlobalMemory->DebugCounters[DebugCounter_##ID].CycleCount += __rdtsc() - startCycleCount##ID; DebugGlobalMemory->DebugCounters[DebugCounter_##ID].HitCount += count;
#else
#define BEGIN_TIMED_BLOCK(ID)
#define END_TIMED_BLOCK(ID)
#endif
#else
#define Assert(expression)
#endif

#if HANDMADE_SLOW
#define InvalidCodePath Assert(!"Invalid code path");
#define InvalidDefaultCase default: { InvalidCodePath; } break;
#else
#define Assert(expression)
#endif


struct thread_context
{
    int Placeholder;  
};

struct debug_read_file_result
{
    void *Content;
    uint32 ContentSize;
};

struct game_offscreen_buffer
{
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel = 4;
    int MemorySize;
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

            game_button_state Terminator; // todo: check if persists
        };
    };
};


struct game_input
{
    game_button_state MouseButtons[5];
    int32 MouseX;
    int32 MouseY;
    int32 MouseZ;

    bool32 ExecutableReloaded;
    real32 DtForFrame;
    
    game_controller_input Controllers[5];
};




#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context *thread, void *memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context *thread, char *filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(thread_context *thread, char *filename, uint32 memorySize, void *memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);


struct game_memory
{
    bool32 IsInitialized;
    uint64 PermanentStorageSize;
    void *PermanentStorage; // should be initialized to zero

    uint64 TransientStorageSize;
    void *TransientStorage; // should be initialized to zero
    debug_platform_free_file_memory *DEBUG_PlatformFreeFileMemory;
    debug_platform_read_entire_file *DEBUG_PlatformReadEntireFile;
    debug_platform_write_entire_file *DEBUG_PlatformWriteEntireFile;

#if HANDMADE_SLOW
    debug_cycle_counter DebugCounters[DebugCounter_Count];
#endif
};





#define GAME_UPDATE_AND_RENDER(name) void name(thread_context *thread, game_memory *memory, game_offscreen_buffer *buffer, game_input *input)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_GET_SOUND_SAMPLES(name) void name(thread_context *thread, game_memory *memory, game_sound_output_buffer *soundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

