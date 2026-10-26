#include "assets/texture/ImageLoader.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <objbase.h>
#include <wincodec.h>
#endif


namespace ve
{
    namespace
    {
        std::string lowercaseExtension( const std::filesystem::path& path ) {
            std::string extension = path.extension().string();

            std::transform(extension.begin(), extension.end(), extension.begin(), []( unsigned char value ) {
                return static_cast<char>(std::tolower(value));
            });

            return extension;
        }


#ifdef _WIN32
        template<typename T>
        class ComHandle {
            private:
                T* pointer_ = nullptr;


            public:
                ComHandle() = default;

                ~ComHandle() {
                    if (pointer_ != nullptr)
                        pointer_->Release();
                }

                ComHandle( const ComHandle& ) = delete;
                ComHandle& operator=( const ComHandle& ) = delete;


                T* get() const {
                    return pointer_;
                }

                T** put() {
                    return &pointer_;
                }
        };


        class ComApartment {
            private:
                bool shouldUninitialize_ = false;


            public:
                ComApartment() {
                    const HRESULT result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

                    if (result == RPC_E_CHANGED_MODE)
                        return;

                    if (FAILED(result))
                        throw std::runtime_error("Failed to initialize COM for image decoding");

                    shouldUninitialize_ = true;
                }

                ~ComApartment() {
                    if (shouldUninitialize_)
                        CoUninitialize();
                }

                ComApartment( const ComApartment& ) = delete;
                ComApartment& operator=( const ComApartment& ) = delete;
        };


        void ensureSuccess( HRESULT result, const char* operation ) {
            if (FAILED(result))
                throw std::runtime_error(operation);
        }
#endif
    }


    ImageData ImageLoader::load( const std::filesystem::path& path, TextureColorSpace colorSpace ) {
        const std::string extension = lowercaseExtension(path);

        if (extension != ".png" && extension != ".jpg" && extension != ".jpeg")
            throw std::invalid_argument("ImageLoader supports PNG and JPEG files only");

        if (!std::filesystem::is_regular_file(path))
            throw std::runtime_error("Image file was not found: " + path.string());

#ifdef _WIN32
        ComApartment apartment;

        ComHandle<IWICImagingFactory> factory;

#if defined(_MSC_VER)
        ensureSuccess(
            CoCreateInstance(
                CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                __uuidof(IWICImagingFactory), reinterpret_cast<void**>(factory.put())
            ),
            "Could not create WIC imaging factory"
        );
#else
        ensureSuccess(
            CoCreateInstance(
                CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                IID_IWICImagingFactory, reinterpret_cast<void**>(factory.put())
            ),
            "Could not create WIC imaging factory"
        );
#endif

        ComHandle<IWICBitmapDecoder> decoder;

        ensureSuccess(
            factory.get()->CreateDecoderFromFilename(
                path.c_str(), nullptr, GENERIC_READ,
                WICDecodeMetadataCacheOnLoad, decoder.put()
            ),
            "Could not decode PNG/JPEG image"
        );

        ComHandle<IWICBitmapFrameDecode> frame;

        ensureSuccess(
            decoder.get()->GetFrame(0, frame.put()),
            "Image does not contain a valid frame"
        );

        UINT width = 0;
        UINT height = 0;

        ensureSuccess(
            frame.get()->GetSize(&width, &height),
            "Could not read image dimensions"
        );

        constexpr std::uint64_t MaxPixels = 64ull * 1024ull * 1024ull;

        if (width == 0 || height == 0 ||
            width > static_cast<UINT>(std::numeric_limits<int>::max()) ||
            height > static_cast<UINT>(std::numeric_limits<int>::max()) ||
            static_cast<std::uint64_t>(width) * height > MaxPixels)
            throw std::invalid_argument("Image dimensions are invalid or too large");

        ComHandle<IWICFormatConverter> converter;

        ensureSuccess(
            factory.get()->CreateFormatConverter(converter.put()),
            "Could not create WIC format converter"
        );

        ensureSuccess(
            converter.get()->Initialize(
                frame.get(), GUID_WICPixelFormat32bppRGBA,
                WICBitmapDitherTypeNone, nullptr, 0.0,
                WICBitmapPaletteTypeCustom
            ),
            "Could not convert image pixels to straight RGBA8"
        );

        const UINT stride = width * 4;
        const UINT byteCount = stride * height;

        std::vector<std::uint8_t> topDown(static_cast<std::size_t>(byteCount));

        ensureSuccess(
            converter.get()->CopyPixels(nullptr, stride, byteCount, topDown.data()),
            "Could not copy decoded image pixels"
        );

        ImageData image{};

        image.width = static_cast<int>(width);
        image.height = static_cast<int>(height);
        image.colorSpace = colorSpace;
        image.rgba8.resize(topDown.size());

        const std::size_t rowBytes = static_cast<std::size_t>(stride);

        for (UINT y = 0; y < height; ++y) {
            const std::size_t source = static_cast<std::size_t>(y) * rowBytes;
            const std::size_t destination = static_cast<std::size_t>(height - y - 1) * rowBytes;

            std::copy_n(topDown.data() + source, rowBytes, image.rgba8.data() + destination);
        }

        image.validate();
        return image;
#else
        (void)colorSpace;
        throw std::runtime_error("ImageLoader requires Windows Imaging Component (Windows)");
#endif
    }
}