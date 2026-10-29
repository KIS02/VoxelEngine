#pragma once

#include <array>
#include <filesystem>
#include <optional>
#include <vector>

#include "gameplay/items/ItemStack.h"


namespace ve
{
    struct CraftingRecipe
    {
        int width = 2;
        std::array<ItemId, 9> pattern{};
        ItemStack result{};
    };


    class RecipeRegistry {
        private:
            std::vector<CraftingRecipe> recipes_{};

        public:
            RecipeRegistry();
            bool loadFile( const std::filesystem::path& file, const ItemRegistry& items );
            std::optional<ItemStack> match( const std::array<std::optional<ItemStack>, 9>& grid,
                                            int width ) const;
            std::size_t size() const;
    };
}
