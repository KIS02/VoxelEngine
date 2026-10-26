#pragma once

#include "math/Vec3.h"
#include "math/Color.h"
#include "render/backends/software/RasterVertex.h"


namespace ve
{
    class Viewport {
        private:
            int width_;
            int height_;


        public:
            Viewport( int width, int height ) : width_(width), height_(height) { }


            RasterVertex transform( const Vec3& ndcPosition, Color32 color ) const {
                const float screenX =
                    (ndcPosition.x + 1.0f) * 0.5f * static_cast<float>(width_);

                const float screenY =
                    (1.0f - ndcPosition.y) * 0.5f * static_cast<float>(height_);

                const float windowDepth = (ndcPosition.z + 1.0f) * 0.5f;

                return RasterVertex{
                    Vec2{ screenX, screenY },
                    windowDepth,
                    color
                };
            }


            int width() const {
                return width_;
            }

            int height() const {
                return height_;
            }
    };
}