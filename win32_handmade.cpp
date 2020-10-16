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

#include "handmade.cpp"
#include <malloc.h>

#include <windows.h>
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>


global_variable int  GlobalXAcl        = 0;
global_variable int  GlobalYAcl        = 0;
global_variable int  GlobalXOffset     = 0;
global_variable int  GlobalYOffset     = 0;
global_variable bool isUpPressed       = 0;
global_variable bool isDownPressed     = 0;
global_variable bool isLeftPressed     = 0;
global_variable bool isRightPressed    = 0;
global_variable bool isUpKeyPressed    = 0;
global_variable bool isDownKeyPressed  = 0;
global_variable int Hz                 = 400;

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
        uint32 virtualKeyCode = wParam;
        bool wasDown = ((lParam & (1 << 30)) != 0);
        bool isDown ((lParam & (1 << 31)) == 0);

        if (isDown != wasDown)
        {
            if (virtualKeyCode == 'W')
            {
                isUpPressed = isDown;
            }
            else if (virtualKeyCode == 'A')
            {
                isLeftPressed = isDown;
            }
            else if (virtualKeyCode == 'S')
            {
                isDownPressed = isDown;
            }
            else if (virtualKeyCode == 'D')
            {
                isRightPressed = isDown;
            }
            else if (virtualKeyCode == 'Q')
            {
            
            }
            else if (virtualKeyCode == 'E')
            {
            
            }
            else if (virtualKeyCode == VK_UP)
            {
                isUpKeyPressed = isDown;
            }
            else if (virtualKeyCode == VK_DOWN)
            {
                isDownKeyPressed = isDown;
            }
            else if (virtualKeyCode == VK_RIGHT)
            {
            
            }
            else if (virtualKeyCode == VK_LEFT)
            {
            
            }
            else if (virtualKeyCode == VK_ESCAPE)
            {
                OutputDebugStringA("Escape:");
                if (isDown)
                {
                    OutputDebugStringA(" is down\n");
                }
                else if (wasDown)
                {
                    OutputDebugStringA(" was down\n");
                }
            }
            else if (virtualKeyCode == VK_SPACE)
            {
            
            }
            else if (virtualKeyCode == VK_F4)
            {
                bool32 isAlt = (lParam & (1 << 29));
                if (isAlt)
                {
                    running = false;
                }
            }
        }

        
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
    int wavePeriod;
    int halfWavePeriod;
    int bytesPerSample;
    int secondaryBufferSize;
    real32 t;
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

int CALLBACK WinMain(
    HINSTANCE instance,
    HINSTANCE prevInstance,
    LPSTR     commandLine,
    int       showCode)
{
    Win32InitXInput();
    
    WNDCLASSA windowClass = {};
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = MainWindowCallback;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = "HandmadeHeroWindowClass";

    Win32ResizeDIBSection(&globalBackbuffer, 1280, 720);

    LARGE_INTEGER perfCounterFrequencyResult;
    QueryPerformanceFrequency(&perfCounterFrequencyResult);
    int64 perfCounterFrequency = perfCounterFrequencyResult.QuadPart;

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
            soundOutput.wavePeriod = soundOutput.samplesPerSecond / Hz;
            soundOutput.LatencySampleCount = soundOutput.samplesPerSecond / 20;
            soundOutput.halfWavePeriod = soundOutput.wavePeriod / 2;
            soundOutput.bytesPerSample = sizeof(int16) * 2;
            soundOutput.secondaryBufferSize = soundOutput.samplesPerSecond * soundOutput.bytesPerSample;

            Win32InitDirectSound(windowHandle, soundOutput.secondaryBufferSize, soundOutput.samplesPerSecond);

            Win32ClearSoundBuffer(&soundOutput);
            globalSecondaryBuffer->Play(0,0,DSBPLAY_LOOPING);

            LARGE_INTEGER lastCounter;
            QueryPerformanceCounter(&lastCounter);
            uint64 lastCycleCount =  __rdtsc();

            int16 *samples = (int16 *)VirtualAlloc(0, soundOutput.secondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            
            running = true;
            while(running)
            {
                MSG message;
                
                while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
                {
                    if (message.message == WM_QUIT)
                    {
                        running = false;
                    }
                    
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }

                // should pull this more frequently?
                for (DWORD controllerIndex=0; controllerIndex < XUSER_MAX_COUNT; controllerIndex++)
                {
                    XINPUT_STATE controllerState;
                    if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
                    {
                        // controller is ok

                        // look at controllerState.DwPacketNumber

                        XINPUT_GAMEPAD *pad = &controllerState.Gamepad;

                        bool dPadUp = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool dPadDown = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool dPadLeft = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool dPadRight = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool dPadStart = (pad->wButtons & XINPUT_GAMEPAD_START);
                        bool dPadBack = (pad->wButtons & XINPUT_GAMEPAD_BACK);
                        bool dPadLeftShoulder = (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool dPadRightShoulder = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool dPadA = (pad->wButtons & XINPUT_GAMEPAD_A);
                        bool dPadB = (pad->wButtons & XINPUT_GAMEPAD_B);
                        bool dPadX = (pad->wButtons & XINPUT_GAMEPAD_X);
                        bool dPadY = (pad->wButtons & XINPUT_GAMEPAD_Y);

                        int16 stickX = pad->sThumbLX;
                        int16 stickY = pad->sThumbLY;


                        Hz = 512 + (int)(256.0f * (stickY / 30000.0f));
                    }
                    else
                    {
                        // controller not available
                    }
                }

                // send vibrations

                // XINPUT_VIBRATION vibration;
                // vibration.wLeftMotorSpeed = 569000;
                // vibration.wRightMotorSpeed = 569000;
                // XInputSetState(0, &vibration);

                if (isLeftPressed)
                {
                    GlobalXAcl += 1;
                }
                if (isRightPressed)
                {
                    GlobalXAcl -= 1;
                }
                if (isUpPressed)
                {
                    GlobalYAcl += 1;
                }
                if (isDownPressed)
                {
                    GlobalYAcl -= 1;
                }

                if (GlobalXAcl > 100)
                {
                    GlobalXAcl = 100;
                }
                if (GlobalXAcl < -100)
                {
                    GlobalXAcl = -100;
                }

                if (GlobalYAcl > 100)
                {
                    GlobalYAcl = 100;
                }
                if (GlobalYAcl < -100)
                {
                    GlobalYAcl = -100;
                }

                // update wave period with keys
                soundOutput.wavePeriod = soundOutput.samplesPerSecond / Hz;

                GlobalXOffset += GlobalXAcl / 25;
                GlobalYOffset += GlobalYAcl / 25;



                bool32 soundIsValid = false;
                DWORD playCursor;
                DWORD writeCursor;
                DWORD byteToLock;
                DWORD targetCursor;
                DWORD bytesToWrite;

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
                GameUpdateAndRender(&buf, &soundBuffer);

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
                
                HDC deviceContext = GetDC(windowHandle);
                
                win32_window_dimension dim = Win32GetWindowDimension(windowHandle);
                Win32DisplayBufferInWindow(deviceContext, dim.Width, dim.Height, &globalBackbuffer, 0, 0, dim.Width, dim.Height);
                
                ReleaseDC(windowHandle, deviceContext);

                LARGE_INTEGER endCounter;
                QueryPerformanceCounter(&endCounter);

                int64 counterElapsed = endCounter.QuadPart - lastCounter.QuadPart;

                real32 msPerFrame = (real32)((1000.0f * (real32)counterElapsed) / (real32)perfCounterFrequency);
                real32 FPS = (real32)((real32)perfCounterFrequency / (real32)counterElapsed);

                #if 0

                uint64 endCycleCount = __rdtsc();
                real32 megaCyclesElapsed = (real32)((real32)endCycleCount - (real32)lastCycleCount) / (1000.0f * 1000.0f);

                char buffer[256];
                sprintf(buffer, "%fms, %f, %fmc/f\n", msPerFrame, FPS, megaCyclesElapsed);
                OutputDebugStringA(buffer);

                lastCounter = endCounter;
                lastCycleCount = endCycleCount;
                #endif
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
