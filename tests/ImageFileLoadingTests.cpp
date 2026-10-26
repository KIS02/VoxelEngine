#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "assets/AssetManager.h"
#include "assets/texture/ImageData.h"
#include "assets/texture/ImageLoader.h"
#include "assets/texture/Texture2D.h"
#include "math/Color.h"
#include "math/Vec2.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    void testImageData() {
        ve::ImageData image{};
        image.width = 2;
        image.height = 1;
        image.colorSpace = ve::TextureColorSpace::Linear;
        image.rgba8 = { 128, 64, 32, 73, 10, 20, 30, 255 };

        image.validate();

        const ve::Texture2D texture(image);

        require(texture.width() == 2 && texture.height() == 1, "Incorrect Texture dimensions");
        require(texture.colorSpace() == ve::TextureColorSpace::Linear, "Linear map encoding was lost");
        require(texture.pixel(0, 0) == ve::makeColor(128, 64, 32, 73), "RGBA8 channels or alpha changed");
        require(texture.pixel(1, 0) == ve::makeColor(10, 20, 30, 255), "Second texel changed");
        require(texture.revision() == 1, "Initial image should have a single Texture revision");

        ve::Texture2D movedDestination(1, 1);
        ve::Texture2D temporary(image);
        const std::uint64_t destinationId = movedDestination.id();

        movedDestination = std::move(temporary);

        require(movedDestination.id() == destinationId, "Move assignment must preserve Asset ID");
        require(movedDestination.colorSpace() == ve::TextureColorSpace::Linear,
                "Move assignment must preserve Image color space");

        image.rgba8.pop_back();

        bool rejected = false;

        try {
            image.validate();
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        require(rejected, "Malformed RGBA data must be rejected");

        std::cout << "[PASS] ImageData layout, alpha and revision\n";
    }


    void testFileTextures() {
#ifdef _WIN32
        const std::filesystem::path directory = std::filesystem::path("assets") / "textures";

        ve::AssetManager assets;

        const std::uint64_t pngId = assets.loadTexture(directory / "phase11_1_demo.png");
        const ve::Texture2D& png = assets.texture(pngId);

        require(png.width() == 32 && png.height() == 32, "Incorrect PNG dimensions");
        require(png.colorSpace() == ve::TextureColorSpace::SRGB, "PNG BaseColor must be sRGB");
        require(png.pixel(3, 3) == ve::makeColor(255, 0, 0, 73), "PNG bottom-left/alpha incorrect");
        require(png.pixel(28, 3) == ve::makeColor(0, 255, 0), "PNG bottom-right incorrect");
        require(png.pixel(3, 28) == ve::makeColor(0, 0, 255), "PNG top-left incorrect");
        require(png.pixel(28, 28) == ve::makeColor(255, 255, 0), "PNG top-right incorrect");

        require(
            png.sampleNearest(ve::Vec2{ 0.1f, 0.1f }) == ve::makeColor(255, 0, 0, 73),
            "UV origin differs from existing CPU sampling contract"
        );

        const std::uint64_t jpgId = assets.loadTexture(directory / "phase11_1_demo.jpg");
        const ve::Texture2D& jpg = assets.texture(jpgId);

        require(jpg.width() == 32 && jpg.height() == 32, "Incorrect JPEG dimensions");

        const ve::Color32 red = jpg.pixel(4, 4);

        require(
            ve::getRed(red) > 230 && ve::getGreen(red) < 30 && ve::getBlue(red) < 30,
            "JPEG RGB channels or origin are incorrect"
        );

        require(ve::getAlpha(red) == 255, "JPEG must expand to opaque RGBA8");

        const std::uint64_t linearId = assets.loadTexture(
            directory / "phase11_1_demo.png", ve::TextureColorSpace::Linear
        );

        require(
            assets.texture(linearId).colorSpace() == ve::TextureColorSpace::Linear,
            "Data texture encoding is incorrect"
        );

        bool missingRejected = false;

        try {
            (void)assets.loadTexture(directory / "missing_phase11_1.png");
        } catch (const std::runtime_error&) {
            missingRejected = true;
        }

        require(missingRejected, "Missing image must fail without creating Asset");

        std::cout << "[PASS] PNG/JPEG WIC decode, UV, colorspace and errors\n";
#else
        std::cout << "[SKIP] WIC decode is Windows-only\n";
#endif
    }
}


int main() {
    try {
        testImageData();
        testFileTextures();

        std::cout << "All image loading tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}