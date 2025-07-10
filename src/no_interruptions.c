#include "roll_momentum.h"

#include "overlays/actors/ovl_En_Bom/z_en_bom.h"

RECOMP_HOOK("Player_ActionHandler_Talk") s32 Player_ActionHandler_Talk_Hook(Player* this, PlayState* play) {
    if (mProcessingRollAction && ENABLE_NO_INTERRUPTIONS) {
        mTalkActor = this->talkActor;
        this->talkActor = NULL;
    }
}

RECOMP_HOOK_RETURN("Player_ActionHandler_Talk") s32 Player_ActionHandler_Talk_Return() {
    if (mTalkActor) {
        mThis->talkActor = mTalkActor;
        mTalkActor = NULL;
    }
}

RECOMP_HOOK("Player_ActionHandler_2") s32 Player_ActionHandler_2_Hook(Player* this, PlayState* play) {
    if (mProcessingRollAction && ENABLE_NO_INTERRUPTIONS) { 
        if (gSaveContext.save.saveInfo.playerData.health != 0 && this->interactRangeActor != NULL) {
            if (this->csAction == PLAYER_CSACTION_NONE && (this->getItemId == GI_NONE)) {
                if (!(this->stateFlags1 & PLAYER_STATE1_8000000) && (this->transformation != PLAYER_FORM_DEKU)) {
                    if ((this->heldActor == NULL) || Player_IsHoldingHookshot(this)) {
                        EnBom* bomb = (EnBom*)this->interactRangeActor;

                        if (((this->transformation != PLAYER_FORM_GORON) && (((bomb->actor.id == ACTOR_EN_BOM) && bomb->isPowderKeg) ||
                            ((this->interactRangeActor->id == ACTOR_EN_ISHI) && (this->interactRangeActor->params & 1)) || (this->interactRangeActor->id == ACTOR_EN_MM)))) {
                            return false;
                        }

                        mThis = this;
                        mInteractRangeActor = this->interactRangeActor;
                        this->interactRangeActor = NULL;
                    }
                }
            }
        }
    }
}

RECOMP_HOOK_RETURN("Player_ActionHandler_2") s32 Player_ActionHandler_2_Return() {
    if (mInteractRangeActor) {
        mThis->interactRangeActor = mInteractRangeActor;
        mInteractRangeActor = NULL;
    }
}