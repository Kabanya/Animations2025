#pragma once

#include <glm/glm.hpp>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystem.h>
#include <Jolt/Physics/Ragdoll/Ragdoll.h>


constexpr JPH::ObjectLayer sObjectLayer = 0;
constexpr JPH::CollisionGroup::GroupID sCollisionGroup = 0;

struct PhysicsWorld
{
	std::unique_ptr<JPH::JobSystem> mJobSystem;           // The job system that runs physics jobs

	JPH::PhysicsSystem mPhysicsSystem;									  // The physics system that simulates the world
	JPH::PhysicsSettings mPhysicsSettings;							  // Main physics simulation settings
	std::unique_ptr<JPH::TempAllocator> mTempAllocator;   // Temporary allocator for physics jobs

	JPH::Ref<JPH::RagdollSettings> mRagdollSettings;
	std::vector<JPH::Ref<JPH::Ragdoll>> mRagdolls; // Ragdoll

	float mAccumuletedDeltaTime = 0.0f; // Accumulated time since last physics update
	PhysicsWorld();
	~PhysicsWorld();

	void update_physics(float dt);

	// world_to_screen == camera.projection * inverse(camera.transform)
	void debug_render(const glm::mat4 &world_to_screen, glm::vec3 camera_position);

	// return weak reference to the ragdoll
	JPH::Ragdoll *create_ragdoll(uint64_t inUserData)
	{
		JPH::Ragdoll *ragdoll = mRagdollSettings->CreateRagdoll(sCollisionGroup, inUserData, &mPhysicsSystem);
		mRagdolls.push_back(JPH::Ref<JPH::Ragdoll>(ragdoll));
		return ragdoll;
	}

};

