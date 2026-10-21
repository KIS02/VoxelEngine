#pragma once

#include <cstdint>
#include <stdexcept>


namespace ve
{
    class MeshIdGenerator {
        private:
            std::uint64_t nextId_ = 1;


        public:
            std::uint64_t generate() {
                if (nextId_ == 0)
                    throw std::overflow_error("Mesh ID space exhausted");
                return nextId_++;
            }
    };
}
