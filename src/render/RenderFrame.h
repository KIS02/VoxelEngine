#pragma once

#include <cstdint>
#include <vector>

#include "math/Mat4.h"
#include "math/Vec3.h"
#include "render/Color.h"



namespace ve
{
    class MeshData;



    struct RenderDraw
    {
        const MeshData* mesh = nullptr;

        Mat4 model = Mat4::identity();

        std::uint64_t meshRevision = 0;
        std::uint64_t meshId = 0;
    };



    struct CrosshairSettings
    {
        bool enabled = false;

        Color32 color = makeColor(255, 255, 255);

        int armLength = 7;
        int thickness = 3;
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

        std::vector<RenderDraw> draws{};

        CrosshairSettings crosshair{};
        SelectionOutlineSettings selectionOutline{};
    };
}
