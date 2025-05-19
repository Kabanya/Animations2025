#include "scene.h"
#include "blend_space_2d.h" 

#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/blending_job.h>

#include "animation_graph.h"
#include "blend_space_1d.h"

void application_update(Scene& scene)
{
  arcball_camera_update(
    scene.userCamera.arcballCamera,
    scene.userCamera.transform,
    engine::get_delta_time());

  for (Character& character : scene.characters)
  {
    AnimationContext& animationContext = character.animationContext;

    std::vector<WeightedAnimation> animations;
    for (auto& controller : character.controllers)
    {
      if (BlendSpace1D* blendSpace = dynamic_cast<BlendSpace1D*>(controller.get()))
      {
        blendSpace->set_parameter(glm::length(character.linearVelocity));
      }
      if (BlendSpace2D* blendSpace2D = dynamic_cast<BlendSpace2D*>(controller.get()))
      {
        blendSpace2D->set_parameter(character.linearVelocityX, character.linearVelocityY);
      }
      if (AnimationGraph* graph = dynamic_cast<AnimationGraph*>(controller.get()))
      {
        graph->set_state(character.state, &character);
        for (auto& node : graph->nodes)
        {
          if (BlendSpace1D* blendSpace = dynamic_cast<BlendSpace1D*>(node.animation.get()))
          {
            blendSpace->set_parameter(glm::length(character.linearVelocity));
          }
          if (BlendSpace2D* blendSpace2D = dynamic_cast<BlendSpace2D*>(node.animation.get()))
          {
            blendSpace2D->set_parameter(character.linearVelocityX, character.linearVelocityY);
          }
        }
      }
    }
    for (auto& controller : character.controllers)
    {
      controller->update(engine::get_delta_time());
      controller->collect_animations(animations, 1.f);
    }

    animationContext.clear_animation_layers();
    for (const WeightedAnimation& wa : animations)
      animationContext.add_animation(wa.animation, wa.progress, wa.weight);

    // Animation sampling
    for (AnimationLayer& layer : animationContext.layers)
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
  }
}
