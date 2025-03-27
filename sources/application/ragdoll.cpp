#include "physics_world.h"
#include <Jolt/Physics/Ragdoll/Ragdoll.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/SliderConstraint.h>
#include <Jolt/Physics/Constraints/ConeConstraint.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>

JPH::Ref<JPH::RagdollSettings> create_ragdoll_settings()
{
	// Create skeleton
	JPH::Ref<JPH::Skeleton> skeleton = new JPH::Skeleton;
	JPH::uint lower_body = skeleton->AddJoint("LowerBody");
	JPH::uint mid_body = skeleton->AddJoint("MidBody", lower_body);
	JPH::uint upper_body = skeleton->AddJoint("UpperBody", mid_body);
	/*JPH::uint head =*/ skeleton->AddJoint("Head", upper_body);
	JPH::uint upper_arm_l = skeleton->AddJoint("UpperArmL", upper_body);
	JPH::uint upper_arm_r = skeleton->AddJoint("UpperArmR", upper_body);
	/*JPH::uint lower_arm_l =*/ skeleton->AddJoint("LowerArmL", upper_arm_l);
	/*JPH::uint lower_arm_r =*/ skeleton->AddJoint("LowerArmR", upper_arm_r);
	JPH::uint upper_leg_l = skeleton->AddJoint("UpperLegL", lower_body);
	JPH::uint upper_leg_r = skeleton->AddJoint("UpperLegR", lower_body);
	/*JPH::uint lower_leg_l =*/ skeleton->AddJoint("LowerLegL", upper_leg_l);
	/*JPH::uint lower_leg_r =*/ skeleton->AddJoint("LowerLegR", upper_leg_r);

	// Create shapes for limbs
	JPH::Ref<JPH::Shape> shapes[] = {
		new JPH::CapsuleShape(0.15f, 0.10f),		// Lower Body
		new JPH::CapsuleShape(0.15f, 0.10f),		// Mid Body
		new JPH::CapsuleShape(0.15f, 0.10f),		// Upper Body
		new JPH::CapsuleShape(0.075f, 0.10f),	// Head
		new JPH::CapsuleShape(0.15f, 0.06f),		// Upper Arm L
		new JPH::CapsuleShape(0.15f, 0.06f),		// Upper Arm R
		new JPH::CapsuleShape(0.15f, 0.05f),		// Lower Arm L
		new JPH::CapsuleShape(0.15f, 0.05f),		// Lower Arm R
		new JPH::CapsuleShape(0.2f, 0.075f),		// Upper Leg L
		new JPH::CapsuleShape(0.2f, 0.075f),		// Upper Leg R
		new JPH::CapsuleShape(0.2f, 0.06f),		// Lower Leg L
		new JPH::CapsuleShape(0.2f, 0.06f),		// Lower Leg R
	};

	// Positions of body parts in world space
	JPH::RVec3 positions[] = {
		JPH::RVec3(0, 1.15f, 0),					// Lower Body
		JPH::RVec3(0, 1.35f, 0),					// Mid Body
		JPH::RVec3(0, 1.55f, 0),					// Upper Body
		JPH::RVec3(0, 1.825f, 0),				// Head
		JPH::RVec3(-0.425f, 1.55f, 0),			// Upper Arm L
		JPH::RVec3(0.425f, 1.55f, 0),			// Upper Arm R
		JPH::RVec3(-0.8f, 1.55f, 0),				// Lower Arm L
		JPH::RVec3(0.8f, 1.55f, 0),				// Lower Arm R
		JPH::RVec3(-0.15f, 0.8f, 0),				// Upper Leg L
		JPH::RVec3(0.15f, 0.8f, 0),				// Upper Leg R
		JPH::RVec3(-0.15f, 0.3f, 0),				// Lower Leg L
		JPH::RVec3(0.15f, 0.3f, 0),				// Lower Leg R
	};

	// Rotations of body parts in world space
	JPH::Quat rotations[] = {
		JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), 0.5f * JPH::JPH_PI),		 // Lower Body
		JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), 0.5f * JPH::JPH_PI),		 // Mid Body
		JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), 0.5f * JPH::JPH_PI),		 // Upper Body
		JPH::Quat::sIdentity(),									 // Head
		JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), 0.5f * JPH::JPH_PI),		 // Upper Arm L
		JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), 0.5f * JPH::JPH_PI),		 // Upper Arm R
		JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), 0.5f * JPH::JPH_PI),		 // Lower Arm L
		JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), 0.5f * JPH::JPH_PI),		 // Lower Arm R
		JPH::Quat::sIdentity(),									 // Upper Leg L
		JPH::Quat::sIdentity(),									 // Upper Leg R
		JPH::Quat::sIdentity(),									 // Lower Leg L
		JPH::Quat::sIdentity()									 // Lower Leg R
	};

	// World space constraint positions
	JPH::RVec3 constraint_positions[] = {
		JPH::RVec3::sZero(),				// Lower Body (unused, there's no parent)
		JPH::RVec3(0, 1.25f, 0),			// Mid Body
		JPH::RVec3(0, 1.45f, 0),			// Upper Body
		JPH::RVec3(0, 1.65f, 0),			// Head
		JPH::RVec3(-0.225f, 1.55f, 0),	// Upper Arm L
		JPH::RVec3(0.225f, 1.55f, 0),	// Upper Arm R
		JPH::RVec3(-0.65f, 1.55f, 0),	// Lower Arm L
		JPH::RVec3(0.65f, 1.55f, 0),		// Lower Arm R
		JPH::RVec3(-0.15f, 1.05f, 0),	// Upper Leg L
		JPH::RVec3(0.15f, 1.05f, 0),		// Upper Leg R
		JPH::RVec3(-0.15f, 0.55f, 0),	// Lower Leg L
		JPH::RVec3(0.15f, 0.55f, 0),		// Lower Leg R
	};

	// World space twist axis directions
	JPH::Vec3 twist_axis[] = {
		JPH::Vec3::sZero(),				// Lower Body (unused, there's no parent)
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
	float twist_angle[] = {
		0.0f,		// Lower Body (unused, there's no parent)
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

	float normal_angle[] = {
		0.0f,		// Lower Body (unused, there's no parent)
		10.0f,		// Mid Body
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

	float plane_angle[] = {
		0.0f,		// Lower Body (unused, there's no parent)
		10.0f,		// Mid Body
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
		part.mPosition = positions[p];
		part.mRotation = rotations[p];
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