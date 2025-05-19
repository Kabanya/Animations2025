#pragma once

#include "engine/3dmath.h"
#include "engine/import/model.h"
#include "animation_controller.h"
#include "character.h"
#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

struct AnimationGraphNode;

struct AnimationGraphEdge {
  AnimationGraphNode* from;
  AnimationGraphNode* to;
  std::shared_ptr<IAnimationController> animation;
  float transitionDuration = 0.3f;
  float transitionProgress = 0.0f;
  float transitionStartTime = 0.0f;
  std::function<float(float)> easing = nullptr;
  std::function<bool(const std::unordered_map<std::string, float>&)> condition;
};

struct AnimationGraphNode {
  std::shared_ptr<IAnimationController> animation;
  AnimationState state = AnimationState::Idle;
  std::vector<AnimationGraphEdge> edges;

  AnimationGraphNode() = default;
};

struct AnimationGraph final : IAnimationController
{
  std::vector<AnimationGraphNode> nodes;
  std::unordered_map<std::string, float> parameters;
  AnimationGraphNode* currentNode = nullptr;
  AnimationGraphEdge* currentEdge = nullptr;
  AnimationState goalState = AnimationState::Idle;
  float currentNodeProgress = 0.0f; // [0..1] прогресс текущей анимации

  AnimationGraph(std::vector<AnimationGraphNode>&& _nodes, AnimationState _initial_state)
    : nodes(std::move(_nodes)), goalState(_initial_state)
  {
    for (AnimationGraphNode& node : nodes)
    {
      if (node.state == goalState)
      {
        currentNode = &node;
        break;
      }
    }
  }

  void set_state(AnimationState state, const Character* character = nullptr)
  {
    if (goalState == state)
      return;
    goalState = state;
    if (currentNode)
    {
      for (AnimationGraphEdge& edge : currentNode->edges) {
        if (edge.to->state == state) {
          if (!edge.condition || edge.condition(parameters)) {
            currentEdge = &edge;
            currentEdge->transitionProgress = 0.0f;
            break;
          }
        }
      }
    }
  }

  float duration() const override
  {
    return -1;
  }

  void update(float dt) override
  {
    if (currentNode && currentNode->animation) {
      float dur = currentNode->animation->duration();
      if (dur > 0.0f) {
        std::vector<WeightedAnimation> tmp;
        currentNode->animation->collect_animations(tmp, 1.0f);
        if (!tmp.empty())
          currentNodeProgress = tmp[0].progress;
        else
          currentNodeProgress = 0.0f;
      }
      else {
        currentNodeProgress = 0.0f;
      }
    }

    if (!currentEdge && currentNode) {
      for (AnimationGraphEdge& edge : currentNode->edges) {
        if (!edge.condition || edge.condition(parameters)) {

          if (currentNodeProgress >= edge.transitionStartTime) {
            currentEdge = &edge;
            currentEdge->transitionProgress = 0.0f;
            break;
          }
        }
      }
    }

    if (currentEdge)
    {
      assert(currentNode == currentEdge->from);
      if (currentEdge->from->animation)
        currentEdge->from->animation->update(dt);
      if (currentEdge->to->animation)
        currentEdge->to->animation->update(dt);
      if (currentEdge->animation)
        currentEdge->animation->update(dt);

      float t = currentEdge->transitionProgress;
      float duration = currentEdge->transitionDuration > 0.0f ? currentEdge->transitionDuration : 1.0f;
      t += dt / duration;
      currentEdge->transitionProgress = std::min(t, 1.0f);

      if (currentEdge->transitionProgress >= 1.0f - std::numeric_limits<float>::epsilon())
      {
        if (currentEdge->from && currentEdge->from->animation)
          currentEdge->from->animation->reset();
        if (currentEdge->animation)
          currentEdge->animation->reset();
        if (currentEdge->to && currentEdge->to->animation)
          currentEdge->to->animation->reset();

        currentNode = currentEdge->to;
        currentEdge = nullptr;
        currentNodeProgress = 0.0f;

        if (currentNode && currentNode->animation)
          currentNode->animation->reset();

        if (currentNode && currentNode->state != goalState) {
          set_state(goalState);
        }
      }
    }
    else if (currentNode)
    {
      if (currentNode->animation)
        currentNode->animation->update(dt);
    }
  }

  void collect_animations(std::vector<WeightedAnimation>& out, float) override
  {
    if (currentEdge)
    {
      float p = currentEdge->transitionProgress;
      if (currentEdge->easing)
        p = std::clamp(currentEdge->easing(p), 0.0f, 1.0f);

      float w_from = 1.0f - p;
      float w_to = p;

      if (currentEdge->from->animation)
        currentEdge->from->animation->collect_animations(out, w_from);
      if (currentEdge->to->animation)
        currentEdge->to->animation->collect_animations(out, w_to);
    }
    else if (currentNode)
    {
      if (currentNode->animation)
        currentNode->animation->collect_animations(out, 1.f);
    }
  }

  void set_parameter(const std::string& name, float value)
	{
    parameters[name] = value;
  }
  float get_parameter(const std::string& name) const
	{
    auto it = parameters.find(name);
    return it != parameters.end() ? it->second : 0.0f;
  }

  void set_speed(float value) { set_parameter("speed", value); }
  void set_is_jumping(bool value) { set_parameter("isJumping", value ? 1.0f : 0.0f); }
  float get_speed() const { return get_parameter("speed"); }
  bool get_is_jumping() const { return get_parameter("isJumping") != 0.0f;}
};
