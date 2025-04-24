#pragma once

#include "engine/3dmath.h"
#include "engine/import/model.h"
#include "animation_controller.h"


struct SingleAnimation final : IAnimationController
{
  const ozz::animation::Animation *animation = nullptr;
  float progress = 0; // in [0, 1]
  SingleAnimation(const ozz::animation::Animation *_animation) : animation(_animation) {}

  void set_animation(const ozz::animation::Animation *_animation, float _progress = 0.f)
  {
    animation = _animation;
    progress = _progress;
  }

  float duration() const override
  {
    return animation->duration();
  }
  void update(float dt) override
  {
    float duration = animation->duration();
    progress += dt / duration;
    if (progress > 1.f || progress < 0.f)
      progress -= floorf(progress);
  }

  void collect_animations(std::vector<WeightedAnimation> &out, float weight) override
  {
    out.push_back({animation, 1.f * weight, progress});
  }
};