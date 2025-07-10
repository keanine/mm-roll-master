#ifndef ROLL_MOMENTUM_H
#define ROLL_MOMENTUM_H

#include "modding.h"
#include "global.h"
#include "recomputils.h"
#include "recompconfig.h"

#define ENABLE_DUST_PARTICLES (bool)recomp_get_config_u32("dust_particles")
#define ENABLE_BUNNY_ROLL (bool)recomp_get_config_u32("bunny_roll")
#define ENABLE_ROLL_TURNING (bool)recomp_get_config_u32("roll_turning")
#define ENABLE_INPUT_BUFFER (bool)recomp_get_config_u32("input_buffer")
#define ENABLE_LOUD_LINK (bool)recomp_get_config_u32("loud_link")
#define ENABLE_NO_INTERRUPTIONS (bool)recomp_get_config_u32("no_interruptions")

#define INPUT_BUFFER_LENGTH 3

typedef enum PlayerActionInterruptResult {
    /* -1 */ PLAYER_INTERRUPT_NONE = -1,
    /*  0 */ PLAYER_INTERRUPT_NEW_ACTION,
    /*  1 */ PLAYER_INTERRUPT_MOVE
} PlayerActionInterruptResult;

typedef struct AnimSfxEntry {
    /* 0x0 */ u16 sfxId;
    /* 0x2 */ s16 flags; // negative marks the end
} AnimSfxEntry;          // size = 0x4

typedef enum ActionHandlerIndex {
    /* 0x0 */ PLAYER_ACTION_HANDLER_0,
    /* 0x1 */ PLAYER_ACTION_HANDLER_1,
    /* 0x2 */ PLAYER_ACTION_HANDLER_2,
    /* 0x3 */ PLAYER_ACTION_HANDLER_3,
    /* 0x4 */ PLAYER_ACTION_HANDLER_TALK,
    /* 0x5 */ PLAYER_ACTION_HANDLER_5,
    /* 0x6 */ PLAYER_ACTION_HANDLER_6,
    /* 0x7 */ PLAYER_ACTION_HANDLER_7,
    /* 0x8 */ PLAYER_ACTION_HANDLER_8,
    /* 0x9 */ PLAYER_ACTION_HANDLER_9,
    /* 0xA */ PLAYER_ACTION_HANDLER_10,
    /* 0xB */ PLAYER_ACTION_HANDLER_11,
    /* 0xC */ PLAYER_ACTION_HANDLER_12,
    /* 0xD */ PLAYER_ACTION_HANDLER_13,
    /* 0xE */ PLAYER_ACTION_HANDLER_14,
    /* 0xF */ PLAYER_ACTION_HANDLER_MAX
} ActionHandlerIndex;

extern Vec3f D_8085D270;
extern Color_RGBA8 D_8085D26C;

extern s32 Player_SetAction(PlayState* play, Player* this, PlayerActionFunc actionFunc, s32 arg3);
extern void Player_Action_13(Player* this, PlayState* play);
extern void Player_Action_26(Player* this, PlayState* play);
extern bool func_8083FE38(Player* this, PlayState* play);
extern s32 func_80840A30(PlayState* play, Player* this, f32* arg2, f32 arg3);
extern s32 Player_ActionHandler_7(Player* this, PlayState* play);
extern void func_8083CB58(Player* this, f32 arg1, s16 arg2);
extern void func_80836A5C(Player* this, PlayState* play);
extern bool func_8082DA90(PlayState* play);
extern PlayerActionInterruptResult Player_TryActionInterrupt(PlayState* play, Player* this, SkelAnime* skelAnime, f32 frameRange);

extern PlayerAnimationHeader* D_8085BE84[PLAYER_ANIMGROUP_MAX][PLAYER_ANIMTYPE_MAX];
extern AnimSfxEntry D_8085D61C[];
extern s16 sControlStickWorldYaw;
extern FloorProperty sPrevFloorProperty;
extern f32 sPlayerYDistToFloor;
extern f32 sControlStickMagnitude;
extern s32 sWorldYawToTouchedWall;
extern s8 sActionHandlerListIdle[];
extern Input* sPlayerControlInput;

extern Player* mThis;
extern PlayState* mPlay;
extern f32 mCurrentSpeed;

extern bool mProcessingRollAction;
extern bool mIsCurrentActionRolling;
extern bool mIsRolling;
extern bool mBunnyHoodActive;
extern bool mTriggeredDuringBuffer;

extern Actor* mTalkActor;
extern Actor* mInteractRangeActor;
extern PlayerActionFunc mSavedActionFunc;

extern u16 mSavedSfxId;
extern s8* sSavedActionHandlerListIdle;


#endif // ROLL_MOMENTUM_H