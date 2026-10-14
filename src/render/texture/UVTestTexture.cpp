#include "render/texture/UVTestTexture.h"

#include <cstdint>

#include "render/Color.h"



namespace ve
{
    Texture2D createUVTestTexture() {
        constexpr int width = 64;
        constexpr int height = 64;
        constexpr int cellSize = 8;
        constexpr int markerSize = 16;


        Texture2D texture{ width, height };



        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const bool isWhite = ((x / cellSize) + (y / cellSize)) % 2 == 0;

                const std::uint8_t checkerColor = isWhite ? 255 : 40;

                Color32 color = makeColor(
                    checkerColor,
                    checkerColor,
                    checkerColor
                );



                // Bottom-Left: Red

                if (x < markerSize && y < markerSize) {
                    color = makeColor(255, 0, 0);
                }



                // Bottom-Right: Green

                else if (x >= width - markerSize && y < markerSize) {
                    color = makeColor(0, 255, 0);
                }



                // Top-Left: Blue

                else if (x < markerSize && y >= height - markerSize) {
                    color = makeColor(0, 0, 255);
                }



                // Top-Right: Yellow

                else if (x >= width - markerSize && y >= height - markerSize) {
                    color = makeColor(255, 255, 0);
                }



                texture.setPixel(x, y, color);
            }
        }


        return texture;
    }
}