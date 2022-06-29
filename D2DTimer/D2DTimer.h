#pragma once

#include "resource.h"


// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER              // Allow use of features specific to Windows 7 or later.
#define WINVER 0x0700       // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT        // Allow use of features specific to Windows 7 or later.
#define _WIN32_WINNT 0x0700 // Change this to the appropriate value to target other versions of Windows.
#endif


#ifndef UNICODE
#define UNICODE
#endif

#define VC_EXTRALEAN     // Exclude rarely-used stuff from visual c++ headers
#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers

// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#include <math.h>

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>

/******************************************************************
*                                                                 *
*  Macros                                                         *
*                                                                 *
******************************************************************/

template<class Interface>
inline void
SafeRelease(
    Interface** ppInterfaceToRelease
)
{
    if (*ppInterfaceToRelease != NULL)
    {
        (*ppInterfaceToRelease)->Release();

        (*ppInterfaceToRelease) = NULL;
    }
}

#ifndef Assert
#if defined( DEBUG ) || defined( _DEBUG )
#define Assert(b) if (!(b)) {OutputDebugStringA("Assert: " #b "\n");}
#else
#define Assert(b)
#endif //DEBUG || _DEBUG
#endif


#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif


enum StyleType
{
    Timer,
    Clock,
    Timestamp,
};


enum FrameRateSpan
{
    By1 = 1000,
    By5 = 200,
    By10 = 100,
    By15 = 66,
    By20 = 50,
    By25 = 40,
    By30 = 33,
    By40 = 25,
    By50 = 20,
    By60 = 16,
    By90 = 11,
    By120 = 8,
    By240 = 4,
};


/******************************************************************
*                                                                 *
*  D2DTimer                                                        *
*                                                                 *
******************************************************************/

class D2DTimer
{
public:
    D2DTimer();
    ~D2DTimer();

    HRESULT Initialize();

    void RunMessageLoop(HINSTANCE hInstance);

private:
    HRESULT CreateDeviceIndependentResources();
    HRESULT CreateDeviceResources();
    void DiscardDeviceResources();

    HRESULT OnRender();

    void OnResize(
        UINT width,
        UINT height
    );

    static LRESULT CALLBACK WndProc(
        HWND hWnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam
    );

private:
    HWND m_hwnd;
    
    ID2D1Factory* m_pD2DFactory;
    IDWriteFactory* m_pDWriteFactory;
    ID2D1HwndRenderTarget* m_pRenderTarget;
    IDWriteTextFormat* m_pTextFormat;
    ID2D1SolidColorBrush* m_pRedBrush;

    StyleType m_eStyle;
    FrameRateSpan m_eSpan;

    FLOAT m_fFontSize;

    INT m_nTimer = 12;
    WCHAR m_scTimer[13];
    WCHAR m_scFormatTimer[20];

    INT m_nClock = 12;
    WCHAR m_scClock[13];
    WCHAR m_scFormatClock[20];

    INT m_nTimestamp = 14;
    WCHAR m_scTimestamp[15];
    WCHAR m_scFormatTimestamp[7];

    WCHAR m_scPlayPause[6];

    static ULONGLONG m_ullTickcount;
    static ULONGLONG m_ullLastTickcount;

    INT m_LastStyle;
    INT m_LastFrameRate;
};
