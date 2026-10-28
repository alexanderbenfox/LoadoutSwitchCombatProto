#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Combat/CombatHitSettings.h"
#include "AnimNotifyState_HitWindow.generated.h"

/**
 *  Marks when an attack's weapon can hit. While the window is open, the owner's resolver sweeps the weapon's hit segment
 *  from where it was last frame to where it is now, and hits each enemy at most once per window.
 *  A multi-hit attack uses one window per hit. Windows don't overlap: only one is open at a time.
 *  Holds no logic of its own: it tells the owner's resolver when the window opens, ticks and closes.
 */
UCLASS(meta = (DisplayName = "Hit Window"))
class UAnimNotifyState_HitWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:

	/** Multiplies each weapon's Hit Radius during this window. Above 1 makes the attack more forgiving than the weapon really is */
	UPROPERTY(EditAnywhere, Category = "Hit Window", meta = (ClampMin = 0.1))
	float HitRadiusScale = 1.0f;

	/**
	 *  Which of the character's Combat Weapon components hit during this window, by component name as shown in its Components panel
	 *  (e.g. Weapon). Empty means all of them
	 */
	UPROPERTY(EditAnywhere, Category = "Hit Window")
	TArray<FName> Weapons;

	/** This hit's changes to the ability's Hit settings, e.g. a bigger knockback on a combo finisher. Unticked values use the ability's */
	UPROPERTY(EditAnywhere, Category = "Hit Window", meta = (ShowOnlyInnerProperties))
	FCombatHitSettingsOverride HitOverrides;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

private:

	/** The resolver on MeshComp's owner, or null (e.g. in the montage editor's preview) */
	static class UCombatResolverComponent* FindResolver(const USkeletalMeshComponent* MeshComp);
};
