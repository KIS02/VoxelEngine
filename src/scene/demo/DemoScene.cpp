#include "scene/demo/DemoScene.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>
#include <cstdlib>
#include <cerrno>
#include <cmath>

#include "scene/demo/DemoChunkGenerator.h"
#include "assets/geometry/primitives/CubeMeshGenerator.h"
#include "assets/texture/UVTestTexture.h"
#include "gameplay/interaction/BlockInteractor.h"
#include "gameplay/items/InventorySave.h"
#include "gameplay/save/PlayerSave.h"
#include "gameplay/save/WorldTimeSave.h"
#include "gameplay/world/SunCycle.h"
#include "ui/WorldTimeHud.h"
#include "ui/SurvivalHud.h"
#include "ui/InventoryHud.h"
#include "ui/CraftingHud.h"
#include "assets/block/BlockName.h"

#include "voxel/generation/TerrainGenerator.h"
#include "assets/block/VoxelMaterialLibrary.h"
#include "assets/block/VoxelTextureSetLibrary.h"

namespace ve
{
    namespace
    {
        bool sameBlockPosition( const WorldBlockPosition& a, const WorldBlockPosition& b ) {
            return a.x == b.x && a.y == b.y && a.z == b.z;
        }

        std::filesystem::path savePathForSeed( WorldSeed seed ) {
            return std::filesystem::path("saves") / ("world_" + std::to_string(seed) + ".vew");
        }
    }

    DemoScene::DemoScene( AssetManager& assets, DemoTerrainMode terrainMode, WorldSeed worldSeed )
        : assets_(assets), voxel_(assets.meshIds()), terrainMode_(terrainMode), worldSeed_(worldSeed) {

        // Set the initial preference once; M-key input owns subsequent transitions.
        const char* pbrPreview = std::getenv("VOXELENGINE_DEMO_PBR");
        shadingModel_ = pbrPreview != nullptr && pbrPreview[0] == '1' && pbrPreview[1] == '\0'
            ? ShadingModel::Pbr : ShadingModel::BlinnPhong;

        textureId_ = assets_.addTexture(createUVTestTexture());
        defaultMaterialId_ = assets_.addMaterial(textureId_, 1.0f, 16.0f);
        cubeMeshId_ = assets_.addMesh(CubeMeshGenerator::create(1.0f));

        resetCamera();
        gameplay_.loadCatalogs("assets/gameplay/blocks.txt", "assets/gameplay/items.txt");
        gameplay_.start(camera_.position());
        if (const char* duration = std::getenv("VOXELENGINE_WORLD_DAY_SECONDS")) {
            char* end = nullptr;
            errno = 0;
            const double seconds = std::strtod(duration, &end);
            if (errno == ERANGE || end == duration || *end != '\0')
                throw std::invalid_argument("Invalid VOXELENGINE_WORLD_DAY_SECONDS");
            gameplay_.worldClock().setDayLengthSeconds(seconds);
        }
        if (terrainMode_ == DemoTerrainMode::Procedural) {
            const std::filesystem::path save = std::filesystem::path("saves") /
                ("time_" + std::to_string(worldSeed_) + ".vet");
            try {
                if (WorldTimeSave::read(save, gameplay_.worldClock()))
                    std::cout << "[WorldTime] Restored " << save.string() << '\n';
            } catch (const std::exception& error) {
                worldTimeSaveWritable_ = false;
                std::cerr << "[WorldTime] Load rejected: " << error.what() << '\n';
            }
        }
        if (terrainMode_ == DemoTerrainMode::Procedural) {
            try {
                const std::filesystem::path save = std::filesystem::path("saves") /
                    ("inventory_" + std::to_string(worldSeed_) + ".vei");
                if (InventorySave::read(save, gameplay_.inventory(), &gameplay_.worldItems())) {
                    std::cout << "[Inventory] Restored " << save.string() << '\n';
                } else {
                    gameplay_.inventory().insert(ItemStack{ ItemId::Dirt, 32 });
                    gameplay_.inventory().insert(ItemStack{ ItemId::Stone, 16 });
                    gameplay_.inventory().insert(ItemStack{ ItemId::Grass, 16 });
                    gameplay_.inventory().insert(ItemStack{ ItemId::WoodenPickaxe, 1 });
                    gameplay_.inventory().insert(ItemStack{ ItemId::Bread, 4 });
                    std::cout << "[Inventory] New world starter items assigned\n";
                }
            } catch (const std::exception& error) {
                inventorySaveWritable_ = false;
                std::cerr << "[Inventory] Load rejected: " << error.what() << '\n';
            }
        }
        std::cout << "[Gameplay] Initial mode: " << gameModeName(gameplay_.player().mode) << '\n';


        const char* multiLightPreview = std::getenv("VOXELENGINE_DEMO_MULTI_LIGHT");

        if (multiLightPreview != nullptr && multiLightPreview[0] == '1' &&
            multiLightPreview[1] == '\0') {
            SceneObject pointLightObject{};

            pointLightObject.position = Vec3{ 8.0f, 20.0f, 8.0f };

            LightComponent pointLight{};

            pointLight.type = LightType::Point;
            pointLight.color = LinearRGB{ 1.0f, 0.3f, 0.2f };
            pointLight.ambient = 0.05f;
            pointLight.diffuse = 1.0f;
            pointLight.intensity = 100.0f;

            pointLightObject.light = pointLight;

            objects().create(pointLightObject);
        }

        SceneObject cube{};
        cube.position = Vec3{ 0.0f, 7.0f, 0.0f };
        cube.rotationDegrees = Vec3{ 0.0f, 30.0f, 0.0f };
        cube.meshAssetId = cubeMeshId_;
        cube.materialAssetId = defaultMaterialId_;

        const char* previewPath = std::getenv("VOXELENGINE_DEMO_TEXTURE");

        if (previewPath != nullptr && previewPath[0] != '\0') {
            const std::filesystem::path imagePath(previewPath);

            if (!std::filesystem::is_regular_file(imagePath))
                throw std::runtime_error("Demo Texture file not found: " + imagePath.string());

            const std::uint64_t imageId = assets_.loadTexture(imagePath);
            const std::uint64_t materialId = assets_.addMaterial(imageId, 1.0f, 16.0f);

            cube.materialAssetId = materialId;

            // Place the preview Cube directly in front of the initial camera.
            cube.position = camera_.position() + camera_.forward() * 4.0f;
            cube.scale = Vec3{ 2.0f, 2.0f, 2.0f };

            std::cout << "[Texture] Loaded demo image: " << imagePath.string() << '\n';
            std::cout << "[Texture] Texture ID: " << imageId << '\n';
            std::cout << "[Texture] Material ID: " << materialId << '\n';
            std::cout << "[Texture] Preview Cube placed in front of camera\n";
        }

        objects().create(cube);

        const char* shadowPreview = std::getenv("VOXELENGINE_DEMO_SHADOW");

        const bool enableShadows = shadowPreview != nullptr &&
                                shadowPreview[0] == '1' && shadowPreview[1] == '\0';

        SceneObject sun{};

        sun.rotationDegrees = Vec3{ -60.31307f, -52.12502f, 0.0f };

        LightComponent sunlight{};

        sunlight.type = LightType::Directional;
        sunlight.color = LinearRGB{ 1.0f, 0.7f, 0.2f };
        sunlight.ambient = 0.25f;
        sunlight.diffuse = 0.75f;
        sunlight.intensity = 1.0f;
        sunlight.castsShadows = enableShadows;

        sun.light = sunlight;

        sunObjectId_ = objects().create(sun);
        sunShadowsEnabled_ = enableShadows;
        updateSunlight();


        controller_.setMoveSpeed(8.0f);
        controller_.setMouseSensitivity(0.1f);

        populateDemoWorld();
        voxel_.rebuildDirtyMeshes();
        gameplay_.configurePhysics(voxel_.blocks());
        gameplay_.configureMining(voxel_.blocks());
        if (terrainMode_ == DemoTerrainMode::Procedural) {
            const std::filesystem::path save = std::filesystem::path("saves") /
                ("player_" + std::to_string(worldSeed_) + ".vep");
            try {
                PlayerState saved = gameplay_.player();
                if (PlayerSave::read(save, saved)) {
                    gameplay_.restorePlayer(saved);
                    camera_.setPosition(gameplay_.player().position);
                    std::cout << "[PlayerSave] Restored " << save.string() << '\n';
                }
            } catch (const std::exception& error) {
                playerSaveWritable_ = false;
                std::cerr << "[PlayerSave] Load rejected: " << error.what() << '\n';
            }
        }
        try {
            gameplay_.crafting().recipes().loadFile("assets/gameplay/recipes.txt", gameplay_.items());
        } catch (const std::exception& error) {
            std::cerr << "[Crafting] Recipe load rejected; using built-in recipes: "
                      << error.what() << '\n';
        }
        if (terrainMode_ == DemoTerrainMode::Procedural) {
            try { orientedBlocks_.load(std::filesystem::path("saves") /
                ("oriented_" + std::to_string(worldSeed_) + ".vbo")); }
            catch (const std::exception& error) {
                orientationSaveWritable_ = false;
                std::cerr << "[Orientation] Load rejected: " << error.what() << '\n';
            }
        }

        voxelMaterials_ = createVoxelMaterials(assets_);
        voxelTextureSets_[0] = voxelMaterials_;
    }

    DemoScene::~DemoScene() {
        if (terrainMode_ == DemoTerrainMode::Procedural && worldTimeSaveWritable_) {
            try {
                const std::filesystem::path path = std::filesystem::path("saves") /
                    ("time_" + std::to_string(worldSeed_) + ".vet");
                WorldTimeSave::write(path, gameplay_.worldClock());
                std::cout << "[WorldTime] Saved " << path.string() << '\n';
            } catch (const std::exception& error) {
                std::cerr << "[WorldTime] Save failed: " << error.what() << '\n';
            }
        }
        if (terrainMode_ == DemoTerrainMode::Procedural && playerSaveWritable_) {
            try {
                const std::filesystem::path path = std::filesystem::path("saves") /
                    ("player_" + std::to_string(worldSeed_) + ".vep");
                PlayerSave::write(path, gameplay_.player());
                std::cout << "[PlayerSave] Saved " << path.string() << '\n';
            } catch (const std::exception& error) {
                std::cerr << "[PlayerSave] Save failed: " << error.what() << '\n';
            }
        }
        if (terrainMode_ == DemoTerrainMode::Procedural && orientationSaveWritable_) {
            try {
                orientedBlocks_.save(std::filesystem::path("saves") /
                    ("oriented_" + std::to_string(worldSeed_) + ".vbo"));
            } catch (const std::exception& error) {
                std::cerr << "[Orientation] Save failed: " << error.what() << '\n';
            }
        }
        try {
            gameplay_.crafting().putAway(gameplay_.inventory(), gameplay_.worldItems(),
                gameplay_.player().position);
        } catch (const std::exception& error) {
            inventorySaveWritable_ = false;
            std::cerr << "[Crafting] Return ingredients failed; inventory save skipped: "
                      << error.what() << '\n';
        }

        if (terrainMode_ == DemoTerrainMode::Procedural && inventorySaveWritable_) {
            try {
                if (!inventoryCursor_.putAway(gameplay_.inventory())) {
                    gameplay_.worldItems().spawn(*inventoryCursor_.carried(), gameplay_.player().position);
                    inventoryCursor_.restore(std::nullopt);
                    std::cerr << "[Inventory] Full inventory: carried stack dropped to world\n";
                }
                InventorySave::write(std::filesystem::path("saves") /
                    ("inventory_" + std::to_string(worldSeed_) + ".vei"), gameplay_.inventory(), &gameplay_.worldItems());
            } catch (const std::exception& error) {
                std::cerr << "[Inventory] Save failed: " << error.what() << '\n';
            }
        }
        if (!streaming_.has_value())
            return;

        try {
            const std::filesystem::path path = savePathForSeed(worldSeed_);
            streaming_->saveWorld(path);
            std::cout << "[WorldSave] Saved " << path.string() << '\n';
        } catch (const std::exception& error) {
            std::cerr << "[WorldSave] Save failed: " << error.what() << '\n';
        }
    }


    bool DemoScene::inventoryOpen() const {
        return inventoryOpen_;
    }

    bool DemoScene::gameplayHudWithoutCapture() const {
        return inventoryOpen_;
    }

    void DemoScene::appendGameplayHud( UIFrameData& ui, int width, int height ) const {
        InventoryHud::append(ui, gameplay_.inventory(), inventoryCursor_, inventoryOpen_,
                             width, height, inventoryPointerX_, inventoryPointerY_);
        CraftingHud::append(ui, gameplay_.crafting(), inventoryOpen_);
        WorldTimeHud::append(ui, gameplay_.worldClock());
        SurvivalHud::append(ui, gameplay_.player());
        if (!inventoryOpen_ && gameplay_.player().mode == GameMode::Survival) {
            const MiningProgress& mining = gameplay_.miningProgress();
            if (mining.block) {
                const int filled = std::clamp(static_cast<int>(mining.normalized * 10.0f), 0, 10);
                ui.hotbarOverlay.lines.push_back("MINING CRACK " + std::to_string(mining.crackStage) +
                    " [" + std::string(static_cast<std::size_t>(filled), '#') +
                    std::string(static_cast<std::size_t>(10 - filled), '-') + "]");
            }
        }
    }

    GameplaySession& DemoScene::gameplay() {
        return gameplay_;
    }

    const GameplaySession& DemoScene::gameplay() const {
        return gameplay_;
    }
    void DemoScene::resetCamera() {
        if (terrainMode_ == DemoTerrainMode::Procedural) {
            camera_.setPosition(Vec3{ 8.5f, 21.5f, 27.5f });
            camera_.lookAt(Vec3{ 8.5f, 7.0f, 8.5f });
            return;
        }

        camera_.setPosition(Vec3{ 8.5f, 6.5f, 10.5f });
        camera_.lookAt(Vec3{ 8.5f, 2.5f, 8.5f });
    }

    void DemoScene::clearSelection() {
        selectedBlock_.reset();
    }

    void DemoScene::disableChunkRemovalDemo() {
        chunkRemovalDemoEnabled_ = false;
    }

    void DemoScene::populateDemoWorld() {
        if (terrainMode_ == DemoTerrainMode::Procedural) {
            populateProceduralWorld();
            return;
        }

        for (int z = -1; z <= 0; ++z) {
            for (int x = -1; x <= 0; ++x) {
                const ChunkPosition position{ x, 0, z };

                Chunk& chunk = voxel_.blocks().getOrCreateChunk(position);

                chunk = createDemoChunk();

                voxel_.blocks().markChunkDirty(position);
            }
        }
    }

    void DemoScene::populateProceduralWorld() {
        TerrainGenerationConfig config{};
        config.seed = worldSeed_;
        config.surfaceY = 8;
        config.dirtDepth = 2;
        config.heightAmplitude = 14.0;

        streaming_.emplace(voxel_, config, 1);
        const std::filesystem::path path = savePathForSeed(worldSeed_);

        if (streaming_->loadWorld(path))
            std::cout << "[WorldSave] Loaded " << path.string() << '\n';
        else
            std::cout << "[WorldSave] New world: " << path.string() << '\n';

        streaming_->update(camera_.position());
    }

    void DemoScene::updateBlockSelection() {
        const std::optional<VoxelRaycastHit> hit = VoxelRaycaster::cast(
            voxel_.blocks(),
            camera_.position(),
            camera_.forward(),
            blockReach_
        );


        if (!hit.has_value()) {
            if (selectedBlock_.has_value()) {
                std::cout << "[Raycast] No block in range" << std::endl;
            }

            selectedBlock_.reset();

            return;
        }


        const bool selectionChanged =
            !selectedBlock_.has_value() ||
            !sameBlockPosition(selectedBlock_->block, hit->block) ||
            !sameBlockPosition(selectedBlock_->faceNormal, hit->faceNormal) ||
            selectedBlock_->type != hit->type;


        selectedBlock_ = hit;


        if (!selectionChanged) {
            return;
        }


        std::cout << "[Raycast] Hit " << blockTypeName(hit->type)
                  << " | Block: ("
                  << hit->block.x << ", "
                  << hit->block.y << ", "
                  << hit->block.z << ")"
                  << " | Normal: ("
                  << hit->faceNormal.x << ", "
                  << hit->faceNormal.y << ", "
                  << hit->faceNormal.z << ")"
                  << " | Distance: " << hit->distance
                  << std::endl;
    }


    void DemoScene::updateSunlight() {
        SceneObject* sun = objects().find(sunObjectId_);
        if (sun == nullptr || !sun->light.has_value())
            throw std::logic_error("Demo sun SceneObject is missing");

        const SunCycleState state = SunCycle::sample(gameplay_.worldClock(), sunShadowsEnabled_);
        sun->rotationDegrees = state.rotationDegrees;
        sun->light->color = state.color;
        sun->light->intensity = state.intensity;
        sun->light->ambient = state.ambient;
        sun->light->diffuse = state.diffuse;
        sun->light->castsShadows = state.castsShadows;
    }

    void DemoScene::update( float deltaTime, bool cursorCaptured, const MouseDelta& mouseDelta,
                            const InputActions& actions ) {
        inventoryPointerX_ = actions.cursorX;
        inventoryPointerY_ = actions.cursorY;
        if (!gameplay_.player().alive && inventoryOpen_) {
            if (!inventoryCursor_.putAway(gameplay_.inventory())) {
                gameplay_.worldItems().spawn(*inventoryCursor_.carried(), gameplay_.player().position);
                inventoryCursor_.restore(std::nullopt);
            }
            gameplay_.crafting().putAway(gameplay_.inventory(), gameplay_.worldItems(),
                gameplay_.player().position);
            gameplay_.crafting().setWorkbench(false);
            inventoryOpen_ = false;
        }
        if (gameplay_.player().alive && (actions.toggleInventory || actions.closeInventory)) {
            if (inventoryOpen_) {
                if (!inventoryCursor_.putAway(gameplay_.inventory())) {
                    gameplay_.worldItems().spawn(*inventoryCursor_.carried(), gameplay_.player().position);
                    inventoryCursor_.restore(std::nullopt);
                    std::cout << "[Inventory] Inventory full; carried items dropped to world\n";
                }
                gameplay_.crafting().putAway(gameplay_.inventory(), gameplay_.worldItems(),
                    gameplay_.player().position);
                gameplay_.crafting().setWorkbench(false);
                inventoryOpen_ = false;
            } else if (actions.toggleInventory) {
                inventoryOpen_ = true;
            }
        }
        if (inventoryOpen_) {
            const auto craftSlot = CraftingHud::slotAt(actions.cursorX, actions.cursorY, gameplay_.crafting());
            if (craftSlot) {
                if (actions.inventoryLeft)
                    inventoryCursor_.leftClick(gameplay_.crafting().slots(), *craftSlot);
                else if (actions.inventoryRight)
                    inventoryCursor_.rightClick(gameplay_.crafting().slots(), *craftSlot);
            } else if (actions.inventoryLeft && CraftingHud::resultAt(
                actions.cursorX, actions.cursorY, gameplay_.crafting())) {
                gameplay_.craftSelectedRecipe();
            }
            const auto slot = InventoryHud::slotAt(actions.cursorX, actions.cursorY);
            if (slot) {
                if (actions.inventoryLeft && actions.shiftHeld && !inventoryCursor_.carried())
                    gameplay_.inventory().quickMove(*slot);
                else if (actions.inventoryLeft)
                    inventoryCursor_.leftClick(gameplay_.inventory(), *slot);
                else if (actions.inventoryRight)
                    inventoryCursor_.rightClick(gameplay_.inventory(), *slot);
            }
        }
        if (actions.hotbarNumber >= 0 && actions.hotbarNumber < 9)
            gameplay_.inventory().selectHotbar(static_cast<std::size_t>(actions.hotbarNumber));
        if (actions.hotbarScroll != 0)
            gameplay_.inventory().cycleHotbar(-actions.hotbarScroll);
        if (actions.dropSelected && !inventoryOpen_)
            gameplay_.dropSelected(camera_.forward());
        if (actions.consumeFood && !inventoryOpen_ && cursorCaptured &&
            gameplay_.consumeSelectedFood())
            std::cout << "[Survival] Food consumed" << '\n';
        if (actions.toggleShadingModel) {
            shadingModel_ = shadingModel_ == ShadingModel::BlinnPhong
                ? ShadingModel::Pbr : ShadingModel::BlinnPhong;
            std::cout << "[Shading] " << (shadingModel_ == ShadingModel::Pbr ? "PBR" : "Blinn-Phong") << '\n';
        }

        if (actions.cycleTextureSet) {
            const std::uint8_t next = static_cast<std::uint8_t>(
                (activeVoxelTextureSet_ + 1) % voxelTextureSets_.size()
            );
            if (!loadedTextureSets_[next]) {
                const auto configs = voxelTextureSetConfigs();
                voxelTextureSets_[next] = createVoxelTextureSetMaterials(
                    assets_, voxelTextureSets_[0], configs[next - 1]
                );
                loadedTextureSets_[next] = true;
            }
            voxelMaterials_ = voxelTextureSets_[next];
            activeVoxelTextureSet_ = next;
            std::cout << "[TextureSet] Active: " <<
                (next == 0 ? "Default" : next == 1 ? "Set 1" : "Set 2") << '\n';
        }

        if (actions.toggleGameMode && gameplay_.player().alive) {
            try {
                gameplay_.toggleMode();
                std::cout << "[Gameplay] Mode: " << gameModeName(gameplay_.player().mode) << '\n';
            } catch (const std::exception& error) {
                std::cerr << "[Gameplay] Mode switch rejected: " << error.what() << '\n';
            }
        }

        if (actions.respawn && !gameplay_.player().alive &&
            gameplay_.player().mode == GameMode::Survival) {
            try {
                gameplay_.respawn();
                std::cout << "[Gameplay] Player respawned" << '\n';
            } catch (const std::exception& error) {
                std::cerr << "[Gameplay] Respawn rejected: " << error.what() << '\n';
            }
        }
        // Camera Movement and Mouse Look

        if (cursorCaptured) {


            controller_.updateLook(camera_, mouseDelta);


            if (allowsFlight(gameplay_.player().mode))
                controller_.update(camera_, actions.movement, deltaTime);
        }


        gameplay_.synchronizeCreativePosition(camera_.position());

        std::vector<DynamicCollider> colliders;
        objects().forEach([&]( const SceneObject& object ) {
            if (!object.collision.has_value())
                return;
            colliders.push_back(DynamicCollider{ object.id.value,
                object.collision->boundsAt(object.position),
                object.collision->velocity, object.collision->solid });
        });
        gameplay_.setDynamicColliders(std::move(colliders));

        PlayerMovementCommand movement{};
        if (cursorCaptured) {
            movement.forward = static_cast<float>(actions.movement.forward) -
                               static_cast<float>(actions.movement.backward);
            movement.strafe = static_cast<float>(actions.movement.right) -
                              static_cast<float>(actions.movement.left);
            movement.jump = actions.movement.up;
            movement.sprint = actions.movement.down;
        }
        movement.viewForward = camera_.forward();
        gameplay_.setMovementCommand(movement);

        gameplay_.setMiningInput(cursorCaptured && actions.breakHeld && !inventoryOpen_,
            camera_.forward(), blockReach_);
        const GameplayClockResult gameplayTime = gameplay_.advance(deltaTime);
        if (gameplayTime.steps > 0)
            updateSunlight();
        // World drops use SceneObject visuals; the source of truth remains GameplaySession.
        for (const WorldItem& drop : gameplay_.worldItems().all()) {
            auto found = dropVisuals_.find(drop.id);
            if (found == dropVisuals_.end()) {
                SceneObject preview{};
                preview.position = drop.position;
                preview.scale = Vec3{ 0.24f, 0.24f, 0.24f };
                preview.meshAssetId = cubeMeshId_;
                preview.materialAssetId = defaultMaterialId_;
                dropVisuals_.emplace(drop.id, objects().create(preview));
            } else if (SceneObject* preview = objects().find(found->second)) {
                preview->position = drop.position;
            }
        }
        for (auto it = dropVisuals_.begin(); it != dropVisuals_.end();) {
            const bool exists = std::any_of(gameplay_.worldItems().all().begin(),
                gameplay_.worldItems().all().end(), [&]( const WorldItem& drop ) {
                    return drop.id == it->first;
                });
            if (exists) { ++it; continue; }
            objects().remove(it->second);
            it = dropVisuals_.erase(it);
        }
        if (gameplay_.player().mode == GameMode::Survival)
            camera_.setPosition(gameplay_.player().position);
        if (gameplayTime.discardedSeconds > 0.0 && gameplay_.clock().ticks() % 60 < 8)
            std::cout << "[Gameplay] Tick catch-up limit; discarded "
                      << gameplayTime.discardedSeconds << "s of simulation time\n";
        if (terrainMode_ == DemoTerrainMode::Procedural && streaming_.has_value()) {
            const ChunkStreamingResult result = streaming_->update(camera_.position());

            if (result.changed) {
                const ChunkCacheStatistics stats = streaming_->cacheStatistics();

                std::cout << "[Streaming] Generated " << result.generated
                        << " | Restored " << result.restored
                        << " | Unloaded " << result.unloaded
                        << " | Resident " << voxel_.blocks().chunkCount()
                        << " | Cached " << stats.cachedChunks
                        << " | DeltaBlocks " << stats.changedBlocks
                        << " | UsedBytes " << stats.usedDeltaBytes
                        << " | ReservedBytes " << stats.reservedDeltaBytes
                        << " | AccountedBytes " << stats.accountedDeltaBytes()
                        << " | FullValueBaseline " << stats.fullMapValueBytes << '\n';
            }
        }


        // The workstation interaction takes priority over block placement.
        bool workbenchUsed = false;
        if (cursorCaptured && actions.placeBlock && gameplay_.player().alive) {
            const auto targeted = VoxelRaycaster::cast(voxel_.blocks(), camera_.position(),
                camera_.forward(), blockReach_);
            if (targeted && targeted->type == BlockId::CraftingTable) {
                gameplay_.crafting().setWorkbench(true);
                inventoryOpen_ = true;
                workbenchUsed = true;
                std::cout << "[Crafting] Workbench 3x3 opened\n";
            }
        }

        // Break or Place a Block Using the Current Camera Direction

        if (cursorCaptured) {
            if (actions.breakBlock && allowsInstantBreak(gameplay_.player().mode)) {
                const std::optional<VoxelRaycastHit> broken = BlockInteractor::breakBlock(
                    voxel_.blocks(),
                    camera_.position(),
                    camera_.forward(),
                    blockReach_
                );


                if (broken.has_value()) {
                    orientedBlocks_.erase(broken->block);
                    gameplay_.events().publish(GameplayEvent{ GameplayEventType::BlockBroken,
                        gameplay_.player().id, Vec3{ static_cast<float>(broken->block.x),
                        static_cast<float>(broken->block.y), static_cast<float>(broken->block.z) },
                        gameplay_.player().mode });
                    std::cout << "[Block] Broken " << blockTypeName(broken->type)
                            << " | Position: ("
                            << broken->block.x << ", "
                            << broken->block.y << ", "
                            << broken->block.z << ")"
                            << " | Dirty Chunks: " << voxel_.blocks().dirtyChunkCount()
                            << std::endl;
                }
            } else if (actions.placeBlock && !workbenchUsed) {
                const std::size_t selectedSlot = gameplay_.inventory().selectedHotbar();
                const auto selectedItem = gameplay_.inventory().at(selectedSlot);
                BlockId placedType = BlockId::Air;
                if (selectedItem) {
                    switch (selectedItem->id) {
                        case ItemId::Grass: placedType = BlockId::Grass; break;
                        case ItemId::Dirt: placedType = BlockId::Dirt; break;
                        case ItemId::Stone: placedType = BlockId::Stone; break;
                        case ItemId::CraftingTable: placedType = BlockId::CraftingTable; break;
                        default: break;
                    }
                }
                if (placedType == BlockId::Air && gameplay_.player().mode == GameMode::Creative)
                    placedType = BlockId::Dirt;
                if (placedType != BlockId::Air && gameplay_.player().alive) {
                    const CollisionAabb playerBounds = gameplay_.playerBounds();
                    const std::optional<BlockPlacementResult> placed = BlockInteractor::placeBlock(
                        voxel_.blocks(),
                        camera_.position(),
                        camera_.forward(),
                        blockReach_,
                        placedType, &playerBounds
                    );
    
    
                    if (placed.has_value()) {
                        if (placedType == BlockId::CraftingTable)
                            orientedBlocks_.set(placed->placedBlock, OrientedBlocks::facing(camera_.forward()));
                        else
                            orientedBlocks_.erase(placed->placedBlock);
                        if (gameplay_.player().mode == GameMode::Survival)
                            gameplay_.inventory().remove(selectedSlot, 1);
                        gameplay_.events().publish(GameplayEvent{ GameplayEventType::BlockPlaced,
                            gameplay_.player().id, Vec3{ static_cast<float>(placed->placedBlock.x),
                            static_cast<float>(placed->placedBlock.y), static_cast<float>(placed->placedBlock.z) },
                            gameplay_.player().mode });
                        std::cout << "[Block] Placed " << blockTypeName(placed->type)
                                << " | Position: ("
                                << placed->placedBlock.x << ", "
                                << placed->placedBlock.y << ", "
                                << placed->placedBlock.z << ")"
                                << " | Attached To: ("
                                << placed->attachedTo.x << ", "
                                << placed->attachedTo.y << ", "
                                << placed->attachedTo.z << ")"
                                << " | Dirty Chunks: " << voxel_.blocks().dirtyChunkCount()
                                << std::endl;
                    }
                }
            }
        }


        // Consume confirmed events at the Scene boundary; future systems may
        // dispatch them before the next gameplay mutation.
        for (const GameplayEvent& event : gameplay_.events().take()) {
            if (event.type == GameplayEventType::BlockBroken)
                orientedBlocks_.erase(WorldBlockPosition{
                    static_cast<int>(event.position.x), static_cast<int>(event.position.y),
                    static_cast<int>(event.position.z) });
        }
        // Input actions are frame-local values and require no manual reset.


        // Existing Optional Chunk Removal Demo

        if (chunkRemovalDemoEnabled_ && !pitOpen_) {
            editElapsed_ += deltaTime;


            if (editElapsed_ >= 3.0f) {
                const ChunkPosition targetPosition{ 0, 0, 0 };

                if (!voxel_.removeChunk(targetPosition)) {
                    throw std::runtime_error("Target Chunk does not exist");
                }




                pitOpen_ = true;


                std::cout << "[VoxelStorage] Chunk (0, 0, 0) REMOVED"
                        << " | Remaining Chunks: " << voxel_.blocks().chunkCount()
                        << " | Dirty Chunks: " << voxel_.blocks().dirtyChunkCount()
                        << std::endl;
            }
        }


        // Refresh the Selected Block After All VoxelStorage Changes

        updateBlockSelection();
    }

    std::uint64_t DemoScene::rebuildDirtyMeshes() {
        return voxel_.rebuildDirtyMeshes();
    }

    const ChunkMeshCache& DemoScene::chunkMeshes() const {
        return voxel_.meshes();
    }

    Camera& DemoScene::camera() {
        return camera_;
    }

    const Camera& DemoScene::camera() const {
        return camera_;
    }
    const CameraController& DemoScene::cameraController() const {
        return controller_;
    }
    std::uint64_t DemoScene::voxelMaterialId() const {
        return defaultMaterialId_;
    }

    ToneMappingSettings DemoScene::toneMappingSettings() const {
        ToneMappingSettings settings{};
        const char* preview = std::getenv("VOXELENGINE_DEMO_HDR");
        settings.enabled = preview != nullptr && preview[0] == '1' && preview[1] == '\0';
        settings.exposure = 1.0f;
        settings.method = ToneMappingOperator::Reinhard;
        return settings;
    }


    std::uint64_t DemoScene::voxelMaterialId( BlockMaterialKey material ) const {
        return voxelMaterials_.at(material);
    }

    VoxelWorld* DemoScene::voxelWorld() {
        return &voxel_;
    }

    const VoxelWorld* DemoScene::voxelWorld() const {
        return &voxel_;
    }

    const VoxelStorage& DemoScene::world() const {
        return voxel_.blocks();
    }
    const MeshData& DemoScene::cubeMesh() const {
        return assets_.mesh(cubeMeshId_);
    }
    const MaterialData& DemoScene::defaultMaterial() const {
        return assets_.material(defaultMaterialId_);
    }
    const Texture2D& DemoScene::texture() const {
        return assets_.texture(textureId_);
    }
    std::optional<VoxelRaycastHit> DemoScene::selectedBlock() const {
        return selectedBlock_;
    }
    std::uint64_t DemoScene::cubeMeshId() const {
        return cubeMeshId_;
    }
    float DemoScene::blockReach() const {
        return blockReach_;
    }
    bool DemoScene::chunkRemovalDemoEnabled() const {
        return chunkRemovalDemoEnabled_;
    }

    ShadingModel DemoScene::shadingModel() const {
        return shadingModel_;
    }

    std::uint8_t DemoScene::voxelTextureSetIndex() const {
        return activeVoxelTextureSet_;
    }

}
