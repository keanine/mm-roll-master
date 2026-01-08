#include "roll_momentum.h"

#include "z64player.h"

Vec3f dustAccel = { 0.0f, 0.04f, 0.0f };
Color_RGBA8 dustColorPrim = { 100, 90, 80, 64 };
Color_RGBA8 dustColorEnv = { 100, 90, 80, 32 };

Player* mThis;
PlayState* mPlay;
f32 mCurrentSpeed;

bool mProcessingRollAction;
bool mIsCurrentActionRolling;
bool mIsRolling;
bool mBunnyHoodActive;
bool mTriggeredDuringBuffer;
bool mBbhAvailable;

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

RECOMP_CALLBACK("*", recomp_on_init) void on_init() {
    mBbhAvailable = recomp_is_dependency_met("mm_recomp_better_bunny") == DEPENDENCY_STATUS_FOUND;
    // recomp_printf("recomp_is_dependency_met: %d\n", recomp_is_dependency_met("mm_recomp_better_bunny"));
}

RECOMP_HOOK("Player_Action_26") void Player_Action_Rolling_Hook(Player* this, PlayState* play) {
    mThis = this;
    mPlay = play;
    mCurrentSpeed = this->speedXZ;
    mProcessingRollAction = true;

    if (ENABLE_BUNNY_ROLL) {
        if (this->currentMask == PLAYER_MASK_BUNNY) {
                mBunnyHoodActive = true;
        }
        else if (mBbhAvailable) {
            // recomp_printf("IsBBHModeEnabled: %d\n", IsBBHModeEnabled());
            if (IsBBHModeEnabled()) {
                mBunnyHoodActive = true; 
            }
        }
    }

    if (ENABLE_QUIET_LINK) {
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
    if (ENABLE_QUIET_LINK) {
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
    }
    mProcessingRollAction = false;
    mBunnyHoodActive = false;
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

RECOMP_HOOK("func_8083FBC4") s32 func_8083FBC4_Hook(PlayState* play, Player* this) {
    if ((this->floorSfxOffset == NA_SE_PL_WALK_GROUND - SFX_FLAG) ||
        (this->floorSfxOffset == NA_SE_PL_WALK_SAND - SFX_FLAG) ||
        (this->floorSfxOffset == NA_SE_PL_WALK_SNOW - SFX_FLAG)) {
        return false;
    }

    if (true) { // if ((this->floorSfxOffset == NA_SE_PL_WALK_GRASS - SFX_FLAG)) {
        if (ENABLE_DUST_PARTICLES) {
            if (this->skelAnime.curFrame > 6.0f && this->skelAnime.curFrame < 16.0f) {
                s32 i;

                Vec3f velocity = gZeroVec3f;
                Math3D_Vec3f_Cross(&this->actor.velocity, &(Vec3f){ 0, 1, 0}, &velocity);
                Math_Vec3f_Scale(&velocity, 0.2f);
                Math_Vec3f_Scale(&velocity, Rand_ZeroFloat(2) - 1);
                velocity.y += Rand_ZeroFloat(2);

                for (i = 0; i < ARRAY_COUNT(this->actor.shape.feetPos); i++) {
                    EffectSsDust_Spawn(play, 0, &this->actor.world.pos, &velocity, &dustAccel, 
                        &dustColorPrim, &dustColorEnv, 50, 30, 10, 0);
                }
                return true;
            }
        }
    }
    return false;
}