#include <iostream>
#include <stdexcept>

#include "assets/material/MaterialData.h"
#include "assets/texture/Texture2D.h"
#include "render/api/RenderFrame.h"


int main() {
    try {
        ve::Texture2D texture(1, 1);
        ve::MaterialData material{ &texture };

        ve::RenderDraw draw{};
        draw.material = &material;

        if (draw.material != &material || draw.material->texture != &texture)
            throw std::runtime_error("Material texture binding was not preserved");

        ve::RenderDraw legacyDraw{};
        if (legacyDraw.material != nullptr)
            throw std::runtime_error("Legacy RenderDraw default material must be null");

        std::cout << "[PASS] Shared MaterialData and RenderDraw binding\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
