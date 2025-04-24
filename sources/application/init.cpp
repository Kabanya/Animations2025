#include "scene.h"
#include "motion_matching/feature_data_base.h"

JPH::Ref<JPH::RagdollSettings> create_ragdoll_settings(const SkeletonPtr &skeleton_src);

static glm::mat4 get_projective_matrix()
{
  const float fovY = 90.f * DegToRad;
  const float zNear = 0.01f;
  const float zFar = 500.f;
  return glm::perspective(fovY, engine::get_aspect_ratio(), zNear, zFar);
}

void application_init(Scene &scene)
{
  init_phys_globals();
  scene.light.lightDirection = glm::normalize(glm::vec3(-1, -1, 0));
  scene.light.lightColor = glm::vec3(1.f);
  scene.light.ambient = glm::vec3(0.2f);
  scene.userCamera.projection = get_projective_matrix();

  engine::onWindowResizedEvent += [&](const std::pair<int, int> &)
  { scene.userCamera.projection = get_projective_matrix(); };

  ArcballCamera &cam = scene.userCamera.arcballCamera;
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

  engine::onMouseButtonEvent += [&](const SDL_MouseButtonEvent &e)
  { arccam_mouse_click_handler(e, scene.userCamera.arcballCamera); };
  engine::onMouseMotionEvent += [&](const SDL_MouseMotionEvent &e)
  { arccam_mouse_move_handler(e, scene.userCamera.arcballCamera, scene.userCamera.transform); };
  engine::onMouseWheelEvent += [&](const SDL_MouseWheelEvent &e)
  { arccam_mouse_wheel_handler(e, scene.userCamera.arcballCamera); };

  engine::onKeyboardEvent += [](const SDL_KeyboardEvent &e)
  { if (e.keysym.sym == SDLK_F5 && e.state == SDL_RELEASED) recompile_all_shaders(); };

  auto material = make_material("character", "sources/shaders/character_vs.glsl", "sources/shaders/character_ps.glsl");
  material->set_property("mainTex", create_texture2d("resources/MotusMan_v55/MCG_diff.jpg"));

  scene.animationDataBase = load_animations("resources/Animations/Animations.ozz");

  scene.featureDataBase = build_feature_data_base(scene.animationDataBase);
  ModelAsset motusMan = load_model("resources/MotusMan_v55/MotusMan_v55.fbx");
  ModelAsset ruby = load_model("resources/sketchfab/ruby.fbx");

  const bool initBlendTreeCharacter = false;
  if (initBlendTreeCharacter)
  {
    Character character;
    character.name = "MotusMan_v55";
    character.transform = glm::identity<glm::mat4>();
    character.meshes = motusMan.meshes;
    character.material = material;
    character.skeletonInfo = SkeletonInfo(motusMan.skeleton);
    character.ragdollSettings = create_ragdoll_settings(motusMan.skeleton.skeleton);
    character.animationContext.setup(motusMan.skeleton.skeleton.get());
    std::vector<AnimationNode1D> movementAnimations = {
      {scene.animationDataBase.find_animation("MOB1_Walk_F_Loop_IPC"), 1.f},
      {scene.animationDataBase.find_animation("MOB1_Jog_F_Loop_IPC"), 2.f},
      {scene.animationDataBase.find_animation("MOB1_Run_F_Loop_IPC"), 3.f}
    };
    std::vector<AnimationNode1D> idleToMovementAnimations = {
      {scene.animationDataBase.find_animation("MOB1_Stand_Relaxed_To_Walk_F_IPC"), 1.f},
      {scene.animationDataBase.find_animation("MOB1_Stand_Relaxed_To_Jog_F_IPC"), 2.f},
      {scene.animationDataBase.find_animation("MOB1_Stand_Relaxed_To_Run_F_IPC"), 3.f}
    };
    std::vector<AnimationNode1D> movementToIdleAnimations = {
      {scene.animationDataBase.find_animation("MOB1_Walk_F_To_Stand_Relaxed_IPC"), 1.f},
      {scene.animationDataBase.find_animation("MOB1_Jog_F_To_Stand_Relaxed_IPC"), 2.f},
      {scene.animationDataBase.find_animation("MOB1_Run_F_To_Stand_Relaxed_IPC"), 3.f}
    };


    std::vector<AnimationGraphNode> nodes(4);
    nodes[0].animation = std::make_shared<SingleAnimation>(scene.animationDataBase.find_animation("MOB1_Stand_Relaxed_Idle_v2_IPC"));
    nodes[0].state = AnimationState::Idle;
    nodes[1].animation = std::make_shared<BlendSpace1D>(std::move(movementAnimations));
    nodes[1].state = AnimationState::Movement;

    nodes[0].edges.push_back({&nodes[0], &nodes[1], std::make_shared<BlendSpace1D>(std::move(idleToMovementAnimations)), 0.4f});
    nodes[1].edges.push_back({&nodes[1], &nodes[0], std::make_shared<BlendSpace1D>(std::move(movementToIdleAnimations)), 0.4f});

    character.controllers.push_back(std::make_shared<AnimationGraph>(std::move(nodes), AnimationState::Idle));

    scene.characters.push_back(std::move(character));
  }
  const bool initMxMCharacter = true;
  if (initMxMCharacter)
  {
    Character character;
    character.name = "MotusMan_v55";
    character.transform = glm::identity<glm::mat4>();
    character.meshes = motusMan.meshes;
    character.material = material;
    character.skeletonInfo = SkeletonInfo(motusMan.skeleton);
    character.ragdollSettings = create_ragdoll_settings(motusMan.skeleton.skeleton);
    character.animationContext.setup(motusMan.skeleton.skeleton.get());
    character.controllers.push_back(std::make_shared<SingleAnimation>(scene.animationDataBase.animations[0].get()));
    scene.characters.push_back(std::move(character));
  }

  const bool initRubyCharacter = false;
  if (initRubyCharacter)
  {
    auto whiteMaterial = make_material("character", "sources/shaders/character_vs.glsl", "sources/shaders/character_ps.glsl");

    const uint8_t whiteColor[4] = {255, 255, 255, 255};
    whiteMaterial->set_property("mainTex", create_texture2d(whiteColor, 1, 1, 4));
    Character character;
    character.name = "Ruby";
    character.transform = glm::translate(glm::identity<glm::mat4>(), glm::vec3(2.f, 0.f, 0.f));
    character.meshes = ruby.meshes;
    character.material = std::move(whiteMaterial);
    character.skeletonInfo = SkeletonInfo(ruby.skeleton);
    character.animationContext.setup(ruby.skeleton.skeleton.get());
    character.selectedAnimation = 0;
    character.controllers.push_back(std::make_shared<SingleAnimation>(ruby.animations[character.selectedAnimation].get()));
    scene.characters.push_back(std::move(character));
  }

  scene.models.push_back(std::move(motusMan));
  scene.models.push_back(std::move(ruby));


  std::fflush(stdout);
}