#include <cmath>
#include <iostream>
#include <stdexcept>

#include "render/backends/software/FrameBuffer.h"
#include "render/backends/software/resources/HdrColorBuffer.h"
#include "render/color/ColorSpace.h"


namespace
{
    void require( bool valid, const char* message ) {
        if (!valid)
            throw std::runtime_error(message);
    }
}


int main() {
    try {
        ve::HdrColorBuffer hdr(2, 1);
        hdr.clear(ve::HdrPixel{ ve::LinearRGB{ 0.0f, 0.0f, 0.0f }, 255 });
        hdr.setPixel(0, 0, ve::HdrPixel{ ve::LinearRGB{ 4.0f, 2.0f, 1.0f }, 73 });
        require(hdr.pixel(0, 0).color.r == 4.0f, "HDR buffer clamped the intermediate result");
        ve::FrameBuffer sdr(2, 1);
        const ve::ToneMappingSettings settings{ true, 1.0f, ve::ToneMappingOperator::Reinhard };
        hdr.resolve(sdr, settings);
        require(ve::getAlpha(sdr.pixel(0, 0)) == 73, "Alpha was not preserved");
        require(ve::getRed(sdr.pixel(0, 0)) > ve::getGreen(sdr.pixel(0, 0)), "Channel ordering is wrong");
        require(ve::getRed(sdr.pixel(1, 0)) == 0, "Cleared pixel is not black");
        std::cout << "[PASS] HDR storage and SDR resolution\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
