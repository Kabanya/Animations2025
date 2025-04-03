#pragma once
#include "engine/3dmath.h"
#include "engine/render/material.h"
#include "engine/render/mesh.h"
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Ragdoll/Ragdoll.h>
#include "animation_controller.h"
#include "single_animation.h"
#include "blend_space_1d.h"
#include "animation_graph.h"
struct SkeletonInfo
{
  std::vector<std::string> names;
  std::vector<int> hierarchyDepth; // only for ui
  std::vector<int> parents;
  std::map<std::string, int> nodesMap;

  SkeletonInfo() = default;
  SkeletonInfo(const SkeletonOffline &skeleton) : names(skeleton.names), hierarchyDepth(skeleton.hierarchyDepth), parents(skeleton.parents)
  {
    for (size_t i = 0; i < names.size(); i++)
      nodesMap[names[i]] = i;
  }

};

struct AnimationLayer
{
  std::vector<ozz::math::SoaTransform> localLayerTransforms;
  const ozz::animation::Animation *currentAnimation = nullptr;
  std::unique_ptr<ozz::animation::SamplingJob::Context> samplingCache;
  float currentProgress = 0;
  float weight = 1.f;
};



struct AnimationContext
{
  std::vector<ozz::math::SoaTransform> localTransforms;
  std::vector<ozz::math::Float4x4> worldTransforms;
  const ozz::animation::Skeleton *skeleton = nullptr;
  std::vector<AnimationLayer> layers;

  void setup(const ozz::animation::Skeleton *_skeleton)
  {
    skeleton = _skeleton;
    worldTransforms.resize(skeleton->num_joints());
    localTransforms.resize(skeleton->num_soa_joints());


    ozz::animation::LocalToModelJob localToModelJob;
    localToModelJob.skeleton = _skeleton;
    localToModelJob.input = _skeleton->joint_rest_poses();
    localToModelJob.output = ozz::make_span(worldTransforms);
  }

  void add_animation(const ozz::animation::Animation *animation, float progress, float weight = 1.f )
  {
    AnimationLayer &layer = layers.emplace_back();
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

struct Character
{
  std::string name;
  glm::mat4 transform;
  glm::mat4 ragdollTargetTransform = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 2.f, 0.f));
  std::vector<MeshPtr> meshes;
  MaterialPtr material;
  SkeletonInfo skeletonInfo;
  AnimationContext animationContext;
  JPH::Ref<JPH::RagdollSettings> ragdollSettings;
  JPH::Ref<JPH::Ragdoll> ragdoll;
  float ragdollToAnimationDeltaTime = 1.f / 60.f;

  std::vector<std::shared_ptr<IAnimationController>> controllers;
  float linearVelocity = 0.f;
  AnimationState state = AnimationState::Idle;

  Character() = default;
  Character(Character &&) = default;
  Character &operator=(Character &&) = default;
  Character(const Character &) = delete;
  Character &operator=(const Character &) = delete;
  ~Character()
  {
    if (ragdoll)
    {
      ragdoll->RemoveFromPhysicsSystem();
      ragdoll = nullptr;
    }
  }
};
