struct win32_game_code
{
    HMODULE HandmadeModule;
    bool32 IsValid;
    FILETIME DLLLastWriteTime;
    game_update_and_render *UpdateAndRender;
    game_get_sound_samples *GetSoundSamples;
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
    int samplesPerSecond;
    uint32 runningSampleIndex;
    int bytesPerSample;
    int secondaryBufferSize;
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

struct win32_state
{
    HANDLE RecordingFile;
    int InputRecordingIndex;

    HANDLE PlayBackFile;
    int InputPlayBackIndex;

    void *GameMemory;
    uint64 GameMemorySize;
};
