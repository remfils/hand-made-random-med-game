#include <windows.h>

static bool running;
static BITMAPINFO bitmapInfo;
static void *bitmapMemory;

static void
Win32ResizeDIBSection(int width, int height)
{
    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    int bytesPerPixel = 4
    int bitmapMemorySize = Width * Height * bytesPerPixel;
    bitmapMemory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    
}

static void
Win32UpdateWindow(HDC deviceContext, int x, int y, int width, int height)
{
    BitBlt();
    StretchDIBits(
        deviceContext,
        x, y, width, height, // dst
        x, y, width, height, // src
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

        Win32UpdateWindow(deviceContext, x, y, width, height);

        static DWORD operation = WHITENESS;
        
        PatBlt(deviceContext, x, y, width, height, operation);

        if (operation == WHITENESS)
        {
            operation = BLACKNESS;
        }
        else
        {
            operation = WHITENESS;
        }

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
    // HICON     WindowClass.hIcon;
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
            while(running)
            {
                MSG message;
                BOOL msgResult = GetMessage(&message, 0, 0, 0);

                if (msgResult > 0)
                {
                    TranslateMessage(&message);

                    DispatchMessage(&message);
                }
                else
                {
                    break;
                }
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


