/*
  HANDMADE_INTERNAL:
  	0 - build for public release
        1 - build for developer ony

  HANDMADE_SLOW:
  	0 - no slow code is allowed
        1 - can be less performant
 */


#include <stdint.h>
// todo: remove
#include <math.h>

#define internal static
#define local_persist static
#define global_variable static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef float real32;
typedef double real64;

#define Pi32 3.1415926535f

#if HANDMADE_SLOW
#define Assert(expression) if (!(expression)) {*(int *)0 = 0;}
#else
#define Assert(expression)
#endif


#define Kilobytes(val) ((val)*1024LL)
#define Megabytes(val) (Kilobytes((val)*1024LL))
#define Gigabytes(val) (Megabytes((val)*1024LL))
#define Terabytes(val) (Gigabytes((val)*1024LL))
#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))


uint32
SafeTruncateUInt64(uint64 value)
{
    Assert(value <= 0xFFFFFFFF);
    uint32 result = (uint32)value;
    return (result);
}

struct thread_context
{
    int Placeholder;  
};

struct color
{
    real32 Red;
    real32 Green;
    real32 Blue;
};

#if HANDMADE_INTERNAL
struct debug_read_file_result
{
    void *Content;
    uint32 ContentSize;
};

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context *thread, void *memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context *thread, char *filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(thread_context *thread, char *filename, uint32 memorySize, void *memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);


/* void Debug_PlatformFreeFileMemory(void *memory); */
/* debug_read_file_result Debug_PlatformReadEntireFile(char *filename); */
/* bool32 Debug_PlatformWriteEntireFile(char *filename, uint32 memorySize, void *memory);  */
#endif

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

    real32 DtForFrame;
    
    game_controller_input Controllers[5];
};

inline game_controller_input *GetController(game_input *input, int controllerIndex)
{
    Assert(controllerIndex < ArrayCount(input->Controllers));
    return &input->Controllers[controllerIndex];
}

struct tile_chunk
{
    
    uint32 *Tiles;
};

struct tile_chunk_position
{
    uint32 TileChunkX;
    uint32 TileChunkY;

    uint32 RelTileX;
    uint32 RelTileY;
};

struct world_position
{
    uint32 AbsTileX;
    uint32 AbsTileY;

    // this is tile relative X and Y
    real32 RelX;
    real32 RelY;
};


struct world
{
    uint32 ChunkShift;
    uint32 ChunkMask;
    uint32 ChunkDim;
    
    real32 TileSideInMeters;
    int32 TileSideInPixels;
    real32 MetersToPixels;
    
    real32 LowerLeftX;
    real32 LowerLeftY;
        
    uint32 TileMapCountX;
    uint32 TileMapCountY;

    tile_chunk *TileChunk;
};

struct game_state
{
    int ToneHz;
    int XOffset;
    int YOffset;
    real32 TSine;

    world_position PlayerPosition;
};

struct game_memory
{
    bool32 IsInitialized;
    uint64 PermanentStorageSize;
    void *PermanentStorage; // should be initialized to zero

    uint64 TransientStorageSize;
    void *TransientStorage; // should be initialized to zero

#if HANDMADE_INTERNAL
    debug_platform_free_file_memory *DEBUG_PlatformFreeFileMemory;
    debug_platform_read_entire_file *DEBUG_PlatformReadEntireFile;
    debug_platform_write_entire_file *DEBUG_PlatformWriteEntireFile;
#endif
};



#define GAME_UPDATE_AND_RENDER(name) void name(thread_context *thread, game_memory *memory, game_offscreen_buffer *buffer, game_input *input)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_GET_SOUND_SAMPLES(name) void name(thread_context *thread, game_memory *memory, game_sound_output_buffer *soundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

/* void GameUpdateAndRender(game_memory *memory, game_offscreen_buffer *buffer, game_input *input); */

/* void GameGetSoundSamples(game_memory *memory, game_sound_output_buffer *soundBuffer); */
