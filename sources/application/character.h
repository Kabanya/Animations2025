#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "engine/3dmath.h"
#include "engine/render/material.h"
#include "engine/render/mesh.h"
#include <ozz/base/maths/soa_transform.h>
#include "ozz/animation/runtime/sampling_job.h"

#include "ozz/base/maths/simd_math.h"

struct IAnimationController;

enum class AnimationState
{
  Idle,
  Movement,
  JumpLeft,
  JumpRight,
  Run
};

struct SkeletonInfo
{
  std::vector<std::string> names;
  std::vector<int> hierarchyDepth; // only for ui
  std::vector<int> parents;
  std::map<std::string, int> nodesMap;

  SkeletonInfo() = default;
  SkeletonInfo(const SkeletonOffline& skeleton) : names(skeleton.names), hierarchyDepth(skeleton.hierarchyDepth), parents(skeleton.parents)
  {
    for (size_t i = 0; i < names.size(); i++)
      nodesMap[names[i]] = i;
  }
};

struct AnimationLayer
{
  std::vector<ozz::math::SoaTransform> localLayerTransforms;
  const ozz::animation::Animation* currentAnimation = nullptr;
  std::unique_ptr<ozz::animation::SamplingJob::Context> samplingCache;
  float currentProgress = 0;
  float weight = 1.f;
};

struct AnimationContext
{
  std::vector<ozz::math::SoaTransform> localTransforms;
  std::vector<ozz::math::Float4x4> worldTransforms;
  const ozz::animation::Skeleton* skeleton = nullptr;
  std::vector<AnimationLayer> layers;

  void setup(const ozz::animation::Skeleton* _skeleton)
  {
    skeleton = _skeleton;
    worldTransforms.resize(skeleton->num_joints());
    localTransforms.resize(skeleton->num_soa_joints());
  }

  void add_animation(const ozz::animation::Animation* animation, float progress, float weight = 1.f)
  {
    AnimationLayer& layer = layers.emplace_back();
    layer.localLayerTransforms.resize(skeleton->num_soa_joints());
    layer.currentAnimation = animation;
    layer.currentProgress = progress;
    layer.weight = weight;
    if (!layer.samplingCache)
      layer.samplingCache = std::make_unique<ozz::animation::SamplingJob::Context>(skeleton->num_joints());
    else
      layer.samplingCache->Resize(skeleton->num_joints());
  }

  void clear_animation_layers()
  {
    layers.clear();
  }
};

struct AnimatorParameters {
  std::unordered_map<std::string, float> floats;
  std::unordered_map<std::string, int> ints;
  std::unordered_map<std::string, bool> bools;

  void set(const std::string& name, float value) { floats[name] = value; }
  void set(const std::string& name, int value) { ints[name] = value; }
  void set(const std::string& name, bool value) { bools[name] = value; }

  float getFloat(const std::string& name) const { auto it = floats.find(name); return it != floats.end() ? it->second : 0.0f; }
  int getInt(const std::string& name) const { auto it = ints.find(name); return it != ints.end() ? it->second : 0; }
  bool getBool(const std::string& name) const { auto it = bools.find(name); return it != bools.end() ? it->second : false; }
};

struct Character
{
  std::string name;
  glm::mat4 transform = glm::identity<glm::mat4>();
  std::vector<MeshPtr> meshes;
  MaterialPtr material;
  SkeletonInfo skeletonInfo;
  AnimationContext animationContext;
  std::vector<std::shared_ptr<IAnimationController>> controllers;
  int currentController = 0;

  int walkFoot = 0; // 0 = left, 1 = right
  float speed = 0.0f;
  bool jumping = false;

  float linearVelocity = 0.f;
  float linearVelocityX = 0.f;
  float linearVelocityY = 0.f;

  AnimationState state = AnimationState::Idle;

  Character() = default;
  Character(Character&&) = default;
  Character& operator=(Character&&) = default;
	AnimatorParameters animatorParameters;
};

