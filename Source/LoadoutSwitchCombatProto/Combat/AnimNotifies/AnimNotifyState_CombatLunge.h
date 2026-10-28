#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_CombatLunge.generated.h"

/**
 *  Marks the frames an attack steps in on. When the window opens, the owner lunges towards its target
 *  for the length of the window, using the playing ability's reach and stop distance.
 *  Holds no logic of its own: it hands the window's length to the owner's resolver.
 */
UCLASS(meta = (DisplayName = "Combat Lunge"))
class UAnimNotifyState_CombatLunge : public UAnimNotifyState
{
	GENERATED_BODY()

public:

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
