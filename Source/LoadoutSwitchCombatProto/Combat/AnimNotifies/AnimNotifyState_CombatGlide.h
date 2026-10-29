#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_CombatGlide.generated.h"

/**
 *  Air only: for the window, the character glides towards the target while still falling, on lowered gravity. It's steered like
 *  a spring towards a point the ability's Lunge Stop Distance in front of the target, matching the target's own movement: it picks
 *  up the speed it already has, eases in, follows a target being knocked away, and settles without overshooting, so hit to hit
 *  reads as one continuous arc. Steering is across only; height is left to
 *  gravity. When the window closes the speed carries on and the character's air braking bleeds it off.
 *  With no target it stalls instead (stops the fall and hangs on Gravity Scale). Does nothing on the ground.
 */
UCLASS(meta = (DisplayName = "Combat Glide"))
class UAnimNotifyState_CombatGlide : public UAnimNotifyState
{
	GENERATED_BODY()

public:

	/** Gravity while gliding, as a multiple of normal: low floats towards the target, 1 drops at the normal rate */
	UPROPERTY(EditAnywhere, Category = "Glide", meta = (ClampMin = 0, ClampMax = 1))
	float GravityScale = 0.15f;

	/**
	 *  Stops the fall when the window opens, so the glide starts level and lingers before dropping. Off, a character already
	 *  falling keeps its downward speed into the glide (a rise is stopped either way)
	 */
	UPROPERTY(EditAnywhere, Category = "Glide")
	bool bStopFall = true;

	/** The fastest the glide moves across */
	UPROPERTY(EditAnywhere, Category = "Glide", meta = (ClampMin = 0, Units = "CentimetersPerSecond"))
	float MaxSpeed = 900.0f;

	/**
	 *  How hard the spring pulls towards the target: higher closes the gap sooner and snappier. The glide covers most of the
	 *  distance in about 3 / Stiffness seconds (0.25 s at 12), and keeps pace with a target that's moving, e.g. knocked back
	 */
	UPROPERTY(EditAnywhere, Category = "Glide", meta = (ClampMin = 0.1))
	float Stiffness = 12.0f;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
