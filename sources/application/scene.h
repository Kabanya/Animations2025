#pragma once

#include "engine/render/direction_light.h"
#include "engine/import/model.h"
#include "user_camera.h"
#include "character.h"
#include "physics_world.h"

struct Scene
{
  std::vector<ModelAsset> models;
  DirectionLight light;

  UserCamera userCamera;

  std::vector<Character> characters;

  std::unique_ptr<PhysicsWorld> physicsWorld;
  ~Scene()
  {
    characters.clear();
    physicsWorld.reset();
    destroy_phys_globals();
  }
};