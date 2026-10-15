#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "CombatContextComponent.generated.h"

UENUM(BlueprintType)
enum class ECombatRangeBand : uint8
{
	Close,
	Far
};

/**
 *  Reads the fight as a set of Context.* gameplay tags.
 *  The resolver asks for the context on each attack press and uses it to pick a slot.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class UCombatContextComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UCombatContextComponent();

	/** Snapshot of the current context. Sampled on the attack press, never mid-move. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Context")
	FGameplayTagContainer BuildContext() const;

	UFUNCTION(BlueprintCallable, Category = "Combat|Context")
	void SetTarget(AActor* NewTarget);

	UFUNCTION(BlueprintPure, Category = "Combat|Context")
	AActor* GetTarget() const { return Target.Get(); }

	/** Opens the post-dodge window. Called by the dodge once it exists. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Context")
	void NotifyDodged();

	/** Opens the swap-in window. Called by the loadout component once it exists. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Context")
	void NotifySwapped();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	/** Targets nearer than this are Close, the rest are Far. Measured horizontally; height is the stance's job. */
	UPROPERTY(EditAnywhere, Category = "Context|Range", meta = (ClampMin = 0, Units = "cm"))
	float CloseRange = 250.0f;

	/** Width of the dead zone centred on CloseRange: the band flips to Close at CloseRange - half of this and back to Far at CloseRange + half, so it doesn't flicker on the boundary. */
	UPROPERTY(EditAnywhere, Category = "Context|Range", meta = (ClampMin = 0, Units = "cm"))
	float RangeHysteresis = 50.0f;

	/** Enemies within this distance of the target count toward a crowd. Measured horizontally, like the range. */
	UPROPERTY(EditAnywhere, Category = "Context|Crowd", meta = (ClampMin = 0, Units = "cm"))
	float CrowdRadius = 400.0f;

	/** This many enemies in the radius, the target included, makes it a Crowd; fewer is Single */
	UPROPERTY(EditAnywhere, Category = "Context|Crowd", meta = (ClampMin = 1))
	int32 CrowdThreshold = 3;

	UPROPERTY(EditAnywhere, Category = "Context|Windows", meta = (ClampMin = 0, Units = "s"))
	float PostDodgeWindow = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Context|Windows", meta = (ClampMin = 0, Units = "s"))
	float SwapInWindow = 0.5f;

	/** Prints the live context tags and draws the two range-flip rings and the crowd radius around the target */
	UPROPERTY(EditAnywhere, Category = "Context|Debug")
	bool bShowDebug = true;

private:

	/** Band for a target at Distance, given the band it was in last frame */
	ECombatRangeBand ComputeRangeBand(float Distance, ECombatRangeBand Previous, bool UseHysteresis ) const;

	/** Horizontal distance from the owner to the target, or a negative value with no target */
	float GetDistanceToTarget() const;

	/** How many enemies are within CrowdRadius of the target, the target included; 0 with no target */
	int32 CountEnemiesNearTarget() const;

	void DrawDebug() const;

	TWeakObjectPtr<AActor> Target;

	/** Updated every tick so the hysteresis has a "last frame" to compare against */
	ECombatRangeBand RangeBand = ECombatRangeBand::Far;

	/** World time of the last dodge / swap, or a negative value if there hasn't been one */
	double LastDodgeTime = -1.0;
	double LastSwapTime = -1.0;
};
