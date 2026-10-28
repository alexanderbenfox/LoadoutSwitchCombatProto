#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_CombatRise.generated.h"

/**
 *  Marks the frames an attack carries the character upwards on, e.g. a riser going up with the enemy it launches.
 *  When the window opens, the owner rises by the playing ability's Rise Height over the length of the window, then falls normally.
 *  Holds no logic of its own: it hands the window's length to the owner's resolver.
 */
UCLASS(meta = (DisplayName = "Combat Rise"))
class UAnimNotifyState_CombatRise : public UAnimNotifyState
{
	GENERATED_BODY()

public:

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
