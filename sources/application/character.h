#pragma once
#include "engine/3dmath.h"
#include "engine/render/material.h"
#include "engine/render/mesh.h"
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/maths/soa_transform.h>

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

struct AnimationContext
{
  std::vector<ozz::math::SoaTransform> localTransforms;
  std::vector<ozz::math::Float4x4> worldTransforms;
  const ozz::animation::Skeleton *skeleton = nullptr;
  const ozz::animation::Animation *currentAnimation = nullptr;
  std::unique_ptr<ozz::animation::SamplingJob::Context> samplingCache;
  float currentProgress = 0;

  void setup(const ozz::animation::Skeleton *_skeleton)
  {
    skeleton = _skeleton;
    localTransforms.resize(skeleton->num_soa_joints());
    worldTransforms.resize(skeleton->num_joints());
    if (!samplingCache)
      samplingCache = std::make_unique<ozz::animation::SamplingJob::Context>(skeleton->num_joints());
    else
      samplingCache->Resize(skeleton->num_joints());
  }

};

struct Character
{
  std::string name;
  glm::mat4 transform;
  std::vector<MeshPtr> meshes;
  MaterialPtr material;
  SkeletonInfo skeletonInfo;
  AnimationContext animationContext;
};
