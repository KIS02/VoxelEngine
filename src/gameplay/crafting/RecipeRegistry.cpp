#include "gameplay/crafting/RecipeRegistry.h"

#include <algorithm>
#include <initializer_list>
#include <set>
#include <stdexcept>
#include <string>

#include "gameplay/core/ContentTextParser.h"


namespace ve
{
    namespace
    {
        CraftingRecipe recipe( int width, std::initializer_list<ItemId> pattern,
                               ItemId result, std::uint16_t count ) {
            CraftingRecipe value{};
            value.width = width;
            std::copy(pattern.begin(), pattern.end(), value.pattern.begin());
            value.result = ItemStack{ result, count };
            return value;
        }

        std::string signature( const CraftingRecipe& recipe ) {
            std::string signature = std::to_string(recipe.width) + ':';
            for (int i = 0; i < recipe.width * recipe.width; ++i)
                signature += std::to_string(static_cast<int>(recipe.pattern[i])) + ',';
            return signature;
        }
    }

    RecipeRegistry::RecipeRegistry() {
        recipes_.push_back(recipe(2, { ItemId::Dirt, ItemId::Dirt, ItemId::Dirt, ItemId::Dirt }, ItemId::Grass, 1));
        recipes_.push_back(recipe(2, { ItemId::Stone, ItemId::Stone, ItemId::Stone, ItemId::Stone }, ItemId::CraftingTable, 1));
        recipes_.push_back(recipe(2, { ItemId::Grass, ItemId::Grass, ItemId::None, ItemId::None }, ItemId::Bread, 2));
        recipes_.push_back(recipe(3, { ItemId::Grass, ItemId::Grass, ItemId::Grass,
            ItemId::None, ItemId::Dirt, ItemId::None, ItemId::None, ItemId::Dirt, ItemId::None }, ItemId::WoodenPickaxe, 1));
        recipes_.push_back(recipe(3, { ItemId::Grass, ItemId::Grass, ItemId::None,
            ItemId::Grass, ItemId::Dirt, ItemId::None, ItemId::None, ItemId::Dirt, ItemId::None }, ItemId::WoodenAxe, 1));
        recipes_.push_back(recipe(3, { ItemId::None, ItemId::Grass, ItemId::None,
            ItemId::None, ItemId::Dirt, ItemId::None, ItemId::None, ItemId::Dirt, ItemId::None }, ItemId::WoodenShovel, 1));
    }

    bool RecipeRegistry::loadFile( const std::filesystem::path& file, const ItemRegistry& items ) {
        std::vector<CraftingRecipe> next;
        std::set<std::string> seen;
        const bool loaded = content::readLines(file, [&]( const std::vector<std::string>& fields ) {
            if (fields.size() != 4)
                throw std::invalid_argument("Expected width|comma-separated slots|resultItemId|count");
            CraftingRecipe entry{};
            entry.width = content::parseInt(fields[0]);
            if (entry.width != 2 && entry.width != 3)
                throw std::invalid_argument("Only 2x2 and 3x3 recipes are supported");
            const auto ids = content::split(fields[1], ',');
            if (ids.size() != static_cast<std::size_t>(entry.width * entry.width))
                throw std::invalid_argument("Recipe slot count does not match grid width");
            bool anyIngredient = false;
            for (std::size_t i = 0; i < ids.size(); ++i) {
                const int id = content::parseInt(ids[i]);
                if (id < 0 || id > 65535 || (id && !items.contains(static_cast<ItemId>(id))))
                    throw std::invalid_argument("Invalid ingredient ItemId");
                entry.pattern[i] = static_cast<ItemId>(id);
                anyIngredient |= id != 0;
            }
            const int result = content::parseInt(fields[2]);
            const int count = content::parseInt(fields[3]);
            if (!anyIngredient || result <= 0 || result > 65535 ||
                !items.contains(static_cast<ItemId>(result)) || count < 1 ||
                count > items.definition(static_cast<ItemId>(result)).maxStack)
                throw std::invalid_argument("Invalid crafting result or ingredients");
            entry.result = ItemStack{ static_cast<ItemId>(result), static_cast<std::uint16_t>(count) };
            if (!seen.insert(signature(entry)).second)
                throw std::invalid_argument("Duplicate shaped recipe");
            next.push_back(entry);
            if (next.size() > 512)
                throw std::invalid_argument("Too many recipes");
        });
        if (loaded)
            recipes_ = std::move(next);
        return loaded;
    }

    std::optional<ItemStack> RecipeRegistry::match( const std::array<std::optional<ItemStack>, 9>& grid,
                                                    int width ) const {
        for (const auto& recipe : recipes_) {
            if (recipe.width != width)
                continue;
            bool valid = true;
            for (int i = 0; i < width * width; ++i) {
                const ItemId actual = grid[i] ? grid[i]->id : ItemId::None;
                if (actual != recipe.pattern[i]) {
                    valid = false;
                    break;
                }
            }
            if (valid)
                return recipe.result;
        }
        return std::nullopt;
    }

    std::size_t RecipeRegistry::size() const {
        return recipes_.size();
    }
}
