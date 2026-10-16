#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "assets/texture/Texture2D.h"
#include "render/common/MaterialCachePolicy.h"


namespace
{
    void check( bool value, const char* message ) {
        if (!value)
            throw std::runtime_error(message);
    }


    class CacheSimulator {
        private:
            std::unordered_map<std::uint64_t, std::uint64_t> revisions_;
            int created_ = 0;
            int reused_ = 0;
            int updated_ = 0;
            int released_ = 0;

        public:
            void submit( const ve::Texture2D& texture ) {
                const auto found = revisions_.find(texture.id());
                const bool exists = found != revisions_.end();
                const std::uint64_t cached = exists ? found->second : 0;
                const auto action = ve::MaterialCachePolicy::decide(exists, cached, texture.revision());

                if (action == ve::MaterialCacheAction::Create) {
                    revisions_.emplace(texture.id(), texture.revision());
                    ++created_;
                } else if (action == ve::MaterialCacheAction::Update) {
                    found->second = texture.revision();
                    ++updated_;
                } else {
                    ++reused_;
                }
            }

            void endFrame( const std::unordered_set<std::uint64_t>& activeIds ) {
                for (auto it = revisions_.begin(); it != revisions_.end();) {
                    if (ve::MaterialCachePolicy::shouldRelease(it->first, activeIds)) {
                        it = revisions_.erase(it);
                        ++released_;
                    } else {
                        ++it;
                    }
                }
            }

            int created() const { return created_; }
            int reused() const { return reused_; }
            int updated() const { return updated_; }
            int released() const { return released_; }
            std::size_t size() const { return revisions_.size(); }
        };


    void runTests() {
        using ve::MaterialCacheAction;
        using ve::MaterialCachePolicy;
        using ve::Texture2D;

        check(MaterialCachePolicy::decide(false, 0, 1) == MaterialCacheAction::Create,
              "new resource must create");
        std::cout << "[PASS] Resource creation decision\n";

        Texture2D first(4, 4);
        CacheSimulator cache;
        cache.submit(first);
        cache.submit(first);
        check(cache.created() == 1 && cache.reused() == 1 && cache.size() == 1,
              "unchanged texture must reuse cache");
        std::cout << "[PASS] Cache reuse\n";

        const std::uint64_t previous = first.revision();
        first.setPixel(0, 0, ve::makeColor(20, 30, 40));
        check(first.revision() > previous, "mutation must increase revision");
        cache.submit(first);
        check(cache.updated() == 1 && cache.size() == 1, "revision must trigger update");
        std::cout << "[PASS] Revision update\n";

        first.setPixel(0, 0, ve::makeColor(20, 30, 40));
        cache.submit(first);
        check(cache.reused() == 2 && cache.updated() == 1,
              "same pixel value must not cause update");
        std::cout << "[PASS] Unchanged pixel does not invalidate cache\n";

        Texture2D second(4, 4);
        check(first.id() != second.id(), "unique resources need unique ID");
        cache.submit(second);
        check(cache.created() == 2 && cache.size() == 2, "distinct textures must not collide");
        std::cout << "[PASS] Two-material frame\n";

        cache.endFrame({first.id(), second.id()});
        check(cache.size() == 2 && cache.released() == 0, "both active resources must survive");
        cache.endFrame({second.id()});
        check(cache.size() == 1 && cache.released() == 1, "unused resource must release");
        std::cout << "[PASS] Release unused resource\n";

        Texture2D copy(second);
        check(copy.id() != second.id(), "copy must receive new resource ID");
        cache.submit(copy);
        check(cache.created() == 3 && cache.size() == 2, "copy must create distinct cache entry");
        std::cout << "[PASS] Copied texture identity\n";

        const auto transferred = second.id();
        Texture2D moved(std::move(second));
        check(moved.id() == transferred && second.id() != transferred,
              "move must transfer resource ID");
        cache.submit(moved);
        check(cache.reused() == 3, "moved texture must reuse cache by ID");
        std::cout << "[PASS] Move transfers cache identity\n";

        cache.endFrame({moved.id()});
        check(cache.size() == 1 && cache.released() == 2,
              "end of frame must release unused copied material");
        cache.submit(copy);
        check(cache.created() == 4, "released material must re-create when used again");
        std::cout << "[PASS] Re-introduction after release\n";
    }
}


int main() {
    try {
        runTests();
        std::cout << "ALL MATERIAL CACHE POLICY TESTS PASSED\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
