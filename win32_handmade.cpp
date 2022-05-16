/*
  TODO: this is not a platform layer

  - saved game locations (save state)
  - getting a handle to our own exec file
  - asset loading path
  - threading
  - raw input (support for multiple keyboard)
  - sleep/timebeginperiod
  - clipcursor - multimonitor support
  - fullscreen
  - WM_SETCURSOR - control cursor visible
  - blit speed 
  - hardware acceleration
  - GetKeyboardLayout for international support

  just a partial list of stuff
 */
#include <stdint.h>

#include "handmade_platform.h"
#include "handmade_platform.cpp"

#include <malloc.h>
#include <windows.h>
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>

#include "win32_handmade.h"


global_variable int64 GlobalPerfCounterFrequency;


global_variable bool running;
global_variable win32_offscreen_buffer globalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER globalSecondaryBuffer;
global_variable bool DEBUGShowCursor = true;
global_variable WINDOWPLACEMENT globalWindowsPosition = { sizeof(globalWindowsPosition) };


internal void
Win32ToggleFullscreen(HWND window)
{
    // src: https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353

    DWORD style = GetWindowLong(window, GWL_STYLE);
    if (style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO mi = { sizeof(mi) };
        if (GetWindowPlacement(window, &globalWindowsPosition) &&
            GetMonitorInfo(MonitorFromWindow(window,
                    MONITOR_DEFAULTTOPRIMARY), &mi))
        {
            SetWindowLong(window, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(window, HWND_TOP,
                mi.rcMonitor.left, mi.rcMonitor.top,
                mi.rcMonitor.right - mi.rcMonitor.left,
                mi.rcMonitor.bottom - mi.rcMonitor.top,
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(window, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(window, &globalWindowsPosition);
        SetWindowPos(window, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}


internal int
StringLength(char *str)
{
    int count =0;
    while (*str++)
    {
        ++count;
    }
    return(count);
}

internal void
CatStrings(size_t sourceACount, char *sourceA,
    size_t sourceBCount, char *sourceB,
    size_t destCount, char *dest)
{
    for (int index = 0; index < sourceACount; index ++)
    {
        *dest++ = *sourceA++;
    }

    for (int index = 0; index < sourceBCount; index ++)
    {
        *dest++ = *sourceB++;
    }

    *dest++ = '\0';
}

internal void
Win32BuildAppPathFileName(win32_state *winState, char *filename, int destCount, char *dest)
{
    CatStrings(winState->ExeFullDirName - winState->ExeFullFilename, winState->ExeFullFilename, StringLength(filename), filename, destCount, dest);
}

internal void
Win32SetGameFunctionsToStubs(win32_game_code *game)
{
    game->UpdateAndRender = 0;
    game->GetSoundSamples = 0;
}

inline FILETIME
Win32GetLastWriteTime(char *filename)
{
    FILETIME lastWriteTime = {};

    WIN32_FILE_ATTRIBUTE_DATA attributes = {};
    BOOL isOk =  GetFileAttributesEx(filename, GetFileExInfoStandard, &attributes);
    if (isOk)
    {
        lastWriteTime = attributes.ftLastWriteTime;
    }

    return(lastWriteTime);
}

inline BOOL
Win32IsFileExists(char *filename)
{
    WIN32_FILE_ATTRIBUTE_DATA attributes = {};
    BOOL isOk =  GetFileAttributesEx(filename, GetFileExInfoStandard, &attributes);
    return isOk;
}

internal win32_game_code
Win32LoadGameCode(char *sourceName, char *tmpName)
{
    win32_game_code game = {};

    BOOL fileResult = CopyFileA(sourceName, tmpName, FALSE);

    game.DLLLastWriteTime = Win32GetLastWriteTime(sourceName);

    game.HandmadeModule = LoadLibrary(tmpName);
    
    if (game.HandmadeModule)
    {
        game.UpdateAndRender = (game_update_and_render *)GetProcAddress(game.HandmadeModule, "GameUpdateAndRender");
        game.GetSoundSamples = (game_get_sound_samples *)GetProcAddress(game.HandmadeModule, "GameGetSoundSamples");

        game.IsValid = (game.UpdateAndRender
            && game.GetSoundSamples);
    }

    if (!game.IsValid)
    {
        Win32SetGameFunctionsToStubs(&game);
    }

    return game;
}

internal void
Win32UnloadGameCode(win32_game_code *game)
{
    if (game->HandmadeModule)
    {
        FreeLibrary(game->HandmadeModule);
    }

    game->IsValid = false;

    Win32SetGameFunctionsToStubs(game);
}

// XInputGetState

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
internal x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// XInputSetState

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);

X_INPUT_SET_STATE(XInputSetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
internal x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void
Win32InitXInput(void)
{
    HMODULE xInputLibrary = LoadLibrary("xinput1_4.dll");
    if (!xInputLibrary)
    {
        xInputLibrary = LoadLibrary("xinput1_3.dll");
    }

    if (xInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(xInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state *)GetProcAddress(xInputLibrary, "XInputSetState");
    }
}


win32_window_dimension
Win32GetWindowDimension(HWND window)
{
     win32_window_dimension dim;

     RECT winRect;
     GetClientRect(window, &winRect);

     dim.Width = winRect.right - winRect.left;
     dim.Height = winRect.bottom - winRect.top;

     return dim;
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *buffer, int width, int height)
{
    if (buffer->Memory)
    {
        VirtualFree(buffer->Memory, 0, MEM_RELEASE);
    }

    buffer->Width = width;
    buffer->Height = height;
    buffer->BytesPerPixel = 4;
    
    buffer->Info.bmiHeader.biSize = sizeof(buffer->Info.bmiHeader);
    buffer->Info.bmiHeader.biWidth = buffer->Width;
    buffer->Info.bmiHeader.biHeight = -buffer->Height;
    buffer->Info.bmiHeader.biPlanes = 1;
    buffer->Info.bmiHeader.biBitCount = 32;
    buffer->Info.bmiHeader.biCompression = BI_RGB;

    int bitmapMemorySize = buffer->Width * buffer->Height * buffer->BytesPerPixel;
    buffer->Memory = VirtualAlloc(0, bitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    buffer->Pitch = buffer->Width * buffer->BytesPerPixel;

    // drawing stuff

    // RenderGradient(0, 0);
}

internal void
Win32DisplayBufferInWindow(HDC deviceContext, int windowWidth, int windowHeight, win32_offscreen_buffer *buffer, int x, int y, int width, int height)
{
    int offsetX = 10;
    int offsetY = 10;

    if ((windowWidth > buffer->Width * 2) &&
        (windowHeight > buffer->Height * 2))
    {
        StretchDIBits(
            deviceContext,
            0, 0, windowWidth, windowHeight,
            0, 0, buffer->Width, buffer->Height,
            buffer->Memory,
            &buffer->Info,
            DIB_RGB_COLORS, // should be this most of the time
            SRCCOPY // rastor operation code
            );   
    }
    else
    {
        PatBlt(deviceContext, 0, 0, windowWidth, offsetY, BLACKNESS);
        PatBlt(deviceContext, 0, offsetY + buffer->Height, windowWidth, windowHeight, BLACKNESS);
        PatBlt(deviceContext, 0, 0, offsetX, windowHeight, BLACKNESS);
        PatBlt(deviceContext, offsetX + buffer->Width, 0, windowWidth, windowHeight, BLACKNESS);

        StretchDIBits(
            deviceContext,
            offsetX, offsetY, buffer->Width, buffer->Height,
            0, 0, buffer->Width, buffer->Height,
            buffer->Memory,
            &buffer->Info,
            DIB_RGB_COLORS, // should be this most of the time
            SRCCOPY // rastor operation code
            );   
    }
}

#if HANDMADE_INTERNAL

DEBUG_PLATFORM_FREE_FILE_MEMORY(Debug_PlatformFreeFileMemory)
{
    if (memory)
    {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(Debug_PlatformReadEntireFile)
{
    debug_read_file_result result = {};
    
    HANDLE fileHandle = CreateFileA(
        filename,
        GENERIC_READ,
        FILE_SHARE_READ,
        0,
        OPEN_EXISTING,
        0,
        0);
    

    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER fileSize;
        if (GetFileSizeEx(fileHandle, &fileSize))
        {
            uint32 fileSize32 = SafeTruncateUInt64(fileSize.QuadPart);
                
            result.Content = VirtualAlloc(0, fileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            
            if (result.Content)
            {
                DWORD bytesRead;
                if (ReadFile(fileHandle, result.Content, fileSize32, &bytesRead, 0) && (fileSize32 == bytesRead))
                {
                    result.ContentSize = bytesRead;
                    // success
                }
                else
                {
                    Debug_PlatformFreeFileMemory(thread, result.Content);
                    result.Content = 0;
                }

            }
            else
            {
            
            }
        }
        else
        {
        
        }
    }

    CloseHandle(fileHandle);

    return result;
}


DEBUG_PLATFORM_WRITE_ENTIRE_FILE(Debug_PlatformWriteEntireFile)
{
    bool32 result = false;
    
    HANDLE fileHandle = CreateFileA(
        filename,
        GENERIC_WRITE,
        0,
        0,
        CREATE_ALWAYS,
        0,
        0);

    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten;
        if (WriteFile(fileHandle, memory, memorySize, &bytesWritten, 0))
        {
            result = memorySize == bytesWritten;
        }
        else
        {
            
        }
    }

    CloseHandle(fileHandle);

    return(result);
}
#endif


LRESULT CALLBACK
MainWindowCallback(HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    LRESULT result = 0;
    
    switch (message)
    {
    case WM_SETCURSOR:
    {
        if (DEBUGShowCursor)
        {
            SetCursor(LoadCursorA(0, IDC_CROSS));
        }
        else
        {
            SetCursor(0);
        }
    } break;
    case WM_ACTIVATEAPP:
    {
        if (wParam == 1)
        {
            SetLayeredWindowAttributes(window, RGB(0,0,0), 255, LWA_ALPHA);
        }
        else
        {
            SetLayeredWindowAttributes(window, RGB(0,0,0), 120, LWA_ALPHA);
        }
    } break;
        
    case WM_SIZE:
    {
        OutputDebugStringA("WM_SIZE\n");
    } break;
    case WM_DESTROY:
    {
        OutputDebugStringA("WM_DESTROY\n");
        running = false;
    } break;
    case WM_CLOSE:
    {
        OutputDebugStringA("WM_CLOSE\n");
        running = false;
    } break;
    case WM_PAINT:
    {
        PAINTSTRUCT paint;
        HDC deviceContext = BeginPaint(window, &paint);

        LONG width = paint.rcPaint.right - paint.rcPaint.left;
        LONG height = paint.rcPaint.bottom - paint.rcPaint.top;
        LONG x = paint.rcPaint.left;
        LONG y = paint.rcPaint.top;

        win32_window_dimension dim = Win32GetWindowDimension(window);
        Win32DisplayBufferInWindow(deviceContext, dim.Width, dim.Height, &globalBackbuffer, x, y, width, height);

        EndPaint(window, &paint);
    } break;
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        Assert(!"no code shoulde be here");
    } break;
    default:
    {
        result = DefWindowProc(window, message, wParam, lParam);
    } break;
    }

    return result;
}

// DirectSoundCreate
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void
Win32InitDirectSound(HWND window, int32 bufferSize, int32 samplesPerSecond)
{
    // load library. Game can be run without sound

    HMODULE directSoundLib = LoadLibraryA("dsound.dll");

    if (directSoundLib)
    {
        // get DirectSound object (cooperative)
 
        direct_sound_create *DirectSoundCreate = (direct_sound_create *) GetProcAddress(directSoundLib, "DirectSoundCreate");

        LPDIRECTSOUND directSound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &directSound, 0)))
        {
            WAVEFORMATEX waveFormat;
            waveFormat.wFormatTag      = WAVE_FORMAT_PCM;
            waveFormat.nChannels       = 2;
            waveFormat.nSamplesPerSec  = samplesPerSecond;
            waveFormat.wBitsPerSample  = 16;
            waveFormat.nBlockAlign     = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
            waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
            waveFormat.cbSize          = 0;
        
            if (SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
            {
                // create a primary buffer
                
                DSBUFFERDESC bufferDescription = {};
                bufferDescription.dwSize = sizeof(bufferDescription);
                bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
                
                LPDIRECTSOUNDBUFFER primaryBuffer;
                HRESULT result = directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0);
                if (SUCCEEDED(result))
                {
                    if (SUCCEEDED(primaryBuffer->SetFormat(&waveFormat)))
                    {
                        
                    }
                    else
                    {
                        
                    }
                }

                
            }
            else
            {
                
            }
            
            DSBUFFERDESC bufferDescription = {};
            bufferDescription.dwSize = sizeof(bufferDescription);
            bufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
            bufferDescription.dwBufferBytes = bufferSize;
            bufferDescription.lpwfxFormat = &waveFormat;
            HRESULT result = directSound->CreateSoundBuffer(&bufferDescription, &globalSecondaryBuffer, 0);
            if (SUCCEEDED(result))
            {
                OutputDebugStringA("this is test");
            }
        }
        else
        {
            // log
        }
    }

    
}

internal void
Win32ClearSoundBuffer(win32_sound_output *soundOutput)
{
    void *region1;
    DWORD regionSize1;
    void *region2;
    DWORD regionSize2;
    
    HRESULT result = globalSecondaryBuffer->Lock(0, soundOutput->secondaryBufferSize,
        &region1, &regionSize1, &region2, &regionSize2,
        0);

    if (SUCCEEDED(result))
    {
        uint8 *destSample = (uint8 *)region1;
        for (DWORD byteIndex = 0; byteIndex < regionSize1; ++byteIndex)
        {
            *destSample++ = 0;
        }

        destSample = (uint8 *)region2;
        for (DWORD byteIndex = 0; byteIndex < regionSize2; ++byteIndex)
        {
            *destSample++ = 0;
        } 

        globalSecondaryBuffer->Unlock(region1, regionSize1, region2, regionSize2);
    }
}

internal void
Win32FillSoundBuffer(win32_sound_output *soundOutput, DWORD byteToLock, DWORD bytesToWrite, game_sound_output_buffer *sourceBuffer)
{
    void *region1;
    DWORD regionSize1;
    void *region2;
    DWORD regionSize2;

    HRESULT result = globalSecondaryBuffer->Lock(byteToLock, bytesToWrite,
        &region1, &regionSize1, &region2, &regionSize2,
        0); 
    if (SUCCEEDED(result))
    {
        DWORD region1SampleCount = regionSize1 / soundOutput->bytesPerSample;
        int16 *sampleOut = (int16 *)region1;
        int16 *sourceSample = sourceBuffer->Samples;

        for (DWORD sampleIndex = 0;
             sampleIndex < region1SampleCount;
             ++sampleIndex)
        {
            *sampleOut++ = *sourceSample++;
            *sampleOut++ = *sourceSample++;
            ++soundOutput->runningSampleIndex;
        }

        DWORD region2SampleCount = regionSize2 / soundOutput->bytesPerSample;
        sampleOut = (int16 *)region2;
        for (DWORD sampleIndex = 0;
             sampleIndex < region2SampleCount;
             ++sampleIndex)
        {
            *sampleOut++ = *sourceSample++;
            *sampleOut++ = *sourceSample++;
            ++soundOutput->runningSampleIndex;
        }

        globalSecondaryBuffer->Unlock(region1, regionSize1, region2, regionSize2);
    }
}

internal void
Win32ProcessKeyboardButtonPress(game_button_state *newState, bool32 isDown)
{
    if (newState->EndedDown != isDown)
    {
        newState->EndedDown = isDown;
        ++newState->HalfTransitionCount;
    }
}

internal void
Win32ProcessXInputDigitalButton(DWORD xInputButtonState, game_button_state *oldState, DWORD buttonBit, game_button_state *newState)
{
    newState->EndedDown = (xInputButtonState & buttonBit) == buttonBit;
    newState ->HalfTransitionCount = (oldState->EndedDown != newState->EndedDown) ? 1 : 0;
}

internal real32
Win32ProcessXInputLeftStickValue(SHORT stickValue, SHORT stickDeadzoneThreshold)
{
    real32 value = 0;
    
    if (stickValue < -stickDeadzoneThreshold)
    {
        value = (real32)stickValue / (32768.0f - stickDeadzoneThreshold);
    }
    else if (stickValue > stickDeadzoneThreshold)
    {
        value = (real32)stickValue / (32767.0f - stickDeadzoneThreshold);
    }

    return value;
                        
}

internal void
Win32GetDumpInputFilePath(win32_state *winState, bool32 isState, int slotIndex, int destCount, char *dest)
{
    char filename[64];
    wsprintf(filename, "input-dump_%d_%s.hmi", slotIndex, isState ? "state" : "input");
    Win32BuildAppPathFileName(winState, filename, destCount, dest);
}

internal void
Win32BeginRecordingInput(win32_state *winState, int inputRecordingIndex)
{
    winState->InputRecordingIndex = inputRecordingIndex;

    char filename[MAX_PATH];
    Win32GetDumpInputFilePath(winState, false, inputRecordingIndex, MAX_PATH, filename);
    
    winState->RecordingFile = CreateFileA(
        filename,
        GENERIC_WRITE,
        0,
        0,
        CREATE_ALWAYS,
        0,
        0);
    
    Assert(inputRecordingIndex < ArrayCount(winState->ReplayBuffers));
    win32_replay_buffer replayBuffer = winState->ReplayBuffers[inputRecordingIndex];

    if (replayBuffer.MemoryBlock)
    {
        CopyMemory(replayBuffer.MemoryBlock, winState->GameMemory, winState->GameMemorySize);
        // pretend that we wrote a GameMemorySize bytes of memory to file

        // LARGE_INTEGER filePos;
        // filePos.QuadPart = winState->GameMemorySize;
        //SetFilePointerEx(winState->RecordingFile, filePos, 0, FILE_BEGIN);

        // copty memory

        

        // DWORD bytesWritten;
        // WriteFile(winState->RecordingFile, winState->GameMemory, winState->GameMemorySize, &bytesWritten, 0);
    }
}

internal void
Win32EndRecordingInput(win32_state *winState)
{
    winState->InputRecordingIndex = 0;
    CloseHandle(winState->RecordingFile);
}

internal void
Win32BeginPlayBackInput(win32_state *winState, int inputPlayBackIndex)
{
    winState->InputPlayBackIndex = inputPlayBackIndex;
    
    char filename[MAX_PATH];
    Win32GetDumpInputFilePath(winState, false, inputPlayBackIndex, MAX_PATH, filename);
    
    winState->PlayBackFile = CreateFileA(
        filename,
        GENERIC_READ,
        FILE_SHARE_READ,
        0,
        OPEN_EXISTING,
        0,
        0);
    
    Assert(inputPlayBackIndex < ArrayCount(winState->ReplayBuffers));
    win32_replay_buffer replayBuffer = winState->ReplayBuffers[inputPlayBackIndex];

    if (replayBuffer.MemoryBlock)
    {
        // LARGE_INTEGER filePos;
        // filePos.QuadPart = winState->GameMemorySize;
        // SetFilePointerEx(winState->RecordingFile, filePos, 0, FILE_BEGIN);

        CopyMemory(winState->GameMemory, replayBuffer.MemoryBlock, winState->GameMemorySize);

        // DWORD bytesRead;
        // ReadFile(winState->PlayBackFile, winState->GameMemory, winState->GameMemorySize, &bytesRead, 0);
    }
}

internal void
Win32EndPlayBackInput(win32_state *winState)
{
    winState->InputPlayBackIndex = 0;
    CloseHandle(winState->PlayBackFile);
}

internal void
Win32RecordUserInput(win32_state *winState, game_input *newInput)
{
    DWORD bytesWritten;
    WriteFile(winState->RecordingFile, newInput, sizeof(*newInput), &bytesWritten, 0);
}

internal void
Win32PlayBackUserInput(win32_state *winState, game_input *newInput)
{
    DWORD bytesRead;
    if (ReadFile(winState->PlayBackFile, newInput, sizeof(*newInput), &bytesRead, 0) && !bytesRead)
    {
        int playingIndex = winState->InputPlayBackIndex;
        Win32EndPlayBackInput(winState);
        Win32BeginPlayBackInput(winState, playingIndex);
    }
}

internal void
Win32ProcessWindowMessages(win32_state *winState, game_controller_input *keyboardController)
{
    MSG message;
    while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
    {
        if (message.message == WM_QUIT)
        {
            running = false;
        }

        switch(message.message)
        {
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint32 virtualKeyCode = (uint32)message.wParam;
            bool wasDown = ((message.lParam & (1 << 30)) != 0);
            bool isDown ((message.lParam & (1 << 31)) == 0);

            if (isDown != wasDown)
            {
                if (virtualKeyCode == 'W')
                {
                    Win32ProcessKeyboardButtonPress(&keyboardController->MoveUp, isDown);
                }
                else if (virtualKeyCode == 'A')
                {
                    Win32ProcessKeyboardButtonPress(&keyboardController->MoveLeft, isDown);
                }
                else if (virtualKeyCode == 'S')
                {
                    Win32ProcessKeyboardButtonPress(&keyboardController->MoveDown, isDown);
                }
                else if (virtualKeyCode == 'D')
                {
                    Win32ProcessKeyboardButtonPress(&keyboardController->MoveRight, isDown);
                }
                else if (virtualKeyCode == 'Q')
                {
                    Win32ProcessKeyboardButtonPress(&keyboardController->LeftShoulder, isDown);
                }
                else if (virtualKeyCode == 'E')
                {
                    Win32ProcessKeyboardButtonPress(&keyboardController->RightShoulder, isDown);
                }
                else if (virtualKeyCode == VK_UP)
                {
                    Win32ProcessKeyboardButtonPress(&keyboardController->ActionUp, isDown);
                }
                else if (virtualKeyCode == VK_DOWN)
                {
                    Win32ProcessKeyboardButtonPress(&keyboardController->ActionDown, isDown);
                }
                else if (virtualKeyCode == VK_RIGHT)
                {
                    Win32ProcessKeyboardButtonPress(&keyboardController->ActionRight, isDown);
                }
                else if (virtualKeyCode == VK_LEFT)
                {
                    Win32ProcessKeyboardButtonPress(&keyboardController->ActionLeft, isDown);
                }
                else if (virtualKeyCode == VK_ESCAPE)
                {
                    Win32ProcessKeyboardButtonPress(&keyboardController->Back, isDown);
                }
                else if (virtualKeyCode == VK_SPACE)
                {
                    Win32ProcessKeyboardButtonPress(&keyboardController->Start, isDown);
                }
                else if (virtualKeyCode == VK_F4)
                {
                    bool32 isAlt = (message.lParam & (1 << 29));
                    if (isAlt)
                    {
                        running = false;
                    }
                }
                else if (virtualKeyCode == 'L')
                {
                    if (isDown)
                    {
                        if (winState->InputRecordingIndex == 0)
                        {
                            Win32BeginRecordingInput(winState, 1);
                        }
                        else
                        {
                            Win32EndRecordingInput(winState);
                        }
                    }
                    
                }
                else if (virtualKeyCode == 'P')
                {
                    
                    if (isDown)
                    {
                        if (winState->InputPlayBackIndex == 0)
                        {
                            Win32BeginPlayBackInput(winState, 1);
                        }
                        else
                        {
                            Win32EndPlayBackInput(winState);
                        }
                    }
                }
                else if (virtualKeyCode == VK_RETURN)
                {
                    if (isDown)
                    {
                        bool32 isAlt = (message.lParam & (1 << 29));
                        if (isAlt)
                        {
                            if (message.hwnd) {
                                Win32ToggleFullscreen(message.hwnd);
                            }
                        }
                    }
                }
            }
        
        } break;
        default:
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
                   
    }
}

inline LARGE_INTEGER
Win32GetWallClock(void)
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return(result);
}

inline real32
Win32GetSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end)
{
    real32 secondsElapsed = (real32)(end.QuadPart - start.QuadPart) / (real32)GlobalPerfCounterFrequency;
    return(secondsElapsed);
}

internal void
Win32DebugDrawVertical(win32_offscreen_buffer *backbuffer, int x, int top, int bottom, uint32 color)
{
    uint8 *pixel = (uint8 *)backbuffer->Memory
            + x * backbuffer->BytesPerPixel
            + top * backbuffer->Pitch;
    
    for (int y = top;
         y < bottom;
       ++y)
    {
        *(uint32 *)pixel = color;
        pixel += backbuffer->Pitch;
    }
}

internal void
Win32DebugSyncDisplay(win32_offscreen_buffer *backbuffer, int soundCursorsCount, win32_debug_sound *soundCursors, win32_sound_output *soundOutput, real32 targetSecondsPerFrame)
{
    int padX = 16;
    int padY = 16;
    
    int top = padY;
    int bottom = backbuffer->Height - padY;
    
    real32 c = (real32)(backbuffer->Width - 2 * padX) / (real32)soundOutput->secondaryBufferSize;
    for (int playIndex=0;
         playIndex < soundCursorsCount;
        ++playIndex)
    {
        win32_debug_sound cur = soundCursors[playIndex];
        int pCur = cur.FlipPlayCursor;
        int x = padX + (int)(c * (real32)pCur);

        Win32DebugDrawVertical(backbuffer, x, top, bottom, 0xffffffff);

        int wCur = cur.FlipWriteCursor;
        x = padX + (int)(c * (real32)wCur);

        Win32DebugDrawVertical(backbuffer, x, top, bottom, 0xfff0000);
    }
}

internal void
Win32SetEXERootPath(win32_state *winState)
{
    DWORD sizeOfFilename = GetModuleFileNameA(0, winState->ExeFullFilename, MAX_PATH);
    winState->ExeFullDirName = winState->ExeFullFilename;
    for (char *scan = winState->ExeFullFilename; *scan; ++scan)
    {
        if (*scan == '\\')
        {
            winState->ExeFullDirName = scan + 1;
        }
    }
}

int CALLBACK WinMain(
    HINSTANCE instance,
    HINSTANCE prevInstance,
    LPSTR     commandLine,
    int       showCode)
{
    UINT desiredSchedulerMS = 1;
    bool32 sleepIsGranural = timeBeginPeriod(desiredSchedulerMS) == TIMERR_NOERROR;
    
    Win32InitXInput();
    
    WNDCLASSA windowClass = {};
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = MainWindowCallback;
    windowClass.hInstance = instance;
    //windowClass.hIcon = ??;
    windowClass.lpszClassName = "HandmadeHeroWindowClass";

    Win32ResizeDIBSection(&globalBackbuffer, 1088, 576);

    LARGE_INTEGER perfCounterFrequencyResult;
    QueryPerformanceFrequency(&perfCounterFrequencyResult);
    GlobalPerfCounterFrequency = perfCounterFrequencyResult.QuadPart;

    int monitorRefreshHz = 60;

    if (RegisterClass(&windowClass))
    {
        HWND windowHandle = CreateWindowExA(
            0,//WS_EX_TOPMOST | WS_EX_LAYERED,
            windowClass.lpszClassName,
            "Handmade window",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            50,
            50,
            1920/2 + 170,
            700 - 50,
            0, // parent window
            0, // menu
            instance,
            0);

        if (windowHandle)
        {

            thread_context thread = {};
            
            HDC refreshDC = GetDC(windowHandle);
            int win32RefreshRate = GetDeviceCaps(refreshDC, VREFRESH);
            if (win32RefreshRate > 1)
            {
                monitorRefreshHz = win32RefreshRate;
            }
    
            int gameUpdateHz = monitorRefreshHz / 1;
            real32 targetSecondsPerFrame = (real32)1.0f / (real32) gameUpdateHz;

    
            
            int xOffset = 0;

            // sound test

            bool soundIsPlaying = false;
            
            win32_sound_output soundOutput = {};

            soundOutput.samplesPerSecond = 48000;
            soundOutput.runningSampleIndex = 0;
            soundOutput.bytesPerSample = sizeof(int16) * 2;
            soundOutput.SafetyBytes = (soundOutput.samplesPerSecond * soundOutput.bytesPerSample / gameUpdateHz) / 3;
            soundOutput.LatencySampleCount = 4 * (soundOutput.samplesPerSecond / gameUpdateHz);
            soundOutput.secondaryBufferSize = soundOutput.samplesPerSecond * soundOutput.bytesPerSample;
            Win32InitDirectSound(windowHandle, soundOutput.secondaryBufferSize, soundOutput.samplesPerSecond);
            Win32ClearSoundBuffer(&soundOutput);
            globalSecondaryBuffer->Play(0,0,DSBPLAY_LOOPING);

            game_input inputs[2] = {};
            game_input *newInput = &inputs[0];
            game_input *oldInput = &inputs[1];

            LARGE_INTEGER lastCounter;
            QueryPerformanceCounter(&lastCounter);
            uint64 lastCycleCount =  __rdtsc();

            LARGE_INTEGER flipWallClock = lastCounter;

            int debugSoundCursorsIndex = 0;
            win32_debug_sound debugSoundCursors[30] = {0};

            int16 *samples = (int16 *)VirtualAlloc(0, soundOutput.secondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

            game_memory  gameMemory = {};
            gameMemory.PermanentStorageSize = Megabytes(64);
            gameMemory.TransientStorageSize = Gigabytes((uint64)1);

            // platform specific functions
#if HANDMADE_INTERNAL
            gameMemory.DEBUG_PlatformFreeFileMemory = Debug_PlatformFreeFileMemory;
            gameMemory.DEBUG_PlatformReadEntireFile = Debug_PlatformReadEntireFile;
            gameMemory.DEBUG_PlatformWriteEntireFile = Debug_PlatformWriteEntireFile;
#endif
            

            #if HANDMADE_INTERNAL
            LPVOID baseAddress = (LPVOID)Terabytes((uint64)2);
            #else
            LPVOID baseAddress = 0;
            #endif

            win32_state winState = {};
            winState.InputRecordingIndex = 0;
            winState.InputPlayBackIndex = 0;

            Win32SetEXERootPath(&winState);

            winState.GameMemorySize = gameMemory.PermanentStorageSize + gameMemory.TransientStorageSize;
            winState.GameMemory = (void *)VirtualAlloc(baseAddress, winState.GameMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            
            gameMemory.PermanentStorage = winState.GameMemory;
            gameMemory.TransientStorage = (uint8 *)gameMemory.PermanentStorage + gameMemory.PermanentStorageSize;

            for (int replayIndex=0; replayIndex < ArrayCount(winState.ReplayBuffers); ++replayIndex)
            {
                win32_replay_buffer replayBuffer = winState.ReplayBuffers[replayIndex];

                Win32GetDumpInputFilePath(&winState, true, replayIndex, sizeof(replayBuffer.Filename), replayBuffer.Filename);

                replayBuffer.FileHandle =
                    CreateFileA(replayBuffer.Filename, GENERIC_WRITE|GENERIC_READ, 0,0, CREATE_ALWAYS, 0,0);

                LARGE_INTEGER maxSize;
                maxSize.QuadPart = winState.GameMemorySize;

                replayBuffer.MemoryMap = CreateFileMapping(replayBuffer.FileHandle, 0, PAGE_READWRITE, maxSize.HighPart, maxSize.LowPart, 0);

                replayBuffer.MemoryBlock = MapViewOfFile(replayBuffer.MemoryMap, FILE_MAP_ALL_ACCESS, 0,0, winState.GameMemorySize);
                
                // replayBuffer.MemoryBlock = (void *)VirtualAlloc(0, winState.GameMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

                if (!winState.ReplayBuffers[replayIndex].MemoryBlock)
                {
                    // TODO: log
                }
            }



            DWORD audioLatencyBytes = 0;
            real32 audioLatencySeconds = 0;
            bool32 soundIsValid = false;

            // dll files
            char sourceDLLFullPath[MAX_PATH];
            Win32BuildAppPathFileName(&winState, "handmade.dll", MAX_PATH, sourceDLLFullPath);
    
            char tmpDLLFullPath[MAX_PATH];
            Win32BuildAppPathFileName(&winState, "handmade_tmp.dll", MAX_PATH, tmpDLLFullPath);

            char lockFullFilePath[MAX_PATH];
            Win32BuildAppPathFileName(&winState, "tmp.lock", MAX_PATH, lockFullFilePath);

            win32_game_code game = Win32LoadGameCode(sourceDLLFullPath, tmpDLLFullPath);



            {
                HDC deviceContext = GetDC(windowHandle);
                win32_window_dimension dim = Win32GetWindowDimension(windowHandle);
                PatBlt(deviceContext, 0, 0, dim.Width, dim.Height, BLACKNESS);
            }

            running = true;
            while(running)
            {
                newInput->DtForFrame = targetSecondsPerFrame;
                
                // check for dll reload
                FILETIME newDLLWriteTime = Win32GetLastWriteTime(sourceDLLFullPath);
                if (CompareFileTime(&newDLLWriteTime, &game.DLLLastWriteTime) != 0)
                {
                    if (!Win32IsFileExists(lockFullFilePath)) {
                        Win32UnloadGameCode(&game);
                        game = Win32LoadGameCode(sourceDLLFullPath, tmpDLLFullPath);
                    }
                }

                // mouse

                POINT mouseP;
                GetCursorPos(&mouseP);
                ScreenToClient(windowHandle, &mouseP);

                newInput->MouseX = mouseP.x;
                newInput->MouseY = mouseP.y;
                newInput->MouseZ = 0;

                Win32ProcessKeyboardButtonPress(&newInput->MouseButtons[0], GetKeyState(VK_LBUTTON) & (1 << 15));
                Win32ProcessKeyboardButtonPress(&newInput->MouseButtons[1], GetKeyState(VK_MBUTTON) & (1 << 15));
                Win32ProcessKeyboardButtonPress(&newInput->MouseButtons[2], GetKeyState(VK_RBUTTON) & (1 << 15));

                // controllers
                
                game_controller_input *oldKeyboardController = &oldInput->Controllers[0];
                game_controller_input *keyboardController = &newInput->Controllers[0];
                *keyboardController = {};

                keyboardController->IsConnected = true;

                for (int buttonIndex = 0; buttonIndex < ArrayCount(keyboardController->Buttons); ++buttonIndex)
                {
                    keyboardController->Buttons[buttonIndex].EndedDown = oldKeyboardController->Buttons[buttonIndex].EndedDown;
                }

                Win32ProcessWindowMessages(&winState, keyboardController);

                // should pull this more frequently?

                DWORD maxControllerCount = XUSER_MAX_COUNT;
                if (maxControllerCount > ArrayCount(oldInput->Controllers))
                {
                    maxControllerCount = ArrayCount(oldInput->Controllers);
                }

                int gamePadControllerIndex = 1;
                for (DWORD controllerIndex=0; controllerIndex < maxControllerCount; controllerIndex++)
                {
                    game_controller_input *oldController = &oldInput->Controllers[gamePadControllerIndex];
                    game_controller_input *newController = &newInput->Controllers[gamePadControllerIndex];
                    XINPUT_STATE controllerState;
                    if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
                    {
                        // controller is ok
                        newController->IsConnected = true;
                        newController->IsAnalog = oldController->IsAnalog;

                        // look at controllerState.DwPacketNumber

                        XINPUT_GAMEPAD *pad = &controllerState.Gamepad;

                        newController->IsAnalog = true ;

                        newController->StickAverageX = Win32ProcessXInputLeftStickValue(pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                        newController->StickAverageY = Win32ProcessXInputLeftStickValue(pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

                        // process dpad as if it is analog input

                        if (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
                        {
                            newController->StickAverageY = 1.0f;
                            newController->IsAnalog = false;
                        }
                        if (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
                        {
                            newController->StickAverageY = -1.0f;
                            newController->IsAnalog = false;
                        }
                        if (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
                        {
                            newController->StickAverageX = -1.0f;
                            newController->IsAnalog = false;
                        }
                        if (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
                        {
                            newController->StickAverageX = 1.0f;
                            newController->IsAnalog = false;
                        }
                        

                        // process analog input as if it was a dpad

                        real32 inputThreshold = 0.5f;
                        
                        Win32ProcessXInputDigitalButton(
                            newController->StickAverageX < -inputThreshold ? 1 : 0,
                            &oldController->MoveLeft, 1,
                            &newController->MoveLeft);
                        
                        Win32ProcessXInputDigitalButton(
                            newController->StickAverageX > inputThreshold ? 1 : 0,
                            &oldController->MoveRight, 1,
                            &newController->MoveRight);

                        Win32ProcessXInputDigitalButton(
                            newController->StickAverageY < -inputThreshold ? 1 : 0,
                            &oldController->MoveDown, 1,
                            &newController->MoveDown);

                        Win32ProcessXInputDigitalButton(
                            newController->StickAverageY > inputThreshold ? 1 : 0,
                            &oldController->MoveUp, 1,
                            &newController->MoveUp);

                        Win32ProcessXInputDigitalButton(pad->wButtons,
                            &oldController->ActionDown, XINPUT_GAMEPAD_A,
                            &newController->ActionDown);

                        Win32ProcessXInputDigitalButton(pad->wButtons,
                            &oldController->ActionRight, XINPUT_GAMEPAD_B,
                            &newController->ActionRight);
                        
                        Win32ProcessXInputDigitalButton(pad->wButtons,
                            &oldController->ActionLeft, XINPUT_GAMEPAD_X,
                            &newController->ActionLeft);
                        
                        Win32ProcessXInputDigitalButton(pad->wButtons,
                            &oldController->ActionUp, XINPUT_GAMEPAD_Y,
                            &newController->ActionUp);
                        
                        Win32ProcessXInputDigitalButton(pad->wButtons,
                            &oldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
                            &newController->LeftShoulder);
                        
                        Win32ProcessXInputDigitalButton(pad->wButtons,
                            &oldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                            &newController->RightShoulder);

                        Win32ProcessXInputDigitalButton(pad->wButtons,
                            &oldController->Start, XINPUT_GAMEPAD_START,
                            &newController->Start);

                        Win32ProcessXInputDigitalButton(pad->wButtons,
                            &oldController->Back, XINPUT_GAMEPAD_BACK,
                            &newController->Back);
                    }
                    else
                    {
                        newController->IsConnected = false;
                    }

                    gamePadControllerIndex++;
                }

                game_offscreen_buffer buf = {};
                buf.Memory = globalBackbuffer.Memory;
                buf.Width = globalBackbuffer.Width;
                buf.Height = globalBackbuffer.Height;
                buf.Pitch = globalBackbuffer.Pitch;
                buf.BytesPerPixel = globalBackbuffer.BytesPerPixel;

                if (winState.InputRecordingIndex != 0)
                {
                    Win32RecordUserInput(&winState, newInput);
                }

                if (winState.InputPlayBackIndex != 0)
                {
                    Win32PlayBackUserInput(&winState, newInput);
                }

                if (game.UpdateAndRender)
                {
                    game.UpdateAndRender(&thread, &gameMemory, &buf, newInput);
                }


                // send vibrations

                // XINPUT_VIBRATION vibration;
                // vibration.wLeftMotorSpeed = 569000;
                // vibration.wRightMotorSpeed = 569000;
                // XInputSetState(0, &vibration);

                LARGE_INTEGER audioWallClock = Win32GetWallClock();
                real32 fromBeginToAudioSeconds = Win32GetSecondsElapsed(flipWallClock, audioWallClock);

                DWORD playCursor;
                DWORD writeCursor;
                if (globalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor) == DS_OK)
                {
                    /*
                      here is how sound output works:
                  
                      when wake up to write audio

                      1) look and see what the play cursor position is

                      2) forecast where play cursor will be on the next
                      frame boundary (FB)

                      3.1) if write cursor is before FB: write up to next
                      FB from WC and then one frame furher (perfect audio
                      sync with low latency card)

                      3.2) else if WC after next FB write one frames
                      worth of audio plus some samples
                    */

                
                    if (!soundIsValid)
                    {
                        soundOutput.runningSampleIndex = writeCursor / soundOutput.bytesPerSample;
                        soundIsValid = true;
                    }
                    DWORD byteToLock = 0;
                    DWORD targetCursor = 0;
                    DWORD bytesToWrite = 0;
                
                    byteToLock = ((soundOutput.runningSampleIndex * soundOutput.bytesPerSample) % soundOutput.secondaryBufferSize);
                    DWORD expectedBytesPerFrame = (soundOutput.samplesPerSecond *soundOutput.bytesPerSample) / gameUpdateHz;
                    real32 secondsLeftUntilFlip = (targetSecondsPerFrame - fromBeginToAudioSeconds);
                    DWORD expectedBytesUntilFlip = (DWORD)((secondsLeftUntilFlip / targetSecondsPerFrame) * (real32)expectedBytesPerFrame);
                    
                    DWORD expectedFrameBoundaryByte = playCursor + expectedBytesUntilFlip;
                    DWORD safeWriteCursor = writeCursor;
                    if (safeWriteCursor < playCursor)
                    {
                        safeWriteCursor += soundOutput.secondaryBufferSize;
                    }
                    safeWriteCursor += soundOutput.SafetyBytes;
                    
                    bool32 audioCardIsLowLatency = (safeWriteCursor < expectedBytesPerFrame);

                    if (audioCardIsLowLatency)
                    {
                        targetCursor = ((expectedFrameBoundaryByte + expectedBytesPerFrame)
                            % soundOutput.secondaryBufferSize);
                    }
                    else
                    {
                        targetCursor = ((writeCursor + expectedBytesPerFrame + soundOutput.SafetyBytes)
                            % soundOutput.secondaryBufferSize);
                    }
                    
                    if (byteToLock > targetCursor)
                    {
                        bytesToWrite = (soundOutput.secondaryBufferSize - byteToLock);
                        bytesToWrite += targetCursor;
                    }
                    else
                    {
                        bytesToWrite = targetCursor - byteToLock;
                    }

                    game_sound_output_buffer soundBuffer = {};
                    soundBuffer.SamplesPerSecond = soundOutput.samplesPerSecond;
                    soundBuffer.SampleCount = bytesToWrite / soundOutput.bytesPerSample;
                    soundBuffer.Samples = samples;

                    if (game.GetSoundSamples)
                    {
                        game.GetSoundSamples(&thread, &gameMemory, &soundBuffer);
                    }

#if HANDMADE_INTERNAL
                    DWORD debugPlayCursor;
                    DWORD debugWriteCursor;
                        
                    if (globalSecondaryBuffer->GetCurrentPosition(&debugPlayCursor, &debugWriteCursor) == DS_OK)
                    {
                        DWORD unwrappedWriteCursor = debugWriteCursor;
                        if (unwrappedWriteCursor < debugPlayCursor)
                        {
                            unwrappedWriteCursor += soundOutput.secondaryBufferSize;
                        }
                        audioLatencyBytes = unwrappedWriteCursor - debugPlayCursor;
                        audioLatencySeconds = (audioLatencyBytes / (real32)soundOutput.bytesPerSample) / (real32)soundOutput.samplesPerSecond;
                    }
                    
                    // char buffer[256];
                    // sprintf_s(buffer, 256, "LPC: %u, TC:%u, AuLat:%f\n", 0, targetCursor, audioLatencySeconds);
                    // OutputDebugStringA(buffer);
#endif

                    Win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite, &soundBuffer);
                }
                else
                {
                    soundIsValid = false;
                }

                // direct sound output test

                if (!soundIsPlaying)
                {
                    globalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
                    soundIsPlaying = true;
                }

                // count cycles
                
                int64 endCycleCount = __rdtsc();
                int64 cyclesElapsed = endCycleCount - lastCycleCount;

                
                LARGE_INTEGER workCounter = Win32GetWallClock();
                real32 workSecondsElapsed = Win32GetSecondsElapsed(lastCounter, workCounter);
                
                real32 secondsElapsedForFrame = workSecondsElapsed;

                if (secondsElapsedForFrame < targetSecondsPerFrame)
                {
                    LARGE_INTEGER sleepStartTime = Win32GetWallClock();
                    
                    if (sleepIsGranural)
                    {
                        DWORD sleepMs = (DWORD)((targetSecondsPerFrame - secondsElapsedForFrame) * (real32)1000.0f);
                        Sleep(sleepMs);
                    }

                    real32 testSecondsElapsedForFrame = Win32GetSecondsElapsed(lastCounter, Win32GetWallClock());

                    if (testSecondsElapsedForFrame < targetSecondsPerFrame)
                    {
                        // log missed sleep
                    }

                    while(secondsElapsedForFrame < targetSecondsPerFrame)
                    {
                        secondsElapsedForFrame = Win32GetSecondsElapsed(lastCounter, Win32GetWallClock());
                    }
                }
                else
                {
                    int64 DEBUG_VS_CRAP = 0;
                }

                LARGE_INTEGER endCounter = Win32GetWallClock();
                real32 msPerFrame = 1000.0f * Win32GetSecondsElapsed(lastCounter, endCounter);
                lastCounter = endCounter;

                HDC deviceContext = GetDC(windowHandle);
                win32_window_dimension dim = Win32GetWindowDimension(windowHandle);
#if HANDMADE_INTERNAL
                if (soundIsValid)
                {
                    Assert(debugSoundCursorsIndex < ArrayCount(debugSoundCursors));
                    
                    win32_debug_sound *cur = &debugSoundCursors[debugSoundCursorsIndex];
                    cur->FlipPlayCursor = playCursor;
                    cur->FlipWriteCursor = writeCursor;
                }

                debugSoundCursorsIndex++;
                if (debugSoundCursorsIndex == ArrayCount(debugSoundCursors))
                {
                    debugSoundCursorsIndex = 0;
                }
#endif

                flipWallClock = Win32GetWallClock();


#if HANDMADE_INTERNAL
                //Win32DebugSyncDisplay(&globalBackbuffer, ArrayCount(debugSoundCursors), debugSoundCursors, &soundOutput, targetSecondsPerFrame);
#endif

                Win32DisplayBufferInWindow(deviceContext, dim.Width, dim.Height, &globalBackbuffer, 0, 0, dim.Width, dim.Height);

                ReleaseDC(windowHandle, deviceContext);
                
                #if 1

                float FPS = (float)GlobalPerfCounterFrequency / (float)cyclesElapsed;
                real64 MCPF = ((real64)cyclesElapsed / (1000.0f * 1000.0f)); 

                char buffer[256];
                sprintf_s(buffer, "/%fms // FPS: %f // %fmc/f\n", msPerFrame, 1/msPerFrame*1000.0f, MCPF);
                OutputDebugStringA(buffer);
                #endif

                game_input *tmpInput = newInput;
                newInput = oldInput;
                oldInput = tmpInput;
                
            }
        }
        else
        {
            
        }
    }
    else
    {
        
    }
    
    return(0);
}
