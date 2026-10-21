#pragma once

#include "voxel/block/BlockId.h"


namespace ve
{
    inline const char* blockTypeName( BlockId type ) {
        switch (type) {
            case BlockId::Air: return "Air";
            case BlockId::Grass: return "Grass";
            case BlockId::Dirt: return "Dirt";
            case BlockId::Stone: return "Stone";
        }

        return "Unknown";
    }
}
