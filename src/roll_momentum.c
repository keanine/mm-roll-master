#include "roll_momentum.h"

#include "z64player.h"

Vec3f D_8085D270 = { 0.0f, 0.04f, 0.0f };
Color_RGBA8 D_8085D26C = { 255, 255, 255, 128 };

Player* mThis;
PlayState* mPlay;
f32 mCurrentSpeed;

bool mProcessingRollAction;
bool mIsCurrentActionRolling;
bool mIsRolling;
bool mBunnyHoodActive;
bool mTriggeredDuringBuffer;

Actor* mTalkActor;
Actor* mInteractRangeActor;
PlayerActionFunc mSavedActionFunc;

u16 mSavedSfxId;
s8* sSavedActionHandlerListIdle;

// Check for bonk without actualy changing any variables
s32 Player_HasBonked(PlayState* play, Player* this, f32* arg2, f32 arg3) {
    Actor* cylinderOc = NULL;

    if (arg3 <= *arg2) {
        // If interacting with a wall and close to facing it
        if (((this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) && (sWorldYawToTouchedWall < 0x1C00)) ||
            // or, impacting something's cylinder
            (((this->cylinder.base.ocFlags1 & OC1_HIT) && (cylinderOc = this->cylinder.base.oc) != NULL) &&
             // and that something is a Beaver Race ring,
             ((cylinderOc->id == ACTOR_EN_TWIG) ||
              // or something is a tree and `this` is close to facing it (note the this actor's facing direction would
              // be antiparallel to the cylinder's actor's yaw if this was directly facing it)
              (((cylinderOc->id == ACTOR_EN_WOOD02) || (cylinderOc->id == ACTOR_EN_SNOWWD) ||
                (cylinderOc->id == ACTOR_OBJ_TREE)) &&
               (ABS_ALT(BINANG_SUB(this->actor.world.rot.y, cylinderOc->yawTowardsPlayer)) > 0x6000))))) {

            if (!func_8082DA90(play)) {
                return true;
            }
        }
    }
    return false;
}

RECOMP_HOOK_RETURN("Player_UpdateCommon") void Player_UpdateCommon_Return() {
    mBunnyHoodActive = false;
}

RECOMP_HOOK("Player_UpdateBunnyEars") void Player_UpdateBunnyEars_Hook(Player* player) {
    if (ENABLE_BUNNY_ROLL) {
        mBunnyHoodActive = true;
    }
}

RECOMP_HOOK("Player_Action_26") void Player_Action_Rolling_Hook(Player* this, PlayState* play) {
    mThis = this;
    mPlay = play;
    mCurrentSpeed = this->speedXZ;
    mProcessingRollAction = true;

    if (!ENABLE_LOUD_LINK) {
        mSavedSfxId = D_8085D61C->sfxId;
        D_8085D61C->sfxId = NA_SE_NONE;
    }

    s32 hasBonked = Player_HasBonked(mPlay, mThis, &mThis->speedXZ, 6.0f);
    bool rollAnimActive = this->skelAnime.animation == D_8085BE84[PLAYER_ANIMGROUP_landing_roll][this->modelAnimType];

    if (ENABLE_INPUT_BUFFER && this->skelAnime.curFrame > 15.0f - INPUT_BUFFER_LENGTH
        && !hasBonked && CHECK_BTN_ANY(play->state.input->press.button, BTN_A)) {
        if (rollAnimActive) {
            mTriggeredDuringBuffer = true;
        }
    }
    else if (this->skelAnime.curFrame <= 15.0f) {
        mTriggeredDuringBuffer = false;
    }

    if (this->skelAnime.curFrame > 18.0f && mTriggeredDuringBuffer) {
        if (rollAnimActive) {
            this->skelAnime.curFrame = 0.0f;
        }
        mTriggeredDuringBuffer = false;
    }

    if ((mThis->av2.actionVar2 == 0) && !hasBonked &&
        ((mThis->skelAnime.curFrame < 15.0f) || !Player_ActionHandler_7(mThis, mPlay)) && this->av2.actionVar2 == 0) {
        mIsRolling = true;
    } else {
        mIsRolling = false;
    }
}

RECOMP_HOOK_RETURN("Player_Action_26") void Player_Action_Rolling_Return() {   
    if (!ENABLE_LOUD_LINK) {
        D_8085D61C->sfxId = mSavedSfxId;
    }

    if (mIsRolling) {
        if (mThis->actionFunc != Player_Action_26) {
            mIsRolling = false;
        }
    }
    if (mIsRolling) {


        s16 sTurnStrength = BINANG_SUB(sControlStickWorldYaw, mThis->actor.shape.rot.y) / 10;
        f32 speedTarget = CLAMP(mCurrentSpeed, 0, ((mThis->transformation == PLAYER_FORM_ZORA) ? 6.0f : 5.5f));
        s16 yawTarget = mThis->actor.shape.rot.y;

        speedTarget *= 1.5f;

        if (mBunnyHoodActive) {
            speedTarget *= 1.25f;
        }

        if (sControlStickMagnitude != 0.0f && ENABLE_ROLL_TURNING) {
            yawTarget += sTurnStrength;
        }

        if ((speedTarget < 3.0f) || sControlStickMagnitude == 0.0f ||
            (mThis->controlStickDirections[mThis->controlStickDataIndex] == PLAYER_STICK_DIR_BACKWARD)) {
            return;
        }

        func_8083CB58(mThis, speedTarget, yawTarget);

        if (ENABLE_DUST_PARTICLES) {
            if (mThis->skelAnime.curFrame > 6.0f && mThis->skelAnime.curFrame < 16.0f) {
                EffectSsDust_Spawn(mPlay, 0, &mThis->actor.world.pos, &gZeroVec3f, &D_8085D270, &D_8085D26C, &D_8085D26C, 100, 40, 17, 0);
            }
        }
    }
    mProcessingRollAction = false;
}

RECOMP_HOOK("func_808369F4") void Enter_Idle_Hook(Player* this, PlayState* play) {
    mThis = this;
    mPlay = play;
    
    mIsCurrentActionRolling = (this->actionFunc == Player_Action_26);
}

RECOMP_HOOK_RETURN("func_808369F4") void Enter_Idle_Return() {
    if (func_8083FE38(mThis, mPlay)) {
        return;
    }
    if (mIsCurrentActionRolling && mThis->speedXZ != 0) {
        Player_SetAction(mPlay, mThis, Player_Action_13, 1);
    }
}

RECOMP_HOOK("func_8083827C") void ClampJumpSpeed_Hook(Player* this, PlayState* play) {
    s32 angle;

    // All of this just to clamp the speed of a jump :(
    if (!(this->stateFlags1 & (PLAYER_STATE1_8000000 | PLAYER_STATE1_20000000)) &&
        ((this->stateFlags1 & PLAYER_STATE1_80000000) ||
        !(this->stateFlags3 & (PLAYER_STATE3_200 | PLAYER_STATE3_2000))) &&
        !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
            
            angle = BINANG_SUB(this->yaw, this->actor.shape.rot.y);

            if ((this->transformation != PLAYER_FORM_GORON) &&
                ((this->transformation != PLAYER_FORM_DEKU) || (this->remainingHopsCounter != 0)) &&
                (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND_LEAVE)) {
                
                if (!(this->stateFlags1 & PLAYER_STATE1_8000000)) {
                    if ((sPrevFloorProperty != FLOOR_PROPERTY_6) && (sPrevFloorProperty != FLOOR_PROPERTY_9) &&
                        (sPlayerYDistToFloor > 20.0f) && (this->meleeWeaponState == PLAYER_MELEE_WEAPON_STATE_0)) {
                        
                        if ((ABS_ALT(angle) < 0x2000) && (this->speedXZ > 3.0f)) {
                            if (this->actionFunc == Player_Action_26 && mBunnyHoodActive) {
                                this->speedXZ = CLAMP(this->speedXZ, -100, ((this->transformation == PLAYER_FORM_ZORA ? 6.0f : 5.5f) * 1.5f) - 0.1f);
                            }
                        }
                    }
                }
            }
        }
}