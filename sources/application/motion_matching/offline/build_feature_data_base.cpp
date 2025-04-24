#include "application/motion_matching/feature_data_base.h"
#include "application/character.h"
#include "engine/import/model.h"

#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton_utils.h>

const float FPS = 30.f;


struct PoseData
{
  ozz::math::SimdFloat4 leftFoot, rightFoot, Hips;
};

static std::vector<PoseData> sample_animation(AnimationContext &animationContext, const ozz::animation::Animation *animation)
{
  animationContext.add_animation(animation, 0.f, 1.f);
  ozz::animation::SamplingJob samplingJob;
  samplingJob.ratio = 0.f; // sample at the beginning of the animation
  samplingJob.animation = animation;
  samplingJob.output = ozz::make_span(animationContext.localTransforms);
  samplingJob.context = animationContext.layers[0].samplingCache.get();

  ozz::animation::LocalToModelJob localToModelJob;
  localToModelJob.skeleton = animationContext.skeleton;
  localToModelJob.input = ozz::make_span(animationContext.localTransforms);
  localToModelJob.output = ozz::make_span(animationContext.worldTransforms);

  const float duration = animation->duration();
  const int numFrames = static_cast<int>(duration * FPS);
  const float deltaRatio = 1.f / numFrames;

  std::vector<PoseData> poses(numFrames);

  const int leftFootIdx = ozz::animation::FindJoint(*animationContext.skeleton, "LeftFoot");
  const int rightFootIdx = ozz::animation::FindJoint(*animationContext.skeleton, "RightFoot");
  const int hipsIdx = ozz::animation::FindJoint(*animationContext.skeleton, "Hips");
  assert(leftFootIdx != -1 && rightFootIdx != -1 && hipsIdx != -1);

  for (int frameIdx = 0; frameIdx < numFrames; ++frameIdx)
  {
    samplingJob.ratio = frameIdx * deltaRatio;
    {
      assert(0.f <= samplingJob.ratio && samplingJob.ratio <= 1.f);
      assert(animation->num_tracks() == animationContext.skeleton->num_joints());
      assert(samplingJob.Validate());
      const bool success = samplingJob.Run();
      assert(success);
    }

    {
      assert(localToModelJob.Validate());
      const bool success = localToModelJob.Run();
      assert(success);
    }
    // Store the sampled transforms in the clip features
    const ozz::math::Float4x4 &leftFootTransform = animationContext.worldTransforms[leftFootIdx];
    const ozz::math::Float4x4 &rightFootTransform = animationContext.worldTransforms[rightFootIdx];
    const ozz::math::Float4x4 &hipsTransform = animationContext.worldTransforms[hipsIdx];

    poses[frameIdx].leftFoot = leftFootTransform.cols[3];
    poses[frameIdx].rightFoot = rightFootTransform.cols[3];
    poses[frameIdx].Hips = hipsTransform.cols[3];
  }

  return poses;
}

FeatureDataBase build_feature_data_base(const AnimationDataBase &animationDataBase)
{
  FeatureDataBase dataBase;
  AnimationContext animationContext;
  animationContext.setup(animationDataBase.skeleton.get());

  for (const auto &animation : animationDataBase.animations)
  {
    if (std::string_view(animation->name()).ends_with("_IPC"))
      continue;
    std::string IPCPair = animation->name();
    IPCPair += "_IPC";
    const ozz::animation::Animation *IPCAnimation = animationDataBase.find_animation(IPCPair);
    assert(IPCAnimation != nullptr);
    AnimationClipFeatures clip;
    clip.name = IPCPair;
    std::vector<PoseData> posesRootMotion = sample_animation(animationContext, animation.get());
    std::vector<PoseData> posesInPlace = sample_animation(animationContext, IPCAnimation);
    assert(posesRootMotion.size() == posesInPlace.size());
    clip.features.resize(posesRootMotion.size() - 1);
    const auto FPS_v = ozz::math::simd_float4::Load1(FPS);
    for (int i = 0; i < clip.features.size(); i++)
    {
      FrameFeature &feature = clip.features[i];
      auto leftFootVelocity = (posesInPlace[i + 1].leftFoot - posesInPlace[i].leftFoot) * FPS_v;
      auto rightFootVelocity = (posesInPlace[i + 1].rightFoot - posesInPlace[i].rightFoot) * FPS_v;
      ozz::math::Store3PtrU(posesInPlace[i].leftFoot, &feature.leftFootPosition.x);
      ozz::math::Store3PtrU(posesInPlace[i].rightFoot, &feature.rightFootPosition.x);
      ozz::math::Store3PtrU(leftFootVelocity, &feature.leftFootVelocity.x);
      ozz::math::Store3PtrU(rightFootVelocity, &feature.rightFootVelocity.x);
    }
    dataBase.clipMap[IPCPair] = dataBase.clips.size();
    dataBase.clips.push_back(clip);
  }
  return dataBase;
}