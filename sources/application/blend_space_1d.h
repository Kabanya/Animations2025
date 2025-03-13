#pragma once

#include "engine/3dmath.h"
#include "engine/import/model.h"
#include "animation_controller.h"


struct AnimationNode1D
{
  const ozz::animation::Animation *animation = nullptr;
  float parameter = 0;
};

struct BlendSpace1D final : IAnimationController
{
  std::vector<AnimationNode1D> animations;
  std::vector<float> weights;
  float progress = 0; // in [0, 1]
  BlendSpace1D(std::vector<AnimationNode1D> &&_animations) : animations(std::move(_animations)) {
    weights.resize(animations.size());
    set_parameter(0.f);
  }

  void set_parameter(float parameter)
  {
    if (animations.empty())
      return;
    weights.assign(animations.size(), 0.f);

    if (parameter < animations[0].parameter)
    {
      weights[0] = 1.f;
      return;
    }
    for (size_t i = 0; i < animations.size() - 1; i++)
    {
      const AnimationNode1D &curNode = animations[i];
      const AnimationNode1D &nextNode = animations[i + 1];
      if (curNode.parameter <= parameter && parameter <= nextNode.parameter)
      {
        float t = (parameter - curNode.parameter) / (nextNode.parameter - curNode.parameter);
        weights[i] = 1.f - t;
        weights[i + 1] = t;
        break;
      }
    }
    if (parameter > animations.back().parameter)
      weights.back() = 1.f;
  }

  float duration() const override
  {
    float weightedDuration = 0.f;
    for (int i = 0; i < animations.size(); i++)
    {
      weightedDuration += animations[i].animation->duration() * weights[i];
    }
    return weightedDuration;
  }
  void update(float dt) override
  {
    float weightedDuration = duration();
    assert(weightedDuration > 0);
    progress += dt / weightedDuration;
    if (progress > 1.f || progress < 0.f)
      progress -= floorf(progress);
  }

  void collect_animations(std::vector<WeightedAnimation> &out, float weight) override
  {
    for (size_t i = 0; i < animations.size(); i++)
    {
      if (weights[i] * weight < 0.001f)
        continue;
      out.push_back({animations[i].animation, weights[i] * weight, progress});
    }
  }
};