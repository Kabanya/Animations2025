#pragma once
#include <vector>
#include <string>
#include <map>
#include "engine/import/model.h"

struct Float2
{
  float x = 0, y = 0;
};
struct Float3
{
  float x = 0, y = 0, z = 0;
};

struct FrameFeature
{
  Float3 leftFootPosition;
  Float3 leftFootVelocity;
  Float3 rightFootPosition;
  Float3 rightFootVelocity;
  Float2 hipsVelocity;

  Float2 trajectoryPosition0;
  Float2 trajectoryPosition1;
  Float2 trajectoryPosition2;
  Float2 trajectoryDirection0;
  Float2 trajectoryDirection1;
  Float2 trajectoryDirection2;
};

struct AnimationClipFeatures
{
  std::vector<FrameFeature> features;
  std::string name;
};

struct FeatureDataBase
{
  std::vector<AnimationClipFeatures> clips;
  std::map<std::string, int> clipMap;
};

FeatureDataBase build_feature_data_base(const AnimationDataBase &animationDataBase);