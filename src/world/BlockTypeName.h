#pragma once

#include "world/BlockType.h"


namespace ve
{
    inline const char* blockTypeName( BlockType type ) {
        switch (type) {
            case BlockType::Air: return "Air";
            case BlockType::Grass: return "Grass";
            case BlockType::Dirt: return "Dirt";
            case BlockType::Stone: return "Stone";
        }

        return "Unknown";
    }
}
