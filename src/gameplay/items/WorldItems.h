#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

#include "gameplay/items/Inventory.h"
#include "math/Vec3.h"


namespace ve
{
    struct WorldItem
    {
        std::uint64_t id = 0;
        ItemStack stack{};
        Vec3 position{};
        float pickupDelay = 0.65f;
    };


    class WorldItems {
        private:
            std::vector<WorldItem> drops_{};
            std::uint64_t nextId_ = 1;


        public:
            const std::vector<WorldItem>& all() const {
                return drops_;
            }

            void restore( std::vector<WorldItem> saved ) {
                if (saved.size() > 4096)
                    throw std::invalid_argument("Too many saved world items");
                std::uint64_t next = 1;
                for (const auto& item : saved) {
                    item.stack.validate(*registry_);
                    if (item.id == 0 || item.id == std::numeric_limits<std::uint64_t>::max() ||
                        !std::isfinite(item.position.x) || !std::isfinite(item.position.y) ||
                        !std::isfinite(item.position.z))
                        throw std::invalid_argument("Invalid saved world item");
                    next = std::max(next, item.id + 1);
                }
                std::sort(saved.begin(), saved.end(), []( const WorldItem& a, const WorldItem& b ) {
                    return a.id < b.id;
                });
                for (std::size_t i = 1; i < saved.size(); ++i) {
                    if (saved[i - 1].id == saved[i].id)
                        throw std::invalid_argument("Duplicate saved world item ID");
                }
                drops_ = std::move(saved);
                nextId_ = next;
            }

            bool remove( std::uint64_t id ) {
                const std::size_t size = drops_.size();
                std::erase_if(drops_, [&]( const WorldItem& item ) { return item.id == id; });
                return drops_.size() != size;
            }

            std::uint64_t spawn( ItemStack stack, const Vec3& position ) {
                stack.validate(registryForSpawn());
                if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
                    throw std::invalid_argument("Non-finite world item position");
                if (drops_.size() >= 4096)
                    throw std::length_error("Too many world items to save");
                if (nextId_ == 0 || nextId_ == std::numeric_limits<std::uint64_t>::max())
                    throw std::overflow_error("World item IDs exhausted");
                const std::uint64_t id = nextId_;
                drops_.push_back(WorldItem{ id, std::move(stack), position });
                ++nextId_;
                return id;
            }

            // The registry is explicitly bound once to prevent accepting unknown item kinds.
            explicit WorldItems( const ItemRegistry& registry ) : registry_(&registry) { }

            std::vector<ItemStack> advance( float dt, const Vec3& playerEye,
                                           Inventory& inventory, bool canCollect ) {
                std::vector<ItemStack> collected;
                if (!std::isfinite(dt) || dt < 0.0f)
                    throw std::invalid_argument("Invalid item tick duration");
                for (auto& item : drops_) {
                    item.pickupDelay = std::max(0.0f, item.pickupDelay - dt);
                    if (!canCollect || item.pickupDelay > 0.0f)
                        continue;
                    const Vec3 delta = item.position - playerEye;
                    if (delta.x * delta.x + delta.y * delta.y + delta.z * delta.z > 2.25f)
                        continue;
                    const std::uint16_t before = item.stack.count;
                    item.stack.count = inventory.insert(item.stack);
                    if (item.stack.count < before)
                        collected.push_back(ItemStack{ item.stack.id,
                            static_cast<std::uint16_t>(before - item.stack.count), item.stack.metadata });
                }
                std::erase_if(drops_, []( const WorldItem& item ) { return item.stack.count == 0; });
                return collected;
            }

        private:
            const ItemRegistry* registry_ = nullptr;
            const ItemRegistry& registryForSpawn() const {
                return *registry_;
            }
    };
}
