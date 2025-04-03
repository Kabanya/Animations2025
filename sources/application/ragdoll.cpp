#include "physics_world.h"
#include <Jolt/Physics/Ragdoll/Ragdoll.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/SliderConstraint.h>
#include <Jolt/Physics/Constraints/ConeConstraint.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include "import/model.h"
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton_utils.h>
#include <ozz/base/maths/simd_math.h>

static JPH::Ref<JPH::Shape> create_capsule_shape(float half_height, float radius, JPH::Vec3Arg offset, JPH::Vec3Arg euler)
{
	JPH::CapsuleShape *capsule = new JPH::CapsuleShape(half_height, radius);
	capsule->SetDensity(1.0f); // Density of human body is arond of 1000 kg/m^3
	JPH::Quat euler_quat = JPH::Quat::sEulerAngles(euler);
	JPH::Vec3 rotatedOffset = euler_quat * offset;
	JPH::Ref<JPH::Shape> shape = capsule;
	return new JPH::RotatedTranslatedShape(rotatedOffset, euler_quat, shape.GetPtr());
}

static JPH::Ref<JPH::Shape> create_capsule_limb(float half_height, float radius, JPH::Vec3Arg euler)
{
	return create_capsule_shape(half_height, radius, JPH::Vec3(0, half_height + radius, 0), euler);
}

static JPH::Ref<JPH::Shape> create_capsule_body(float half_height, float radius, JPH::Vec3Arg euler)
{
	return create_capsule_shape(half_height, radius, JPH::Vec3(0, 0, 0), euler);
}

JPH::Ref<JPH::RagdollSettings> create_ragdoll_settings(const SkeletonPtr &skeleton_src)
{
  std::vector<ozz::math::Float4x4> transforms;
	{
		transforms.resize(skeleton_src->num_joints());
		ozz::animation::LocalToModelJob localToModelJob;
		localToModelJob.skeleton = skeleton_src.get();
		localToModelJob.input = skeleton_src->joint_rest_poses();
		localToModelJob.output = ozz::make_span(transforms);
		assert(localToModelJob.Validate());
		const bool success = localToModelJob.Run();
		assert(success);
	}
	const int JOINT_COUNT = 13;
	const char *jointNames[JOINT_COUNT] = {
		"Hips",
		"Spine",
		"Spine1",
		"Spine2",
		"Neck",
		"LeftArm",
		"RightArm",
		"LeftForeArm",
		"RightForeArm",
		"LeftUpLeg",
		"RightUpLeg",
		"LeftLeg",
		"RightLeg"
	};
	int jointParents[JOINT_COUNT] = {
		-1, // Hips
		0, // Spine -> Hips
		1, // Spine1 -> Spine
		2, // Spine2 -> Spine1
		3, // Head -> Spine2
		3, // LeftArm -> Spine2
		3, // RightArm -> Spine2
		5, // LeftForeArm -> LeftArm
		6, // RightForeArm -> RightArm
		0, // LeftUpLeg -> Hips
		0, // RightUpLeg -> Hips
		9, // LeftLeg -> LeftUpLeg
		10, // RightLeg -> RightUpLeg
	};
	int jointIndices[JOINT_COUNT];
	for (int i = 0; i < JOINT_COUNT; i++)
	{
		jointIndices[i] = ozz::animation::FindJoint(*skeleton_src, jointNames[i]);
		assert(jointIndices[i] != -1);
	}
	// Create skeleton
	JPH::Ref<JPH::Skeleton> skeleton = new JPH::Skeleton;
	for (int i = 0; i < JOINT_COUNT; i++)
	{
		skeleton->AddJoint(jointNames[i], jointParents[i]);
	}

	ozz::math::SimdFloat4 nodeTranslations[JOINT_COUNT];
	ozz::math::SimdFloat4 nodeRotations[JOINT_COUNT];
	for (int i = 0; i < JOINT_COUNT; i++)
	{
		ozz::math::SimdFloat4 unusedScale;
		const bool decomposed = ToAffine(transforms[jointIndices[i]], nodeTranslations + i, nodeRotations + i, &unusedScale);
		assert(decomposed);
	}

	// Create shapes for limbs
	const float HALF_PI = JPH::JPH_PI * 0.5f;
	JPH::Ref<JPH::Shape> shapes[JOINT_COUNT] = {
		create_capsule_body(0.10f, 0.07f, JPH::Vec3(HALF_PI, 0.f, 0.f)),		// Hips
		create_capsule_body(0.10f, 0.07f, JPH::Vec3(HALF_PI, 0.f, 0.f)),		// Spine
		create_capsule_body(0.11f, 0.07f, JPH::Vec3(HALF_PI, 0.f, 0.f)),		// Spine1
		create_capsule_body(0.12f, 0.07f, JPH::Vec3(HALF_PI, 0.f, 0.f)),		// Spine2
		create_capsule_limb(0.065f, 0.10f, JPH::Vec3(0.f,  0.f, -HALF_PI)),	  // Head
		create_capsule_limb(0.11f, 0.06f, JPH::Vec3(0.f,  0.f, -HALF_PI)),		// LeftArm
		create_capsule_limb(0.11f, 0.06f, JPH::Vec3(0.f,  0.f, HALF_PI)),		// RightArm
		create_capsule_limb(0.15f, 0.05f, JPH::Vec3(0.f,  0.f, -HALF_PI)),		// LeftForeArm
		create_capsule_limb(0.15f, 0.05f, JPH::Vec3(0.f,  0.f, HALF_PI)),		// RightForeArm
		create_capsule_limb(0.16f, 0.075f, JPH::Vec3(0.f,  0.f, HALF_PI)),		// LeftUpLeg
		create_capsule_limb(0.16f, 0.075f, JPH::Vec3(0.f,  0.f, -HALF_PI)),		// RightUpLeg
		create_capsule_limb(0.2f, 0.06f, JPH::Vec3(0.f,  0.f, HALF_PI)),		// LeftLeg
		create_capsule_limb(0.2f, 0.06f, JPH::Vec3(0.f,  0.f, -HALF_PI)),		// RightLeg
	};

	JPH::RVec3 constraint_positions[JOINT_COUNT];
	for (int i = 0; i < JOINT_COUNT; i++)
	{
		alignas(16) float position[3];
		ozz::math::Store3Ptr(nodeTranslations[i], position);
		constraint_positions[i] = JPH::RVec3(position[0], position[1], position[2]);
	}

	// World space twist axis directions
	JPH::Vec3 twist_axis[JOINT_COUNT] = {
		JPH::Vec3::sZero(),				// Lower Body (unused, there's no parent)
		JPH::Vec3::sAxisY(),				// Mid Body
		JPH::Vec3::sAxisY(),				// Mid Body
		JPH::Vec3::sAxisY(),				// Upper Body
		JPH::Vec3::sAxisY(),				// Head
		-JPH::Vec3::sAxisX(),			// Upper Arm L
		JPH::Vec3::sAxisX(),				// Upper Arm R
		-JPH::Vec3::sAxisX(),			// Lower Arm L
		JPH::Vec3::sAxisX(),				// Lower Arm R
		-JPH::Vec3::sAxisY(),			// Upper Leg L
		-JPH::Vec3::sAxisY(),			// Upper Leg R
		-JPH::Vec3::sAxisY(),			// Lower Leg L
		-JPH::Vec3::sAxisY(),			// Lower Leg R
	};

	// Constraint limits
	float twist_angle[JOINT_COUNT] = {
		0.0f,		// Lower Body (unused, there's no parent)
		5.0f,		// Mid Body
		5.0f,		// Mid Body
		5.0f,		// Upper Body
		90.0f,		// Head
		45.0f,		// Upper Arm L
		45.0f,		// Upper Arm R
		45.0f,		// Lower Arm L
		45.0f,		// Lower Arm R
		45.0f,		// Upper Leg L
		45.0f,		// Upper Leg R
		45.0f,		// Lower Leg L
		45.0f,		// Lower Leg R
	};

	float normal_angle[JOINT_COUNT] = {
		0.0f,		// Lower Body (unused, there's no parent)
		10.0f,		// Mid Body
		10.0f,		// Upper Body
		10.0f,		// Upper Body
		45.0f,		// Head
		90.0f,		// Upper Arm L
		90.0f,		// Upper Arm R
		0.0f,		// Lower Arm L
		0.0f,		// Lower Arm R
		45.0f,		// Upper Leg L
		45.0f,		// Upper Leg R
		0.0f,		// Lower Leg L
		0.0f,		// Lower Leg R
	};

	float plane_angle[JOINT_COUNT] = {
		0.0f,		// Lower Body (unused, there's no parent)
		10.0f,		// Mid Body
		10.0f,		// Upper Body
		10.0f,		// Upper Body
		45.0f,		// Head
		45.0f,		// Upper Arm L
		45.0f,		// Upper Arm R
		90.0f,		// Lower Arm L
		90.0f,		// Lower Arm R
		45.0f,		// Upper Leg L
		45.0f,		// Upper Leg R
		60.0f,		// Lower Leg L (cheating here, a knee is not symmetric, we should have rotated the twist axis)
		60.0f,		// Lower Leg R
	};

	// Create ragdoll settings
	JPH::RagdollSettings *settings = new JPH::RagdollSettings;
	settings->mSkeleton = skeleton;
	settings->mParts.resize(skeleton->GetJointCount());
	for (int p = 0; p < skeleton->GetJointCount(); ++p)
	{
		JPH::RagdollSettings::Part &part = settings->mParts[p];
		part.SetShape(shapes[p]);

		alignas(16) float position[3];
		alignas(16) float rotation[4];
		ozz::math::Store3Ptr(nodeTranslations[p], position);
		ozz::math::StorePtr(nodeRotations[p], rotation);
		part.mPosition = JPH::RVec3(position[0], position[1], position[2]);
		part.mRotation = JPH::Quat(rotation[0], rotation[1], rotation[2], rotation[3]);
		part.mMotionType = JPH::EMotionType::Dynamic;
		part.mObjectLayer = sObjectLayer;

		// First part is the root, doesn't have a parent and doesn't have a constraint
		if (p > 0)
		{
			JPH::SwingTwistConstraintSettings *constraint = new JPH::SwingTwistConstraintSettings;
			constraint->mDrawConstraintSize = 0.1f;
			constraint->mPosition1 = constraint->mPosition2 = constraint_positions[p];
			constraint->mTwistAxis1 = constraint->mTwistAxis2 = twist_axis[p];
			constraint->mPlaneAxis1 = constraint->mPlaneAxis2 = JPH::Vec3::sAxisZ();
			constraint->mTwistMinAngle = -JPH::DegreesToRadians(twist_angle[p]);
			constraint->mTwistMaxAngle = JPH::DegreesToRadians(twist_angle[p]);
			constraint->mNormalHalfConeAngle = JPH::DegreesToRadians(normal_angle[p]);
			constraint->mPlaneHalfConeAngle = JPH::DegreesToRadians(plane_angle[p]);
			part.mToParent = constraint;
		}
	}

	// Optional: Stabilize the inertia of the limbs
	settings->Stabilize();

	// Disable parent child collisions so that we don't get collisions between constrained bodies
	// settings->DisableParentChildCollisions();

	// Calculate the map needed for GetBodyIndexToConstraintIndex()
	settings->CalculateBodyIndexToConstraintIndex();

	return settings;
}