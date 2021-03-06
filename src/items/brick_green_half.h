#ifndef BRICK_GREEN_HALF_H
#define BRICK_GREEN_HALF_H

#include <iostream>
#include <vector>
#include <scene_object.h>
#include <state_machine.h>
#include <vec2.h>
#include <sprite.h>
#include <position.h>
#include <items/brick.h>

using namespace std;

class BrickGreenHalf : public Brick
{
public:
  BrickGreenHalf();
  ~BrickGreenHalf();
  void InitWithSpriteSheet(ObjectSpriteSheet*);
  void PrintName();
  static ISceneObject* Create();
};

#endif
