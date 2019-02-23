#include "main_character.h"
#include <chrono>

enum MainCharacterAnimation: uint16 { STAND_BY_RIGHT = 0, STAND_BY_LEFT = 1, WALK_TO_RIGHT = 2, WALK_TO_LEFT = 3, DIRECTION_TO_RIGHT = 4, DIRECTION_TO_LEFT = 5 };

class MainCharacterIdleState;
class MainCharacterRunToRightState;

MainCharacter::MainCharacter() :
        ISceneObject(SceneObjectIdentificator::MAIN_CHARACTER, MainCharacterStateIdentificator::MAIN_CHARACTER_MAX_STATES) {
        cout << " INSTANCIA NOVA DE MainCharacter CREADA" << endl;
        position.x = 100;
        position.y = 100;
}

uint16 MainCharacter::Width() {
        return currentSprite.width;
}

uint16 MainCharacter::Height() {
        return currentSprite.height;
}

void MainCharacter::PrintName() {
        std::cout << "Main character." << std::endl;
}

bool MainCharacter::Update(uchar pressedKeys) {

        if(pressedKeys != KeyboardKeyCode::KEY_NONE) {
                ProcessPressedKeys(pressedKeys);
        } else if(pressedKeys != prevPressedKeys) {
                ProcessReleasedKeys();
        }

        if(!animationLoaded) {
                return false;
        }

        if(animationHasOnlyOneSprite && firstSpriteOfCurrentAnimationIsLoaded) {
                return false;
        }

        if(std::chrono::system_clock::now() >= nextSpriteTime) {
                // Load next sprite of the current animation
                SpriteData spriteData = NextSpriteData();
                nextSpriteTime = (std::chrono::system_clock::now() + std::chrono::milliseconds(spriteData.duration));

                currentSprite.width = spriteData.width;
                currentSprite.height = spriteData.height;
                currentSprite.u1 = spriteData.u1;
                currentSprite.v1 = spriteData.v1;
                currentSprite.u2 = spriteData.u2;
                currentSprite.v2 = spriteData.v2;

                firstSpriteOfCurrentAnimationIsLoaded = true;
                if(spriteData.beginNewLoop) {
                  cout << "*** BEGIN NEW LOOP ***" << endl;
                }
                return true;
        }

        return false;
}

void MainCharacter::ProcessPressedKeys(uchar pressedKeys)
{
        // User pressed KEY_RIGHT
        if((pressedKeys & KeyboardKeyCode::KEY_RIGHT) == KeyboardKeyCode::KEY_RIGHT) {
                // If is not KEY_RIGHT repeated press then change character state
                if((prevPressedKeys & KeyboardKeyCode::KEY_RIGHT) != KeyboardKeyCode::KEY_RIGHT) {
                        //transit<MainCharacterRunToRightState>();
                        //stateMachine.dispatch(KeyRightPressedEvent());
//                        LoadAnimationWithId(MainCharacterAnimation::WALK_TO_RIGHT);
                        headedToRight = true;
                        RightKeyPressed();

                        prevPressedKeys = pressedKeys;
                }
//                position.addX(1.3f);

        } else if((prevPressedKeys & KeyboardKeyCode::KEY_RIGHT) == KeyboardKeyCode::KEY_RIGHT) {
                RightKeyReleased();

                prevPressedKeys = pressedKeys;
        }

        if((pressedKeys & KeyboardKeyCode::KEY_LEFT) == KeyboardKeyCode::KEY_LEFT) {
                // If is not KEY_LEFT repeated press then change character state
                if((prevPressedKeys & KeyboardKeyCode::KEY_LEFT) != KeyboardKeyCode::KEY_LEFT) {
                        cout << "KEY LEFT PRESSED" << endl;
//                        LoadAnimationWithId(MainCharacterAnimation::WALK_TO_LEFT);
                        headedToRight = false;
                        LeftKeyPressed();

                        prevPressedKeys = pressedKeys;
                }
//                position.addX(-1.3f);

        } else if((prevPressedKeys & KeyboardKeyCode::KEY_LEFT) == KeyboardKeyCode::KEY_LEFT) {
                cout << "KEY LEFT RELEASED" << endl;
                LeftKeyReleased();

                prevPressedKeys = pressedKeys;
        }

}

void MainCharacter::ProcessReleasedKeys() {
  if((prevPressedKeys & KeyboardKeyCode::KEY_RIGHT) == KeyboardKeyCode::KEY_RIGHT) {
          RightKeyReleased();
  }

  if((prevPressedKeys & KeyboardKeyCode::KEY_LEFT) == KeyboardKeyCode::KEY_LEFT) {
          LeftKeyReleased();
  }

  // character goes quiet headed in the proper direction
//  LoadAnimationWithId(headedToRight ? MainCharacterAnimation::STAND_BY_RIGHT : MainCharacterAnimation::STAND_BY_LEFT);

  prevPressedKeys = KeyboardKeyCode::KEY_NONE;
}

void MainCharacter::InitWithSpriteSheet(ObjectSpriteSheet *_spriteSheet) {
        spriteSheet = _spriteSheet;
        test = new Position();
        LoadAnimationWithId(MainCharacterAnimation::STAND_BY_RIGHT);
}

void MainCharacter::LoadAnimationWithId(uint16 animationId) {
        ObjectSpriteSheetAnimation *currentAnimation = spriteSheet->GetAnimationWithId(animationId);
        currentAnimationSprites = currentAnimation->GetSprites();
        animationHasOnlyOneSprite = (currentAnimationSprites.size() <= 1);
        currentAnimationSpriteIterator = std::begin(currentAnimationSprites);
        animationLoaded = true;
        firstSpriteOfCurrentAnimationIsLoaded = false;
        nextSpriteTime = std::chrono::system_clock::now();
}

SpriteData MainCharacter::NextSpriteData()
{
        if(currentAnimationSpriteIterator == std::end(currentAnimationSprites)) {
                currentAnimationSpriteIterator = std::begin(currentAnimationSprites);
                (*currentAnimationSpriteIterator).beginNewLoop = true;
        }

        return *currentAnimationSpriteIterator++;
}

ISceneObject* MainCharacter::Create() {
        return new MainCharacter();
}

MainCharacter::~MainCharacter() {

}

void MainCharacter::RightKeyPressed()
{
        cout << "MainCharacter::RightKeyPressed()" << endl;
        BEGIN_TRANSITION_MAP                    // - Current State -
        TRANSITION_MAP_ENTRY (STATE_RUN_RIGHT)  // STATE_Idle_Right
        TRANSITION_MAP_ENTRY (STATE_CHANGE_DIRECTION_RIGHT)  // STATE_Idle_Left
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Run_Right
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Run_Left
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fast_Run_Right
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fast_Run_Left
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Short_Break_Right
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Short_Break_Left
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Change_Direction_Right
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Change_Direction_Left
        END_TRANSITION_MAP(NULL)
}

void MainCharacter::RightKeyReleased()
{
        cout << "MainCharacter::RightKeyReleased()" << endl;
        BEGIN_TRANSITION_MAP                    // - Current State -
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Idle_Right
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Idle_Left
        TRANSITION_MAP_ENTRY (STATE_IDLE_RIGHT)    // STATE_Run_Right
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Run_Left
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fast_Run_Right
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fast_Run_Left
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Short_Break_Right
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Short_Break_Left
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Change_Direction_Right
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Change_Direction_Left
        END_TRANSITION_MAP(NULL)
}

void MainCharacter::LeftKeyPressed()
{
        cout << "MainCharacter::LeftKeyPressed()" << endl;
        BEGIN_TRANSITION_MAP                                  // - Current State -
        TRANSITION_MAP_ENTRY (STATE_CHANGE_DIRECTION_LEFT)    // STATE_Idle_Right
        TRANSITION_MAP_ENTRY (STATE_RUN_LEFT)                 // STATE_Idle_Left
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)                  // STATE_Run_Right
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)                  // STATE_Run_Left
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)                  // STATE_Fast_Run_Right
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)                  // STATE_Fast_Run_Left
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)                  // STATE_Short_Break_Right
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)                  // STATE_Short_Break_Left
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)                  // STATE_Change_Direction_Right
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)                  // STATE_Change_Direction_Left
        END_TRANSITION_MAP(NULL)
}

void MainCharacter::LeftKeyReleased()
{
        cout << "MainCharacter::LeftKeyReleased()" << endl;
        BEGIN_TRANSITION_MAP                    // - Current State -
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Idle_Right
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Idle_Left
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Run_Right
        TRANSITION_MAP_ENTRY (STATE_IDLE_LEFT)    // STATE_Run_Left
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fast_Run_Right
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fast_Run_Left
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Short_Break_Right
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Short_Break_Left
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Change_Direction_Right
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Change_Direction_Left
        END_TRANSITION_MAP(NULL)
}

void MainCharacter::STATE_Idle_Right()
{
        cout << "MainCharacter::STATE_Idle_Right" << endl;
        LoadAnimationWithId(MainCharacterAnimation::STAND_BY_RIGHT);
}

void MainCharacter::STATE_Idle_Left()
{
        cout << "MainCharacter::STATE_Idle_Left" << endl;
        LoadAnimationWithId(MainCharacterAnimation::STAND_BY_LEFT);
}

void MainCharacter::STATE_Run_Right()
{
        cout << "MainCharacter::STATE_Run_Right" << endl;
        LoadAnimationWithId(MainCharacterAnimation::WALK_TO_RIGHT);
}

void MainCharacter::STATE_Run_Left()
{
        cout << "MainCharacter::STATE_Run_Left" << endl;
        LoadAnimationWithId(MainCharacterAnimation::WALK_TO_LEFT);
}

void MainCharacter::STATE_Fast_Run_Right()
{
        cout << "MainCharacter::STATE_Fast_Run_Right" << endl;
}

void MainCharacter::STATE_Fast_Run_Left()
{
        cout << "MainCharacter::STATE_Fast_Run_Left" << endl;
}

void MainCharacter::STATE_Short_Break_Right()
{
        cout << "MainCharacter::STATE_Short_Break_Right" << endl;
}

void MainCharacter::STATE_Short_Break_Left()
{
        cout << "MainCharacter::STATE_Short_Break_Left" << endl;
}

void MainCharacter::STATE_Change_Direction_Left()
{
        cout << "MainCharacter::STATE_Change_Direction_Left" << endl;
        LoadAnimationWithId(MainCharacterAnimation::DIRECTION_TO_LEFT);
}

void MainCharacter::STATE_Change_Direction_Right()
{
        cout << "MainCharacter::STATE_Change_Direction_Right" << endl;
        LoadAnimationWithId(MainCharacterAnimation::DIRECTION_TO_RIGHT);
}
