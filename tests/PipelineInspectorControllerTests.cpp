
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "render/MeshData.h"
#include "render/RenderFrame.h"

#include "render/pipeline/PipelineInspectorController.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    ve::MeshData createTriangleMesh( float depth, bool reversed = false ) {
        std::vector<ve::Vertex> vertices(3);

        vertices[0].position = ve::Vec3{ -0.5f, 0.5f, depth };
        vertices[1].position = ve::Vec3{ -0.5f, -0.5f, depth };
        vertices[2].position = ve::Vec3{ 0.5f, 0.5f, depth };

        vertices[0].uv = ve::Vec2{ 0.0f, 0.0f };
        vertices[1].uv = ve::Vec2{ 0.0f, 1.0f };
        vertices[2].uv = ve::Vec2{ 1.0f, 0.0f };

        std::vector<std::uint32_t> indices = reversed
            ? std::vector<std::uint32_t>{ 0, 2, 1 }
            : std::vector<std::uint32_t>{ 0, 1, 2 };

        return ve::MeshData(std::move(vertices), std::move(indices));
    }


    ve::MeshData createPartiallyClippedMesh() {
        std::vector<ve::Vertex> vertices(3);

        vertices[0].position = ve::Vec3{ -2.0f, 0.0f, 0.0f };
        vertices[1].position = ve::Vec3{ 0.0f, -0.8f, 0.0f };
        vertices[2].position = ve::Vec3{ 0.0f, 0.8f, 0.0f };

        return ve::MeshData(std::move(vertices), { 0, 1, 2 });
    }


    ve::RenderFrame createFrame( const ve::MeshData& mesh, std::uint64_t meshId = 42 ) {
        ve::RenderFrame frame{};

        frame.view = ve::Mat4::identity();
        frame.projection = ve::Mat4::identity();

        frame.draws.push_back(ve::RenderDraw{
            &mesh, ve::Mat4::identity(), 7, meshId
        });

        return frame;
    }


    void testInitiallyDisabled() {
        ve::PipelineInspectorController controller{};

        require(!controller.isEnabled(), "Inspector must initially be disabled");
        require(!controller.hasSelection(), "Inspector must initially have no triangle");

        require(controller.stage() == ve::PipelineInspectorStage::Local,
                "Initial stage must be Local Space");

        require(ve::PipelineInspectorController::stageCount() == 9,
                "Inspector must contain nine stages");

        std::cout << "[PASS] Inspector starts disabled without selection\n";
    }


    void testToggleAndSelection() {
        ve::MeshData mesh = createTriangleMesh(0.0f);
        ve::RenderFrame frame = createFrame(mesh);

        ve::PipelineInspectorController controller{};

        require(!controller.selectAtPixel(frame, 8, 8, 3, 3),
                "Disabled Inspector must not select triangles");

        controller.toggle();

        require(controller.isEnabled(), "F4 toggle must enable Inspector");

        require(controller.selectAtPixel(frame, 8, 8, 3, 3),
                "Visible triangle must be selectable");

        require(controller.hasSelection(), "Successful picking must create selection");

        const ve::PipelineTrace& trace = *controller.selectedTrace();

        require(trace.triangle.has_value(), "Selected trace must contain a snapshot");
        require(trace.triangle->identity.meshId == 42, "Incorrect selected Mesh ID");
        require(trace.triangle->identity.triangleIndex == 0, "Incorrect Triangle Index");

        require(trace.pixelSample.has_value(), "Selected trace must contain a pixel sample");
        require(trace.pixelSample->covered, "Selected pixel must be covered");
        require(trace.pixelSample->x == 3 && trace.pixelSample->y == 3,
                "Incorrect selected pixel coordinates");

        require(trace.isConsistent(), "Selected Pipeline Trace must be consistent");

        std::cout << "[PASS] F4 toggle and visible triangle picking\n";
    }


    void testNearestTriangleWins() {
        ve::MeshData farMesh = createTriangleMesh(0.6f);
        ve::MeshData nearMesh = createTriangleMesh(-0.6f);

        ve::RenderFrame farFrame = createFrame(farMesh, 100);
        ve::RenderFrame nearFrame = createFrame(nearMesh, 200);

        ve::RenderFrame combinedFrame = farFrame;

        combinedFrame.draws.push_back(ve::RenderDraw{
            &nearMesh, ve::Mat4::identity(), 3, 200
        });


        // Check Each Triangle Independently

        const auto farOnly = ve::PipelineTrianglePicker::pick(
            farFrame, 8, 8, 3, 3
        );

        const auto nearOnly = ve::PipelineTrianglePicker::pick(
            nearFrame, 8, 8, 3, 3
        );


        // Print Actual Depth Results

        std::cout << "\n[DEBUG] Far Triangle\n";

        if (farOnly.has_value()) {
            std::cout << "  Mesh ID: "
                    << farOnly->triangle->identity.meshId << '\n';

            std::cout << "  Clip Z: "
                    << (*farOnly->clipPositions)[0].z << '\n';

            std::cout << "  Clip W: "
                    << (*farOnly->clipPositions)[0].w << '\n';

            std::cout << "  Incoming Depth: "
                    << farOnly->pixelSample->incomingDepth << '\n';
        } else {
            std::cout << "  No triangle selected\n";
        }


        std::cout << "\n[DEBUG] Near Triangle\n";

        if (nearOnly.has_value()) {
            std::cout << "  Mesh ID: "
                    << nearOnly->triangle->identity.meshId << '\n';

            std::cout << "  Clip Z: "
                    << (*nearOnly->clipPositions)[0].z << '\n';

            std::cout << "  Clip W: "
                    << (*nearOnly->clipPositions)[0].w << '\n';

            std::cout << "  Incoming Depth: "
                    << nearOnly->pixelSample->incomingDepth << '\n';
        } else {
            std::cout << "  No triangle selected\n";
        }


        // Check the Combined Scene

        ve::PipelineInspectorController controller{};
        controller.open();

        require(controller.selectAtPixel(combinedFrame, 8, 8, 3, 3),
                "Overlapping triangles must be selectable");

        const ve::PipelineTrace& trace = *controller.selectedTrace();

        std::cout << "\n[DEBUG] Combined Result\n";
        std::cout << "  Selected Draw Index: "
                << trace.triangle->identity.drawIndex << '\n';

        std::cout << "  Selected Mesh ID: "
                << trace.triangle->identity.meshId << '\n';

        std::cout << "  Selected Depth: "
                << trace.pixelSample->incomingDepth << '\n';

        std::cout << std::endl;


        // Original Regression Assertions

        require(trace.triangle->identity.drawIndex == 1,
                "Closer triangle must win even if rendered later");

        require(trace.triangle->identity.meshId == 200,
                "Incorrect closest Mesh ID");

        require(std::fabs(trace.pixelSample->incomingDepth + 0.6f) < 0.001f,
                "Incorrect closest depth");

        std::cout << "[PASS] Overlapping triangles select the closest depth\n";
    }


    void testBackfaceAndEmptyPixel() {
        ve::MeshData reversedMesh = createTriangleMesh(0.0f, true);
        ve::RenderFrame frame = createFrame(reversedMesh);

        ve::PipelineInspectorController controller{};
        controller.open();

        require(!controller.selectAtPixel(frame, 8, 8, 3, 3),
                "Backfacing triangle must not be selected");

        ve::MeshData frontMesh = createTriangleMesh(0.0f);
        frame = createFrame(frontMesh);

        require(!controller.selectAtPixel(frame, 8, 8, 7, 7),
                "Empty pixel must not select a triangle");

        require(!controller.selectAtPixel(frame, 8, 8, -1, 3),
                "Negative pixel coordinates must fail");

        require(!controller.selectAtPixel(frame, 8, 8, 8, 3),
                "Out-of-bounds pixel must fail");

        require(!controller.hasSelection(),
                "Failed selections must not create a triangle snapshot");

        std::cout << "[PASS] Backface, empty pixel and invalid coordinates\n";
    }


    void testPartiallyClippedTriangle() {
        ve::MeshData mesh = createPartiallyClippedMesh();
        ve::RenderFrame frame = createFrame(mesh);

        ve::PipelineInspectorController controller{};
        controller.open();

        require(controller.selectAtPixel(frame, 8, 8, 2, 3),
                "Partially clipped triangle must remain selectable");

        const ve::PipelineTrace& trace = *controller.selectedTrace();

        require(trace.clippedPolygon.has_value(), "Clipping result missing");
        require(trace.clippedPolygon->size() == 4,
                "Partially clipped triangle must have four polygon vertices");

        require(trace.screenPolygon->size() == 4,
                "Viewport trace must contain four vertices");

        require(trace.pixelSample->covered,
                "Selected pixel must be covered by clipped polygon");

        std::cout << "[PASS] Picking works on partially clipped triangles\n";
    }


    void testStageNavigation() {
        ve::MeshData mesh = createTriangleMesh(0.0f);
        ve::RenderFrame frame = createFrame(mesh);

        ve::PipelineInspectorController controller{};
        controller.open();

        require(controller.selectAtPixel(frame, 8, 8, 3, 3),
                "Stage test selection failed");


        // Navigate Through Every Pipeline Stage

        for (std::size_t i = 1; i < ve::PipelineInspectorController::stageCount(); ++i) {
            controller.nextStage();

            require(controller.stageIndex() == i,
                    "Next Stage must advance by exactly one");
        }

        require(controller.stage() == ve::PipelineInspectorStage::Fragment,
                "Final stage must be Fragment");


        // Further Navigation Must Not Exceed the Last Stage

        controller.nextStage();

        require(controller.stage() == ve::PipelineInspectorStage::Fragment,
                "Stage navigation exceeded the last stage");


        // Navigate Back to Local Space

        for (std::size_t i = 8; i > 0; --i)
            controller.previousStage();

        require(controller.stage() == ve::PipelineInspectorStage::Local,
                "Previous Stage must return to Local Space");

        controller.previousStage();

        require(controller.stageIndex() == 0,
                "Stage navigation went before Local Space");

        std::cout << "[PASS] Nine-stage navigation respects both boundaries\n";
    }


    void testSelectionPersistsAcrossToggleAndFailedClick() {
        ve::MeshData mesh = createTriangleMesh(0.0f);
        ve::RenderFrame frame = createFrame(mesh, 77);

        ve::PipelineInspectorController controller{};
        controller.open();

        require(controller.selectAtPixel(frame, 8, 8, 3, 3),
                "Initial selection failed");

        controller.nextStage();
        controller.close();

        require(!controller.isEnabled(), "Inspector must close");
        require(controller.hasSelection(), "Closing must preserve frozen selection");

        controller.open();

        require(controller.stage() == ve::PipelineInspectorStage::World,
                "Reopening must preserve the current stage");

        require(!controller.selectAtPixel(frame, 8, 8, 7, 7),
                "Empty pixel must not select a new triangle");

        require(controller.selectedTrace()->triangle->identity.meshId == 77,
                "Failed click replaced the previous selection");

        require(controller.stage() == ve::PipelineInspectorStage::World,
                "Failed click changed the current stage");

        controller.clearSelection();

        require(!controller.hasSelection(), "Clear Selection must remove the trace");
        require(controller.stage() == ve::PipelineInspectorStage::Local,
                "Clear Selection must reset the stage");

        std::cout << "[PASS] Toggle, failed click and explicit selection clearing\n";
    }


    void testFrozenSnapshotSurvivesSourceDestruction() {
        ve::PipelineInspectorController controller{};
        controller.open();

        {
            ve::MeshData mesh = createTriangleMesh(-0.3f);
            ve::RenderFrame frame = createFrame(mesh, 500);

            require(controller.selectAtPixel(frame, 8, 8, 3, 3),
                    "Temporary Mesh selection failed");
        }


        // Original Mesh and RenderFrame No Longer Exist

        require(controller.hasSelection(), "Frozen selection was invalidated");

        const ve::PipelineTrace& trace = *controller.selectedTrace();

        require(trace.triangle->identity.meshId == 500,
                "Frozen Mesh identity changed");

        require(trace.triangle->vertices[0].position.x == -0.5f,
                "Frozen vertex position changed");

        require(trace.pixelSample->covered,
                "Frozen pixel coverage result was lost");

        require(trace.isConsistent(), "Frozen Pipeline Trace became invalid");

        std::cout << "[PASS] Frozen selection survives Mesh and RenderFrame destruction\n";
    }
}


int main() {
    try {
        testInitiallyDisabled();
        testToggleAndSelection();
        testNearestTriangleWins();
        testBackfaceAndEmptyPixel();
        testPartiallyClippedTriangle();
        testStageNavigation();
        testSelectionPersistsAcrossToggleAndFailedClick();
        testFrozenSnapshotSurvivesSourceDestruction();

        std::cout << "\nAll Pipeline Inspector Controller tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
