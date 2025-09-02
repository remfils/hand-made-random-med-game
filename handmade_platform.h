#if !defined(HANDMADE_PLATFORM_H)
#define HANDMADE_PLATFORM_H


#if _MSC_VER
#define COMPILER_MSVC 1
#endif

#include <stdint.h>
#include <limits.h>
#include <float.h>

#if COMPILER_MSVC
#include <windows.h>
#include <intrin.h>
#endif

#if !defined(internal)
#define internal static
#endif
#define local_persist static
#define global_variable static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef intptr_t intptr;
typedef uintptr_t uintptr;

typedef size_t memory_index;

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef float real32;
typedef double real64;

// shorten notation, TODO: remove rest
typedef int32 b32;
typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float r32;
typedef double r64;



#define Pi32 3.1415926535f
#define Real32Maximum FLT_MAX

#define Kilobytes(val) ((val)*1024LL)
#define Megabytes(val) (Kilobytes((val)*1024LL))
#define Gigabytes(val) (Megabytes((val)*1024LL))
#define Terabytes(val) (Gigabytes((val)*1024LL))
#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))

#define Align16(value) ((value + 15) & ~15)


#if HANDMADE_SLOW
#define Assert(expression) if (!(expression)) {*(int *)0 = 0;}

extern struct game_memory *DebugGlobalMemory;

#if COMPILER_MSVC
#else
#endif
#else
#define Assert(expression)
#endif

#define InvalidCodePath Assert(!"Invalid code path");
#define InvalidDefaultCase default: { InvalidCodePath; } break;

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
    // NOTE|IMPORTANT: samples must be padded to a multiple of 4
    int16 *Samples;
    int SampleCount;
    int SamplesPerSecond;
};

struct memory_arena
{
    memory_index Size;
    uint8 *Base;
    memory_index Used;

    int32 TempCount;
};

struct temporary_memory
{
    memory_arena *Arena;
    memory_index Used;
};

struct task_with_memory
{
    bool32 IsUsed;
    memory_arena Arena;
    temporary_memory TempMemory;
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

struct platform_file_handle
{
    b32 NoErrors;
    void *Platform;
};

struct platform_file_group
{
    u32 FileCount;
    void *Platform;
};

enum platform_file_type_enum
{
    PlatformFileType_Asset,
    PlatformFileType_Save,
    PlatformFileType_Count,
};

#define PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN(name) platform_file_group name(platform_file_type_enum FileType)
typedef PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN(platform_get_all_files_of_type_begin);

#define PLATFORM_GET_ALL_FILES_OF_TYPE_END(name) void name(platform_file_group *group)
typedef PLATFORM_GET_ALL_FILES_OF_TYPE_END(platform_get_all_files_of_type_end);

#define PLATFORM_OPEN_NEXT_FILE(name) platform_file_handle name(platform_file_group *group)
typedef PLATFORM_OPEN_NEXT_FILE(platform_open_next_file);

#define PLATFORM_READ_DATA_FROM_FILE(name) void name(platform_file_handle *handle, u64 offset, u64 size, void *dest)
typedef PLATFORM_READ_DATA_FROM_FILE(platform_read_data_from_file);

#define PLATFORM_FILE_ERROR(name) void name(platform_file_handle *handle, char *message)
typedef PLATFORM_FILE_ERROR(platform_file_error);

#define PLATFORM_ALLOCATE_MEMORY(name) void* name(memory_index size)
typedef PLATFORM_ALLOCATE_MEMORY(platform_allocate_memory);

#define PLATFORM_FREE_MEMORY(name) void name(void* memory)
typedef PLATFORM_FREE_MEMORY(platform_free_memory);

    
#define PlatformNoFileErrors(Handle) ((Handle)->NoErrors)


struct platform_work_queue;
typedef void platform_work_queue_callback(platform_work_queue *queue, void *data);
typedef void platform_add_entry(platform_work_queue *queue, platform_work_queue_callback *callback, void *data);
typedef void platform_complete_all_work(platform_work_queue *queue);

#define AlignPow2(value, alignment) ((value + ((alignment - 1))) & ~((alignment) -1))
#define Align4(value) ((value + 3) & ~3)
#define Align8(value) ((value + 7) & ~7)
#define Align16(value) ((value + 15) & ~15)


#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(platform_work_queue *queue, void *data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(platform_work_queue_callback);


#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void *memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(char *filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(char *filename, uint32 memorySize, void *memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);


extern platform_work_queue *RenderQueue;

struct platform_api
{
    platform_add_entry *AddEntry;
    platform_complete_all_work *CompleteAllWork;
    
    platform_get_all_files_of_type_begin *GetAllFilesOfTypeBegin;
    platform_get_all_files_of_type_end *GetAllFilesOfTypeEnd;
    platform_open_next_file *OpenNextFile;
    platform_read_data_from_file *ReadDataFromFile;
    platform_file_error *FileError;

    platform_allocate_memory *AllocateMemory;
    platform_free_memory *FreeMemory;

    debug_platform_free_file_memory *DEBUG_FreeFileMemory;
    debug_platform_read_entire_file *DEBUG_ReadEntireFile;
    debug_platform_write_entire_file *DEBUG_WriteEntireFile;
};

struct game_memory
{
    bool32 IsInitialized;
    uint64 PermanentStorageSize;
    void *PermanentStorage; // should be initialized to zero

    platform_work_queue *HighPriorityQueue;
    platform_work_queue *LowPriorityQueue;

    platform_api PlatformAPI;


    uint64 TransientStorageSize;
    void *TransientStorage; // should be initialized to zero
};





#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *memory, game_offscreen_buffer *buffer, game_input *input)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_GET_SOUND_SAMPLES(name) void name(game_memory *memory, game_sound_output_buffer *soundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);


inline uint32
SafeTruncateUInt64(uint64 value)
{
    Assert(value <= 0xFFFFFFFF);
    uint32 result = (uint32)value;
    return (result);
}

inline u16
SafeTruncateToUInt16(s32 value)
{
    Assert(value <= 65535);
    Assert(value >= 0);
    u16 result = (s16)value;
    return (result);
}

inline s16
SafeTruncateToInt16(s16 value)
{
    Assert(value <= 32767);
    Assert(value >= -32768);
    s16 result = (s16)value;
    return (result);
    
}


#endif
