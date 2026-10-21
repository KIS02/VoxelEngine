#pragma once

#include "assets/mesh/MeshData.h"



namespace ve
{
    class CubeMeshGenerator {
        public:
            static MeshData create( float halfExtent = 1.0f );
    };
}