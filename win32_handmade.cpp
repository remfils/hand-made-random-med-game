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

#include "handmade.h"
#include "handmade.cpp"

#include <malloc.h>

#include <windows.h>
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>


global_variable int  GlobalXAcl        = 0;
global_variable int  GlobalYAcl        = 0;
global_variable bool isUpPressed       = 0;
global_variable bool isDownPressed     = 0;
global_variable bool isLeftPressed     = 0;
global_variable bool isRightPressed    = 0;
global_variable bool isUpKeyPressed    = 0;
global_variable bool isDownKeyPressed  = 0;

global_variable int64 GlobalPerfCounterFrequency;


struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel = 4;
};

global_variable bool running;
global_variable win32_offscreen_buffer globalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER globalSecondaryBuffer;

// XInputGetState

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
static x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// XInputSetState

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);

X_INPUT_SET_STATE(XInputSetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
static x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

static void
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

struct win32_window_dimension
{
    int Width;
    int Height;
};

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

static void
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

static void
Win32DisplayBufferInWindow(HDC deviceContext, int windowWidth, int windowHeight, win32_offscreen_buffer *buffer, int x, int y, int width, int height)
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

#if HANDMADE_INTERNAL
internal void
Debug_PlatformFreeFileMemory(void *memory)
{
    if (memory)
    {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}

internal debug_read_file_result
Debug_PlatformReadEntireFile(char *filename)
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


bool32
Debug_PlatformWriteEntireFile(char *filename, uint32 memorySize, void *memory)
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
    case WM_ACTIVATEAPP:
    {
        OutputDebugStringA("WM_ACTIVATEAPP\n");
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

struct win32_sound_output
{
    int samplesPerSecond;
    uint32 runningSampleIndex;
    int bytesPerSample;
    int secondaryBufferSize;
    int LatencySampleCount;
};

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
                HRESULT Error = directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0);
                if (SUCCEEDED(Error))
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
            HRESULT error = directSound->CreateSoundBuffer(&bufferDescription, &globalSecondaryBuffer, 0);
            if (SUCCEEDED(error))
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
    
    HRESULT error = globalSecondaryBuffer->Lock(0, soundOutput->secondaryBufferSize,
        &region1, &regionSize1, &region2, &regionSize2,
        0);

    if (SUCCEEDED(error))
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
Win32ProcessKeyboardButtonPress(game_button_state *newState, bool32 isDown)
{
    Assert(newState->EndedDown != isDown);
    newState->EndedDown = isDown;
    ++newState->HalfTransitionCount;
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
Win32ProcessWindowMessages(game_controller_input *keyboardController)
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
                    Win32ProcessKeyboardButtonPress(&keyboardController->Start, isDown);
                }
                else if (virtualKeyCode == VK_SPACE)
                {
                    Win32ProcessKeyboardButtonPress(&keyboardController->Back, isDown);
                }
                
                else if (virtualKeyCode == VK_F4)
                {
                    bool32 isAlt = (message.lParam & (1 << 29));
                    if (isAlt)
                    {
                        running = false;
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

internal void
Win32FillSoundBuffer(win32_sound_output *soundOutput, DWORD byteToLock, DWORD bytesToWrite, game_sound_output_buffer *sourceBuffer)
{
    void *region1;
    DWORD regionSize1;
    void *region2;
    DWORD regionSize2;

    HRESULT error = globalSecondaryBuffer->Lock(byteToLock, bytesToWrite,
        &region1, &regionSize1, &region2, &regionSize2,
        0); 
    if (SUCCEEDED(error))
    {
        int16 *sampleOut = (int16 *)region1;
        DWORD region1SampleCount = regionSize1 / soundOutput->bytesPerSample;
        int16 *sourceSample = sourceBuffer->Samples;

        for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount; ++sampleIndex)
        {
            *sampleOut++ = *sourceSample++;
            *sampleOut++ = *sourceSample++;
            ++soundOutput->runningSampleIndex;
        }

        sampleOut = (int16 *)region2;
        DWORD region2SampleCount = regionSize2 / soundOutput->bytesPerSample;
        for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount; ++sampleIndex)
        {
            *sampleOut++ = *sourceSample++;
            *sampleOut++ = *sourceSample++;
            ++soundOutput->runningSampleIndex;
        }

        globalSecondaryBuffer->Unlock(region1, regionSize1, region2, regionSize2);
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
    windowClass.lpszClassName = "HandmadeHeroWindowClass";

    Win32ResizeDIBSection(&globalBackbuffer, 1280, 720);

    LARGE_INTEGER perfCounterFrequencyResult;
    QueryPerformanceFrequency(&perfCounterFrequencyResult);
    GlobalPerfCounterFrequency = perfCounterFrequencyResult.QuadPart;

    int monitorRefreshRate = 60;
    int gameUpdateHz = 60;
    real32 targetSecondsPerFrame = (real32)1.0f / (real32) gameUpdateHz;

    if (RegisterClass(&windowClass))
    {
        HWND windowHandle = CreateWindowExA(
            0,
            windowClass.lpszClassName,
            "Handmade window",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0, // parent window
            0, // menu
            instance,
            0);

        if (windowHandle)
        {
            int xOffset = 0;

            // sound test

            bool soundIsPlaying = false;
            
            win32_sound_output soundOutput = {};

            soundOutput.samplesPerSecond = 48000;
            soundOutput.runningSampleIndex = 0;
            soundOutput.LatencySampleCount = soundOutput.samplesPerSecond / 20;
            soundOutput.bytesPerSample = sizeof(int16) * 2;
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

            int16 *samples = (int16 *)VirtualAlloc(0, soundOutput.secondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

            game_memory gameMemory = {};
            gameMemory.PermanentStorageSize = Megabytes(64);
            gameMemory.TransientStorageSize = Gigabytes((uint64)1);

            #if HANDMADE_INTERNAL
            LPVOID baseAddress = (LPVOID)Terabytes((uint64)2);
            #else
            LPVOID baseAddress = 0;
            #endif

            uint64 totalSize = gameMemory.PermanentStorageSize + gameMemory.TransientStorageSize;
            gameMemory.PermanentStorage = (void *)VirtualAlloc(baseAddress, totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            gameMemory.TransientStorage = (uint8 *)gameMemory.PermanentStorage + gameMemory.PermanentStorageSize;
            
            
            running = true;
            while(running)
            {
                game_controller_input *oldKeyboardController = &oldInput->Controllers[0];
                game_controller_input *keyboardController = &newInput->Controllers[0];
                *keyboardController = {};

                keyboardController->IsConnected = true;

                for (int buttonIndex = 0; buttonIndex < ArrayCount(keyboardController->Buttons); ++buttonIndex)
                {
                    keyboardController->Buttons[buttonIndex].EndedDown = oldKeyboardController->Buttons[buttonIndex].EndedDown;
                }

                Win32ProcessWindowMessages(keyboardController);

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

                // send vibrations

                // XINPUT_VIBRATION vibration;
                // vibration.wLeftMotorSpeed = 569000;
                // vibration.wRightMotorSpeed = 569000;
                // XInputSetState(0, &vibration);


                bool32 soundIsValid = false;
                DWORD playCursor;
                DWORD writeCursor;
                DWORD byteToLock = 0;
                DWORD targetCursor;
                DWORD bytesToWrite = 0;

                HRESULT error = globalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor);
                if (SUCCEEDED(error))
                {
                    byteToLock = ((soundOutput.runningSampleIndex * soundOutput.bytesPerSample) % soundOutput.secondaryBufferSize);
                    targetCursor = ((playCursor +
                        (soundOutput.LatencySampleCount * soundOutput.bytesPerSample)) % soundOutput.secondaryBufferSize);
                    
                    if (byteToLock > targetCursor)
                    {
                        bytesToWrite = soundOutput.secondaryBufferSize - byteToLock;
                        bytesToWrite += targetCursor;
                    }
                    else
                    {
                        bytesToWrite = targetCursor - byteToLock;
                    }

                    soundIsValid = true;
                }
                

                game_sound_output_buffer soundBuffer = {};
                soundBuffer.Samples = samples;
                soundBuffer.SampleCount = bytesToWrite / soundOutput.samplesPerSecond;
                soundBuffer.SamplesPerSecond = soundOutput.samplesPerSecond;

                game_offscreen_buffer buf;
                buf.Memory = globalBackbuffer.Memory;
                buf.Width = globalBackbuffer.Width;
                buf.Height = globalBackbuffer.Height;
                buf.Pitch = globalBackbuffer.Pitch;
                buf.BytesPerPixel = globalBackbuffer.BytesPerPixel;
                GameUpdateAndRender(&gameMemory, &buf, &soundBuffer, newInput);

                if (soundIsValid)
                {
                    Win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite, &soundBuffer);
                }

                // direct sound output test

                if (!soundIsPlaying)
                {
                    globalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
                    soundIsPlaying = true;
                }

                // count cycles
                
                uint64 endCycleCount = __rdtsc();
                uint64 cyclesElapsed = endCycleCount - lastCycleCount;
                
                LARGE_INTEGER workCounter = Win32GetWallClock();
                real32 workSecondsElapsed = Win32GetSecondsElapsed(lastCounter, workCounter);
                
                real32 secondsElapsedForFrame = workSecondsElapsed;

                if (secondsElapsedForFrame < targetSecondsPerFrame)
                {
                    if (sleepIsGranural)
                    {
                        DWORD sleepMs = (DWORD)((targetSecondsPerFrame - secondsElapsedForFrame) * (real32)1000.0f);
                        Sleep(sleepMs);
                    }
                }
                else
                {
                    
                }

                HDC deviceContext = GetDC(windowHandle);
                win32_window_dimension dim = Win32GetWindowDimension(windowHandle);
                Win32DisplayBufferInWindow(deviceContext, dim.Width, dim.Height, &globalBackbuffer, 0, 0, dim.Width, dim.Height);
                ReleaseDC(windowHandle, deviceContext);
               

                // game profile info
                
                #if 1

                real64 msPerFrame = (real64)((1000.0f * (real64)cyclesElapsed) / (real32)GlobalPerfCounterFrequency);
                real64 FPS = (real64)GlobalPerfCounterFrequency / (real64)cyclesElapsed;
                real64 MCPF = ((real64)cyclesElapsed / (1000.0f * 1000.0f)); 

                char buffer[256];
                sprintf_s(buffer, "%fms, %f, %fmc/f\n", msPerFrame, FPS, MCPF);
                OutputDebugStringA(buffer);
                #endif

                QueryPerformanceCounter(&workCounter);

                lastCounter = workCounter;
                lastCycleCount = endCycleCount;
                
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
