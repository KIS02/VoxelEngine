#pragma once


namespace ve
{

class FrameBuffer;


class Win32FramePresenter
{
public:

    explicit Win32FramePresenter(
        void* nativeWindowHandle
    );


    void present(
        const FrameBuffer& frameBuffer
    );


private:

    void* windowHandle_;
};

}