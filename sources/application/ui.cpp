#include "imgui/imgui.h"
#include "imgui/ImGuizmo.h"

#include "scene.h"

static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);

static void show_info()
{
  if (ImGui::Begin("Info"))
  {
    ImGui::Text("ESC - exit");
    ImGui::Text("F5 - recompile shaders");
    ImGui::Text("Left Mouse Button and Wheel - controll camera");
  }
  ImGui::End();
}

static glm::vec2 world_to_screen(const UserCamera &camera, glm::vec3 world_position)
{
  glm::vec4 clipSpace = camera.projection * inverse(camera.transform) * glm::vec4(world_position, 1.f);
  glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;
  glm::vec2 screen = (glm::vec2(ndc) + 1.f) / 2.f;
  ImGuiIO &io = ImGui::GetIO();
  return glm::vec2(screen.x * io.DisplaySize.x, io.DisplaySize.y - screen.y * io.DisplaySize.y);
}

static void manipulate_transform(glm::mat4 &transform, const UserCamera &camera)
{
  ImGuizmo::BeginFrame();
  const glm::mat4 &projection = camera.projection;
  mat4 cameraView = inverse(camera.transform);
  ImGuiIO &io = ImGui::GetIO();
  ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

  glm::mat4 globNodeTm = transform;

  ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(projection), mCurrentGizmoOperation, mCurrentGizmoMode,
                       glm::value_ptr(globNodeTm));

  transform = globNodeTm;
}

static void show_characters(Scene &scene)
{
  // implement showing characters when only one character can be selected
  static uint32_t selectedCharacter = -1u;
  static uint32_t selectedNode = -1u;
  if (ImGui::Begin("Scene"))
  {
    for (size_t i = 0; i < scene.characters.size(); i++)
    {
      Character &character = scene.characters[i];
      ImGui::PushID(i);
      if (ImGui::Selectable(character.name.c_str(), selectedCharacter == i, ImGuiSelectableFlags_AllowDoubleClick))
      {
        selectedCharacter = i;
        if (ImGui::IsMouseDoubleClicked(0))
        {
          scene.userCamera.arcballCamera.targetPosition = vec3(character.transform[3]) + vec3(0, 1, 0);
        }
      }
      if (selectedCharacter == i)
      {
        if (ImGui::SliderFloat("linearVelocity", &character.linearVelocity, 0.f, 3.f))
        {
          // update the parameter of the BlendSpace1D controller
        }
        const char *animationState[] = {
          "Idle",
          "Walk",
        };
        int currentState = (int)character.state;

        if (ImGui::ListBox("AnimationState", &currentState, animationState, IM_ARRAYSIZE(animationState)))
        {
          character.state = (AnimationState)currentState;
          // set the state of the AnimationGraph controller
        }
        const float INDENT = 15.0f;
        ImGui::Indent(INDENT);
        ImGui::Text("Meshes: %zu", character.meshes.size());
        // show skeleton
        SkeletonInfo &skeletonInfo = character.skeletonInfo;
        ImGui::Text("Skeleton Nodes: %zu", skeletonInfo.names.size());
        for (size_t j = 0; j < skeletonInfo.names.size(); j++)
        {
          // show text with tabs for hierarchy
          const std::string &name = skeletonInfo.names[j];
          int depth = skeletonInfo.hierarchyDepth[j];
          std::string tabs(depth, ' ');
          std::string label = tabs + name;
          if (ImGui::Selectable(label.c_str(), selectedNode == j))
          {
            selectedNode = j;
          }
        }
        ImGui::Unindent(INDENT);
      }
      ImGui::PopID();
    }
    if (selectedCharacter < scene.characters.size())
    {
      Character &character = scene.characters[selectedCharacter];
      if (selectedNode < character.skeletonInfo.names.size())
      {
        glm::mat4 &worldTransform = reinterpret_cast<glm::mat4 &>(character.animationContext.worldTransforms[selectedNode]);
        glm::mat4 transform = character.transform * worldTransform;
        manipulate_transform(transform, scene.userCamera);
        worldTransform = inverse(character.transform) * transform;
      }
      else
      {
        manipulate_transform(character.transform, scene.userCamera);
      }
    }
  }
  ImGui::End();
  if (selectedCharacter < scene.characters.size())
  {
    Character &character = scene.characters[selectedCharacter];

    {
      const ImU32 flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;

      ImGuiIO& io = ImGui::GetIO();
      ImGui::SetNextWindowSize(io.DisplaySize);
      ImGui::SetNextWindowPos(ImVec2(0, 0));

      ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);
      ImGui::PushStyleColor(ImGuiCol_Border, 0);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

      ImGui::Begin("gizmo", NULL, flags);

      // vizualize skeleton
      ImDrawList* drawList = ImGui::GetWindowDrawList();
      // nice golden semi-transparent color
      const auto color = IM_COL32(255, 215, 0, 128);
      for (size_t j = 0; j < character.skeletonInfo.names.size(); j++)
      {
        const glm::mat4 &transform = reinterpret_cast<const glm::mat4 &>(character.animationContext.worldTransforms[j]);
        int parent = character.skeletonInfo.parents[j];
        if (parent > 0)
        {
          const glm::mat4 &parentTransform = reinterpret_cast<const glm::mat4 &>(character.animationContext.worldTransforms[parent]);
          const glm::vec3 &from = glm::vec3(parentTransform[3]);
          const glm::vec3 &to = glm::vec3(transform[3]);
          const glm::vec2 fromScreen = world_to_screen(scene.userCamera, from);
          const glm::vec2 toScreen = world_to_screen(scene.userCamera, to);
          drawList->AddLine(ImVec2(fromScreen.x, fromScreen.y), ImVec2(toScreen.x, toScreen.y), color, 2.f);
        }
      }
      ImGui::End();
      ImGui::PopStyleVar();
      ImGui::PopStyleColor(2);
    }
  }
}

void render_imguizmo(ImGuizmo::OPERATION &mCurrentGizmoOperation, ImGuizmo::MODE &mCurrentGizmoMode)
{
  if (ImGui::Begin("gizmo window"))
  {
    if (ImGui::IsKeyPressed('Z'))
      mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed('E'))
      mCurrentGizmoOperation = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed('R')) // r Key
      mCurrentGizmoOperation = ImGuizmo::SCALE;
    if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
      mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
      mCurrentGizmoOperation = ImGuizmo::ROTATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
      mCurrentGizmoOperation = ImGuizmo::SCALE;

    if (mCurrentGizmoOperation != ImGuizmo::SCALE)
    {
      if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
        mCurrentGizmoMode = ImGuizmo::LOCAL;
      ImGui::SameLine();
      if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
        mCurrentGizmoMode = ImGuizmo::WORLD;
    }
  }
  ImGui::End();
}

static void show_models(Scene &scene)
{
  if (ImGui::Begin("Models"))
  {
    static uint32_t selectedModel = -1u;
    for (size_t i = 0; i < scene.models.size(); i++)
    {
      ImGui::PushID(i);
      const ModelAsset &model = scene.models[i];
      if (ImGui::Selectable(model.path.c_str(), selectedModel == i))
      {
        selectedModel = i;
      }
      if (selectedModel == i)
      {
        ImGui::Indent(15.0f);
        ImGui::Text("Path: %s", model.path.c_str());
        ImGui::Text("Meshes: %zu", model.meshes.size());

        for (size_t j = 0; j < model.meshes.size(); j++)
        {
          ImGui::PushID(j);
          const MeshPtr &mesh = model.meshes[j];
          ImGui::Text("%s", mesh->name.c_str());
          ImGui::Text("Bones: %zu", mesh->bonesNames.size());
          if (ImGui::TreeNode("", "Bones: %zu", mesh->bonesNames.size()))
          {
            int boneIndex = 0;
            for (const std::string &boneName : mesh->bonesNames)
            {
              ImGui::Text("%d) %s", boneIndex, boneName.c_str());
              boneIndex++;
            }
            ImGui::TreePop();
          }
          ImGui::PopID();
        }
        // show skeleton
        const SkeletonOffline &skeleton = model.skeleton;
        ImGui::Text("Skeleton Nodes: %zu", skeleton.names.size());
        for (size_t j = 0; j < skeleton.names.size(); j++)
        {
          // show text with tabs for hierarchy
          const std::string &name = skeleton.names[j];
          int depth = skeleton.hierarchyDepth[j];
          std::string tabs(depth, ' ');
          ImGui::Text("%s%s", tabs.c_str(), name.c_str());
        }
        ImGui::Unindent(15.0f);
      }
      ImGui::PopID();
    }
  }
  ImGui::End();
}

static void show_physics(Scene &scene)
{
  if (ImGui::Begin("Phisics"))
  {
    if (!scene.physicsWorld)
    {
      if (ImGui::Button("Create Physics World"))
      {
        scene.physicsWorld = std::make_unique<PhysicsWorld>();
      }
    }
    else
    {
      glm::mat4 worldToScreen = scene.userCamera.projection * inverse(scene.userCamera.transform);
      scene.physicsWorld->debug_render(worldToScreen, vec3(scene.userCamera.transform[3]));
      if (ImGui::Button("Create Ragdoll"))
      {
        auto r = scene.physicsWorld->create_ragdoll(0);
        r->AddToPhysicsSystem(JPH::EActivation::Activate);
      }
      if (ImGui::Button("Destroy Physics World"))
      {
        scene.physicsWorld.reset();
      }
    }
  }
  ImGui::End();
}

void application_imgui_render(Scene &scene)
{
  render_imguizmo(mCurrentGizmoOperation, mCurrentGizmoMode);

  show_info();
  show_characters(scene);
  show_models(scene);
  show_physics(scene);
}