// D2DTimer.cpp : Defines the entry point for the application.
//

#include "D2DTimer.h"

#include <chrono>


using namespace std;


#define MAX_LOADSTRING 100
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name


//
// Provides the entry point to the application.
//
INT WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE /*hPrevInstance*/,
    LPSTR /*lpCmdLine*/,
    INT /*nCmdShow*/
)
{
    // Ignore the return value because we want to run the program even in the
    // unlikely event that HeapSetInformation fails.
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

    if (SUCCEEDED(CoInitialize(NULL)))
    {
        {
            D2DTimer app;

            if (SUCCEEDED(app.Initialize()))
            {
                app.RunMessageLoop(hInstance);
            }
        }
        CoUninitialize();
    }

    return 0;
}


//
// Initialize members.
//
D2DTimer::D2DTimer() :
    m_hwnd(NULL),
    m_pD2DFactory(NULL),
    m_pDWriteFactory(NULL),
    m_pRenderTarget(NULL),
    m_pTextFormat(NULL),
    m_pRedBrush(NULL),
    m_ullTikcount(0),
    m_fFontSize(50),
    m_eStyle(StyleType::Timer),
    m_eSpan(FrameRateSpan::By60)
{
    wcscpy(m_scTimer, L"00:00:00.000\0");
    wcscpy(m_scFormatTimer, L"%02d:%02d:%02d.%03d\0");

    wcscpy(m_scClock, L"00:00:00.000\0");
    wcscpy(m_scFormatClock, L"%02d:%02d:%02d.%03d\0");

    wcscpy(m_scTimestamp, L"0000000000.000\0");
    wcscpy(m_scFormatTimestamp, L"%0.3f\0");
}


//
// Release resources.
//
D2DTimer::~D2DTimer()
{
    SafeRelease(&m_pD2DFactory);
    SafeRelease(&m_pDWriteFactory);
    SafeRelease(&m_pRenderTarget);
    SafeRelease(&m_pTextFormat);
    SafeRelease(&m_pRedBrush);
}


//
// Creates the application window and initializes
// device-independent resources.
//
HRESULT D2DTimer::Initialize()
{
    HRESULT hr;

    // Initialize device-indpendent resources, such
    // as the Direct2D factory.
    hr = CreateDeviceIndependentResources();
    if (SUCCEEDED(hr))
    {
        LoadStringW(HINST_THISCOMPONENT, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
        LoadStringW(HINST_THISCOMPONENT, IDC_PROJECT, szWindowClass, MAX_LOADSTRING);

        // Register the window class.
        WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = D2DTimer::WndProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = sizeof(LONG_PTR);
        wcex.hInstance = HINST_THISCOMPONENT;
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
        wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_PROJECT);
        wcex.hIcon = LoadIcon(HINST_THISCOMPONENT, MAKEINTRESOURCE(IDI_BIG_ICON));
        wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL_ICON));
        wcex.lpszClassName = szWindowClass;

        RegisterClassEx(&wcex);

        // Create the application window.
        //
        // Because the CreateWindow function takes its size in pixels, we
        // obtain the system DPI and use it to scale the window size.
        //FLOAT dpiX, dpiY;
        //m_pD2DFactory->GetDesktopDpi(&dpiX, &dpiY);
        HDC screen = GetDC(0);
        INT dpiX = GetDeviceCaps(screen, LOGPIXELSX);
        INT dpiY = GetDeviceCaps(screen, LOGPIXELSY);
        ReleaseDC(0, screen);

        m_hwnd = CreateWindow(
            szWindowClass,
            szTitle,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            static_cast<UINT>(ceil(640.f * dpiX / 96.f)),
            static_cast<UINT>(ceil(480.f * dpiY / 96.f)),
            NULL,
            NULL,
            HINST_THISCOMPONENT,
            this
        );
        hr = m_hwnd ? S_OK : E_FAIL;
        if (SUCCEEDED(hr))
        {
            ShowWindow(m_hwnd, SW_SHOWNORMAL);

            UpdateWindow(m_hwnd);
        }
    }

    return hr;
}


//
// Create resources which are not bound
// to any device. Their lifetime effectively extends for the
// duration of the app. These resources include the Direct2D and
// DirectWrite factories,  and a DirectWrite Text Format object
// (used for identifying particular font characteristics).
//
HRESULT D2DTimer::CreateDeviceIndependentResources()
{
    HRESULT hr;

    // Create a Direct2D factory.
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);

    if (SUCCEEDED(hr))
    {
        // Create a DirectWrite factory.
        hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(m_pDWriteFactory),
            reinterpret_cast<IUnknown**>(&m_pDWriteFactory)
        );
    }
    if (SUCCEEDED(hr))
    {
        // Create a DirectWrite text format object.
        hr = m_pDWriteFactory->CreateTextFormat(
            L"Consolas",
            NULL,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            m_fFontSize,
            L"",  // locale
            &m_pTextFormat
        );
    }
    if (SUCCEEDED(hr))
    {
        // Center the text horizontally and vertically.
        m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        m_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    return hr;
}


//
//  This method creates resources which are bound to a particular
//  Direct3D device. It's all centralized here, in case the resources
//  need to be recreated in case of Direct3D device loss (eg. display
//  change, remoting, removal of video card, etc).
//
HRESULT D2DTimer::CreateDeviceResources()
{
    HRESULT hr = S_OK;

    if (!m_pRenderTarget)
    {

        RECT rc;
        GetClientRect(m_hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(
            rc.right - rc.left,
            rc.bottom - rc.top
        );

        // Create a Direct2D render target.
        hr = m_pD2DFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &m_pRenderTarget
        );
        if (SUCCEEDED(hr))
        {
            // Create a black brush.
            hr = m_pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::Red),
                &m_pRedBrush
            );
        }

    }

    return hr;
}


//
//  Discard device-specific resources which need to be recreated
//  when a Direct3D device is lost
//
void D2DTimer::DiscardDeviceResources()
{
    SafeRelease(&m_pRenderTarget);
    SafeRelease(&m_pRedBrush);
}


//
// The main window message loop.
//
void D2DTimer::RunMessageLoop(HINSTANCE hInstance)
{
    MSG msg;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PROJECT));

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}


//
//  Called whenever the application needs to display the client
//  window.
//
//  Note that this function will not render anything if the window
//  is occluded (e.g. when the screen is locked).
//  Also, this function will automatically discard device-specific
//  resources if the Direct3D device disappears during function
//  invocation, and will recreate the resources the next time it's
//  invoked.
//
HRESULT D2DTimer::OnRender()
{
    HRESULT hr;

    hr = CreateDeviceResources();

    if (SUCCEEDED(hr) && !(m_pRenderTarget->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
    {
        INT nText = 0;
        WCHAR* scText = nullptr;
        switch (m_eStyle)
        {
            case StyleType::Timer:
                nText = m_nTimer;
                scText = m_scTimer;
                break;
            case StyleType::Clock:
                nText = m_nClock;
                scText = m_scClock;
                break;
            case StyleType::Timestamp:
                nText = m_nTimestamp;
                scText = m_scTimestamp;
                break;
        }

        // Retrieve the size of the render target.
        D2D1_SIZE_F renderTargetSize = m_pRenderTarget->GetSize();

        m_pRenderTarget->BeginDraw();

        m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

        if (m_pTextFormat != NULL)
        {
            m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

            m_pRenderTarget->DrawText(
                scText,
                nText,
                m_pTextFormat,
                D2D1::RectF(0, 0, renderTargetSize.width, renderTargetSize.height),
                m_pRedBrush
            );
        }

        hr = m_pRenderTarget->EndDraw();

        if (hr == D2DERR_RECREATE_TARGET)
        {
            hr = S_OK;
            DiscardDeviceResources();
        }
    }

    return hr;
}


//
//  If the application receives a WM_SIZE message, this method
//  resizes the render target appropriately.
//
void D2DTimer::OnResize(UINT width, UINT height)
{
    if (m_pRenderTarget)
    {
        D2D1_SIZE_U size;
        size.width = width;
        size.height = height;

        // Note: This method can fail, but it's okay to ignore the
        // error here -- it will be repeated on the next call to
        // EndDraw.
        m_pRenderTarget->Resize(size);
    }
}


//
// The window message handler.
//
LRESULT CALLBACK D2DTimer::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    if (message == WM_CREATE)
    {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
        D2DTimer* pApp = (D2DTimer*)pcs->lpCreateParams;

        HMENU hMenu = GetMenu(hwnd);
        CheckMenuRadioItem(hMenu, IDM_STYLE_TIMER, IDM_STYLE_TIMESTAMP, IDM_STYLE_TIMER + pApp->m_eStyle, MF_BYCOMMAND | MF_CHECKED);
        CheckMenuRadioItem(hMenu, IDM_FPS_0, IDM_FPS_1, IDM_FPS_60, MF_BYCOMMAND | MF_CHECKED);

        ::SetWindowLongPtrW(
            hwnd,
            GWLP_USERDATA,
            PtrToUlong(pApp)
        );

        result = 1;
    }
    else
    {
        D2DTimer* pApp = reinterpret_cast<D2DTimer*>(static_cast<LONG_PTR>(
            ::GetWindowLongPtrW(
                hwnd,
                GWLP_USERDATA
            )));

        bool wasHandled = false;

        if (pApp)
        {
            switch (message)
            {
                case WM_SIZE:
                {
                    UINT width = LOWORD(lParam);
                    UINT height = HIWORD(lParam);
                    pApp->OnResize(width, height);
                }
                wasHandled = true;
                result = 0;
                break;

                case WM_PAINT:
                case WM_DISPLAYCHANGE:
                {
                    PAINTSTRUCT ps;
                    BeginPaint(hwnd, &ps);

                    pApp->OnRender();
                    EndPaint(hwnd, &ps);
                }
                wasHandled = true;
                result = 0;
                break;

                case WM_DESTROY:
                {
                    KillTimer(hwnd, IDT_TIMER_RENDER);
                    PostQuitMessage(0);
                }
                wasHandled = true;
                result = 1;
                break;

                case WM_TIMER:
                {
                    switch (wParam)
                    {
                        case IDT_TIMER_RENDER:
                        {
                            switch (pApp->m_eStyle)
                            {
                                case StyleType::Timer:
                                {
                                    ULONGLONG now = GetTickCount64();

                                    ULONGLONG elasped = now - pApp->m_ullTikcount;

                                    ULONGLONG temp = elasped;
                                    SHORT hours = temp / 3600000;

                                    temp %= 3600000;
                                    CHAR minutes = temp / 60000;

                                    temp %= 60000;
                                    CHAR seconds = temp / 1000;

                                    SHORT milliseconds = temp % 1000;

                                    wsprintf(pApp->m_scTimer, pApp->m_scFormatTimer, hours, minutes, seconds, milliseconds);
                                }
                                break;
                                case StyleType::Clock:
                                    SYSTEMTIME clock;
                                    GetSystemTime(&clock);
                                    wsprintf(pApp->m_scClock, pApp->m_scFormatClock, (clock.wHour + 8) % 24, clock.wMinute, clock.wSecond, clock.wMilliseconds);
                                    break;
                                case StyleType::Timestamp:
                                    long long ts = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
                                    swprintf(pApp->m_scTimestamp, pApp->m_scFormatTimestamp, (double)ts / 1000);
                                    break;
                            }

                            RECT rc;
                            GetClientRect(hwnd, &rc);
                            InvalidateRect(hwnd, &rc, FALSE);
                        }
                        wasHandled = true;
                        result = 0;
                        break;
                    }
                }
                break;

                case WM_COMMAND:
                {
                    HMENU hMenu = GetMenu(hwnd);
                    INT wmId = LOWORD(wParam);

                    // Parse the menu selections:
                    switch (wmId)
                    {
                        case IDM_STYLE_TIMER:
                        case IDM_STYLE_CLOCK:
                        case IDM_STYLE_TIMESTAMP:
                        {
                            CheckMenuRadioItem(hMenu, IDM_STYLE_TIMER, IDM_STYLE_TIMESTAMP, wmId, MF_BYCOMMAND | MF_CHECKED);
                            pApp->m_eStyle = (StyleType)(wmId - IDM_STYLE_TIMER);

                            RECT rc;
                            GetClientRect(hwnd, &rc);
                            InvalidateRect(hwnd, &rc, FALSE);
                        }
                        wasHandled = true;
                        result = 0;
                        break;

                        case IDM_FONT_INCREASE:
                        case IDM_FONT_DECREASE:
                        case IDM_FONT_DEFAULT:
                        {
                            switch (wmId)
                            {
                                case IDM_FONT_INCREASE:
                                    pApp->m_fFontSize += 1;
                                    break;
                                case IDM_FONT_DECREASE:
                                    if (pApp->m_fFontSize > 1)
                                    {
                                        pApp->m_fFontSize -= 1;
                                    }
                                    break;
                                case IDM_FONT_DEFAULT:
                                    pApp->m_fFontSize = 50;
                                    break;
                            }
                            if (pApp->m_pDWriteFactory != NULL)
                            {
                                SafeRelease(&pApp->m_pTextFormat);

                                // Create a DirectWrite text format object.
                                HRESULT hr = pApp->m_pDWriteFactory->CreateTextFormat(
                                    L"Consolas",
                                    NULL,
                                    DWRITE_FONT_WEIGHT_NORMAL,
                                    DWRITE_FONT_STYLE_NORMAL,
                                    DWRITE_FONT_STRETCH_NORMAL,
                                    pApp->m_fFontSize,
                                    L"",  // locale
                                    &pApp->m_pTextFormat
                                );
                                if (SUCCEEDED(hr))
                                {
                                    // Center the text horizontally and vertically.
                                    pApp->m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                                    pApp->m_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

                                    RECT rc;
                                    GetClientRect(hwnd, &rc);
                                    InvalidateRect(hwnd, &rc, FALSE);
                                }
                            }

                        }
                        wasHandled = true;
                        result = 0;
                        break;

                        case IDM_FPS_1:
                        case IDM_FPS_5:
                        case IDM_FPS_10:
                        case IDM_FPS_15:
                        case IDM_FPS_20:
                        case IDM_FPS_25:
                        case IDM_FPS_30:
                        case IDM_FPS_40:
                        case IDM_FPS_50:
                        case IDM_FPS_60:
                        case IDM_FPS_90:
                        case IDM_FPS_120:
                        case IDM_FPS_240:
                        {
                            pApp->m_eSpan = (FrameRateSpan)(wmId - IDM_FPS_0);

                            CheckMenuRadioItem(hMenu, IDM_FPS_0, IDM_FPS_1, wmId, MF_BYCOMMAND | MF_CHECKED);
                            GetMenuStringW(hMenu, IDM_ACTION_PAUSE, pApp->m_scPlayPause, 7, MF_BYCOMMAND);
                            if (lstrcmpW(pApp->m_scPlayPause, L"&Pause") == 0)
                            {
                                KillTimer(hwnd, IDT_TIMER_RENDER);
                                SetTimer(hwnd, IDT_TIMER_RENDER, pApp->m_eSpan, (TIMERPROC)D2DTimer::WndProc);
                            }
                        }
                        wasHandled = true;
                        result = 0;
                        break;

                        case IDM_ACTION_PAUSE:
                        {
                            KillTimer(hwnd, IDT_TIMER_RENDER);

                            GetMenuStringW(hMenu, IDM_ACTION_PAUSE, pApp->m_scPlayPause, 7, MF_BYCOMMAND);
                            if (lstrcmpW(pApp->m_scPlayPause, L"&Pause") == 0)
                            {
                                ModifyMenu(hMenu, IDM_ACTION_PAUSE, MF_BYCOMMAND | MF_STRING, IDM_ACTION_PAUSE, L"&Play");
                            }
                            else
                            {
                                pApp->m_ullTikcount = GetTickCount64();
                                ModifyMenu(hMenu, IDM_ACTION_PAUSE, MF_BYCOMMAND | MF_STRING, IDM_ACTION_PAUSE, L"&Pause");
                                SetTimer(hwnd, IDT_TIMER_RENDER, pApp->m_eSpan, (TIMERPROC)D2DTimer::WndProc);
                            }
                        }
                        wasHandled = true;
                        result = 0;
                        break;
                        case IDM_ACTION_CLEAR:
                        {
                            KillTimer(hwnd, IDT_TIMER_RENDER);

                            switch (pApp->m_eStyle)
                            {
                                case StyleType::Timer:
                                    wcscpy(pApp->m_scTimer, L"00:00:00.000");
                                    break;
                                case StyleType::Clock:
                                    wcscpy(pApp->m_scClock, L"00:00:00.000");
                                    break;
                                case StyleType::Timestamp:
                                    wcscpy(pApp->m_scTimestamp, L"0000000000.000");
                                    break;
                            }

                            ModifyMenu(hMenu, IDM_ACTION_PAUSE, MF_BYCOMMAND | MF_STRING, IDM_ACTION_PAUSE, L"&Play");

                            RECT rc;
                            GetClientRect(hwnd, &rc);
                            InvalidateRect(hwnd, &rc, FALSE);
                        }
                        wasHandled = true;
                        result = 0;
                        break;
                    }
                }
                break;
            }
        }

        if (!wasHandled)
        {
            result = DefWindowProc(hwnd, message, wParam, lParam);
        }
    }

    return result;
}

