#pragma once

#include "assets/mesh/MeshData.h"



namespace ve
{
    class CubeMeshFactory {
        public:
            static MeshData create( float halfExtent = 1.0f );
    };
}