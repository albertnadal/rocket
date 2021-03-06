#include "object_sprite_sheet.h"

ObjectSpriteSheet::ObjectSpriteSheet(SceneObjectIdentificator _id)
        : Id(_id)
{

}

ObjectSpriteSheet::~ObjectSpriteSheet() {
  std::map<uint16_t, ObjectSpriteSheetAnimation*>::iterator it = animations.begin();

	while (it != animations.end())
	{
		uint16_t objectAnimationId = it->first;
		ObjectSpriteSheetAnimation* objectAnimation = it->second;
		delete objectAnimation;
		it++;
	}
}

void ObjectSpriteSheet::Print()
{
  printf("SceneObject type Id: %d\n", Id);
  printf("Total object actions: %lu\n", animations.size());

  std::map<uint16_t, ObjectSpriteSheetAnimation*>::iterator it = animations.begin();

	while (it != animations.end())
	{
		it->second->Print();
		it++;
	}
}

void ObjectSpriteSheet::AddAnimation(ObjectSpriteSheetAnimation *animation) {
  animations.insert(std::pair<uint16_t,ObjectSpriteSheetAnimation*>(animation->Id, animation));
}

ObjectSpriteSheetAnimation* ObjectSpriteSheet::GetAnimationWithId(uint16_t AnimationId) {
  ObjectSpriteSheetAnimation* a = animations.find(AnimationId)->second;
  return a;
}
