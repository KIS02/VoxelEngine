#pragma once

#include <cstdint>
#include <string>


namespace ve
{
    enum class ItemId : std::uint16_t {
        None = 0,
        Grass = 1,
        Dirt = 2,
        Stone = 3,
        WoodenPickaxe = 100,
        WoodenAxe = 101,
        WoodenShovel = 102,
        CraftingTable = 103,
        Bread = 104
    };


    enum class ItemKind {
        Block,
        Tool,
        Food
    };


    struct ItemDefinition
    {
        ItemId id = ItemId::None;
        std::string name{};
        ItemKind kind = ItemKind::Block;
        std::uint16_t maxStack = 64;
    };
}
