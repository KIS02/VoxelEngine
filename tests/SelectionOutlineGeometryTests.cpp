#include <array>
#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "math/Mat4.h"
#include "math/Vec3.h"
#include "render/overlay/SelectionOutlineGeometry.h"



namespace
{
    void require( bool condition, const char* message ) {
        if (!condition) throw std::runtime_error(message);
    }

    bool near( float a, float b ) {
        return std::fabs(a - b) < 0.0002f;
    }

    void requireVisible( const ve::ProjectedSelectionOutline& outline ) {
        for (std::size_t i = 0; i < outline.count; ++i) {
            const ve::Vec3& a = outline.lines[i].start;
            const ve::Vec3& b = outline.lines[i].end;

            for (const ve::Vec3* p : { &a, &b }) {
                require(std::isfinite(p->x) && std::isfinite(p->y) && std::isfinite(p->z),
                        "Projected point must be finite");
                require(p->x >= -1.001f && p->x <= 1.001f &&
                        p->y >= -1.001f && p->y <= 1.001f &&
                        p->z >= -1.001f && p->z <= 1.001f,
                        "Projected point must be inside clip volume");
            }
        }
    }

    void testAllTwelveEdges() {
        const auto result = ve::SelectionOutlineGeometry::create(
            ve::Vec3{ -0.5f, -0.5f, -0.5f }, 0.0f,
            ve::Mat4::identity(), ve::Mat4::identity()
        );
        require(result.count == 12, "Visible block must produce 12 outline edges");
        requireVisible(result);
        std::cout << "[PASS] Fully visible box has twelve edges\n";
    }

    void testExpectedCorners() {
        const auto result = ve::SelectionOutlineGeometry::create(
            ve::Vec3{ -0.5f, -0.5f, -0.5f }, 0.0f,
            ve::Mat4::identity(), ve::Mat4::identity()
        );
        const ve::ProjectedLine& first = result.lines[0];
        require(near(first.start.x, -0.5f) && near(first.start.y, -0.5f) && near(first.start.z, -0.5f),
                "First outline edge start mismatch");
        require(near(first.end.x, 0.5f) && near(first.end.y, -0.5f) && near(first.end.z, -0.5f),
                "First outline edge end mismatch");
        std::cout << "[PASS] Projected outline corners and edge order\n";
    }

    void testPartiallyClipped() {
        const auto result = ve::SelectionOutlineGeometry::create(
            ve::Vec3{ 0.5f, -0.5f, -0.5f }, 0.0f,
            ve::Mat4::identity(), ve::Mat4::identity()
        );
        require(result.count == 8, "Partially outside box must retain 8 clipped edges");
        requireVisible(result);
        std::cout << "[PASS] Outline line clipping at view boundary\n";
    }

    void testOutsideView() {
        const auto result = ve::SelectionOutlineGeometry::create(
            ve::Vec3{ 2.0f, -0.5f, -0.5f }, 0.0f,
            ve::Mat4::identity(), ve::Mat4::identity()
        );
        require(result.count == 0, "Outside box must not create outline segments");
        std::cout << "[PASS] Invisible selection produces no edges\n";
    }

    void testExpansion() {
        const auto result = ve::SelectionOutlineGeometry::create(
            ve::Vec3{ -0.5f, -0.5f, -0.5f }, 0.01f,
            ve::Mat4::identity(), ve::Mat4::identity()
        );
        require(result.count == 12, "Expanded box must retain 12 edges");
        require(near(result.lines[0].start.x, -0.51f), "Expansion must extend block lower bound");
        require(near(result.lines[0].end.x, 0.51f), "Expansion must extend block upper bound");
        std::cout << "[PASS] Outline slightly expands beyond block faces\n";
    }

    void testPerspectiveView() {
        constexpr float pi = 3.14159265358979323846f;
        const ve::Mat4 projection = ve::Mat4::perspective(pi / 3.0f, 1.0f, 0.1f, 100.0f);
        const auto result = ve::SelectionOutlineGeometry::create(
            ve::Vec3{ -0.5f, -0.5f, -3.0f }, 0.005f,
            ve::Mat4::identity(), projection
        );
        require(result.count == 12, "Block in front of camera must produce 12 edges");
        requireVisible(result);
        std::cout << "[PASS] Perspective camera projects 3D block outline\n";
    }

    void testNearPlaneClipping() {
        constexpr float pi = 3.14159265358979323846f;
        const ve::Mat4 projection = ve::Mat4::perspective(pi / 3.0f, 1.0f, 0.1f, 100.0f);
        const auto result = ve::SelectionOutlineGeometry::create(
            ve::Vec3{ -0.05f, -0.05f, -0.2f }, 0.0f,
            ve::Mat4::identity(), projection
        );
        require(result.count > 0 && result.count < 12, "Near-plane crossing must clip visible edges");
        requireVisible(result);
        std::cout << "[PASS] Near-plane crossing edges are clipped safely\n";
    }



    void testBehindCamera() {
        constexpr float pi = 3.14159265358979323846f;
        const ve::Mat4 projection = ve::Mat4::perspective(pi / 3.0f, 1.0f, 0.1f, 100.0f);
        const auto result = ve::SelectionOutlineGeometry::create(
            ve::Vec3{ -0.5f, -0.5f, 2.0f }, 0.0f,
            ve::Mat4::identity(), projection
        );
        require(result.count == 0, "Block behind camera must not draw outline");
        std::cout << "[PASS] Block behind camera is rejected\n";
    }



    void testInvalidInput() {
        int rejected = 0;
        const auto invalid = [&]( const ve::Vec3& position, float expansion ) {
            try {
                (void)ve::SelectionOutlineGeometry::create(
                    position, expansion, ve::Mat4::identity(), ve::Mat4::identity()
                );
            }
            catch (const std::invalid_argument&) {
                ++rejected;
            }
        };
        invalid(ve::Vec3{ 0.0f, 0.0f, 0.0f }, -0.01f);
        invalid(ve::Vec3{ 0.0f, 0.0f, 0.0f }, std::numeric_limits<float>::infinity());
        invalid(ve::Vec3{ std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f }, 0.0f);
        require(rejected == 3, "All invalid outline inputs must be rejected");
        std::cout << "[PASS] Invalid outline input validation\n";
    }
}



int main() {
    try {
        testAllTwelveEdges();
        testExpectedCorners();
        testPartiallyClipped();
        testOutsideView();
        testExpansion();
        testPerspectiveView();
        testNearPlaneClipping();
        testBehindCamera();
        testInvalidInput();
        std::cout << "\nAll Selection Outline Geometry tests passed.\n";
        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
