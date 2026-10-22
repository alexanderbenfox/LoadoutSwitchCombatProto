#include "Combat/CombatWeaponComponent.h"

UCombatWeaponComponent::UCombatWeaponComponent()
{
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void UCombatWeaponComponent::GetHitSegment(FVector& OutStart, FVector& OutEnd) const
{
	// hit direction in local space, transform and normalize it
	const FTransform WeaponTransform = GetComponentTransform();
	FVector HitDirectionWorldSpace = WeaponTransform.TransformVectorNoScale(HitDirection);
	HitDirectionWorldSpace.Normalize();

	// project hit start length and total hit length along direction vector
	OutStart = GetComponentLocation() + HitDirectionWorldSpace * HitStart;
	OutEnd = OutStart + HitDirectionWorldSpace * HitLength;
}

int32 UCombatWeaponComponent::GetHitProbeCount(float Radius) const
{
	// spacing of at most one radius means ceil(length / radius) gaps, plus one probe to close the last gap
	const float SafeRadius = FMath::Max(Radius, 1.0f);
	return FMath::Max(2, FMath::CeilToInt(HitLength / SafeRadius) + 1);
}
