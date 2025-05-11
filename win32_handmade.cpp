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

#include <windows.h>
#include <malloc.h>
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>

#include "handmade_platform.h"
#include "handmade_platform.cpp"

#include "win32_handmade.h"
#include "remfils_tracker.cpp"



global_variable bool32 GlobalRunning;
global_variable bool reload_dlls;
global_variable win32_offscreen_buffer globalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER globalSecondaryBuffer;
global_variable bool DEBUGShowCursor = true;
global_variable WINDOWPLACEMENT globalWindowsPosition = { sizeof(globalWindowsPosition) };
global_variable int64 GlobalPerfCounterFrequency;

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


inline win32_window_dimension
Win32GetWindowDimension(HWND window)
{
     win32_window_dimension dim;

     RECT winRect;
     GetClientRect(window, &winRect);

     dim.Width = winRect.right - winRect.left;
     dim.Height = winRect.bottom - winRect.top;

     return dim;
}


// TODO: @comment
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
    buffer->Info.bmiHeader.biHeight = buffer->Height;
    buffer->Info.bmiHeader.biPlanes = 1;
    buffer->Info.bmiHeader.biBitCount = 32;
    buffer->Info.bmiHeader.biCompression = BI_RGB;

    int bitmapMemorySize = buffer->Width * buffer->Height * buffer->BytesPerPixel;
    buffer->Memory = VirtualAlloc(0, bitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    buffer->Pitch = Align16(buffer->Width * buffer->BytesPerPixel);

    // drawing stuff

    // RenderGradient(0, 0);
}

internal void
Win32DisplayBufferInWindow(HDC deviceContext, int windowWidth, int windowHeight, win32_offscreen_buffer *buffer, int x, int y, int width, int height)
{
    int offsetX = 0;
    int offsetY = 0;

    if ((windowWidth > buffer->Width * 1.2) &&
        (windowHeight > buffer->Height * 1.2))
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
                    Debug_PlatformFreeFileMemory(result.Content);
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
        GlobalRunning = false;
    } break;
    case WM_CLOSE:
    {
        OutputDebugStringA("WM_CLOSE\n");
        GlobalRunning = false;
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
            // NOTE: DSBCAPS_GLOBALFOCUS - option to play sound in background even after window is collapsed
            bufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2|DSBCAPS_GLOBALFOCUS;
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
Win32ClearSoundBuffer(win32_sound_output *SoundOutput)
{
    void *region1;
    DWORD regionSize1;
    void *region2;
    DWORD regionSize2;
    
    HRESULT result = globalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize,
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
Win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD byteToLock, DWORD bytesToWrite, game_sound_output_buffer *sourceBuffer)
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
        DWORD region1SampleCount = regionSize1 / SoundOutput->BytesPerSample;
        int16 *sampleOut = (int16 *)region1;
        int16 *sourceSample = sourceBuffer->Samples;

        for (DWORD sampleIndex = 0;
             sampleIndex < region1SampleCount;
             ++sampleIndex)
        {
            *sampleOut++ = *sourceSample++;
            *sampleOut++ = *sourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }

        DWORD region2SampleCount = regionSize2 / SoundOutput->BytesPerSample;
        sampleOut = (int16 *)region2;
        for (DWORD sampleIndex = 0;
             sampleIndex < region2SampleCount;
             ++sampleIndex)
        {
            *sampleOut++ = *sourceSample++;
            *sampleOut++ = *sourceSample++;
            ++SoundOutput->RunningSampleIndex;
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
            GlobalRunning = false;
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
                else if (virtualKeyCode == 'R')
                {
                  reload_dlls = true;
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
                        GlobalRunning = false;
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
    real32 secondsElapsed = ((real32)(end.QuadPart - start.QuadPart)) / (real32)GlobalPerfCounterFrequency;
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
Win32DebugSyncDisplay(win32_offscreen_buffer *backbuffer, int soundCursorsCount, win32_debug_sound *soundCursors, win32_sound_output *SoundOutput, real32 targetSecondsPerFrame)
{
    int padX = 0;
    int padY = 0;
    
    int top = padY;
    int bottom = 100;
    
    real32 c = (real32)(backbuffer->Width - 2 * padX) / (real32)SoundOutput->SecondaryBufferSize;
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


internal void
HandleDebugCycleCounter(game_memory *memory)
{
#if 0

    OutputDebugStringA("DEBUG CYCLES\n");
    
    for (int32 debugIndex=0;
         debugIndex < ArrayCount(memory->DebugCounters);
         debugIndex++)
    {
        debug_cycle_counter *counter = memory->DebugCounters + debugIndex;

        if (counter->HitCount > 0){
            char buffer[256];
            sprintf_s(buffer, 256, "\t%d: %I64u hits: %u, cycles per hit: %I64u\n", debugIndex, counter->CycleCount, counter->HitCount, counter->CycleCount / counter->HitCount);

            OutputDebugStringA(buffer);

            counter->CycleCount = 0;
            counter->HitCount = 0;
        }
    }
#endif
}

struct platform_work_queue_entry
{
    platform_work_queue_callback *Callback;
    void *Data;
};

struct platform_work_queue
{
    uint32 volatile CompletionGoal;
    uint32 volatile CompletionCount;
    uint32 volatile NextEntryToWrite;
    uint32 volatile NextEntryToRead;
    HANDLE SemaphoreHandle;

    platform_work_queue_entry Entries[256];
};


internal void
Win32AddEntry(platform_work_queue *queue, platform_work_queue_callback *callback, void *data)
{
    // TODO: switch to interlocked compare exchange so that not only one thread can add tasks

    uint32 currentNextEntryToWrite = queue->NextEntryToWrite;
    uint32 newNextEntryToWrite = (currentNextEntryToWrite + 1) % ArrayCount(queue->Entries);
    
    Assert(newNextEntryToWrite != queue->NextEntryToRead);

    platform_work_queue_entry *entry = queue->Entries + currentNextEntryToWrite;

    entry->Callback = callback;
    entry->Data = data;
    
    _WriteBarrier();
    _mm_sfence();
    
    queue->NextEntryToWrite = newNextEntryToWrite;
    queue->CompletionGoal++;
    // wake up threads
    ReleaseSemaphore(queue->SemaphoreHandle, 1, 0);
}

internal bool32
Win32DoNextWorkQueueEntry(platform_work_queue *queue)
{
    bool32 isShouldSleep = false;


    uint32 currentNextEntryToRead = queue->NextEntryToRead;
    uint32 newNextEntryToRead = (currentNextEntryToRead + 1) % ArrayCount(queue->Entries);
    if (currentNextEntryToRead != queue->NextEntryToWrite)
    {
        uint32 index = InterlockedCompareExchange((LONG volatile *)&queue->NextEntryToRead, newNextEntryToRead, currentNextEntryToRead);

        if (index == currentNextEntryToRead) {
            platform_work_queue_entry *entry = queue->Entries + index;

            entry->Callback(queue, entry->Data);

            InterlockedIncrement((LONG volatile *)&queue->CompletionCount);
        }
    }
    else
    {
        isShouldSleep = true;
    }
    
    return isShouldSleep;
}

internal void
Win32CompleteAllWork(platform_work_queue *queue)
{
    //platform_work_queue_item item = {};
    while(queue->CompletionGoal != queue->CompletionCount)
    {
        Win32DoNextWorkQueueEntry(queue);
    };

    // TODO: NOT THREAD SAFE
    queue->CompletionCount = 0;
    queue->CompletionGoal = 0;
}

struct win32_platform_file_handle
{
    platform_file_handle H;
    HANDLE Win32Handle;
};

struct win32_platform_file_group
{
    platform_file_group H;
    HANDLE FoundHandle;
    WIN32_FIND_DATAA Data;
};

internal PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN(Win32GetAllFilesOfTypeBegin)
{
    win32_platform_file_group *win32FileGroup = (win32_platform_file_group *)
        VirtualAlloc(0, sizeof(win32_platform_file_group), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    win32FileGroup->H.FileCount = 0;

    WIN32_FIND_DATAA foundData;

    HANDLE searchHandle = FindFirstFileA(wildcard, &foundData);

    if (searchHandle != INVALID_HANDLE_VALUE) {
        do {
            ++win32FileGroup->H.FileCount;
        } while (FindNextFileA(searchHandle, &foundData));

        FindClose(searchHandle);
    }

    win32FileGroup->FoundHandle = FindFirstFileA(wildcard, &win32FileGroup->Data);
    
    return (platform_file_group *)win32FileGroup;
}

internal PLATFORM_GET_ALL_FILES_OF_TYPE_END(Win32GetAllFilesOfTypeEnd)
{
    win32_platform_file_group *win32FileGroup = (win32_platform_file_group *)group;

    if (win32FileGroup) {
        if (win32FileGroup->FoundHandle != INVALID_HANDLE_VALUE) {
            FindClose(win32FileGroup->FoundHandle);
        }
        
        VirtualFree(win32FileGroup, 0, MEM_RELEASE);
    }
}

internal PLATFORM_FILE_ERROR(Win32FileError)
{
    win32_platform_file_handle *w32Handle = (win32_platform_file_handle *)handle;
    w32Handle->H.NoErrors = false;

    #if HANDMADE_INTERNAL
    OutputDebugStringA("win32 file error:");
    OutputDebugStringA(message);
    OutputDebugStringA("\n");
    #endif
}
internal PLATFORM_OPEN_NEXT_FILE(Win32OpenNextFile)
{
    // TODO: better memory menagment. HeadAlloc => special win32 mem areana
    win32_platform_file_handle *result = 0;

    win32_platform_file_group *win32FileGroup = (win32_platform_file_group *)group;

    if (win32FileGroup) {
        if (win32FileGroup->FoundHandle != INVALID_HANDLE_VALUE) {
            result = (win32_platform_file_handle *)VirtualAlloc(0, sizeof(win32_platform_file_handle), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

            if (result) {
                result->Win32Handle = CreateFileA(win32FileGroup->Data.cFileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
                result->H.NoErrors = result->Win32Handle != INVALID_HANDLE_VALUE;
            }

            if (FindNextFileA(win32FileGroup->FoundHandle, &win32FileGroup->Data)) {
                // TODO: do nothing?
            } else {
                FindClose(win32FileGroup->FoundHandle);
                win32FileGroup->FoundHandle = INVALID_HANDLE_VALUE;
            }
        }
    }
    
    return (platform_file_handle *)result;
}

// NOTE|TODO: Win32CloseFile

internal PLATFORM_READ_DATA_FROM_FILE(Win32ReadDataFromFile)
{
    win32_platform_file_handle *w32Handle = (win32_platform_file_handle *)handle;

    if (PlatformNoFileErrors(handle))
    {
        OVERLAPPED overlapped = {};
        overlapped.Offset = (u32)(offset >> 0 & 0xFFFFFFFF);
        overlapped.OffsetHigh = (u32)(offset >> 32 & 0xFFFFFFFF);

        uint32 fileSize32 = SafeTruncateUInt64(size);

        DWORD bytesRead;
        if (ReadFile(w32Handle->Win32Handle, dest, fileSize32, &bytesRead, &overlapped) && (fileSize32 == bytesRead))
        {
            // NOTE: file read ok
        }
        else
        {
            Win32FileError(&w32Handle->H, "Failed reading file");
        }
    }
}

DWORD WINAPI
ThreadProc(LPVOID lpParameter)
{
    platform_work_queue *queue = (platform_work_queue *)lpParameter;
    OutputDebugStringA("starting thread\n");

    for (;;)
    {
        if (Win32DoNextWorkQueueEntry(queue))
        {
            WaitForSingleObjectEx(queue->SemaphoreHandle, INFINITE, false);
        }
    }
    //return 0;
}

internal
PLATFORM_WORK_QUEUE_CALLBACK(DoWorkerWork)
{
    char buf[256];

    wsprintf(buf, "Thread %u: %s\n", GetCurrentThreadId(), (char *)data);
    OutputDebugStringA(buf);
}


#define CompletePastWritesBeforeFutureReads _ReadBarrier()

internal void
Win32MakeQueue(platform_work_queue * queue, uint32 threadCount)
{
    queue->CompletionGoal = 0;
    queue->CompletionCount = 0;
    queue->NextEntryToWrite = 0;
    queue->NextEntryToRead = 0;
    
    uint32 initialCount = 0;
    queue->SemaphoreHandle = CreateSemaphoreExA(0, initialCount, threadCount, 0, 0, SEMAPHORE_ALL_ACCESS);

    for (uint32 threadI = 0; threadI < threadCount; threadI++) {
        HANDLE threadHandle = CreateThread(0, 0, ThreadProc, queue, 0, 0);
        CloseHandle(threadHandle);
    }
}

PLATFORM_ALLOCATE_MEMORY(Win32AllocateMemory)
{
    void *result = VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    return result;
}

PLATFORM_FREE_MEMORY(Win32FreeMemory)
{
    if (memory) {
        VirtualFree(memory, 0, MEM_RELEASE);
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


    // Win32ResizeDIBSection(&globalBackbuffer, 960, 540);
    
    Win32ResizeDIBSection(&globalBackbuffer, 1920, 1080);

    char *param = "thread started";

    platform_work_queue highPriorityQueue = {};
    Win32MakeQueue(&highPriorityQueue, 6);
    
    platform_work_queue lowPriorityQueue = {};
    Win32MakeQueue(&lowPriorityQueue, 1);
    
    Win32CompleteAllWork(&highPriorityQueue);
    
    LARGE_INTEGER perfCounterFrequencyResult;
    QueryPerformanceFrequency(&perfCounterFrequencyResult);
    GlobalPerfCounterFrequency = perfCounterFrequencyResult.QuadPart;

    reload_dlls = true;

    int monitorRefreshHz = 60;


    if (RegisterClassA(&windowClass))
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
            HDC refreshDC = GetDC(windowHandle);
            int win32RefreshRate = GetDeviceCaps(refreshDC, VREFRESH);
            if (win32RefreshRate > 1)
            {
                monitorRefreshHz = win32RefreshRate;
            }
    
            int32 gameUpdateHz = monitorRefreshHz / 2;
            real32 targetSecondsPerFrame = (real32)1.0f / (real32)gameUpdateHz;
            
            int xOffset = 0;

            // sound test

            bool SoundIsPlaying = false;
            
            win32_sound_output SoundOutput = {};

            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.RunningSampleIndex = 0;
            SoundOutput.BytesPerSample = sizeof(int16) * 2;
            SoundOutput.SafetyBytes = (SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample / gameUpdateHz) / 3;
            SoundOutput.LatencySampleCount = 4 * (SoundOutput.SamplesPerSecond / gameUpdateHz);
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
            Win32InitDirectSound(windowHandle, SoundOutput.SecondaryBufferSize, SoundOutput.SamplesPerSecond);
            Win32ClearSoundBuffer(&SoundOutput);
            globalSecondaryBuffer->Play(0,0,DSBPLAY_LOOPING);

            game_input inputs[2] = {};
            game_input *newInput = &inputs[0];
            game_input *oldInput = &inputs[1];

            LARGE_INTEGER lastCounter = Win32GetWallClock();
            uint64 lastCycleCount =  __rdtsc();

            LARGE_INTEGER flipWallClock = lastCounter;


            // TODO: remove this
            u32 maxPossibleOverrun = 2*4*sizeof(u16);
            int16 *samples = (int16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize + maxPossibleOverrun, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

            game_memory  gameMemory = {};
            gameMemory.PermanentStorageSize = Megabytes(64);
            gameMemory.TransientStorageSize = Gigabytes((uint64)1);

            gameMemory.HighPriorityQueue = &highPriorityQueue;
            gameMemory.LowPriorityQueue = &lowPriorityQueue;
            
            gameMemory.PlatformAPI.AddEntry = Win32AddEntry;
            gameMemory.PlatformAPI.CompleteAllWork = Win32CompleteAllWork;

            gameMemory.PlatformAPI.GetAllFilesOfTypeBegin = Win32GetAllFilesOfTypeBegin;
            gameMemory.PlatformAPI.GetAllFilesOfTypeEnd = Win32GetAllFilesOfTypeEnd;
            gameMemory.PlatformAPI.OpenNextFile = Win32OpenNextFile;
            gameMemory.PlatformAPI.ReadDataFromFile = Win32ReadDataFromFile;
            gameMemory.PlatformAPI.FileError = Win32FileError;
            gameMemory.PlatformAPI.AllocateMemory = Win32AllocateMemory;
            gameMemory.PlatformAPI.FreeMemory = Win32FreeMemory;

            // platform specific functions
#if HANDMADE_INTERNAL
            int debugSoundCursorsIndex = 0;
            win32_debug_sound debugSoundCursors[15] = {0};
            
            gameMemory.PlatformAPI.DEBUG_FreeFileMemory = Debug_PlatformFreeFileMemory;
            gameMemory.PlatformAPI.DEBUG_ReadEntireFile = Debug_PlatformReadEntireFile;
            gameMemory.PlatformAPI.DEBUG_WriteEntireFile = Debug_PlatformWriteEntireFile;
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

            remfils_step_state* stepState = RemfilsInitState(GlobalPerfCounterFrequency);

            GlobalRunning = true;

#if 0
            // NOTE: this is debug sound, result for writeCursor delta increase = 1920
            while (GlobalRunning) {
                DWORD playCursor;
                DWORD writeCursor;
                globalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor);

                char buffer[256];
                sprintf_s(buffer, 256, "PC:%u  // WC:%u\n", playCursor, writeCursor);
                OutputDebugStringA(buffer);
            }
#endif
            
            while(GlobalRunning)
            {
                newInput->DtForFrame = targetSecondsPerFrame;

                if (reload_dlls) {
                  // check for dll reload
                  FILETIME newDLLWriteTime = Win32GetLastWriteTime(sourceDLLFullPath);
                  if (CompareFileTime(&newDLLWriteTime, &game.DLLLastWriteTime) != 0) {
                    if (!Win32IsFileExists(lockFullFilePath)) {
                        Win32UnloadGameCode(&game);
                        game = Win32LoadGameCode(sourceDLLFullPath, tmpDLLFullPath);

                        newInput->ExecutableReloaded = true;
                      }
                  }
                  //reload_dlls = false;
                }

                RemfilsStartStep(stepState, "MOS");

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
                RemfilsEndStepAndNew(stepState, "INP");

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

                if (winState.InputRecordingIndex != 0)
                {
                    Win32RecordUserInput(&winState, newInput);
                }

                if (winState.InputPlayBackIndex != 0)
                {
                    Win32PlayBackUserInput(&winState, newInput);
                }

                RemfilsEndStepAndNew(stepState, "UaR");
                
                if (game.UpdateAndRender)
                {
                    // TODO: clear 
                    game.UpdateAndRender(&gameMemory, &buf, newInput);

                    HandleDebugCycleCounter(&gameMemory);
                }


                // send vibrations

                // XINPUT_VIBRATION vibration;
                // vibration.wLeftMotorSpeed = 569000;
                // vibration.wRightMotorSpeed = 569000;
                // XInputSetState(0, &vibration);

                RemfilsEndStepAndNew(stepState, "SND");

                ////////////////////////////////////////////////////////////////////////////////////////////////////
                // SOUND
                ////////////////////////////////////////////////////////////////////////////////////////////////////

                /*
                  how sound output works:

                  when wake up to write audio

                  Define a *safety value* that is the number of samples
                  we think our game update loop may vary by

                  1) look and see what the play cursor position is and
                  forecast ahead where play cursor will be on the next
                  frame boundary.

                  2.1) Look to see if the writeCursor is before forecast
                  play cursor. If it is, write up to the next frame
                  boundary + one frame further

                  2.2) If writeCursor is _after_ next frame
                  boundary. Write one frames worth of audio +
                  safetyBytes
                */
                

                LARGE_INTEGER audioWallClock = Win32GetWallClock();
                real32 fromBeginToAudioSeconds = Win32GetSecondsElapsed(flipWallClock, audioWallClock);

                DWORD byteToLock = 0;
                DWORD targetCursor = 0;
                DWORD bytesToWrite = 0;
                
                DWORD playCursor;
                DWORD writeCursor;
                if (globalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor) == DS_OK)
                {
                    if (!soundIsValid)
                    {
                        SoundOutput.RunningSampleIndex = writeCursor / SoundOutput.BytesPerSample;
                        soundIsValid = true;
                    }
                
                    byteToLock = ((SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize);
                    DWORD expectedBytesPerFrame = (SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample) / gameUpdateHz;
                    real32 secondsLeftUntilFlip = (targetSecondsPerFrame - fromBeginToAudioSeconds);
                    DWORD expectedBytesUntilFlip = (DWORD)((secondsLeftUntilFlip / targetSecondsPerFrame) * (real32)expectedBytesPerFrame);
                    DWORD expectedFrameBoundaryByte = playCursor + expectedBytesUntilFlip;
                    
                    DWORD safeWriteCursor = writeCursor;
                    if (safeWriteCursor < playCursor)
                    {
                        safeWriteCursor += SoundOutput.SecondaryBufferSize;
                    }
                    safeWriteCursor += SoundOutput.SafetyBytes;
                    
                    bool32 audioCardIsLowLatency = safeWriteCursor < expectedFrameBoundaryByte;
                    if (audioCardIsLowLatency) {
                        targetCursor = expectedFrameBoundaryByte + expectedBytesPerFrame;
                    } else {
                        targetCursor = writeCursor + expectedBytesPerFrame + SoundOutput.SafetyBytes;
                    }
                    targetCursor = targetCursor % SoundOutput.SecondaryBufferSize;

                    bytesToWrite = 0;
                    if (byteToLock > targetCursor) {
                        bytesToWrite = SoundOutput.SecondaryBufferSize - byteToLock;
                        bytesToWrite += targetCursor;
                    } else {
                        bytesToWrite = targetCursor - byteToLock;
                    }

                    game_sound_output_buffer soundBuffer = {};
                    soundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                    soundBuffer.SampleCount = Align8(bytesToWrite / SoundOutput.BytesPerSample);
                    // align bytesToWrite by 8
                    bytesToWrite = soundBuffer.SampleCount * SoundOutput.BytesPerSample;
                    soundBuffer.Samples = samples;

                    if (game.GetSoundSamples)
                    {
                        game.GetSoundSamples(&gameMemory, &soundBuffer);
                    }

#if HANDMADE_INTERNAL

                    // DWORD debugPlayCursor;
                    // DWORD debugWriteCursor;
                        
                    // if (globalSecondaryBuffer->GetCurrentPosition(&debugPlayCursor, &debugWriteCursor) == DS_OK)
                    // {
                    //     DWORD unwrappedWriteCursor = debugWriteCursor;
                    //     if (unwrappedWriteCursor < debugPlayCursor)
                    //     {
                    //         unwrappedWriteCursor += SoundOutput.SecondaryBufferSize;
                    //     }
                        // audioLatencyBytes = unwrappedWriteCursor - debugPlayCursor;
                        // audioLatencySeconds = (audioLatencyBytes / (real32)SoundOutput.BytesPerSample) / (real32)SoundOutput.SamplesPerSecond;
                    //}

                    audioLatencyBytes = safeWriteCursor - playCursor;
                    real32 audioLatencyInSeconds = ((real32)audioLatencyBytes / (real32)SoundOutput.BytesPerSample) / ((real32)SoundOutput.SamplesPerSecond);
                    
                    char buffer[256];
                    sprintf_s(buffer, 256,
                              "LPC:%u, BTL: %u, TC:%u /// PC:%u, WC:%u, DELTA: %u (%fs)\n",
                              0, byteToLock, targetCursor, playCursor, writeCursor, 0 //bytesBetweenSecodns
                              , audioLatencyInSeconds);
                    //OutputDebugStringA(buffer);
#endif

                    Win32FillSoundBuffer(&SoundOutput, byteToLock, bytesToWrite, &soundBuffer);
                }
                else
                {
                    soundIsValid = false;
                }

                // direct sound output test

                if (!SoundIsPlaying)
                {
                    globalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
                    SoundIsPlaying = true;
                }

                RemfilsEndStepAndNew(stepState, "WAI");

                // count cycles
                
                int64 endCycleCount = __rdtsc();
                int64 cyclesElapsed = endCycleCount - lastCycleCount;

                
                real32 workSecondsElapsed = Win32GetSecondsElapsed(lastCounter, Win32GetWallClock());
                
                real32 secondsElapsedForFrame = workSecondsElapsed;

                if (secondsElapsedForFrame < targetSecondsPerFrame)
                {
                    LARGE_INTEGER sleepStartTime = Win32GetWallClock();

                    // TODO: investigate missed framerates
                    // if (sleepIsGranural)
                    if (false)
                    {
                        DWORD sleepMs = (DWORD)((targetSecondsPerFrame - secondsElapsedForFrame) * (real32)1000.0f);

                        char buffer[256];
                        sprintf_s(buffer, "Starrt sleep %u\n", sleepMs);
                        OutputDebugStringA(buffer);
                        
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
                    OutputDebugStringA("Missed frame");
                }

                LARGE_INTEGER endCounter = Win32GetWallClock();
                int64 counterElapsed = endCounter.QuadPart - lastCounter.QuadPart;
                real32 msPerFrame = 1000.0f * Win32GetSecondsElapsed(lastCounter, endCounter);
                lastCounter = endCounter;
                lastCycleCount = endCycleCount;

                // TODO(vlad): wtf is this????

                HDC deviceContext = GetDC(windowHandle);
                win32_window_dimension dim = Win32GetWindowDimension(windowHandle);

                flipWallClock = Win32GetWallClock();

#if HANDMADE_INTERNAL
                Win32DebugSyncDisplay(&globalBackbuffer, ArrayCount(debugSoundCursors), debugSoundCursors, &SoundOutput, targetSecondsPerFrame);
#endif

                Win32DisplayBufferInWindow(deviceContext, dim.Width, dim.Height, &globalBackbuffer, 0, 0, dim.Width, dim.Height);
                ReleaseDC(windowHandle, deviceContext);

                // NOTE|TODO: in video here getcurrentposition is used
                // and soundIsValid logic makes sence... (currently it
                // completly doesnt)

#if HANDMADE_INTERNAL
                if (soundIsValid) {
                    win32_debug_sound *cur = &debugSoundCursors[debugSoundCursorsIndex];
                    cur->FlipPlayCursor = playCursor;
                    cur->FlipWriteCursor = writeCursor;
                }

                debugSoundCursorsIndex++;
                if (debugSoundCursorsIndex == ArrayCount(debugSoundCursors)) {
                    debugSoundCursorsIndex = 0;
                }
#endif

#if 0

                int32 FPS = (int32)GlobalPerfCounterFrequency / (int32)counterElapsed;
                real64 MCPF = ((real64)cyclesElapsed / (1000.0f * 1000.0f)); 

                char buffer[256];
                sprintf_s(buffer, "/%fms/f // FPS: %d // %fMc/f\n", msPerFrame, FPS, MCPF);
                OutputDebugStringA(buffer);
#endif

                game_input *tmpInput = newInput;
                newInput = oldInput;
                oldInput = tmpInput;

                RemfilsEndStep(stepState);
            }
        }
        else
        {
            // TODO: logging
        }
    }
    else
    {
        // TODO: logging
    }
    
    return(0);
}
