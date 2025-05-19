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
	std::vector<std::string> animationNames;
};

ModelAsset load_model(const char* path);
