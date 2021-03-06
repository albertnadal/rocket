#include "side_wall.h"
#include <chrono>

SideWall::SideWall() :
        ISceneObject(SceneObjectIdentificator::SIDE_WALL, SceneObjectType::TERRAIN, SideWallStateIdentificator::SIDE_WALL_MAX_STATES) {
}

SideWall::SideWall(SceneObjectIdentificator scene_id, SceneObjectType scene_type, unsigned char max_states) :
        ISceneObject(scene_id, scene_type, max_states) {
}

uint16_t SideWall::Width() {
        return currentSprite.width;
}

uint16_t SideWall::Height() {
        return currentSprite.height;
}

void SideWall::PrintName() {
        std::cout << "SideWall." << std::endl;
}

bool SideWall::Update(uint8_t pressedKeys_) {
        bool needRedraw = false;

        if(!animationLoaded) {
                return false;
        }

        if(animationHasOnlyOneSprite && firstSpriteOfCurrentAnimationIsLoaded) {
                return false;
        }

        if(chrono::system_clock::now() >= nextSpriteTime) {
                // Load next sprite of the current animation
                LoadNextSprite();
                return true;
        }

        return needRedraw;
}

void SideWall::InitWithSpriteSheet(ObjectSpriteSheet *_spriteSheet) {
        spriteSheet = _spriteSheet;
}

void SideWall::LoadAnimationWithId(uint16_t animationId) {
        ObjectSpriteSheetAnimation *currentAnimation = spriteSheet->GetAnimationWithId(animationId);
        currentAnimationSprites = currentAnimation->GetSprites();
        animationHasOnlyOneSprite = (currentAnimationSprites.size() <= 1);
        currentAnimationSpriteIterator = std::begin(currentAnimationSprites);
        animationLoaded = true;
        firstSpriteOfCurrentAnimationIsLoaded = false;
        nextSpriteTime = std::chrono::system_clock::now();
}

void SideWall::LoadNextSprite()
{
  SpriteData spriteData = NextSpriteData();
  if(spriteData.beginNewLoop) {
          cout << "*** BEGIN NEW LOOP ***" << endl;
          if(BeginAnimationLoopAgain()) {
            spriteData = NextSpriteData();
          }
  }

  nextSpriteTime = (chrono::system_clock::now() + std::chrono::milliseconds(spriteData.duration));

  currentSprite.width = spriteData.width;
  currentSprite.height = spriteData.height;
  currentSprite.u1 = spriteData.u1;
  currentSprite.v1 = spriteData.v1;
  currentSprite.u2 = spriteData.u2;
  currentSprite.v2 = spriteData.v2;
  boundingBox = { spriteData.lowerBoundX, spriteData.lowerBoundY, spriteData.upperBoundX, spriteData.upperBoundY };
  firstSpriteOfCurrentAnimationIsLoaded = true;
}

SpriteData SideWall::NextSpriteData()
{
        if(currentAnimationSpriteIterator == std::end(currentAnimationSprites)) {
                currentAnimationSpriteIterator = std::begin(currentAnimationSprites);
                (*currentAnimationSpriteIterator).beginNewLoop = true;
        }

        return *currentAnimationSpriteIterator++;
}

ISceneObject* SideWall::Create() {
          return new SideWall();
}

SideWall::~SideWall() {

}

bool SideWall::BeginAnimationLoopAgain()
{
        return false;
}
