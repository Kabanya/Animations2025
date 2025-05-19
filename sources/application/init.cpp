#include "scene.h"
#include "blend_space_2d.h"
#include "blend_space_1d.h"
#include <memory>
#include "character.h"
#include "animation_graph.h"
#include "single_animation.h"
#include "engine/3dmath.h"
#include "engine/import/model.h"
#include "engine/render/material.h"
#include "engine/render/mesh.h"

static glm::mat4 get_projective_matrix()
{
  const float fovY = 90.f * DegToRad;
  const float zNear = 0.01f;
  const float zFar = 500.f;
  return glm::perspective(fovY, engine::get_aspect_ratio(), zNear, zFar);
}

void application_init(Scene& scene)
{
  scene.light.lightDirection = glm::normalize(glm::vec3(-1, -1, 0));
  scene.light.lightColor = glm::vec3(1.f);
  scene.light.ambient = glm::vec3(0.2f);
  scene.userCamera.projection = get_projective_matrix();

  engine::onWindowResizedEvent += [&](const std::pair<int, int>&)
    { scene.userCamera.projection = get_projective_matrix(); };

  ArcballCamera& cam = scene.userCamera.arcballCamera;
  cam.curZoom = cam.targetZoom = 0.5f;
  cam.maxdistance = 5.f;
  cam.distance = cam.curZoom * cam.maxdistance;
  cam.lerpStrength = 10.f;
  cam.mouseSensitivity = 0.5f;
  cam.wheelSensitivity = 0.05f;
  cam.targetPosition = glm::vec3(0.f, 1.f, 0.f);
  cam.targetRotation = cam.curRotation = glm::vec2(DegToRad * -90.f, DegToRad * -30.f);
  cam.rotationEnable = false;

  scene.userCamera.transform = calculate_transform(scene.userCamera.arcballCamera);

  engine::onMouseButtonEvent += [&](const SDL_MouseButtonEvent& e)
    { arccam_mouse_click_handler(e, scene.userCamera.arcballCamera); };
  engine::onMouseMotionEvent += [&](const SDL_MouseMotionEvent& e)
    { arccam_mouse_move_handler(e, scene.userCamera.arcballCamera, scene.userCamera.transform); };
  engine::onMouseWheelEvent += [&](const SDL_MouseWheelEvent& e)
    { arccam_mouse_wheel_handler(e, scene.userCamera.arcballCamera); };

  engine::onKeyboardEvent += [](const SDL_KeyboardEvent& e)
    { if (e.keysym.sym == SDLK_F5 && e.state == SDL_RELEASED) recompile_all_shaders(); };

  auto material = make_material("character", "sources/shaders/character_vs.glsl", "sources/shaders/character_ps.glsl");
  material->set_property("mainTex", create_texture2d("resources/MotusMan_v55/MCG_diff.jpg"));

  ModelAsset motusMan = load_model("resources/MotusMan_v55/MotusMan_v55.fbx");
  ModelAsset ruby = load_model("resources/sketchfab/ruby.fbx");
  ModelAsset MOB1_Stand_Relaxed_Idle_v2_IPC = load_model("resources/Animations/IPC/MOB1_Stand_Relaxed_Idle_v2_IPC.fbx");
  ModelAsset MOB1_Walk_F_Loop_IPC = load_model("resources/Animations/IPC/MOB1_Walk_F_Loop_IPC.fbx");
  ModelAsset MOB1_Jog_F_Loop_IPC = load_model("resources/Animations/IPC/MOB1_Jog_F_Loop_IPC.fbx");
  ModelAsset MOB1_Run_F_Loop_IPC = load_model("resources/Animations/IPC/MOB1_Run_F_Loop_IPC.fbx");

  ModelAsset MOB1_Walk_F_To_Stand_Relaxed_IPC = load_model("resources/Animations/IPC/MOB1_Walk_F_To_Stand_Relaxed_IPC.fbx");
  ModelAsset MOB1_Jog_F_To_Stand_Relaxed_IPC = load_model("resources/Animations/IPC/MOB1_Jog_F_To_Stand_Relaxed_IPC.fbx");
  ModelAsset MOB1_Run_F_To_Stand_Relaxed_IPC = load_model("resources/Animations/IPC/MOB1_Run_F_To_Stand_Relaxed_IPC.fbx");

  ModelAsset MOB1_Stand_Relaxed_To_Walk_F_IPC = load_model("resources/Animations/IPC/MOB1_Stand_Relaxed_To_Walk_F_IPC.fbx");
  ModelAsset MOB1_Stand_Relaxed_To_Jog_F_IPC = load_model("resources/Animations/IPC/MOB1_Stand_Relaxed_To_Jog_F_IPC.fbx");
  ModelAsset MOB1_Stand_Relaxed_To_Run_F_IPC = load_model("resources/Animations/IPC/MOB1_Stand_Relaxed_To_Run_F_IPC.fbx");

  ModelAsset MOB1_Stand_Relaxed_Jump_IPC = load_model("resources/Animations/IPC/MOB1_Stand_Relaxed_Jump_IPC.fbx");
  ModelAsset MOB1_Walk_F_Jump_LU_IPC = load_model("resources/Animations/IPC/MOB1_Walk_F_Jump_LU_IPC.fbx");
  ModelAsset MOB1_Walk_F_Jump_RU_IPC = load_model("resources/Animations/IPC/MOB1_Walk_F_Jump_RU_IPC.fbx");

	ModelAsset MOB1_Run_R_Loop_IPC = load_model("resources/Animations/IPC/MOB1_Run_R_Loop_IPC.fbx");
	ModelAsset MOB1_Run_L_Loop_IPC = load_model("resources/Animations/IPC/MOB1_Run_L_Loop_IPC.fbx");
	ModelAsset MOB1_Jog_B_Loop_IPC = load_model("resources/Animations/IPC/MOB1_Jog_B_Loop_IPC.fbx");

  //// BlendSpace1D
  //{
  //  Character character;
  //  character.name = "MotusMan_v55";
  //  character.transform = glm::translate(glm::identity<glm::mat4>(), glm::vec3(-4.f, 0.f, 0.f));
  //  character.meshes = motusMan.meshes;
  //  character.material = std::move(material);
  //  character.skeletonInfo = SkeletonInfo(motusMan.skeleton);
  //  character.animationContext.setup(MOB1_Walk_F_Loop_IPC.skeleton.skeleton.get());
  //  std::vector<AnimationNode1D> movementAnimations = {
  //    {MOB1_Walk_F_Loop_IPC.animations[0].get(), 1.f},
  //    {MOB1_Jog_F_Loop_IPC.animations[0].get(), 2.f},
  //    {MOB1_Run_F_Loop_IPC.animations[0].get(), 3.f}
  //  };
  //  std::vector<AnimationNode1D> idleToMovementAnimations = {
  //    {MOB1_Stand_Relaxed_To_Walk_F_IPC.animations[0].get(), 1.f},
  //    {MOB1_Stand_Relaxed_To_Jog_F_IPC.animations[0].get(), 2.f},
  //    {MOB1_Stand_Relaxed_To_Run_F_IPC.animations[0].get(), 3.f}
  //  };
  //  std::vector<AnimationNode1D> movementToIdleAnimations = {
  //    {MOB1_Walk_F_To_Stand_Relaxed_IPC.animations[0].get(), 1.f},
  //    {MOB1_Jog_F_To_Stand_Relaxed_IPC.animations[0].get(), 2.f},
  //    {MOB1_Run_F_To_Stand_Relaxed_IPC.animations[0].get(), 3.f}
  //  };

  //  std::vector<AnimationGraphNode> nodes(4);
  //  nodes[0].animation = std::make_shared<SingleAnimation>(MOB1_Stand_Relaxed_Idle_v2_IPC.animations[0].get());
  //  nodes[0].state = AnimationState::Idle;
  //  nodes[1].animation = std::make_shared<BlendSpace1D>(std::move(movementAnimations));
  //  nodes[1].state = AnimationState::Movement;

  //  nodes[0].edges.push_back({ &nodes[0], &nodes[1], std::make_shared<BlendSpace1D>(std::move(idleToMovementAnimations)), 0.4f });
  //  nodes[1].edges.push_back({ &nodes[1], &nodes[0], std::make_shared<BlendSpace1D>(std::move(movementToIdleAnimations)), 0.4f });

  //  character.controllers.push_back(std::make_shared<AnimationGraph>(std::move(nodes), AnimationState::Idle));
  //  scene.characters.push_back(std::move(character));
  //}


  // main for BlendSpace2D
  {
    Character character;
    character.name = "MotusMan_BlendSpace2D_Task_3.1_Running";
    character.transform = glm::identity<glm::mat4>();
    character.meshes = motusMan.meshes;
    character.material = make_material("character", "sources/shaders/character_vs.glsl", "sources/shaders/character_ps.glsl");
    character.material->set_property("mainTex", create_texture2d("resources/MotusMan_v55/MCG_diff.jpg"));
    character.skeletonInfo = SkeletonInfo(motusMan.skeleton);
    character.animationContext.setup(motusMan.skeleton.skeleton.get());

    std::vector<AnimationNode2D> nodes2d = {
      {MOB1_Run_R_Loop_IPC.animations[0].get(), -1.f,  1.f},
      {MOB1_Run_F_Loop_IPC.animations[0].get(), -1.f, -1.f},
      {MOB1_Run_L_Loop_IPC.animations[0].get(),   1.f, -1.f},
      {MOB1_Jog_B_Loop_IPC.animations[0].get(),   1.f,  1.f}
    };
    character.controllers.push_back(std::make_shared<BlendSpace2D>(std::move(nodes2d)));

    scene.characters.push_back(std::move(character));
  }

  // BlendSpace2D Idle - Бег
  {
    Character character;
    character.name = "MotusMan_BlendSpace2D_Task_3.1_Idle_Run";
    character.transform = glm::translate(glm::identity<glm::mat4>(), glm::vec3(-4.f, 0.f, 0.f));
    character.meshes = motusMan.meshes;
    character.material = make_material("character", "sources/shaders/character_vs.glsl", "sources/shaders/character_ps.glsl");
    character.material->set_property("mainTex", create_texture2d("resources/MotusMan_v55/MCG_diff.jpg"));
    character.skeletonInfo = SkeletonInfo(motusMan.skeleton);
    character.animationContext.setup(motusMan.skeleton.skeleton.get());

    std::vector<AnimationNode2D> nodes2d = {
      {MOB1_Jog_F_Loop_IPC.animations[0].get(), -1.f,  1.f},
      {MOB1_Walk_F_Loop_IPC.animations[0].get(), -1.f, -1.f},
      {MOB1_Run_F_Loop_IPC.animations[0].get(),   1.f, -1.f},
      {MOB1_Stand_Relaxed_Idle_v2_IPC.animations[0].get(),   1.f,  1.f}
    };
    character.controllers.push_back(std::make_shared<BlendSpace2D>(std::move(nodes2d)));

    scene.characters.push_back(std::move(character));
  }

  // GraphController
  {
    Character character;
    character.name = "MotusMan_GraphController_Task_3.2";
    character.transform = glm::translate(glm::identity<glm::mat4>(), glm::vec3(-2.f, 0.f, 0.f));
    character.meshes = motusMan.meshes;
    character.material = make_material("character", "sources/shaders/character_vs.glsl", "sources/shaders/character_ps.glsl");
    character.material->set_property("mainTex", create_texture2d("resources/MotusMan_v55/MCG_diff.jpg"));
    character.skeletonInfo = SkeletonInfo(motusMan.skeleton);
    character.animationContext.setup(motusMan.skeleton.skeleton.get());

    std::vector<AnimationGraphNode> nodes(4);
    nodes[0].animation = std::make_shared<SingleAnimation>(MOB1_Stand_Relaxed_Idle_v2_IPC.animations[0].get());
    nodes[0].state = AnimationState::Idle;
    nodes[1].animation = std::make_shared<BlendSpace1D>(std::vector<AnimationNode1D>{
      {MOB1_Walk_F_Loop_IPC.animations[0].get(), 1.0f},
      { MOB1_Run_F_Loop_IPC.animations[0].get(), 3.0f }
    });
    nodes[1].state = AnimationState::Movement;
    nodes[2].animation = std::make_shared<SingleAnimation>(MOB1_Walk_F_Jump_LU_IPC.animations[0].get());
    nodes[2].state = AnimationState::JumpLeft;
    nodes[3].animation = std::make_shared<SingleAnimation>(MOB1_Walk_F_Jump_RU_IPC.animations[0].get());
    nodes[3].state = AnimationState::JumpRight;

    // Idle -> Movement (speed > 0.1f && !isJumping)
    {
      AnimationGraphEdge edge;
      edge.from = &nodes[0];
      edge.to = &nodes[1];
      edge.animation = nullptr;
      edge.transitionDuration = 0.3f;
      edge.transitionStartTime = 1.0f;
      edge.condition = [](const std::unordered_map<std::string, float>& params) {
        auto itSpeed = params.find("speed");
        auto itJump = params.find("isJumping");
        return itSpeed != params.end() && itJump != params.end() && itSpeed->second > 0.1f && itJump->second == 0.0f;
        };
      nodes[0].edges.push_back(std::move(edge));
    }
    // Movement -> Idle (speed == 0.0f && !isJumping)
    {
      AnimationGraphEdge edge;
      edge.from = &nodes[1];
      edge.to = &nodes[0];
      edge.animation = nullptr;
      edge.transitionDuration = 0.3f;
      edge.transitionStartTime = 1.0f;
      edge.condition = [](const std::unordered_map<std::string, float>& params) {
        auto itSpeed = params.find("speed");
        auto itJump = params.find("isJumping");
        return itSpeed != params.end() && itJump != params.end() && std::abs(itSpeed->second) < 0.05f && itJump->second == 0.0f; };
      nodes[1].edges.push_back(std::move(edge));
    }
    // Movement -> JumpLeft (isJumping && walkFoot == 0)
    {
      AnimationGraphEdge edge;
      edge.from = &nodes[1];
      edge.to = &nodes[2];
      edge.animation = nullptr;
      edge.transitionDuration = 0.1f;
      edge.transitionProgress = 0.0f;
      edge.condition = [](const std::unordered_map<std::string, float>& params) {
        auto itJump = params.find("isJumping");
        auto itFoot = params.find("walkFoot");
        return itJump != params.end() && itFoot != params.end() && itJump->second == 1.0f && itFoot->second == 0.0f;
        };
      nodes[1].edges.push_back(std::move(edge));
    }
    // Movement -> JumpRight (isJumping && walkFoot == 1)
    {
      AnimationGraphEdge edge;
      edge.from = &nodes[1];
      edge.to = &nodes[3];
      edge.animation = nullptr;
      edge.transitionDuration = 0.1f;
      edge.transitionProgress = 0.0f;
      edge.condition = [](const std::unordered_map<std::string, float>& params) {
        auto itJump = params.find("isJumping");
        auto itFoot = params.find("walkFoot");
        return itJump != params.end() && itFoot != params.end() && itJump->second == 1.0f && itFoot->second == 1.0f;
        };
      nodes[1].edges.push_back(std::move(edge));
    }
    // Idle -> JumpLeft (isJumping && walkFoot == 0)
    {
      AnimationGraphEdge edge;
      edge.from = &nodes[0];
      edge.to = &nodes[2];
      edge.animation = std::make_shared<SingleAnimation>(MOB1_Stand_Relaxed_Jump_IPC.animations[0].get());
      edge.transitionDuration = 0.1f;
      edge.condition = [](const std::unordered_map<std::string, float>& params) {
        auto itJump = params.find("isJumping");
        auto itFoot = params.find("walkFoot");
        return itJump != params.end() && itFoot != params.end() && itJump->second == 1.0f && itFoot->second == 0.0f;
        };
      nodes[0].edges.push_back(std::move(edge));
    }
    // JumpLeft -> Movement (isJumping == 0)
    {
      AnimationGraphEdge edge;
      edge.from = &nodes[2];
      edge.to = &nodes[1];
      edge.animation = nullptr;
      edge.transitionDuration = 0.2f;
      edge.condition = [](const std::unordered_map<std::string, float>& params) {
        auto itJump = params.find("isJumping");
        return itJump != params.end() && itJump->second == 0.0f;
        };
      nodes[2].edges.push_back(std::move(edge));
    }
    // JumpRight -> Movement (isJumping == 0)
    {
      AnimationGraphEdge edge;
      edge.from = &nodes[3];
      edge.to = &nodes[1];
      edge.animation = nullptr;
      edge.transitionDuration = 0.2f;
      edge.condition = [](const std::unordered_map<std::string, float>& params) {
        auto itJump = params.find("isJumping");
        return itJump != params.end() && itJump->second == 0.0f;
        };
      nodes[3].edges.push_back(std::move(edge));
    }

    character.controllers.push_back(std::make_shared<AnimationGraph>(std::move(nodes), AnimationState::Idle));
    scene.characters.push_back(std::move(character));
  }

  auto whiteMaterial = make_material("character", "sources/shaders/character_vs.glsl", "sources/shaders/character_ps.glsl");
  const uint8_t whiteColor[4] = { 255, 255, 255, 255 };
  whiteMaterial->set_property("mainTex", create_texture2d(whiteColor, 1, 1, 4));

  {
    Character character;
    character.name = "Ruby";
    character.transform = glm::translate(glm::identity<glm::mat4>(), glm::vec3(2.f, 0.f, 0.f));
    character.meshes = ruby.meshes;
    character.material = std::move(whiteMaterial);
    character.skeletonInfo = SkeletonInfo(ruby.skeleton);
    character.animationContext.setup(ruby.skeleton.skeleton.get());
    character.controllers.push_back(std::make_shared<SingleAnimation>(ruby.animations[0].get()));
    scene.characters.push_back(std::move(character));
  }

  scene.models.push_back(std::move(motusMan));
  scene.models.push_back(std::move(ruby));
  scene.models.push_back(std::move(MOB1_Stand_Relaxed_Idle_v2_IPC));
  scene.models.push_back(std::move(MOB1_Walk_F_Loop_IPC));
  scene.models.push_back(std::move(MOB1_Jog_F_Loop_IPC));
  scene.models.push_back(std::move(MOB1_Run_F_Loop_IPC));
  scene.models.push_back(std::move(MOB1_Walk_F_To_Stand_Relaxed_IPC));
  scene.models.push_back(std::move(MOB1_Jog_F_To_Stand_Relaxed_IPC));
  scene.models.push_back(std::move(MOB1_Run_F_To_Stand_Relaxed_IPC));
  scene.models.push_back(std::move(MOB1_Stand_Relaxed_To_Walk_F_IPC));
  scene.models.push_back(std::move(MOB1_Stand_Relaxed_To_Jog_F_IPC));
  scene.models.push_back(std::move(MOB1_Stand_Relaxed_To_Run_F_IPC));
  scene.models.push_back(std::move(MOB1_Stand_Relaxed_Jump_IPC));
  scene.models.push_back(std::move(MOB1_Walk_F_Jump_LU_IPC));
  scene.models.push_back(std::move(MOB1_Walk_F_Jump_RU_IPC));
	scene.models.push_back(std::move(MOB1_Run_R_Loop_IPC));
	scene.models.push_back(std::move(MOB1_Run_L_Loop_IPC));
	scene.models.push_back(std::move(MOB1_Jog_B_Loop_IPC));

  std::fflush(stdout);
}
