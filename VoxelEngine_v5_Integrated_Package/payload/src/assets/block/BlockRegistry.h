#pragma once

#include "assets/block/BlockDefinition.h"


namespace ve
{
    // Shared definitions: a Chunk stores only lightweight BlockIds.
    class BlockRegistry {
        public:
            static constexpr BlockDefinition definition( BlockId id ) {
                switch (id) {
                    case BlockId::Grass: return { id, true, makeColor(80, 180, 70) };
                    case BlockId::Dirt: return { id, true, makeColor(135, 90, 50) };
                    case BlockId::Stone: return { id, true, makeColor(150, 150, 150) };
                    case BlockId::Air: return { id, false, makeColor(255, 255, 255) };
                }
                return { id, false, makeColor(255, 0, 255) };
            }
    };
}
