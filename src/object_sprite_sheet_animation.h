#ifndef OBJECT_SPRITE_SHEET_ANIMATION_H
#define OBJECT_SPRITE_SHEET_ANIMATION_H

#include <vector>
#include <defines.h>
#include "sprite.h"

struct SpriteData { uint16 width, height; float u1, v1, u2, v2; uint16 duration; bool beginNewLoop; uint16 lowerBoundX, lowerBoundY, upperBoundX, upperBoundY; SpriteAreas *areas; };

class ObjectSpriteSheetAnimation
{
  std::vector<SpriteData> sprites;
public:
  uint16 Id;
  ObjectSpriteSheetAnimation(uint16);
  ~ObjectSpriteSheetAnimation();
  void AddSprite(SpriteData);
  const std::vector<SpriteData>& GetSprites() const;
  void Print();
};
#endif
