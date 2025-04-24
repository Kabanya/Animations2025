#include "scene.h"

#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/blending_job.h>

void application_update(Scene &scene)
{
  if (scene.physicsWorld)
    scene.physicsWorld->update_physics(engine::get_delta_time());

  arcball_camera_update(
    scene.userCamera.arcballCamera,
    scene.userCamera.transform,
    engine::get_delta_time());

  for (Character &character : scene.characters)
  {
    AnimationContext &animationContext = character.animationContext;

    std::vector<WeightedAnimation> animations;
    for (auto &controller : character.controllers)
    {
      // check rtti information that the controller is a BlendSpace1D
      // if it is, then cast it to BlendSpace1D and set the parameter
      // to the linear velocity
      if (SingleAnimation *singleAnimation = dynamic_cast<SingleAnimation *>(controller.get()))
      {
        if (character.selectedAnimation == -1u)
          continue;
        const auto *newAnimation = scene.animationDataBase.animations[character.selectedAnimation].get();
        if (singleAnimation->animation == newAnimation)
          continue;

        singleAnimation->set_animation(newAnimation, 0.f);
      }
      if (BlendSpace1D *blendSpace = dynamic_cast<BlendSpace1D *>(controller.get()))
      {
        blendSpace->set_parameter(glm::length(character.linearVelocity));
      }
      if (AnimationGraph *graph = dynamic_cast<AnimationGraph *>(controller.get()))
      {
        graph->set_state(character.state);
        for (auto &node : graph->nodes)
        {
          if (BlendSpace1D *blendSpace = dynamic_cast<BlendSpace1D *>(node.animation.get()))
          {
            blendSpace->set_parameter(glm::length(character.linearVelocity));
          }
        }
      }
    }
    for (auto &controller : character.controllers)
    {
      controller->update(engine::get_delta_time());
      controller->collect_animations(animations, 1.f);
    }

    animationContext.clear_animation_layers();
    for (const WeightedAnimation &wa : animations)
      animationContext.add_animation(wa.animation, wa.progress, wa.weight);

    // Animation sampling
    for (AnimationLayer &layer : animationContext.layers)
    {
      ozz::animation::SamplingJob samplingJob;
      samplingJob.ratio = layer.currentProgress;
      assert(0.f <= samplingJob.ratio && samplingJob.ratio <= 1.f);
      assert(layer.currentAnimation->num_tracks() == animationContext.skeleton->num_joints());
      samplingJob.animation = layer.currentAnimation;
      samplingJob.context = layer.samplingCache.get();
      samplingJob.output = ozz::make_span(layer.localLayerTransforms);

      assert(samplingJob.Validate());
      const bool success = samplingJob.Run();
      assert(success);
    }

    // Animation blending
    if (!animationContext.layers.empty())
    {
      ozz::animation::BlendingJob blendingJob;
      blendingJob.output = ozz::make_span(animationContext.localTransforms);
      blendingJob.threshold = 0.01f;
      std::vector<ozz::animation::BlendingJob::Layer> layers(animationContext.layers.size());

      for (size_t i = 0; i < animationContext.layers.size(); i++)
      {
        layers[i].weight = animationContext.layers[i].weight;
        layers[i].transform = ozz::make_span(animationContext.layers[i].localLayerTransforms);
        // layers[i].joint_weights
      }
      blendingJob.layers = ozz::make_span(layers);
      blendingJob.rest_pose = animationContext.skeleton->joint_rest_poses();
      assert(blendingJob.Validate());
      const bool success = blendingJob.Run();
      assert(success);
    }
    else
    {
      auto tPose = animationContext.skeleton->joint_rest_poses();
      animationContext.localTransforms.assign(tPose.begin(), tPose.end());
    }

    ozz::animation::LocalToModelJob localToModelJob;
    localToModelJob.skeleton = animationContext.skeleton;
    localToModelJob.input = ozz::make_span(animationContext.localTransforms);
    localToModelJob.output = ozz::make_span(animationContext.worldTransforms);

    ozz::math::Float4x4 root;
    memcpy(&root, &character.transform, sizeof(root));
    localToModelJob.root = &root;

    assert(localToModelJob.Validate());
    const bool success = localToModelJob.Run();
    assert(success);

    if (character.ragdoll)
    {
      const auto &joints = character.ragdoll->GetRagdollSettings()->GetSkeleton()->GetJoints();
      assert(character.ragdoll->GetBodyCount() == joints.size());

      JPH::Array<JPH::Mat44> animtedPose(joints.size());
      for (size_t jointIdx = 0; jointIdx < joints.size(); jointIdx++)
      {
        const auto &joint = joints[jointIdx];
        auto it = character.skeletonInfo.nodesMap.find(joint.mName.c_str());
        if (it == character.skeletonInfo.nodesMap.end())
          continue;
        int nodeIdx = it->second;
        ozz::math::Float4x4 &worldTransform = animationContext.worldTransforms[nodeIdx];
        memcpy(&animtedPose[jointIdx], &worldTransform, sizeof(JPH::Mat44));
      }

      character.ragdoll->DriveToPoseUsingKinematics(JPH::Vec3::sZero(), animtedPose.data(), character.ragdollToAnimationDeltaTime);

      // JPH::SkeletonPose pose;
      // pose.SetSkeleton(character.ragdoll->GetRagdollSettings()->GetSkeleton());
      // pose.GetJointMatrices() = animtedPose;
      // pose.CalculateJointStates();
      // character.ragdoll->DriveToPoseUsingMotors(pose);

      std::vector<JPH::Mat44> outPose(joints.size());
      JPH::RVec3 rootOffset;
      character.ragdoll->GetPose(rootOffset, outPose.data(), true);

      if (true)
      {
        const JPH::BodyID headId = character.ragdoll->GetBodyID(4); // head
        JPH::BodyInterface &bodyInterface = scene.physicsWorld->mPhysicsSystem.GetBodyInterface();
        const auto currentRotation = bodyInterface.GetRotation(headId);

        glm::vec3 targetPosition = character.ragdollTargetTransform[3];
        const float fixedDeltaTime = 1.f / 60.f;
        bodyInterface.MoveKinematic(headId, JPH::Vec3(targetPosition.x, targetPosition.y, targetPosition.z), currentRotation, fixedDeltaTime);
      }
      for (size_t jointIdx = 0; jointIdx < joints.size(); jointIdx++)
      {
        const auto &joint = joints[jointIdx];
        auto it = character.skeletonInfo.nodesMap.find(joint.mName.c_str());
        if (it == character.skeletonInfo.nodesMap.end())
          continue;
        int nodeIdx = it->second;

        ozz::math::Float4x4 ragdollTransformDeltaOzz;
        memcpy(&ragdollTransformDeltaOzz, &outPose[jointIdx], sizeof(JPH::Mat44));
        ozz::math::Float4x4 &worldTransform = animationContext.worldTransforms[nodeIdx];
        worldTransform = ragdollTransformDeltaOzz;
        worldTransform.cols[3] = worldTransform.cols[3] + ozz::math::simd_float4::LoadPtrU(rootOffset.mF32);

        localToModelJob.from = nodeIdx;
        localToModelJob.from_excluded = true;
        assert(localToModelJob.Validate());
        const bool success = localToModelJob.Run();
        assert(success);
      }

    }
  }
}