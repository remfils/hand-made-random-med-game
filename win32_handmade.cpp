#include <windows.h>
#include <stdint.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

static bool running;
static BITMAPINFO bitmapInfo;
static void *bitmapMemory;
static int bitmapWidth;
static int bitmapHeight;
static int bytesPerPixel = 4;
    

static void
RenderGradient(int xOffset, int yOffset)
{
    int bytesPerPixel = 4;
    int width = bitmapWidth;
    int height = bitmapHeight;
    
    int pitch = width * bytesPerPixel;
    uint8 *row = (uint8 *)bitmapMemory;
    for (int y=0; y < bitmapHeight; ++y)
    {
         uint32 *pixel = (uint32 *)row;

        for (int x=0; x < bitmapWidth; ++x)
        {
            uint8 b = x + xOffset;
            uint8 g = y + yOffset;

            *pixel++ = ((g << 8) | b);
        }

        row += pitch;
    }
}

static void
Win32ResizeDIBSection(int width, int height)
{
    if (bitmapMemory)
    {
        VirtualFree(bitmapMemory, 0, MEM_RELEASE);
    }

    bitmapWidth = width;
    bitmapHeight = height;
    
    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth = bitmapWidth;
    bitmapInfo.bmiHeader.biHeight = -bitmapHeight;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    int bitmapMemorySize = bitmapWidth * bitmapHeight * bytesPerPixel;
    bitmapMemory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    // drawing stuff

    // RenderGradient(0, 0);
}

static void
Win32UpdateWindow(HDC deviceContext, RECT *windowRect, int x, int y, int width, int height)
{
    int windowWidth = windowRect->right - windowRect->left;
    int windowHeight = windowRect->bottom - windowRect->top;
    
    StretchDIBits(
        deviceContext,
        0, 0, bitmapWidth, bitmapHeight,
        0, 0, windowWidth, windowHeight,
        bitmapMemory,
        &bitmapInfo,
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
        RECT winRect;
        GetClientRect(window, &winRect);

        LONG width = winRect.right - winRect.left;
        LONG height = winRect.bottom - winRect.top;

        Win32ResizeDIBSection(width, height);
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

        RECT clientRect;
        GetClientRect(window, &clientRect);
        Win32UpdateWindow(deviceContext, &clientRect, x, y, width, height);

        EndPaint(window, &paint);
    } break;
    case WM_ACTIVATEAPP:
    {
        OutputDebugStringA("WM_ACTIVATEAPP\n");
    } break;
    default:
    {
        result = DefWindowProc(window, message, wParam, lParam);
    } break;
    }

    return result;
}

int CALLBACK WinMain(
    HINSTANCE instance,
    HINSTANCE prevInstance,
    LPSTR     commandLine,
    int       showCode)
{
    WNDCLASSA windowClass = {};
    windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = MainWindowCallback;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = "HandmadeHeroWindowClass";

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
            running = true;

            int xOffset = 0;
            
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

                RenderGradient(xOffset++, 0);

                HDC deviceContext = GetDC(windowHandle);
                
                RECT windowRect;
                GetClientRect(windowHandle, &windowRect);
                int windowWidth = windowRect.right - windowRect.left;
                int windowHeight = windowRect.bottom - windowRect.top;
                Win32UpdateWindow(deviceContext, &windowRect, 0, 0, windowWidth, windowHeight);
                
                ReleaseDC(windowHandle, deviceContext);
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
