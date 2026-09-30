#pragma once

#include <vector>


namespace ve
{

class DepthBuffer
{
public:

    DepthBuffer(
        int width,
        int height
    );


    int width() const;

    int height() const;


    void clear(
        float depth
    );


    float depth(
        int x,
        int y
    ) const;


    void setDepth(
        int x,
        int y,
        float depth
    );


private:

    int width_;

    int height_;

    std::vector<float> depths_;
};

}