#include <cassert>
#include <cmath>
#include <stdexcept>

#include "scene/camera/Camera.h"


namespace
{
    bool nearlyEqual( float a, float b ) {
        return std::abs(a - b) < 0.0001f;
    }
}


int main() {
    ve::Camera camera;
    camera.setPosition(ve::Vec3{ 2.0f, 3.0f, 4.0f });
    const ve::Mat4 originalView = camera.viewMatrix();
    const ve::Mat4 perspective = camera.projectionMatrix(4.0f / 3.0f);
    assert(nearlyEqual(perspective(3, 2), -1.0f));

    camera.setProjectionMode(ve::Camera::ProjectionMode::Orthographic);
    camera.setOrthographicHeight(10.0f);
    const ve::Mat4 ortho = camera.projectionMatrix(4.0f / 3.0f);
    assert(nearlyEqual(ortho(0, 0), 0.15f));
    assert(nearlyEqual(ortho(1, 1), 0.2f));
    assert(nearlyEqual(ortho(3, 3), 1.0f));
    assert(nearlyEqual(ortho(3, 2), 0.0f));

    const ve::Mat4 newView = camera.viewMatrix();
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++)
            assert(nearlyEqual(originalView(row, col), newView(row, col)));
    }

    bool rejected = false;
    try {
        camera.setOrthographicHeight(0.0f);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);

    rejected = false;
    try {
        camera.projectionMatrix(0.0f);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);
    return 0;
}
