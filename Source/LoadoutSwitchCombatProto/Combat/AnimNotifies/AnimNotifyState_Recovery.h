#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_Recovery.generated.h"

/**
 *  Marks an attack's recovery: the hold and return to stance after the action is over. From the window's start, an attack press
 *  starts the next attack straight away, and jumping or moving stops the attack. Left alone, the recovery plays out.
 *  For attacks with no Combo Window (finishers, launchers); on a combo hit, the recovery starts when its Combo Window ends.
 *  Holds no logic of its own: it tells the owner's resolver when the recovery starts.
 */
UCLASS(meta = (DisplayName = "Recovery"))
class UAnimNotifyState_Recovery : public UAnimNotifyState
{
	GENERATED_BODY()

public:

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
