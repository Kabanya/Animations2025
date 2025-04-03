#include "physics_world.h"
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/Collision/Shape/PlaneShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Renderer/DebugRendererSimple.h>

#include "imgui/imgui.h" // for debug rendering

static glm::vec2 world_to_screen(const glm::mat4 &world_to_screen, glm::vec3 world_position, glm::vec2 display_size)
{
  glm::vec4 clipSpace = world_to_screen * glm::vec4(world_position, 1.f);
  glm::vec3 ndc = glm::vec3(clipSpace) / std::max(clipSpace.w, 0.01f);
  glm::vec2 screen = (glm::vec2(ndc) + 1.f) / 2.f;
  return glm::vec2(screen.x * display_size.x, display_size.y - screen.y * display_size.y);
}

static glm::vec3 to_vec3(const JPH::Vec3 &v)
{
  return glm::vec3(v.GetX(), v.GetY(), v.GetZ());
}

class ImGuiDebugRenderer : public JPH::DebugRendererSimple
{
public:
  glm::mat4 mWorldToScreen;
  glm::vec2 mDisplaySize;

  void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override
  {
    glm::vec2 from = world_to_screen(mWorldToScreen, to_vec3(inFrom), mDisplaySize);
    glm::vec2 to = world_to_screen(mWorldToScreen, to_vec3(inTo), mDisplaySize);

    ImDrawList *drawList = ImGui::GetForegroundDrawList();
    inColor.a /= 2;
    drawList->AddLine(ImVec2(from.x, from.y), ImVec2(to.x, to.y), inColor.GetUInt32());
  }

  void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) override
  {
    glm::vec2 v1 = world_to_screen(mWorldToScreen, to_vec3(inV1), mDisplaySize);
    glm::vec2 v2 = world_to_screen(mWorldToScreen, to_vec3(inV2), mDisplaySize);
    glm::vec2 v3 = world_to_screen(mWorldToScreen, to_vec3(inV3), mDisplaySize);

    ImDrawList *drawList = ImGui::GetForegroundDrawList();
    inColor.a /= 2;
    drawList->AddTriangle(ImVec2(v1.x, v1.y), ImVec2(v2.x, v2.y), ImVec2(v3.x, v3.y), inColor.GetUInt32());

  }

  void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view &inString, JPH::ColorArg inColor, float inHeight) override
  {
    glm::vec2 position = world_to_screen(mWorldToScreen, to_vec3(inPosition), mDisplaySize);
    ImDrawList *drawList = ImGui::GetForegroundDrawList();
    inColor.a /= 2;
    drawList->AddText(/* nullptr, inHeight, */ImVec2(position.x, position.y), inColor.GetUInt32(), inString.data());
  }
};

class StubBroadPhaseLayer final : public JPH::BroadPhaseLayerInterface
{
public:
  JPH::uint GetNumBroadPhaseLayers() const override
  {
    return 1;
  }

  JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
  {
    return JPH::BroadPhaseLayer(0x00);
  }
};


static StubBroadPhaseLayer sStubBroadPhaseLayer;
static JPH::ObjectVsBroadPhaseLayerFilter sObjectVsBroadPhaseLayerFilter;
static JPH::ObjectLayerPairFilter sObjectLayerPairFilter;

PhysicsWorld::PhysicsWorld()
{
  mTempAllocator = std::make_unique<JPH::TempAllocatorImpl>(1024 * 1024);

  mJobSystem = std::make_unique<JPH::JobSystemSingleThreaded>(JPH::cMaxPhysicsJobs);
  int maxBodies = 1024;
  int bodyMutexes = 0; // use 0 to autodetect
  int maxBodiesPairs = 1024;
  int maxBodiesConstraints = 1024;
  mPhysicsSystem.Init(maxBodies, bodyMutexes, maxBodiesPairs, maxBodiesConstraints, sStubBroadPhaseLayer, sObjectVsBroadPhaseLayerFilter, sObjectLayerPairFilter);

  JPH::BodyInterface &bodyInterface = mPhysicsSystem.GetBodyInterface();

  {
    JPH::BodyCreationSettings settings(new JPH::BoxShape(JPH::Vec3(10.0f, 0.1f, 10.0f)), JPH::Vec3(0, -0.1f, 0), JPH::Quat::sIdentity(), JPH::EMotionType::Static, sObjectLayer);
    JPH::Body *floor = bodyInterface.CreateBody(settings);
    bodyInterface.AddBody(floor->GetID(), JPH::EActivation::DontActivate);
  }

  for (JPH::Vec3 position : {JPH::Vec3(2, 2, 0), JPH::Vec3(-2, 3, 0), JPH::Vec3(-1, 4, 2)})
  {
    JPH::BodyCreationSettings settings(new JPH::BoxShape(JPH::Vec3(0.5f, 0.5f, 0.5f)), position, JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, sObjectLayer);
    settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateMassAndInertia;
    settings.mMassPropertiesOverride.mMass = 1.0f;
    JPH::Body *cube = bodyInterface.CreateBody(settings);
    bodyInterface.AddBody(cube->GetID(), JPH::EActivation::Activate);
  }
}

void PhysicsWorld::update_physics(float dt)
{
  const float fixedTimeStep = 1.0f / 60.0f;
  mAccumuletedDeltaTime += dt;
  while (mAccumuletedDeltaTime >= fixedTimeStep)
  {
    mAccumuletedDeltaTime -= fixedTimeStep;
    // We should update physics with fixed delta time
    mPhysicsSystem.Update(fixedTimeStep, 1, mTempAllocator.get(), mJobSystem.get());
  }
}
void PhysicsWorld::debug_render(const glm::mat4 &world_to_screen, glm::vec3 camera_position)
{
  ImGuiDebugRenderer *renderer = static_cast<ImGuiDebugRenderer *>(JPH::DebugRenderer::sInstance);
  renderer->mWorldToScreen = world_to_screen;
  ImGuiIO &io = ImGui::GetIO();
  renderer->mDisplaySize = glm::vec2(io.DisplaySize.x, io.DisplaySize.y);
  renderer->SetCameraPos(JPH::Vec3(camera_position.x, camera_position.y, camera_position.z));
  mPhysicsSystem.DrawBodies(JPH::BodyManager::DrawSettings(), JPH::DebugRenderer::sInstance);
  mPhysicsSystem.DrawConstraints(JPH::DebugRenderer::sInstance);
  mPhysicsSystem.DrawConstraintLimits(JPH::DebugRenderer::sInstance);
}

void init_phys_globals()
{
  JPH::RegisterDefaultAllocator();
  JPH::Factory::sInstance = new JPH::Factory();
  JPH::DebugRenderer::sInstance = new ImGuiDebugRenderer();
  JPH::RegisterTypes();
}

void destroy_phys_globals()
{
  delete JPH::Factory::sInstance;
  JPH::Factory::sInstance = nullptr;
  delete JPH::DebugRenderer::sInstance;
  JPH::DebugRenderer::sInstance = nullptr;
}
