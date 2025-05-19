#include <algorithm>

#include "animation_graph.h"
#include "imgui/imgui.h"
#include "imgui/ImGuizmo.h"
#include "imgui/imgui_internal.h"

#include "scene.h"
#include "blend_space_2d.h"

static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);

static bool showSkeleton = true;

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

static glm::vec2 world_to_screen(const UserCamera& camera, glm::vec3 world_position)
{
  glm::vec4 clipSpace = camera.projection * inverse(camera.transform) * glm::vec4(world_position, 1.f);
  glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;
  glm::vec2 screen = (glm::vec2(ndc) + 1.f) / 2.f;
  ImGuiIO& io = ImGui::GetIO();
  return glm::vec2(screen.x * io.DisplaySize.x, io.DisplaySize.y - screen.y * io.DisplaySize.y);
}

static void manipulate_transform(glm::mat4& transform, const UserCamera& camera)
{
  ImGuizmo::BeginFrame();
  const glm::mat4& projection = camera.projection;
  mat4 cameraView = inverse(camera.transform);
  ImGuiIO& io = ImGui::GetIO();
  ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

  glm::mat4 globNodeTm = transform;

  ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(projection), mCurrentGizmoOperation, mCurrentGizmoMode,
    glm::value_ptr(globNodeTm));

  transform = globNodeTm;
}

static void DrawBlendSpace2DWidget(const char* label, float& paramX, float& paramY)
{
  ImGui::Text("%s", label);
  ImVec2 size(200, 200);
  ImVec2 p0 = ImGui::GetCursorScreenPos();
  ImVec2 p1 = ImVec2(p0.x + size.x, p0.y + size.y);

  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  draw_list->AddRectFilled(p0, p1, IM_COL32(220, 220, 220, 255));
  draw_list->AddRect(p0, p1, IM_COL32(80, 80, 80, 255));

  struct State {
    float startX = 0.0f, startY = 0.0f;
    float targetX = 0.0f, targetY = 0.0f;
    float t = 1.0f; // transition finished
    int clickState = 0;
  };
  static std::map<std::string, State> states;
  State& state = states[label];

  ImGui::InvisibleButton(label, size);
  if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
  {
    ImVec2 mouse = ImGui::GetIO().MousePos;
    float nx = (mouse.x - p0.x) / size.x;
    float ny = (mouse.y - p0.y) / size.y;
    nx = std::clamp(nx, 0.0f, 1.0f);
    ny = std::clamp(ny, 0.0f, 1.0f);
    paramX = nx * 2.0f - 1.0f;
    paramY = (1.0f - ny) * 2.0f - 1.0f;
    state.t = 1.0f;
    state.targetX = paramX;
    state.targetY = paramY;
  }
  if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
  {
    state.startX = paramX;
    state.startY = paramY;
    state.t = 0.0f;
    state.clickState = (state.clickState + 1) % 5;
    switch (state.clickState)
    {
    case 0: state.targetX = 0.0f;   state.targetY = 0.0f;   break;
    case 1: state.targetX = 0.99f;  state.targetY = 0.99f;  break;
    case 2: state.targetX = -0.99f; state.targetY = 0.99f;  break;
    case 3: state.targetX = -0.99f; state.targetY = -0.99f; break;
    case 4: state.targetX = 0.99f;  state.targetY = -0.99f; break;
    }
  }
  if (state.t < 1.0f)
  {
    float speed = 1.0f;
    state.t += ImGui::GetIO().DeltaTime * speed;
    state.t = std::min(state.t, 1.0f);
    float lerpT = state.t;
    paramX = state.startX + (state.targetX - state.startX) * lerpT;
    paramY = state.startY + (state.targetY - state.startY) * lerpT;
  }

  float cx = p0.x + (paramX + 1.0f) * 0.5f * size.x;
  float cy = p0.y + (1.0f - (paramY + 1.0f) * 0.5f) * size.y;

  draw_list->AddCircleFilled(ImVec2(cx, cy), 7.0f, IM_COL32(255, 80, 80, 220));
}

static void visualize_skeleton(const Scene& scene, const Character& character, uint32_t selectedNode)
{
  ImDrawList* drawList = ImGui::GetWindowDrawList();
  constexpr auto boneColor = IM_COL32(255, 215, 0, 175);
  constexpr auto xAxisColor = IM_COL32(255, 0, 0, 200);
  constexpr auto yAxisColor = IM_COL32(0, 255, 0, 200);
  constexpr auto zAxisColor = IM_COL32(0, 120, 255, 200);

  for (size_t j = 0; j < character.skeletonInfo.names.size(); j++)
  {
    const glm::mat4& transform = reinterpret_cast<const glm::mat4&>(character.animationContext.worldTransforms[j]);
    int parent = character.skeletonInfo.parents[j];
    if (parent >= 0)
    {
      const glm::mat4& parentTransform = reinterpret_cast<const glm::mat4&>(character.animationContext.worldTransforms[parent]);
      const glm::vec3 jointPos = glm::vec3(transform[3]);
      const glm::vec3 parentJointPos = glm::vec3(parentTransform[3]);
      const float boneLength = glm::length(jointPos - parentJointPos);

      const glm::vec2 jointScreen = world_to_screen(scene.userCamera, jointPos);
      const glm::vec2 parentScreen = world_to_screen(scene.userCamera, parentJointPos);

      drawList->AddLine(ImVec2(parentScreen.x, parentScreen.y), ImVec2(jointScreen.x, jointScreen.y), boneColor, 2.5f);

      const float axisScale = boneLength * 0.15f;
      glm::vec3 xAxis = glm::normalize(glm::vec3(transform[0])) * axisScale;
      glm::vec3 yAxis = glm::normalize(glm::vec3(transform[1])) * axisScale;
      glm::vec3 zAxis = glm::normalize(glm::vec3(transform[2])) * axisScale;

      glm::vec2 xAxisScreen = world_to_screen(scene.userCamera, jointPos + xAxis);
      glm::vec2 yAxisScreen = world_to_screen(scene.userCamera, jointPos + yAxis);
      glm::vec2 zAxisScreen = world_to_screen(scene.userCamera, jointPos + zAxis);

      drawList->AddCircleFilled(ImVec2(jointScreen.x, jointScreen.y), 4.0f, boneColor);
      drawList->AddLine(ImVec2(jointScreen.x, jointScreen.y), ImVec2(xAxisScreen.x, xAxisScreen.y), xAxisColor, 1.5f);
      drawList->AddLine(ImVec2(jointScreen.x, jointScreen.y), ImVec2(yAxisScreen.x, yAxisScreen.y), yAxisColor, 1.5f);
      drawList->AddLine(ImVec2(jointScreen.x, jointScreen.y), ImVec2(zAxisScreen.x, zAxisScreen.y), zAxisColor, 1.5f);

      const float arrowSize = 4.0f;
      const glm::vec2 boneDir = glm::normalize(jointScreen - parentScreen);
      const glm::vec2 perpDir(-boneDir.y, boneDir.x);
      const glm::vec2 arrowBase = jointScreen - boneDir * arrowSize * 2.0f;
      const glm::vec2 arrowLeft = arrowBase - boneDir * arrowSize + perpDir * arrowSize;
      const glm::vec2 arrowRight = arrowBase - boneDir * arrowSize - perpDir * arrowSize;
      drawList->AddTriangleFilled(ImVec2(jointScreen.x, jointScreen.y), ImVec2(arrowLeft.x, arrowLeft.y), ImVec2(arrowRight.x, arrowRight.y), IM_COL32(80, 200, 120, 230));
    }
    else
    {
      const glm::vec3 rootPos = glm::vec3(transform[3]);
      const glm::vec2 rootScreen = world_to_screen(scene.userCamera, rootPos);
      drawList->AddCircleFilled(ImVec2(rootScreen.x, rootScreen.y), 6.0f, IM_COL32(255, 255, 0, 200));
      const float rootAxisScale = 0.1f;
      glm::vec3 xAxis = glm::normalize(glm::vec3(transform[0])) * rootAxisScale;
      glm::vec3 yAxis = glm::normalize(glm::vec3(transform[1])) * rootAxisScale;
      glm::vec3 zAxis = glm::normalize(glm::vec3(transform[2])) * rootAxisScale;
      glm::vec2 xAxisScreen = world_to_screen(scene.userCamera, rootPos + xAxis);
      glm::vec2 yAxisScreen = world_to_screen(scene.userCamera, rootPos + yAxis);
      glm::vec2 zAxisScreen = world_to_screen(scene.userCamera, rootPos + zAxis);
      drawList->AddLine(ImVec2(rootScreen.x, rootScreen.y), ImVec2(xAxisScreen.x, xAxisScreen.y), xAxisColor, 2.0f);
      drawList->AddLine(ImVec2(rootScreen.x, rootScreen.y), ImVec2(yAxisScreen.x, yAxisScreen.y), yAxisColor, 2.0f);
      drawList->AddLine(ImVec2(rootScreen.x, rootScreen.y), ImVec2(zAxisScreen.x, zAxisScreen.y), zAxisColor, 2.0f);
    }

    if (j == selectedNode)
    {
      const glm::vec3 selectedPos = glm::vec3(transform[3]);
      const glm::vec2 selectedScreen = world_to_screen(scene.userCamera, selectedPos);
      drawList->AddCircle(ImVec2(selectedScreen.x, selectedScreen.y), 8.0f, IM_COL32(255, 255, 255, 255), 12, 2.0f);
    }
  }
}

static void DrawAnimGraphArrow(const ImVec2& from, const ImVec2& to, bool active = false, const char* label = nullptr)
{
  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  ImVec2 start = ImVec2(from.x + 100, from.y + 20);
  ImVec2 end = ImVec2(to.x, to.y + 20);

  ImU32 color = active ? IM_COL32(255, 128, 0, 255) : IM_COL32(80, 80, 80, 180);
  float thickness = active ? 3.5f : 2.0f;

  ImVec2 cp1 = ImVec2(start.x + 40, start.y);
  ImVec2 cp2 = ImVec2(end.x - 40, end.y);
  draw_list->AddBezierCurve(start, cp1, cp2, end, color, thickness);

  ImVec2 dir = ImVec2(end.x - cp2.x, end.y - cp2.y);
  float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
  if (len > 0.0f) {
    dir.x /= len; dir.y /= len;
    ImVec2 ortho(-dir.y, dir.x);
    float arrowSize = 10.0f;
    ImVec2 p1 = ImVec2(end.x - dir.x * arrowSize + ortho.x * arrowSize * 0.5f, end.y - dir.y * arrowSize + ortho.y * arrowSize * 0.5f);
    ImVec2 p2 = ImVec2(end.x - dir.x * arrowSize - ortho.x * arrowSize * 0.5f, end.y - dir.y * arrowSize - ortho.y * arrowSize * 0.5f);
    draw_list->AddTriangleFilled(end, p1, p2, color);
  }

}

static void DrawAnimGraphNode(const ImVec2& pos, const char* label, ImU32 color, bool selected = false)
{
  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  ImVec2 size(100, 40);
  ImVec2 p0 = pos;
  ImVec2 p1 = ImVec2(pos.x + size.x, pos.y + size.y);

  // Shadow
  if (selected)
    draw_list->AddRectFilled(ImVec2(p0.x + 3, p0.y + 3), ImVec2(p1.x + 3, p1.y + 3), IM_COL32(0, 0, 0, 80), 8.0f);

  draw_list->AddRectFilled(p0, p1, color, 8.0f);
  draw_list->AddRect(p0, p1, selected ? IM_COL32(255, 128, 0, 255) : IM_COL32(60, 60, 60, 255), 8.0f, 0, selected ? 4.0f : 2.0f);

  ImVec2 text_size = ImGui::CalcTextSize(label);
  ImVec2 text_pos = ImVec2(p0.x + (size.x - text_size.x) * 0.5f, p0.y + (size.y - text_size.y) * 0.5f);
  draw_list->AddText(text_pos, IM_COL32(0, 0, 0, 255), label);
}

static void DrawJumpAnimGraph(AnimationState currentState)
{
    ImGui::Text("Animation Graph:");
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImVec2 idlePos = ImVec2(origin.x + 120, origin.y + 20);
    ImVec2 walkPos = ImVec2(origin.x + 120, origin.y + 120);
    ImVec2 runPos  = ImVec2(origin.x + 120, origin.y + 220);
    ImVec2 jumpLPos = ImVec2(origin.x + 320, origin.y + 20);
    ImVec2 jumpRPos = ImVec2(origin.x + 320, origin.y + 120);

    DrawAnimGraphArrow(idlePos, walkPos, false);
    DrawAnimGraphArrow(walkPos, idlePos, false);
    DrawAnimGraphArrow(walkPos, runPos, false);
    DrawAnimGraphArrow(runPos, walkPos, false);
    DrawAnimGraphArrow(walkPos, jumpLPos, false);
    DrawAnimGraphArrow(walkPos, jumpRPos, false);
    DrawAnimGraphArrow(runPos, jumpLPos, false);
    DrawAnimGraphArrow(runPos, jumpRPos, false);
    DrawAnimGraphArrow(idlePos, jumpLPos, false);
    DrawAnimGraphArrow(jumpLPos, walkPos, false);
    DrawAnimGraphArrow(jumpRPos, walkPos, false);
    DrawAnimGraphArrow(jumpLPos, runPos, false);
    DrawAnimGraphArrow(jumpRPos, runPos, false);

    DrawAnimGraphNode(idlePos, "Idle", IM_COL32(255, 200, 80, 255), currentState == AnimationState::Idle);
    DrawAnimGraphNode(walkPos, "Walk", IM_COL32(180, 180, 180, 255), currentState == AnimationState::Movement);
    DrawAnimGraphNode(runPos,  "Run",  IM_COL32(255, 120, 40, 255), currentState == AnimationState::Run);
    DrawAnimGraphNode(jumpLPos, "JumpLeft", IM_COL32(180, 180, 255, 255), currentState == AnimationState::JumpLeft);
    DrawAnimGraphNode(jumpRPos, "JumpRight", IM_COL32(180, 255, 180, 255), currentState == AnimationState::JumpRight);

    ImGui::Dummy(ImVec2(450, 300)); // Reserve space
}

static void show_characters(Scene& scene)
{
  static uint32_t selectedCharacter = -1u;
  static uint32_t selectedNode = -1u;
  if (ImGui::Begin("Scene"))
  {
    ImGui::Checkbox("Show Skeleton", &showSkeleton);

    for (size_t i = 0; i < scene.characters.size(); i++)
    {
      Character& character = scene.characters[i];
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
        for (auto& controller : character.controllers)
        {
          if (character.name == "MotusMan_v55")
          {
            ImGui::SliderFloat("linearVelocity", &character.linearVelocity, 0.f, 3.f);
          }
          else if (character.name == "MotusMan_BlendSpace2D_Task_3.1_Idle_Run")
          {
            DrawBlendSpace2DWidget("BlendSpace2D", character.linearVelocityX, character.linearVelocityY);
            ImGui::Text("X: %.2f  Y: %.2f", character.linearVelocityX, character.linearVelocityY);
          }
          else if (character.name == "MotusMan_BlendSpace2D_Task_3.1_Running")
          {
            DrawBlendSpace2DWidget("BlendSpace2D", character.linearVelocityX, character.linearVelocityY);
            ImGui::Text("X: %.2f  Y: %.2f", character.linearVelocityX, character.linearVelocityY);
          }
          else if (character.name == "MotusMan_GraphController_Task_3.2")
          {
            ImGui::RadioButton("Left Foot Forward", &character.walkFoot, 0); ImGui::SameLine();
            ImGui::RadioButton("Right Foot Forward", &character.walkFoot, 1);

            ImGui::SliderFloat("Speed", &character.linearVelocity, 0.0f, 3.0f);
            ImGui::Checkbox("Is Jumping", &character.jumping);

            if (character.jumping)
            {
              if (character.walkFoot == 0)
                character.state = AnimationState::JumpLeft;
              else
                character.state = AnimationState::JumpRight;
            }
            else if (character.linearVelocity > 1.5f)
            {
              character.state = AnimationState::Run;
            }
            else if (character.linearVelocity > 0.1f)
            {
              character.state = AnimationState::Movement;
            }
            else
            {
              character.state = AnimationState::Idle;
            }

            if (character.linearVelocity > 1.5f) {
              ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Run");
            }
            else if (character.linearVelocity > 0.1f) {
              ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1), "Walk");
            }
            else {
              ImGui::Text("Idle");
            }

            for (auto& controller : character.controllers) {
              if (auto* graph = dynamic_cast<AnimationGraph*>(controller.get())) {
                graph->set_speed(character.linearVelocity);
                graph->set_parameter("speed", character.linearVelocity);
                graph->set_is_jumping(character.jumping);
                graph->set_parameter("walkFoot", static_cast<float>(character.walkFoot));
              }
            }

            DrawJumpAnimGraph(character.state);
          }
        }

        const char* animationState[] = {
          "Idle",
          "Walk"
        };
        int currentState = static_cast<int>(character.state);

        if (ImGui::ListBox("AnimationState", &currentState, animationState, IM_ARRAYSIZE(animationState)))
        {
          character.state = static_cast<AnimationState>(currentState);
        }
        constexpr float INDENT = 15.0f;
        ImGui::Indent(INDENT);
        ImGui::Text("Meshes: %zu", character.meshes.size());
        SkeletonInfo& skeletonInfo = character.skeletonInfo;
        ImGui::Text("Skeleton Nodes: %zu", skeletonInfo.names.size());
        for (size_t j = 0; j < skeletonInfo.names.size(); j++)
        {
          const std::string& name = skeletonInfo.names[j];
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
      Character& character = scene.characters[selectedCharacter];
      if (selectedNode < character.skeletonInfo.names.size())
      {
        glm::mat4& worldTransform = reinterpret_cast<glm::mat4&>(character.animationContext.worldTransforms[selectedNode]);
        glm::mat4 transform = character.transform * worldTransform;
        manipulate_transform(transform, scene.userCamera);
        worldTransform = inverse(character.transform) * transform;
      }
      else
      {
        manipulate_transform(character.transform, scene.userCamera);
      }
    }
    ImGui::End();
  }

  if (selectedCharacter < scene.characters.size())
  {
    Character& character = scene.characters[selectedCharacter];
    if (showSkeleton)
    {
      const ImU32 flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;

      ImGuiIO& io = ImGui::GetIO();
      ImGui::SetNextWindowSize(io.DisplaySize);
      ImGui::SetNextWindowPos(ImVec2(0, 0));

      ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);
      ImGui::PushStyleColor(ImGuiCol_Border, 0);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

      ImGui::Begin("gizmo", NULL, flags);

      visualize_skeleton(scene, character, selectedNode);

      ImGui::End();
      ImGui::PopStyleVar();
      ImGui::PopStyleColor(2);
    }
  }
}

void render_imguizmo(ImGuizmo::OPERATION& mCurrentGizmoOperation, ImGuizmo::MODE& mCurrentGizmoMode)
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

static void show_models(Scene& scene)
{
  if (ImGui::Begin("Models"))
  {
    static uint32_t selectedModel = -1u;
    for (size_t i = 0; i < scene.models.size(); i++)
    {
      ImGui::PushID(i);
      const ModelAsset& model = scene.models[i];
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
          const MeshPtr& mesh = model.meshes[j];
          ImGui::Text("%s", mesh->name.c_str());
          ImGui::Text("Bones: %zu", mesh->bonesNames.size());
          if (ImGui::TreeNode("", "Bones: %zu", mesh->bonesNames.size()))
          {
            int boneIndex = 0;
            for (const std::string& boneName : mesh->bonesNames)
            {
              ImGui::Text("%d) %s", boneIndex, boneName.c_str());
              boneIndex++;
            }
            ImGui::TreePop();
          }
          ImGui::PopID();
        }
        // show skeleton
        const SkeletonOffline& skeleton = model.skeleton;
        ImGui::Text("Skeleton Nodes: %zu", skeleton.names.size());
        for (size_t j = 0; j < skeleton.names.size(); j++)
        {
          // show text with tabs for hierarchy
          const std::string& name = skeleton.names[j];
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

void application_imgui_render(Scene& scene)
{
  render_imguizmo(mCurrentGizmoOperation, mCurrentGizmoMode);

  show_info();
  show_characters(scene);
  show_models(scene);
}