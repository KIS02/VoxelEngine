#include "Application.h"

#include <iostream>

#include <array>
#include <cstddef>
#include <cstdint>

#include "math/Mat4.h"
#include "math/Vec2.h"
#include "math/Vec3.h"

#include "render/Color.h"
#include "render/RasterVertex.h"
#include "render/Texture2D.h"
#include "render/Vertex.h"
#include "render/VertexProcessor.h"

namespace ve
{

Application::Application()
    :
    window_(      800, 600, "Voxel Engine" ),
    frameBuffer_( 800, 600 ),
    depthBuffer_( 800, 600 ),
    viewport_(    800, 600 ),
    rasterizer_( frameBuffer_, depthBuffer_ ),
    presenter_( window_.nativeHandle() )
{
}


void Application::run()
{
    std::cout
        << "Voxel Engine Started"
        << '\n';


    while (!window_.shouldClose())
    {
        window_.pollEvents();


        const float deltaTime =
            timer_.tick();


        processInput();


        update(deltaTime);


        render();


        window_.swapBuffers();
    }


    std::cout
        << "Voxel Engine Shutdown"
        << '\n';
}


void Application::processInput()
{
    if (window_.isKeyPressed(Key::Escape))
    {
        window_.requestClose();
    }
}


void Application::update(float deltaTime)
{
    // 아직 Game Logic 없음.

    (void)deltaTime;
}

void Application::render()
{
    frameBuffer_.clear(
        makeColor(
            0,
            0,
            0
        )
    );


    depthBuffer_.clear(
        1.0f
    );


    static const Texture2D texture =
        Texture2D::checkerboard(
            64,
            64,
            8,

            makeColor(
                255,
                255,
                255
            ),

            makeColor(
                40,
                40,
                40
            )
        );


    const Color32 white =
        makeColor(
            255,
            255,
            255
        );


    const std::array<Vertex, 24> vertices{

        // Front
        Vertex{
            Vec3{
                -1.0f,
                -1.0f,
                1.0f
            },
            white,
            Vec2{
                0.0f,
                1.0f
            }
        },

        Vertex{
            Vec3{
                1.0f,
                -1.0f,
                1.0f
            },
            white,
            Vec2{
                1.0f,
                1.0f
            }
        },

        Vertex{
            Vec3{
                1.0f,
                1.0f,
                1.0f
            },
            white,
            Vec2{
                1.0f,
                0.0f
            }
        },

        Vertex{
            Vec3{
                -1.0f,
                1.0f,
                1.0f
            },
            white,
            Vec2{
                0.0f,
                0.0f
            }
        },


        // Back
        Vertex{
            Vec3{
                -1.0f,
                -1.0f,
                -1.0f
            },
            white,
            Vec2{
                0.0f,
                1.0f
            }
        },

        Vertex{
            Vec3{
                -1.0f,
                1.0f,
                -1.0f
            },
            white,
            Vec2{
                1.0f,
                1.0f
            }
        },

        Vertex{
            Vec3{
                1.0f,
                1.0f,
                -1.0f
            },
            white,
            Vec2{
                1.0f,
                0.0f
            }
        },

        Vertex{
            Vec3{
                1.0f,
                -1.0f,
                -1.0f
            },
            white,
            Vec2{
                0.0f,
                0.0f
            }
        },


        // Left
        Vertex{
            Vec3{
                -1.0f,
                -1.0f,
                -1.0f
            },
            white,
            Vec2{
                0.0f,
                1.0f
            }
        },

        Vertex{
            Vec3{
                -1.0f,
                -1.0f,
                1.0f
            },
            white,
            Vec2{
                1.0f,
                1.0f
            }
        },

        Vertex{
            Vec3{
                -1.0f,
                1.0f,
                1.0f
            },
            white,
            Vec2{
                1.0f,
                0.0f
            }
        },

        Vertex{
            Vec3{
                -1.0f,
                1.0f,
                -1.0f
            },
            white,
            Vec2{
                0.0f,
                0.0f
            }
        },


        // Right
        Vertex{
            Vec3{
                1.0f,
                -1.0f,
                1.0f
            },
            white,
            Vec2{
                0.0f,
                1.0f
            }
        },

        Vertex{
            Vec3{
                1.0f,
                -1.0f,
                -1.0f
            },
            white,
            Vec2{
                1.0f,
                1.0f
            }
        },

        Vertex{
            Vec3{
                1.0f,
                1.0f,
                -1.0f
            },
            white,
            Vec2{
                1.0f,
                0.0f
            }
        },

        Vertex{
            Vec3{
                1.0f,
                1.0f,
                1.0f
            },
            white,
            Vec2{
                0.0f,
                0.0f
            }
        },


        // Top
        Vertex{
            Vec3{
                -1.0f,
                1.0f,
                1.0f
            },
            white,
            Vec2{
                0.0f,
                1.0f
            }
        },

        Vertex{
            Vec3{
                1.0f,
                1.0f,
                1.0f
            },
            white,
            Vec2{
                1.0f,
                1.0f
            }
        },

        Vertex{
            Vec3{
                1.0f,
                1.0f,
                -1.0f
            },
            white,
            Vec2{
                1.0f,
                0.0f
            }
        },

        Vertex{
            Vec3{
                -1.0f,
                1.0f,
                -1.0f
            },
            white,
            Vec2{
                0.0f,
                0.0f
            }
        },


        // Bottom
        Vertex{
            Vec3{
                -1.0f,
                -1.0f,
                -1.0f
            },
            white,
            Vec2{
                0.0f,
                1.0f
            }
        },

        Vertex{
            Vec3{
                1.0f,
                -1.0f,
                -1.0f
            },
            white,
            Vec2{
                1.0f,
                1.0f
            }
        },

        Vertex{
            Vec3{
                1.0f,
                -1.0f,
                1.0f
            },
            white,
            Vec2{
                1.0f,
                0.0f
            }
        },

        Vertex{
            Vec3{
                -1.0f,
                -1.0f,
                1.0f
            },
            white,
            Vec2{
                0.0f,
                0.0f
            }
        }
    };


    const std::array<std::uint32_t, 36> indices{
        0, 1, 2,
        0, 2, 3,

        4, 5, 6,
        4, 6, 7,

        8, 9, 10,
        8, 10, 11,

        12, 13, 14,
        12, 14, 15,

        16, 17, 18,
        16, 18, 19,

        20, 21, 22,
        20, 22, 23
    };


    constexpr float pi =
        3.14159265358979323846f;


    constexpr float rotationXDegrees =
        25.0f;


    constexpr float rotationYDegrees =
        35.0f;


    constexpr float rotationXRadians =
        rotationXDegrees
        *
        pi
        /
        180.0f;


    constexpr float rotationYRadians =
        rotationYDegrees
        *
        pi
        /
        180.0f;


    const Mat4 rotationX =
        Mat4::rotationX(
            rotationXRadians
        );


    const Mat4 rotationY =
        Mat4::rotationY(
            rotationYRadians
        );


    const Mat4 translation =
        Mat4::translation(
            Vec3{
                0.0f,
                0.0f,
                -5.0f
            }
        );


    const Mat4 model =
        translation
        *
        rotationY
        *
        rotationX;


    const Mat4 view =
        Mat4::identity();


    constexpr float fovYDegrees =
        90.0f;


    constexpr float fovYRadians =
        fovYDegrees
        *
        pi
        /
        180.0f;


    const float aspectRatio =
        static_cast<float>(
            frameBuffer_.width()
        )
        /
        static_cast<float>(
            frameBuffer_.height()
        );


    const Mat4 projection =
        Mat4::perspective(
            fovYRadians,
            aspectRatio,
            0.1f,
            100.0f
        );


    std::array<RasterVertex, 24> rasterVertices{};


    for (
        std::size_t i = 0;
        i < vertices.size();
        ++i
    )
    {
        rasterVertices[i] =
            VertexProcessor::process(
                vertices[i],
                model,
                view,
                projection,
                viewport_
            );
    }


    for (
        std::size_t i = 0;
        i < indices.size();
        i += 3
    )
    {
        const std::uint32_t indexA =
            indices[i];


        const std::uint32_t indexB =
            indices[i + 1];


        const std::uint32_t indexC =
            indices[i + 2];


        rasterizer_.drawTriangleTextured(
            rasterVertices[indexA],
            rasterVertices[indexB],
            rasterVertices[indexC],
            texture
        );
    }


    presenter_.present(
        frameBuffer_
    );
}

}