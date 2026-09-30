#include "Win32FramePresenter.h"

#include "render/FrameBuffer.h"

#include <windows.h>


namespace ve
{

Win32FramePresenter::Win32FramePresenter( void* nativeWindowHandle)
    : windowHandle_(nativeWindowHandle) { }


void Win32FramePresenter::present( const FrameBuffer& frameBuffer ) {
    HWND hwnd =
        static_cast<HWND>(
            windowHandle_
        );


    HDC deviceContext =
        GetDC(hwnd);


    if (!deviceContext)
    {
        return;
    }


    RECT clientRect{};

    GetClientRect(
        hwnd,
        &clientRect
    );


    const int destinationWidth =
        clientRect.right -
        clientRect.left;


    const int destinationHeight =
        clientRect.bottom -
        clientRect.top;


    BITMAPINFO bitmapInfo{};

    bitmapInfo.bmiHeader.biSize =
        sizeof(BITMAPINFOHEADER);

    bitmapInfo.bmiHeader.biWidth =
        frameBuffer.width();

    bitmapInfo.bmiHeader.biHeight =
        -frameBuffer.height();

    bitmapInfo.bmiHeader.biPlanes =
        1;

    bitmapInfo.bmiHeader.biBitCount =
        32;

    bitmapInfo.bmiHeader.biCompression =
        BI_RGB;


    StretchDIBits(
        deviceContext,

        0,
        0,
        destinationWidth,
        destinationHeight,

        0,
        0,
        frameBuffer.width(),
        frameBuffer.height(),

        frameBuffer.data(),

        &bitmapInfo,

        DIB_RGB_COLORS,
        SRCCOPY
    );


    ReleaseDC(
        hwnd,
        deviceContext
    );
}

}