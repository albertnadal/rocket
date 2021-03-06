#include "items/main_character.h"
#include <chrono>

MainCharacter::MainCharacter() :
        ISceneObject(SceneObjectIdentificator::MAIN_CHARACTER, SceneObjectType::PLAYER,
                     MainCharacterStateIdentificator::MAIN_CHARACTER_MAX_STATES) {
    //Initially the object is quiet
    vectorDirection.x = 0;
    vectorDirection.y = 0;
    prevVectorDirection.x = 0;
    prevVectorDirection.y = 0;
}

uint16_t MainCharacter::Width() {
    return currentSprite.width;
}

uint16_t MainCharacter::Height() {
    return currentSprite.height;
}

void MainCharacter::PrintName() {
    std::cout << "Main character." << std::endl;
}

bool MainCharacter::Update(const uint8_t pressedKeys_) {

    bool needRedraw = false;
    pressedKeys = pressedKeys_;

    if (isJumping) {
        UpdateJump();
        needRedraw = true;
    } else if (isFalling) {
        UpdateFall();
        needRedraw = true;
    } else {

        if (pressedKeys != KeyboardKeyCode::KEY_NONE) {
            ProcessPressedKeys();
        } else if (pressedKeys != prevPressedKeys) {
            ProcessReleasedKeys();
        }

    }

    // Check for collisions
    UpdateCollisions();

    if (!animationLoaded) {
        return false;
    }

    if (animationHasOnlyOneSprite && firstSpriteOfCurrentAnimationIsLoaded) {
        return false;
    }

    if (chrono::system_clock::now() >= nextSpriteTime) {
        // Load next sprite of the current animation
        LoadNextSprite();

        // Check for possible collisions with the solid areas of the currrent sprite
        UpdateCollisions();

        return true;
    }

    return needRedraw;
}

// Search for collisions with solid objects
void MainCharacter::GetSolidCollisions(std::vector<ObjectCollisionData> &collidingSolidObjects) {
    if (currentSprite.areas == nullptr) return;

    // Check for possible potential collision candidates objects
    std::vector<ISceneObject *> potentialCollisionCandidatesObjects = spacePartitionObjectsTree->query(GetLowerBound(),GetUpperBound());

    uint16_t potentialCollisionObjectsCount = potentialCollisionCandidatesObjects.size();
    if (potentialCollisionObjectsCount) {

        // Iterate all potential collision candidates and check for real collisions
        for (uint16_t i = 0; i < potentialCollisionObjectsCount; i++) {
            ISceneObject *collisionCandidateObject = potentialCollisionCandidatesObjects[i];
            if (collisionCandidateObject != this) {

                // Check precise collision of every solid area of the collision candidate object with every solid area of the main character
                std::vector<Area> &collisionCandidateObjectSolidAreas = collisionCandidateObject->GetSolidAreas();
                for (auto & collisionCandidateObjectSolidArea : collisionCandidateObjectSolidAreas) {
                    collision::Rectangle candidateSolidAreaRectangle = collisionCandidateObjectSolidArea.rectangle;

                    // Check collision with all main character solid areas
                    std::vector<Area> &mainCharacterSolidAreas = GetSolidAreas();

                    collision::Penetration penetration;
                    for (auto & mainCharacterSolidArea : mainCharacterSolidAreas) {
                        collision::Rectangle mainCharacterSolidAreaRectangle = mainCharacterSolidArea.rectangle;
                        bool collision = collisionDetector.checkCollision(mainCharacterSolidAreaRectangle,
                                                                          candidateSolidAreaRectangle, penetration,
                                                                          PlayerIsQuiet() ? prevVectorDirection
                                                                                          : vectorDirection);

                        if (collision) {
                            //mainCharacterSolidAreaRectangle.Print();
                            //candidateSolidAreaRectangle.Print();
                            collidingSolidObjects.push_back({collisionCandidateObject, penetration.depth.x, penetration.depth.y,&vectorDirection});
                        }
                    }
                }
            }
        }
    }
}

void MainCharacter::MoveToPositionOfNoCollision(std::vector<ObjectCollisionData> &collidingSolidObjectsData) {
    if (currentSprite.areas == nullptr) return;

    std::vector<collision::Rectangle> targetRectangles;
    for(auto & solidObjectData : collidingSolidObjectsData) {
        for (auto & solidArea : solidObjectData.object->GetSolidAreas()) {
            targetRectangles.push_back(solidArea.rectangle);
        }
    }

    std::vector<collision::Rectangle> movingRectangles;
    for (auto & solidArea : GetSolidAreas()) {
        movingRectangles.push_back(solidArea.rectangle);
    }

    collisionDetector.updateWithNonCollidingPosition(targetRectangles, movingRectangles, position,
                                                     PlayerIsQuiet() ? prevVectorDirection : vectorDirection);
}

void MainCharacter::UpdateCollisions() {
    std::cout << "TRAJECTORY TANGENT: " << position.getTrajectoryTangent() << "\n";

    std::vector<ObjectCollisionData> collidingSolidObjects;
    // Search for collisions with solid objects
    this->GetSolidCollisions(collidingSolidObjects);

    // Process colliding objects according to the current scenario
    uint16_t collidingSolidObjectsCount = collidingSolidObjects.size();
    if (collidingSolidObjectsCount) {

        if ((vectorDirection.x == 0) || (vectorDirection.y == 0)) {
            std::cout << " >>>>>> VERTICAL or HORIZONTAL COLLISION\n";
            // Collision when object displacement is vertical or horizontal. Non diagonal displacement.
            int16_t maxHorizontalPenetrationDepth = 0;
            int16_t maxVerticalPenetrationDepth = 0;

            for (uint16_t i = 0; i < collidingSolidObjectsCount; i++) {
                ObjectCollisionData objectCollisionData = collidingSolidObjects.at(i);
                if (std::abs(objectCollisionData.penetration_y) > std::abs(maxVerticalPenetrationDepth)) {
                    maxVerticalPenetrationDepth = objectCollisionData.penetration_y;
                }

                if (std::abs(objectCollisionData.penetration_x) > std::abs(maxHorizontalPenetrationDepth)) {
                    maxHorizontalPenetrationDepth = objectCollisionData.penetration_x;
                }
            }

            // Update the array of pillar objects (objects who are currently sustaining the main character from the bottom side)
            pillarObjects.clear();
            for (uint16_t i = 0; i < collidingSolidObjectsCount; i++) {
                ObjectCollisionData objectCollisionData = collidingSolidObjects.at(i);
                if (objectCollisionData.penetration_y == maxVerticalPenetrationDepth) {
                    pillarObjects.push_back(objectCollisionData.object);
                }
            }

            PositionAddX(int16_t(-maxHorizontalPenetrationDepth));
            PositionAddY(int16_t(-maxVerticalPenetrationDepth));

            // Colliding during jump causes finish jump and fall
            if (isJumping) {
                // Check if collision has been detected on top of the main character during jumping
                if (maxVerticalPenetrationDepth > 0) {
                    // Causes main character fall down
                    TopCollisionDuringJump();
                } else {
                    // Causes main character lay on the ground
                    FinishJump();
                }
            } else if (isFalling) {
                // If collision is produced during fall then finish the fall;
                FinishFall();
            }

        } else {
            std::cout << " >>>>>> DIAGONAL COLLISION\n";
            // 
            this->MoveToPositionOfNoCollision(collidingSolidObjects);

            if (isJumping) { FinishJump(); }
        }

        //ProcessPressedKeys(false);
    }
}

void MainCharacter::UpdatePreviousDirection() {
    if ((vectorDirection.x != 0) || (vectorDirection.y != 0)) {
        prevVectorDirection.x = vectorDirection.x;
        prevVectorDirection.y = vectorDirection.y;
    }
}

bool MainCharacter::PlayerIsQuiet() {
    return (vectorDirection.x == vectorDirection.y) && (vectorDirection.x == 0);
}

void MainCharacter::ProcessPressedKeys(bool checkPreviousPressedKeys) {
    // User pressed KEY_RIGHT
    if ((pressedKeys & KeyboardKeyCode::KEY_RIGHT) == KeyboardKeyCode::KEY_RIGHT) {
        // If is not KEY_RIGHT repeated press then change character state
        if ((!checkPreviousPressedKeys) || ((checkPreviousPressedKeys) && ((prevPressedKeys & KeyboardKeyCode::KEY_RIGHT) != KeyboardKeyCode::KEY_RIGHT))) {
            //transit<MainCharacterRunToRightState>();
            //stateMachine.dispatch(KeyRightPressedEvent());
//                        LoadAnimationWithId(MainCharacterAnimation::RUN_TO_RIGHT);
            headedToRight = true;
            RightKeyPressed();
        }
        MoveTo(MainCharacterDirection::RIGHT);
    } else if ((prevPressedKeys & KeyboardKeyCode::KEY_RIGHT) == KeyboardKeyCode::KEY_RIGHT) {
        RightKeyReleased();
    }

    if ((pressedKeys & KeyboardKeyCode::KEY_LEFT) == KeyboardKeyCode::KEY_LEFT) {
        // If is not KEY_LEFT repeated press then change character state
        if ((!checkPreviousPressedKeys) || ((checkPreviousPressedKeys) &&
                                            ((prevPressedKeys & KeyboardKeyCode::KEY_LEFT) !=
                                             KeyboardKeyCode::KEY_LEFT))) {
            //cout << "KEY LEFT PRESSED" << endl;
//                        LoadAnimationWithId(MainCharacterAnimation::RUN_TO_LEFT);
            headedToRight = false;
            LeftKeyPressed();
        }
        MoveTo(MainCharacterDirection::LEFT);
    } else if ((prevPressedKeys & KeyboardKeyCode::KEY_LEFT) == KeyboardKeyCode::KEY_LEFT) {
        //cout << "KEY LEFT RELEASED" << endl;
        LeftKeyReleased();
    }

    if (!isJumping && ((pressedKeys & KeyboardKeyCode::KEY_UP) == KeyboardKeyCode::KEY_UP)) {
        // If is not KEY_LEFT repeated press then change character state
        if ((!checkPreviousPressedKeys) ||
            ((checkPreviousPressedKeys) && ((prevPressedKeys & KeyboardKeyCode::KEY_UP) != KeyboardKeyCode::KEY_UP))) {
            //cout << "KEY UP PRESSED" << endl;
            UpKeyPressed();
        }
    }

    if (!isJumping && !isHitting && ((pressedKeys & KeyboardKeyCode::KEY_SPACE) == KeyboardKeyCode::KEY_SPACE)) {
        // If is not KEY_LEFT repeated press then change character state
        if ((!checkPreviousPressedKeys) || ((checkPreviousPressedKeys) &&
                                            ((prevPressedKeys & KeyboardKeyCode::KEY_SPACE) !=
                                             KeyboardKeyCode::KEY_SPACE))) {
            //cout << "KEY SPACE PRESSED" << endl;
            isHitting = true;
            SpaceKeyPressed();
        }
    }

    if (!isJumping && ((pressedKeys & KeyboardKeyCode::KEY_DOWN) == KeyboardKeyCode::KEY_DOWN)) {
        // If is not KEY_LEFT repeated press then change character state
        if ((!checkPreviousPressedKeys) || ((checkPreviousPressedKeys) &&
                                            ((prevPressedKeys & KeyboardKeyCode::KEY_DOWN) !=
                                             KeyboardKeyCode::KEY_DOWN))) {
            //cout << "KEY UP PRESSED" << endl;
            DownKeyPressed();
        }
    }

    prevPressedKeys = pressedKeys;
}

void MainCharacter::ProcessReleasedKeys() {
    if ((prevPressedKeys & KeyboardKeyCode::KEY_RIGHT) == KeyboardKeyCode::KEY_RIGHT) {
        RightKeyReleased();
    }

    if ((prevPressedKeys & KeyboardKeyCode::KEY_LEFT) == KeyboardKeyCode::KEY_LEFT) {
        LeftKeyReleased();
    }

    // character goes quiet headed in the proper direction
//  LoadAnimationWithId(headedToRight ? MainCharacterAnimation::STAND_BY_RIGHT : MainCharacterAnimation::STAND_BY_LEFT);

    prevPressedKeys = KeyboardKeyCode::KEY_NONE;
}

void MainCharacter::InitWithSpriteSheet(ObjectSpriteSheet *_spriteSheet) {
    spriteSheet = _spriteSheet;
    LoadAnimationWithId(MainCharacterAnimation::STAND_BY_RIGHT);
}

void MainCharacter::LoadAnimationWithId(uint16_t animationId) {
    ObjectSpriteSheetAnimation *currentAnimation = spriteSheet->GetAnimationWithId(animationId);
    currentAnimationSprites = currentAnimation->GetSprites();
    animationHasOnlyOneSprite = (currentAnimationSprites.size() <= 1);
    currentAnimationSpriteIterator = std::begin(currentAnimationSprites);
    animationLoaded = true;
    firstSpriteOfCurrentAnimationIsLoaded = false;
    nextSpriteTime = std::chrono::system_clock::now();
}

void MainCharacter::LoadNextSprite() {
    SpriteData spriteData = NextSpriteData();

    if (spriteData.beginNewLoop) {
        if (ShouldBeginAnimationLoopAgain()) {
            spriteData = NextSpriteData();
        }
    }

    nextSpriteTime = (chrono::system_clock::now() + chrono::milliseconds(spriteData.duration));

    currentSprite.width = spriteData.width;
    currentSprite.height = spriteData.height;
    currentSprite.xOffset = spriteData.xOffset;
    currentSprite.yOffset = spriteData.yOffset;
    currentSprite.u1 = spriteData.u1;
    currentSprite.v1 = spriteData.v1;
    currentSprite.u2 = spriteData.u2;
    currentSprite.v2 = spriteData.v2;
    currentSprite.areas = spriteData.areas;

    // Adjusts objectposition according to the sprite offset
    PositionSetOffset(spriteData.xOffset, spriteData.yOffset);

    recalculateAreasDataIsNeeded = true; // Is necessary because the current sprite may have different areas
    boundingBox = {spriteData.lowerBoundX, spriteData.lowerBoundY, spriteData.upperBoundX, spriteData.upperBoundY};
    firstSpriteOfCurrentAnimationIsLoaded = true;
}

SpriteData MainCharacter::NextSpriteData() {
    if (currentAnimationSpriteIterator == std::end(currentAnimationSprites)) {
        currentAnimationSpriteIterator = std::begin(currentAnimationSprites);
        (*currentAnimationSpriteIterator).beginNewLoop = true;
    }

    return *currentAnimationSpriteIterator++;
}

ISceneObject *MainCharacter::Create() {
    return new MainCharacter();
}

MainCharacter::~MainCharacter() = default;

bool MainCharacter::ShouldBeginAnimationLoopAgain() {
    if (currentState == MainCharacterStateIdentificator::STATE_HIT_RIGHT) {
        isHitting = false;
        ExternalEvent(MainCharacterStateIdentificator::STATE_IDLE_RIGHT, nullptr);
        return true;
    } else if (currentState == MainCharacterStateIdentificator::STATE_HIT_LEFT) {
        isHitting = false;
        ExternalEvent(MainCharacterStateIdentificator::STATE_IDLE_LEFT, nullptr);
        return true;
    } else {
        return false;
    }
}

void MainCharacter::UpdateJump() {
    tJump += 0.2f;
    //PositionSetX(hInitialJumpPosition + (hInitialJumpSpeed * tJump));
    float vOffset = (vInitialJumpSpeed * tJump - (0.5f) * gravity * tJump * tJump);

    /*if(vOffset <= 0.0f) {
      // Last position of trajectory
      PositionSetY(vInitialJumpPosition);
      // Finish jump
      FinishJump();
    } else {*/
    // Update vertical jumpt position
    //PositionSetY(vInitialJumpPosition + vOffset);
    PositionSetXY(hInitialJumpPosition + (hInitialJumpSpeed * tJump), vInitialJumpPosition + vOffset);
    /*}*/
    UpdatePreviousDirection();
    vectorDirection.x = (hInitialJumpSpeed > 0.0f) ? 1 : (hInitialJumpSpeed < 0.0f) ? -1 : 0;
    vectorDirection.y = (previous_vOffset < vOffset) ? 1 : -1;
    std::cout << "                          ==================> X: " << vectorDirection.x << " Y: " << vectorDirection.y << "\n";
    previous_vOffset = vOffset;
}

void MainCharacter::FinishJump() {
    isJumping = false;
    isLeaningOnTheGround = true;
    hMomentum = 0;
    UpdatePreviousDirection();
    vectorDirection.x = 0;
    vectorDirection.y = 0;
    JumpLanding();
}

void MainCharacter::UpdateFall() {
    tFall += 0.2f;
    PositionSetX(hInitialFallPosition + (hInitialFallSpeed * tFall));
    float vOffset = -(0.5f) * gravity * tFall * tFall;
    /*if(vOffset <= 0.0f) {
      // Jump landing
      position.setY(vInitialJumpPosition);
      isJumping = false;
      isLeaningOnTheGround = true;
      hMomentum = 0;
      JumpLanding();
    } else {*/
    PositionSetY(vInitialFallPosition + vOffset);
    /*}*/
    UpdatePreviousDirection();
    vectorDirection.x = (hInitialFallSpeed > 0.0f) ? 1 : (hInitialFallSpeed < 0.0f) ? -1 : 0;
    vectorDirection.y = -1;
}

void MainCharacter::FinishFall() {
    isFalling = false;
    isLeaningOnTheGround = true;
    UpdatePreviousDirection();
    //vectorDirection.x = 0;
    //vectorDirection.y = 0;
    FallLanding();
}

void MainCharacter::MoveTo(MainCharacterDirection direction) {
    if (!isJumping && !isHitting) {
        PositionAddX(direction == MainCharacterDirection::RIGHT ? 4.0f : -4.0f);
        if (hMomentum < maxMomentum) {
            hMomentum++;
        }
    }
}

void MainCharacter::Jump(float vSpeed, float hSpeed) {
    cout << "MainCharacter::Jump" << endl;
    UpdatePreviousDirection();
    vectorDirection.x = (hSpeed > 0.0f) ? 1 : (hSpeed < 0.0f) ? -1 : 0;
    vectorDirection.y = (vSpeed > 0.0f) ? 1 : (vSpeed < 0.0f) ? -1 : 0;
    vInitialJumpSpeed = vSpeed;
    hInitialJumpSpeed = hSpeed;
    vInitialJumpPosition = position.GetRealY();
    hInitialJumpPosition = position.GetRealX();
    cout << "Initial JUMP X: " << hInitialJumpPosition << " Initial JUMP Y: " << vInitialJumpPosition << endl;
    tJump = 0.0f;
    isJumping = true;
    isFalling = false;
    isLeaningOnTheGround = false;
}

void MainCharacter::Fall(float hSpeed) {
    cout << " >>>>>>>>>>>>>>>>>>>>>> MainCharacter::Fall" << endl;
    UpdatePreviousDirection();
    vectorDirection.x = (hSpeed > 0.0f) ? 1 : (hSpeed < 0.0f) ? -1 : 0;
    vectorDirection.y = -1;
    vInitialFallSpeed = 0.0f;
    hInitialFallSpeed = hSpeed;
    vInitialFallPosition = position.GetRealY();
    hInitialFallPosition = position.GetRealX();
    cout << "Initial FALL X: " << hInitialFallPosition << " Initial FALL Y: " << vInitialFallPosition << endl;
    tFall = 0.0f;
    isJumping = false;
    isFalling = true;
    isLeaningOnTheGround = false;
}

void MainCharacter::RightKeyPressed() {
    cout << "MainCharacter::RightKeyPressed()" << endl;
    BEGIN_TRANSITION_MAP                    // - Current State -
            TRANSITION_MAP_ENTRY (STATE_RUN_RIGHT) // STATE_Idle_Right
            TRANSITION_MAP_ENTRY (STATE_RUN_RIGHT)  // STATE_Idle_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Idle_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Idle_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Idle_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Idle_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Jump_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Jump_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Hit_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Hit_Left
    END_TRANSITION_MAP(nullptr)
}

void MainCharacter::RightKeyReleased() {
    cout << "MainCharacter::RightKeyReleased()" << endl;
    BEGIN_TRANSITION_MAP                    // - Current State -
            TRANSITION_MAP_ENTRY (EVENT_IGNORED) // STATE_Idle_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Idle_Left
            TRANSITION_MAP_ENTRY (STATE_IDLE_RIGHT)    // STATE_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Idle_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Idle_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Idle_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Idle_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Jump_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Jump_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Hit_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Hit_Left
    END_TRANSITION_MAP(nullptr)
}

void MainCharacter::LeftKeyPressed() {
    cout << "MainCharacter::LeftKeyPressed()" << endl;
    BEGIN_TRANSITION_MAP                                  // - Current State -
            TRANSITION_MAP_ENTRY (STATE_RUN_LEFT)                 // STATE_Idle_Right
            TRANSITION_MAP_ENTRY (STATE_RUN_LEFT)                 // STATE_Idle_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)                  // STATE_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)                  // STATE_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Idle_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Idle_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Idle_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Idle_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Jump_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Jump_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Hit_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Hit_Left
    END_TRANSITION_MAP(nullptr)
}

void MainCharacter::LeftKeyReleased() {
    cout << "MainCharacter::LeftKeyReleased()" << endl;
    BEGIN_TRANSITION_MAP                    // - Current State -
            TRANSITION_MAP_ENTRY (EVENT_IGNORED) // STATE_Idle_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Idle_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Run_Right
            TRANSITION_MAP_ENTRY (STATE_IDLE_LEFT)    // STATE_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Idle_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Idle_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Idle_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Idle_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Jump_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Jump_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Hit_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Hit_Left
    END_TRANSITION_MAP(nullptr)
}

void MainCharacter::UpKeyPressed() {
    cout << "MainCharacter::UpKeyPressed()" << endl;
    BEGIN_TRANSITION_MAP                                  // - Current State -
            TRANSITION_MAP_ENTRY (STATE_JUMP_IDLE_RIGHT)          // STATE_Idle_Right
            TRANSITION_MAP_ENTRY (STATE_JUMP_IDLE_LEFT)                 // STATE_Idle_Left
            TRANSITION_MAP_ENTRY (STATE_JUMP_RUN_RIGHT)                  // STATE_Run_Right
            TRANSITION_MAP_ENTRY (STATE_JUMP_RUN_LEFT)                  // STATE_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Idle_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Idle_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Idle_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Idle_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Jump_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Jump_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Hit_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Hit_Left
    END_TRANSITION_MAP(nullptr)
}

void MainCharacter::DownKeyPressed() {
    cout << "MainCharacter::DownKeyPressed()" << endl;
    BEGIN_TRANSITION_MAP                                  // - Current State -
            TRANSITION_MAP_ENTRY (STATE_FALL_IDLE_RIGHT)          // STATE_Idle_Right
            TRANSITION_MAP_ENTRY (STATE_FALL_IDLE_LEFT)                 // STATE_Idle_Left
            TRANSITION_MAP_ENTRY (STATE_FALL_RUN_RIGHT)                  // STATE_Run_Right
            TRANSITION_MAP_ENTRY (STATE_FALL_RUN_LEFT)                  // STATE_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Idle_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Idle_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Idle_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Idle_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Jump_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Jump_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Hit_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Hit_Left
    END_TRANSITION_MAP(nullptr)
}

void MainCharacter::TopCollisionDuringJump() {
    cout << "MainCharacter::TopCollisionDuringJump()" << endl;
    BEGIN_TRANSITION_MAP                                  // - Current State -
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)          // STATE_Idle_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)                 // STATE_Idle_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)                  // STATE_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)                  // STATE_Run_Left
            TRANSITION_MAP_ENTRY (STATE_FALL_IDLE_RIGHT)    // STATE_Jump_Idle_Right
            TRANSITION_MAP_ENTRY (STATE_FALL_IDLE_LEFT)    // STATE_Jump_Idle_Left
            TRANSITION_MAP_ENTRY (STATE_FALL_RUN_RIGHT)    // STATE_Jump_Run_Right
            TRANSITION_MAP_ENTRY (STATE_FALL_RUN_LEFT)    // STATE_Jump_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Idle_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Idle_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Jump_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Jump_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Hit_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Hit_Left
    END_TRANSITION_MAP(nullptr)
}

void MainCharacter::SpaceKeyPressed() {
    cout << "MainCharacter::UpKeyPressed()" << endl;
    BEGIN_TRANSITION_MAP                                  // - Current State -
            TRANSITION_MAP_ENTRY (STATE_HIT_RIGHT)          // STATE_Idle_Right
            TRANSITION_MAP_ENTRY (STATE_HIT_LEFT)                 // STATE_Idle_Left
            TRANSITION_MAP_ENTRY (STATE_HIT_RIGHT)                  // STATE_Run_Right
            TRANSITION_MAP_ENTRY (STATE_HIT_LEFT)                  // STATE_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Idle_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Idle_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Idle_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Idle_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Jump_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Jump_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Hit_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Hit_Left
    END_TRANSITION_MAP(nullptr)
}

void MainCharacter::JumpLanding() {
    cout << "MainCharacter::JumpLanding()" << endl;
    BEGIN_TRANSITION_MAP                                  // - Current State -
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)          // STATE_Idle_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)                 // STATE_Idle_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)                  // STATE_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)                  // STATE_Run_Left
            TRANSITION_MAP_ENTRY (STATE_IDLE_RIGHT)    // STATE_Jump_Idle_Right
            TRANSITION_MAP_ENTRY (STATE_IDLE_LEFT)    // STATE_Jump_Idle_Left
            TRANSITION_MAP_ENTRY (STATE_IDLE_RIGHT)    // STATE_Jump_Run_Right
            TRANSITION_MAP_ENTRY (STATE_IDLE_LEFT)    // STATE_Jump_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Idle_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Idle_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Jump_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Jump_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Hit_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Hit_Left
    END_TRANSITION_MAP(nullptr)
}

void MainCharacter::FallLanding() {
    cout << "MainCharacter::FallLanding()" << endl;
    BEGIN_TRANSITION_MAP                                  // - Current State -
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)          // STATE_Idle_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)                 // STATE_Idle_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)                  // STATE_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)                  // STATE_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Idle_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Idle_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Jump_Run_Left
            TRANSITION_MAP_ENTRY (STATE_IDLE_RIGHT)    // STATE_Fall_Idle_Right
            TRANSITION_MAP_ENTRY (STATE_IDLE_LEFT)    // STATE_Fall_Idle_Left
            TRANSITION_MAP_ENTRY (STATE_IDLE_RIGHT)    // STATE_Fall_Run_Right
            TRANSITION_MAP_ENTRY (STATE_IDLE_LEFT)    // STATE_Fall_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Jump_Run_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Fall_Jump_Run_Left
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Hit_Right
            TRANSITION_MAP_ENTRY (EVENT_IGNORED)    // STATE_Hit_Left
    END_TRANSITION_MAP(nullptr)
}

void MainCharacter::STATE_Idle_Right() {
    cout << "MainCharacter::STATE_Idle_Right" << endl;
    hMomentum = 0;
    UpdatePreviousDirection();
    vectorDirection.x = 0;
    vectorDirection.y = 0;
    LoadAnimationWithId(MainCharacterAnimation::STAND_BY_RIGHT);
    ProcessPressedKeys(false);
}

void MainCharacter::STATE_Idle_Left() {
    cout << "MainCharacter::STATE_Idle_Left" << endl;
    hMomentum = 0;
    UpdatePreviousDirection();
    vectorDirection.x = 0;
    vectorDirection.y = 0;
    LoadAnimationWithId(MainCharacterAnimation::STAND_BY_LEFT);
    cout << "ProcessPressedKeys(false)" << endl;
    ProcessPressedKeys(false);
}

void MainCharacter::STATE_Run_Right() {
    cout << "MainCharacter::STATE_Run_Right" << endl;
    UpdatePreviousDirection();
    vectorDirection.x = 1;
    vectorDirection.y = 0;
    LoadAnimationWithId(MainCharacterAnimation::RUN_TO_RIGHT);
    ProcessPressedKeys(false);
}

void MainCharacter::STATE_Run_Left() {
    cout << "MainCharacter::STATE_Run_Left" << endl;
    UpdatePreviousDirection();
    vectorDirection.x = -1;
    vectorDirection.y = 0;
    LoadAnimationWithId(MainCharacterAnimation::RUN_TO_LEFT);
    ProcessPressedKeys(false);
}

void MainCharacter::STATE_Jump_Idle_Right() {
    Jump(45.0f, 0.0f);
    LoadAnimationWithId(MainCharacterAnimation::JUMP_RIGHT);
    ProcessPressedKeys(false);
}

void MainCharacter::STATE_Jump_Idle_Left() {
    Jump(45.0f, 0.0f);
    LoadAnimationWithId(MainCharacterAnimation::JUMP_LEFT);
    ProcessPressedKeys(false);
}

void MainCharacter::STATE_Jump_Run_Right() {
    // More momentum produces a longer jump
    Jump(45.0f, hMomentum == maxMomentum ? 10.0f : 4.0f);
    LoadAnimationWithId(MainCharacterAnimation::JUMP_RIGHT);
    ProcessPressedKeys(false);
}

void MainCharacter::STATE_Jump_Run_Left() {
    // More momentum produces a longer jump
    Jump(45.0f, hMomentum == maxMomentum ? -10.0f : -4.0f);
    LoadAnimationWithId(MainCharacterAnimation::JUMP_LEFT);
    ProcessPressedKeys(false);
}

void MainCharacter::STATE_Fall_Idle_Right() {
    cout << "MainCharacter::STATE_Fall_Idle_Right" << endl;
    Fall(0.0f);
    LoadAnimationWithId(MainCharacterAnimation::FALL_RIGHT);
    ProcessPressedKeys(false);
}

void MainCharacter::STATE_Fall_Idle_Left() {
    cout << "MainCharacter::STATE_Fall_Idle_Left" << endl;
    Fall(0.0f);
    LoadAnimationWithId(MainCharacterAnimation::FALL_LEFT);
    ProcessPressedKeys(false);
}

void MainCharacter::STATE_Fall_Run_Right() {
    cout << "MainCharacter::STATE_Fall_Run_Right" << endl;
    Fall(hMomentum == maxMomentum ? 10.0f : 4.0f);
    LoadAnimationWithId(MainCharacterAnimation::FALL_RIGHT);
    ProcessPressedKeys(false);
}

void MainCharacter::STATE_Fall_Run_Left() {
    cout << "MainCharacter::STATE_Fall_Run_Left" << endl;
    Fall(hMomentum == maxMomentum ? -10.0f : -4.0f);
    LoadAnimationWithId(MainCharacterAnimation::FALL_LEFT);
    ProcessPressedKeys(false);
}

void MainCharacter::STATE_Fall_Jump_Run_Right() {
    cout << "MainCharacter::STATE_Fall_Jump_Run_Right" << endl;
    LoadAnimationWithId(MainCharacterAnimation::FALL_RIGHT);
    ProcessPressedKeys(false);
}

void MainCharacter::STATE_Fall_Jump_Run_Left() {
    cout << "MainCharacter::STATE_Fall_Jump_Run_Left" << endl;
    LoadAnimationWithId(MainCharacterAnimation::FALL_LEFT);
    ProcessPressedKeys(false);
}

void MainCharacter::STATE_Hit_Right() {
    cout << "MainCharacter::STATE_Hit_Right" << endl;
    LoadAnimationWithId(MainCharacterAnimation::HIT_RIGHT);
    ProcessPressedKeys(false);
}

void MainCharacter::STATE_Hit_Left() {
    cout << "MainCharacter::STATE_Hit_Left" << endl;
    LoadAnimationWithId(MainCharacterAnimation::HIT_LEFT);
    ProcessPressedKeys(false);
}
