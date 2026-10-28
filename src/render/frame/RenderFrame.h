#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "math/Mat4.h"
#include "math/Vec3.h"
#include "math/Color.h"

#include "render/lighting/Lighting.h"
#include "render/color/ToneMapping.h"
#include "ui/UIFrameData.h"


namespace ve
{
    class MeshData;
    struct MaterialData;


    struct RenderDraw
    {
        const MeshData* mesh = nullptr;

        Mat4 model = Mat4::identity();

        std::uint64_t meshRevision = 0;
        std::uint64_t meshId = 0;

        const MaterialData* material = nullptr;
    };


    struct SelectionOutlineSettings
    {
        bool enabled = false;

        Vec3 blockMin{};
        Color32 color = makeColor(255, 215, 40);

        float expansion = 0.005f;
    };


    struct RenderFrame
    {
        Color32 clearColor = makeColor(20, 31, 77);

        Mat4 view = Mat4::identity();
        Mat4 projection = Mat4::identity();
        Vec3 cameraPosition{};

        LightingSettings lighting{};
        ToneMappingSettings toneMapping{};

        std::vector<RenderDraw> draws{};

        UIFrameData ui{};
        SelectionOutlineSettings selectionOutline{};
    };
}
