#pragma once

#include "assets/mesh/MeshData.h"


namespace ve
{
    class CylinderMeshGenerator {
        public:
            static MeshData create( float radius = 1.0f, float halfHeight = 1.0f, int segments = 16 );
    };
}
