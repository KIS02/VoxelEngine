#include <cassert>
#include <memory>
#include <string>

#include "scene/SceneManager.h"
#include "scene/camera/Camera.h"


namespace
{
    class MockScene final : public ve::Scene {
        private:
            int updateCount_ = 0;
            ve::Camera camera_{};

        public:
            ve::Camera& camera() override { return camera_; }
            const ve::Camera& camera() const override { return camera_; }
            void update( float deltaTime, bool, const ve::MouseDelta&, const ve::InputActions& ) override {
                assert(deltaTime >= 0.0f);
                ++updateCount_;
            }

            int updateCount() const {
                return updateCount_;
            }
    };
}


int main() {
    ve::SceneManager manager;
    assert(manager.active() == nullptr);
    assert(manager.add("game", std::make_unique<MockScene>()));
    assert(manager.add("editor", std::make_unique<MockScene>()));
    assert(!manager.add("editor", std::make_unique<MockScene>()));
    assert(!manager.requestActivation("missing"));
    assert(manager.requestActivation("game"));
    assert(manager.active() == nullptr);
    assert(manager.applyPendingActivation());
    assert(manager.activeName() == "game");
    assert(!manager.remove("game"));

    ve::Scene* scene = manager.active();
    auto firstId = scene->objects().create();
    auto secondId = scene->objects().create();
    assert(firstId.valid() && secondId.valid() && firstId.value != secondId.value);
    auto* object = scene->objects().find(firstId);
    assert(object && object->id == firstId);
    object->meshAssetId = 123;
    assert(scene->objects().find(firstId)->meshAssetId == 123);

    ve::Entity entity{};
    entity.kind = ve::EntityKind::InteractiveObject;
    entity.visualObject = firstId;
    const ve::EntityId entityId = scene->entities().create(entity);
    assert(scene->entities().find(entityId)->visualObject == firstId);
    assert(scene->removeObject(firstId));
    assert(scene->objects().find(firstId) == nullptr);
    assert(!scene->entities().find(entityId)->visualObject.valid());
    assert(!scene->removeObject(firstId));

    scene->objects().clear();
    const auto newId = scene->objects().create();
    assert(newId.value > secondId.value); // A stale ID must never be reused.
    scene->update(1.0f / 60.0f, false, ve::MouseDelta{}, ve::InputActions{});
    assert(static_cast<MockScene*>(scene)->updateCount() == 1);

    assert(manager.requestActivation("editor"));
    assert(manager.activeName() == "game"); // Activation is deferred to a safe frame boundary.
    assert(manager.applyPendingActivation());
    assert(manager.activeName() == "editor");
    assert(manager.active()->objects().size() == 0);
    assert(manager.remove("game"));
    assert(manager.size() == 1);
    return 0;
}
