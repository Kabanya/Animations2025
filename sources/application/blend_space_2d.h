#pragma once

#include "engine/3dmath.h"
#include "engine/import/model.h"
#include "animation_controller.h"
#include <limits>
#include <algorithm>

struct AnimationNode2D
{
  const ozz::animation::Animation* animation = nullptr;
  float x = 0.f;
  float y = 0.f;
};

struct BlendSpace2D final : IAnimationController
{
  std::vector<AnimationNode2D> animations;
  std::vector<float> weights;
  float progress = 0.f; // in [0, 1]

  BlendSpace2D(std::vector<AnimationNode2D>&& _animations)
    : animations(std::move(_animations))
  {
    weights.resize(animations.size());
    set_parameter(0.f, 0.f);
  }

  void set_parameter(float x, float y)
  {
    if (animations.size() < 4)
      return;

    std::vector<float> xs, ys;
    for (const auto& n : animations) {
      xs.push_back(n.x);
      ys.push_back(n.y);
    }
    std::sort(xs.begin(), xs.end());
    std::sort(ys.begin(), ys.end());
    xs.erase(std::ranges::unique(xs).begin(), xs.end());
    ys.erase(std::ranges::unique(ys).begin(), ys.end());


    float x0 = xs.front(), x1 = xs.back();
    for (size_t i = 1; i < xs.size(); ++i)
    {
      if (x < xs[i]) { x0 = xs[i - 1]; x1 = xs[i]; break; }
    }

    if (x <= xs.front()) { x0 = x1 = xs.front(); }
    if (x >= xs.back()) { x0 = x1 = xs.back(); }

    float y0 = ys.front(), y1 = ys.back();
    for (size_t i = 1; i < ys.size(); ++i)
    {
      if (y < ys[i]) { y0 = ys[i - 1]; y1 = ys[i]; break; }
    }

    if (y <= ys.front()) { y0 = y1 = ys.front(); }
    if (y >= ys.back()) { y0 = y1 = ys.back(); }

    const AnimationNode2D* c00 = nullptr;
    const AnimationNode2D* c10 = nullptr;
    const AnimationNode2D* c01 = nullptr;
    const AnimationNode2D* c11 = nullptr;
    for (const auto& n : animations)
    {
      if (n.x == x0 && n.y == y0) c00 = &n;
      if (n.x == x1 && n.y == y0) c11 = &n;
      if (n.x == x0 && n.y == y1) c01 = &n;
      if (n.x == x1 && n.y == y1) c10 = &n;
    }

    weights.assign(animations.size(), 0.f);
    if (!c00 || !c10 || !c01 || !c11 || x0 == x1 || y0 == y1)
    {
      float minDist = std::numeric_limits<float>::max();
      int bestIdx = -1;
      for (size_t i = 0; i < animations.size(); ++i)
      {
        float dx = animations[i].x - x;
        float dy = animations[i].y - y;
        float dist = dx * dx + dy * dy;
        if (dist < minDist) { minDist = dist; bestIdx = (int)i; }
      }
      if (bestIdx >= 0) weights[bestIdx] = 1.0f;
      return;
    }

    // Bilinear weights
    float tx = (x0 == x1) ? 0.0f : (x - x0) / (x1 - x0);
    float ty = (y0 == y1) ? 0.0f : (y - y0) / (y1 - y0);

    auto set_weight = [&](const AnimationNode2D* node, float w)
  	{
      auto it = std::find_if(animations.begin(), animations.end(),
        [&](const AnimationNode2D& n) { return node == &n; });
      if (it != animations.end())
        weights[std::distance(animations.begin(), it)] = w;
    };

    set_weight(c00, (1 - tx) * (1 - ty));
    set_weight(c10, tx * (1 - ty));
    set_weight(c01, (1 - tx) * ty);
    set_weight(c11, tx * ty);
  }

  float duration() const override
  {
    float weightedDuration = 0.f;
    for (size_t i = 0; i < animations.size(); i++)
      if (animations[i].animation)
        weightedDuration += animations[i].animation->duration() * weights[i];
    if (weightedDuration <= 0.f) {
      for (const auto& anim : animations)
        if (anim.animation)
          return anim.animation->duration();
      return 1.0f;
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

  void collect_animations(std::vector<WeightedAnimation>& out, float weight) override
  {
    for (size_t i = 0; i < animations.size(); i++)
    {
      if (weights[i] * weight < 0.001f)
        continue;
      out.push_back({ animations[i].animation, weights[i] * weight, progress });
    }
  }

  void set_parameters(float x, float y) override { set_parameter(x, y); }
};
