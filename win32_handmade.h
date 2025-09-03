#if !defined(WIN32_HANDMADE_H)
#define WIN32_HANDMADE_H


struct win32_game_code
{
    HMODULE HandmadeModule;
    bool32 IsValid;
    FILETIME DLLLastWriteTime;
    game_update_and_render *UpdateAndRender;
    game_get_sound_samples *GetSoundSamples;
    game_frame_end *FrameEnd;
};

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel = 4;
};

struct win32_window_dimension
{
    int Width;
    int Height;
};

struct win32_sound_output
{
    int SamplesPerSecond;
    uint32 RunningSampleIndex;
    int BytesPerSample;
    int SecondaryBufferSize;
    int LatencySampleCount;
    DWORD SafetyBytes;
};

struct win32_debug_sound
{
    DWORD OutputPlayCursor;
    DWORD OutputWriteCursor;

    DWORD OutputLocation;
    DWORD OutputByteCount;
    
    DWORD FlipPlayCursor;
    DWORD FlipWriteCursor;
};

struct win32_replay_buffer
{
    HANDLE FileHandle;
    char Filename[MAX_PATH];
    void *MemoryBlock;
    HANDLE MemoryMap;
};

struct win32_state
{
    HANDLE RecordingFile;
    int InputRecordingIndex;

    HANDLE PlayBackFile;
    int InputPlayBackIndex;

    char ExeFullFilename[MAX_PATH];
    char *ExeFullDirName;

    void *GameMemory;
    uint64 GameMemorySize;
    win32_replay_buffer ReplayBuffers[4];
};

#endif
