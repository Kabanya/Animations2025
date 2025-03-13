#pragma once

#include "engine/3dmath.h"
#include "engine/import/model.h"
#include "animation_controller.h"

enum class AnimationState
{
  Idle,
  Movement
};

struct AnimationGraphNode;

struct AnimationGraphEdge
{
  AnimationGraphNode *from = nullptr;
  AnimationGraphNode *to = nullptr;
  std::shared_ptr<IAnimationController> animation;
  float transitionDuration = 0;
  float transitionProgress = 0;

};

struct AnimationGraphNode
{
  std::shared_ptr<IAnimationController> animation;
  AnimationState state = AnimationState::Idle;
  std::vector<AnimationGraphEdge> edges;

};

struct AnimationGraph final : IAnimationController
{
  std::vector<AnimationGraphNode> nodes;
  AnimationGraph(std::vector<AnimationGraphNode> &&_nodes, AnimationState _initial_state) : nodes(std::move(_nodes)), goalState(_initial_state)
  {
    for (AnimationGraphNode &node : nodes)
    {
      if (node.state == goalState)
      {
        currentNode = &node;
        break;
      }
    }
  }
  AnimationGraphNode *currentNode = nullptr;
  AnimationGraphEdge *currentEdge = nullptr;

  AnimationState goalState = AnimationState::Idle;

  void set_state(AnimationState state)
  {
    if (goalState == state)
      return;
    goalState = state;
    if (currentNode)
    {
      for (AnimationGraphEdge &edge : currentNode->edges)
      {
        if (edge.to->state == state)
        {
          currentEdge = &edge;
          currentEdge->transitionProgress = 0;
          break;
        }
      }
    }
  }

  float duration() const override
  {
    // if (currentEdge)
    //   return currentEdge->animation->duration() + currentEdge->transitionDuration;
    // if (currentNode)
    //   return currentNode->animation->duration();
    return -1;
  }

  void update(float dt) override
  {
    if (currentEdge)
    {
      assert(currentNode == currentEdge->from);
      currentEdge->from->animation->update(dt);
      currentEdge->to->animation->update(dt);
      currentEdge->animation->update(dt);
      currentEdge->transitionProgress += dt / currentEdge->transitionDuration;
      if (currentEdge->transitionProgress >= 1)
      {
        currentNode = currentEdge->to;
        currentEdge = nullptr;
      }
    }
    else if (currentNode)
    {
      currentNode->animation->update(dt);
    }
  }

  void collect_animations(std::vector<WeightedAnimation> &out, float) override
  {
    if (currentEdge)
    {
      const float p = currentEdge->transitionProgress;
      // transitionProgress == 0| from->1, edge->0, to->0
      // transitionProgress == 0.5| from->0.0, edge->1.0, to->0
      // transitionProgress == 1| from->0, edge->0, to->1
      float weights[3] = {0, 0, 0};
      if (p < 0.5)
      {
        weights[0] = 1 - p * 2;
        weights[1] = p * 2;
      }
      else
      {
        weights[1] = 1 - (p - 0.5) * 2;
        weights[2] = (p - 0.5) * 2;
      }
      currentEdge->from->animation->collect_animations(out, weights[0]);
      currentEdge->animation->collect_animations(out, weights[1]);
      currentEdge->to->animation->collect_animations(out, weights[2]);
    }
    else if (currentNode)
    {
      currentNode->animation->collect_animations(out, 1.f);
    }
  }
};
