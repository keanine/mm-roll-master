#include "modding.h"
#include "global.h"
#include "recomputils.h"
#include "recompconfig.h"

#include "z64player.h"

#define ENABLE_DUST_PARTICLES (bool)recomp_get_config_u32("dust_particles")
#define ENABLE_BUNNY_ROLL (bool)recomp_get_config_u32("bunny_roll")
#define ENABLE_ROLL_TURNING (bool)recomp_get_config_u32("roll_turning")

typedef enum PlayerActionInterruptResult {
    /* -1 */ PLAYER_INTERRUPT_NONE = -1,
    /*  0 */ PLAYER_INTERRUPT_NEW_ACTION,
    /*  1 */ PLAYER_INTERRUPT_MOVE
} PlayerActionInterruptResult;

Vec3f D_8085D270 = { 0.0f, 0.04f, 0.0f };
Color_RGBA8 D_8085D26C = { 255, 255, 255, 128 };

extern s32 Player_SetAction(PlayState* play, Player* this, PlayerActionFunc actionFunc, s32 arg3);
extern void Player_Action_13(Player* this, PlayState* play);
extern void Player_Action_26(Player* this, PlayState* play);
extern bool func_8083FE38(Player* this, PlayState* play);
extern s32 func_80840A30(PlayState* play, Player* this, f32* arg2, f32 arg3);
extern s32 Player_ActionHandler_7(Player* this, PlayState* play);
extern void func_8083CB58(Player* this, f32 arg1, s16 arg2);
extern void func_80836A5C(Player* this, PlayState* play);
extern PlayerActionInterruptResult Player_TryActionInterrupt(PlayState* play, Player* this, SkelAnime* skelAnime, f32 frameRange);

extern s16 sControlStickWorldYaw;
extern FloorProperty sPrevFloorProperty;
extern f32 sPlayerYDistToFloor;
extern f32 sControlStickMagnitude;

Player* mThis;
PlayState* mPlay;
f32 mCurrentSpeed;

bool mIsCurrentActionRolling = false;
bool mIsRolling = false;
bool mBunnyHoodActive = false;

RECOMP_HOOK_RETURN("Player_UpdateCommon") void return_Player_UpdateCommon() {
    mBunnyHoodActive = false;
}

RECOMP_HOOK("Player_UpdateBunnyEars") void Player_UpdateBunnyEars(Player* player) {
    if (ENABLE_BUNNY_ROLL) {
        mBunnyHoodActive = true;
    }
}

RECOMP_HOOK("Player_Action_26") void on_Player_Action_Rolling(Player* this, PlayState* play) {
    mThis = this;
    mPlay = play;
    mCurrentSpeed = this->speedXZ;

    if ((mThis->av2.actionVar2 == 0) && !func_80840A30(mPlay, mThis, &mThis->speedXZ, 6.0f) &&
        ((mThis->skelAnime.curFrame < 15.0f) || !Player_ActionHandler_7(mThis, mPlay)) && this->av2.actionVar2 == 0 && this->talkActor == NULL) {
        
        mIsRolling = true;
    } else {
        mIsRolling = false;
    }
}

RECOMP_HOOK_RETURN("Player_Action_26") void return_Player_Action_Rolling() {
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
}

RECOMP_HOOK("func_808369F4") void on_Enter_Idle(Player* this, PlayState* play) {
    mThis = this;
    mPlay = play;
    
    mIsCurrentActionRolling = (this->actionFunc == Player_Action_26);
}

RECOMP_HOOK_RETURN("func_808369F4") void return_Enter_Idle() {
    if (func_8083FE38(mThis, mPlay)) {
        return;
    }
    if (mIsCurrentActionRolling && mThis->speedXZ != 0) {
        Player_SetAction(mPlay, mThis, Player_Action_13, 1);
    }
}

// All of this just to clamp the speed of a jump
RECOMP_HOOK("func_8083827C") void func_8083827C(Player* this, PlayState* play) {
    s32 angle;

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