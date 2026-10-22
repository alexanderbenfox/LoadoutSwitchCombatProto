#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "CombatWeaponComponent.generated.h"

/**
 *  A weapon held by a combat character: its mesh, plus the hit segment, the line along the weapon that the
 *  resolver's hit sweep follows while a Hit Window is open (a blade's edge, a hammer's head, a spear's shaft).
 *  Visual only: the mesh has no collision, hits come from the sweep.
 *  Later the mesh and hit segment come from the active loadout's weapon data instead of being set here.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class LOADOUTSWITCHCOMBATPROTO_API UCombatWeaponComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:

	UCombatWeaponComponent();

	/** Where the hit segment is right now, in world space, from OutStart to OutEnd */
	void GetHitSegment(FVector& OutStart, FVector& OutEnd) const;

	/** How thick the weapon is to the hit sweep, before a Hit Window's Hit Radius Scale */
	float GetHitRadius() const { return HitRadius; }

	/**
	 *  How many spheres of Radius to sweep along the hit segment so they leave no gaps: spaced at most one radius apart,
	 *  so neighbours overlap by half. Always at least 2, one at each end.
	 */
	int32 GetHitProbeCount(float Radius) const;

protected:

	/** Which way the hit segment runs from the weapon's origin, in this component's own space */
	UPROPERTY(EditAnywhere, Category = "Weapon|Hit Segment")
	FVector HitDirection = FVector::UpVector;

	/** How far from the weapon's origin, along HitDirection, the hit segment starts. Skips the grip, or makes up for a pivot that isn't at the handle */
	UPROPERTY(EditAnywhere, Category = "Weapon|Hit Segment", meta = (Units = "cm"))
	float HitStart = 0.0f;

	/** How long the hit segment is, from HitStart to its end */
	UPROPERTY(EditAnywhere, Category = "Weapon|Hit Segment", meta = (ClampMin = 0, Units = "cm"))
	float HitLength = 90.0f;

	/** Radius of the spheres swept along the hit segment: how thick the weapon is to hits. A Hit Window can scale it for a more forgiving attack */
	UPROPERTY(EditAnywhere, Category = "Weapon|Hit Segment", meta = (ClampMin = 1, Units = "cm"))
	float HitRadius = 20.0f;
};
