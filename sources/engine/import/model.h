#pragma once
#include "render/mesh.h"
#include <vector>
#include <ozz/base/memory/unique_ptr.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/animation.h>

using SkeletonPtr = ozz::unique_ptr<ozz::animation::Skeleton>;
using AnimationPtr = ozz::unique_ptr<ozz::animation::Animation>;

struct SkeletonOffline
{
  std::vector<std::string> names;
  std::vector<mat4> localTransform;
  std::vector<int> parents;
  std::vector<int> hierarchyDepth; // only for ui
  SkeletonPtr skeleton;
};

struct ModelAsset
{
  std::string path;
  std::vector<MeshPtr> meshes;
  SkeletonOffline skeleton;
  std::vector<AnimationPtr> animations;
};

ModelAsset load_model(const char *path);
void build_animations(const std::vector<std::string> &paths, const std::string &output_path);

struct AnimationDataBase
{
  std::string path;
  SkeletonPtr skeleton;
  std::vector<AnimationPtr> animations;
  std::map<std::string, int> animationMap;

  const ozz::animation::Animation *find_animation(const std::string &name) const
  {
    auto it = animationMap.find(name);
    if (it != animationMap.end())
    {
      return animations[it->second].get();
    }
    return nullptr;
  }
};

AnimationDataBase load_animations(const std::string &path);
