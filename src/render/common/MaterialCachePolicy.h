#pragma once

#include <cstdint>
#include <unordered_set>


namespace ve
{
    enum class MaterialCacheAction
    {
        Create,
        Reuse,
        Update
    };


    struct MaterialCachePolicy
    {
        static constexpr MaterialCacheAction decide( bool found, std::uint64_t cachedRevision,
                                                     std::uint64_t sourceRevision ) {
            if (!found)
                return MaterialCacheAction::Create;

            if (cachedRevision == sourceRevision)
                return MaterialCacheAction::Reuse;

            return MaterialCacheAction::Update;
        }

        static bool shouldRelease( std::uint64_t resourceId,
                                   const std::unordered_set<std::uint64_t>& activeIds ) {
            return activeIds.find(resourceId) == activeIds.end();
        }
    };
}
