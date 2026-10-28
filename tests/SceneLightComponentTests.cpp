#include <cmath>
#include <iostream>
#include <stdexcept>

#include "scene/lighting/SceneLightCollector.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    bool near( float a, float b, float tolerance = 0.0001f ) {
        return std::fabs(a - b) < tolerance;
    }


    void testDirectionAndSnapshot() {
        ve::SceneObjectRegistry objects;

        ve::SceneObject sun{};
        sun.rotationDegrees = ve::Vec3{ 0.0f, 90.0f, 0.0f };
        sun.light = ve::LightComponent{};

        const ve::SceneObjectId id = objects.create(sun);

        ve::LightingSettings frameLighting{};
        ve::SceneLightCollector::collect(objects, frameLighting);

        require(frameLighting.sceneLights.size() == 1, "Directional snapshot missing");
        require(frameLighting.sceneLights[0].objectId == id.value, "Scene ID missing");
        require(near(frameLighting.directional.directionToLight.x, 1.0f), "Rotation not applied");
        require(near(frameLighting.directional.directionToLight.y, 0.0f), "Rotation y invalid");
        require(near(frameLighting.directional.directionToLight.z, 0.0f), "Rotation z invalid");

        ve::SceneObject* object = objects.find(id);

        require(object != nullptr, "Light object vanished");

        object->rotationDegrees = ve::Vec3{ 0.0f, 0.0f, 0.0f };

        ve::SceneLightCollector::collect(objects, frameLighting);

        require(near(frameLighting.directional.directionToLight.z, 1.0f),
                "Moving Light rotation ignored");

        std::cout << "[PASS] Directional world direction and rotation\n";
    }


    void testDefaultSunParity() {
        ve::SceneObjectRegistry objects;

        ve::SceneObject sun{};
        sun.rotationDegrees = ve::Vec3{ -60.31307f, -52.12502f, 0.0f };

        ve::LightComponent component{};
        component.color = ve::LinearRGB{ 1.0f, 0.7f, 0.2f };
        component.ambient = 0.25f;
        component.diffuse = 0.75f;

        sun.light = component;
        objects.create(sun);

        ve::LightingSettings lighting{};
        ve::SceneLightCollector::collect(objects, lighting);

        const ve::Vec3 expected = ve::Vec3{ -0.45f, 1.0f, 0.35f }.normalized();

        require(near(lighting.directional.directionToLight.x, expected.x), "Sun X direction changed");
        require(near(lighting.directional.directionToLight.y, expected.y), "Sun Y direction changed");
        require(near(lighting.directional.directionToLight.z, expected.z), "Sun Z direction changed");
        require(near(lighting.directional.ambient, 0.25f), "Sun ambient changed");
        require(near(lighting.directional.diffuse, 0.75f), "Sun diffuse changed");
        require(near(lighting.directional.color.g, 0.7f), "Sun color changed");

        std::cout << "[PASS] Existing directional sunlight preserved\n";
    }


    void testPointSpotAndLifetime() {
        ve::SceneObjectRegistry objects;

        ve::SceneObject point{};
        point.position = ve::Vec3{ 3.0f, 4.0f, 5.0f };

        ve::LightComponent pointComponent{};
        pointComponent.type = ve::LightType::Point;
        pointComponent.range = 14.0f;

        point.light = pointComponent;

        const ve::SceneObjectId pointId = objects.create(point);

        ve::SceneObject spot{};
        spot.position = ve::Vec3{ 1.0f, 2.0f, 3.0f };
        spot.rotationDegrees = ve::Vec3{ 90.0f, 0.0f, 0.0f };

        ve::LightComponent spotComponent{};
        spotComponent.type = ve::LightType::Spot;
        spotComponent.innerConeDegrees = 15.0f;
        spotComponent.outerConeDegrees = 25.0f;

        spot.light = spotComponent;

        const ve::SceneObjectId spotId = objects.create(spot);

        ve::LightingSettings snapshot{};
        ve::SceneLightCollector::collect(objects, snapshot);

        require(snapshot.sceneLights.size() == 2, "Both Light types must be retained");
        require(snapshot.sceneLights[0].type == ve::LightType::Point, "Point Light type changed");
        require(near(snapshot.sceneLights[0].position.x, 3.0f), "Point position missing");
        require(snapshot.sceneLights[1].type == ve::LightType::Spot, "Spot Light type changed");
        require(near(snapshot.sceneLights[1].emissionDirection.y, 1.0f), "Spot direction not rotated");
        require(near(snapshot.sceneLights[1].outerConeDegrees, 25.0f), "Spot cone missing");
        require(near(snapshot.directional.diffuse, 0.0f),
                "Scene without sun must not cast fake light");

        ve::SceneObject* found = objects.find(pointId);

        require(found != nullptr, "Point Light not found");

        found->light->enabled = false;

        ve::SceneLightCollector::collect(objects, snapshot);

        require(snapshot.sceneLights.size() == 1, "Disabled Light was included");
        require(snapshot.sceneLights.front().objectId == spotId.value, "Wrong Light retained");

        const auto stableSnapshot = snapshot.sceneLights;

        require(objects.remove(spotId), "Removal failed");

        ve::SceneLightCollector::collect(objects, snapshot);

        require(snapshot.sceneLights.empty(), "Removed Light remained in next frame");
        require(stableSnapshot.front().objectId == spotId.value,
                "Previous snapshot must own its values");

        std::cout << "[PASS] Point/Spot/disable/delete and snapshot lifetime\n";
    }


    void testValidation() {
        ve::SceneObjectRegistry objects;

        ve::SceneObject broken{};

        ve::LightComponent light{};
        light.type = ve::LightType::Spot;
        light.innerConeDegrees = 40.0f;
        light.outerConeDegrees = 20.0f;

        broken.light = light;
        objects.create(broken);

        bool caught = false;

        try {
            ve::LightingSettings settings{};
            ve::SceneLightCollector::collect(objects, settings);
        } catch (const std::invalid_argument&) {
            caught = true;
        }

        require(caught, "Invalid Spotlight cone must be rejected");

        std::cout << "[PASS] Invalid Spot Light rejected\n";
    }
}


int main() {
    try {
        testDirectionAndSnapshot();
        testDefaultSunParity();
        testPointSpotAndLifetime();
        testValidation();

        std::cout << "All SceneLightComponent tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}