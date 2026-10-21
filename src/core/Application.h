#pragma once

#include <memory>

#include "render/api/RenderBackend.h"


namespace ve
{
    class Application {
        private:
            struct Impl;
            std::unique_ptr<Impl> impl_;


        public:
            explicit Application( RenderBackend backend );
            ~Application();

            Application( const Application& ) = delete;
            Application& operator=( const Application& ) = delete;

            void run( bool benchmarkMode = false );
    };
}
