#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_CombatStall.generated.h"

/**
 *  Air only: the character hangs in the air for the window. When it opens, upward speed stops (and the fall too, with Stop Fall),
 *  and gravity is Gravity Scale until it closes; after that the character falls normally for the rest of the swing.
 *  E.g. a short window at the start of an air opener: the swing stops the jump, lingers, then drops. Does nothing on the ground.
 */
UCLASS(meta = (DisplayName = "Combat Stall"))
class UAnimNotifyState_CombatStall : public UAnimNotifyState
{
	GENERATED_BODY()

public:

	/** Gravity while the window is open, as a multiple of normal: 0 hangs still, 1 doesn't hang at all */
	UPROPERTY(EditAnywhere, Category = "Stall", meta = (ClampMin = 0, ClampMax = 1))
	float GravityScale = 0.1f;

	/** Also stops a fall when the window opens, not only a rise. Off, a character already falling keeps its downward speed */
	UPROPERTY(EditAnywhere, Category = "Stall")
	bool bStopFall = true;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
