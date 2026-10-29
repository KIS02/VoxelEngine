#pragma once

#include "assets/block/BlockDefinition.h"


namespace ve
{
    class BlockRegistry {
        public:
            static constexpr BlockDefinition definition( BlockId id ) {
                switch (id) {
                    case BlockId::Grass:
                        return { id, true, makeColor(80, 180, 70), {
                            BlockMaterialKey::GrassSide, BlockMaterialKey::GrassSide,
                            BlockMaterialKey::GrassSide, BlockMaterialKey::GrassSide,
                            BlockMaterialKey::GrassTop, BlockMaterialKey::Dirt
                        } };

                    case BlockId::Dirt:
                        return { id, true, makeColor(135, 90, 50), {
                            BlockMaterialKey::Dirt, BlockMaterialKey::Dirt,
                            BlockMaterialKey::Dirt, BlockMaterialKey::Dirt,
                            BlockMaterialKey::Dirt, BlockMaterialKey::Dirt
                        } };

                    case BlockId::Stone:
                        return { id, true, makeColor(150, 150, 150), {
                            BlockMaterialKey::Stone, BlockMaterialKey::Stone,
                            BlockMaterialKey::Stone, BlockMaterialKey::Stone,
                            BlockMaterialKey::Stone, BlockMaterialKey::Stone
                        } };

                    case BlockId::CraftingTable:
                        return { id, true, makeColor(145, 105, 65), {
                            BlockMaterialKey::Dirt, BlockMaterialKey::Dirt,
                            BlockMaterialKey::Stone, BlockMaterialKey::Stone,
                            BlockMaterialKey::Stone, BlockMaterialKey::Dirt
                        } };

                    case BlockId::Air:
                        return { id, false, makeColor(255, 255, 255), {} };
                }

                return { id, false, makeColor(255, 0, 255), {} };
            }


            static constexpr BlockMaterialKey material( BlockId block, BlockFace face ) {
                return definition(block).materials.forFace(face);
            }
    };
}