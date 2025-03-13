#pragma once


#include "engine/3dmath.h"
#include "engine/import/model.h"

struct WeightedAnimation
{
  const ozz::animation::Animation *animation = nullptr;
  float weight = 1.f;
  float progress = 0.f;
};

struct IAnimationController
{
  virtual ~IAnimationController() = default;
  virtual float duration() const = 0;
  virtual void update(float dt) = 0;
  virtual void collect_animations(std::vector<WeightedAnimation> &out, float weight) = 0;
};