#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_ComboWindow.generated.h"

/**
 *  Marks when an attack can chain into the next one. Presses from the window's start are kept;
 *  when the window ends, a kept press chains, and a press after that (in the recovery) chains straight away.
 *  The window's start sets how early input is accepted, its end sets the rhythm of the combo.
 *  Holds no logic of its own: it tells the owner's resolver when the window opens and closes.
 */
UCLASS(meta = (DisplayName = "Combo Window"))
class UAnimNotifyState_ComboWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

private:

	/** The resolver on MeshComp's owner, or null (e.g. in the montage editor's preview) */
	static class UCombatResolverComponent* FindResolver(const USkeletalMeshComponent* MeshComp);
};
