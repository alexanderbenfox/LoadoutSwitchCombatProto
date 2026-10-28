#pragma once

#include "CoreMinimal.h"
#include "CombatHitSettings.generated.h"

/**
 *  What one hit does to its target. An ability holds the defaults for all its hits;
 *  a Hit Window can override any of them for its own hit, e.g. a combo finisher that knocks back further.
 *  Stand-in for the Gameplay Effect a hit will apply once damage and stagger move to GAS.
 */
USTRUCT(BlueprintType)
struct FCombatHitSettings
{
	GENERATED_BODY()

	/** Damage dealt to the target */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit", meta = (ClampMin = 0))
	float Damage = 10.0f;

	/** How long the target is stunned (can't act) after the hit. At 60 fps, 0.1 s is 6 frames */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit", meta = (ClampMin = 0, Units = "s"))
	float HitStun = 0.3f;

	/**
	 *  Vertical speed the hit gives the target: above 0 launches it up (a launcher, a juggle re-pop), below 0 slams it down
	 *  (an air combo finisher). 0 leaves its vertical movement alone; hit in the air, it's held up for the hit stun instead
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit", meta = (Units = "cm/s"))
	float LaunchSpeed = 0.0f;

	/**
	 *  Horizontal speed the hit pushes the target with, along the attacker's facing; 0 means no push.
	 *  Braking slows it, on the ground like any slide, in the air by the target's Air Braking Deceleration
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit", meta = (ClampMin = 0, Units = "cm/s"))
	float KnockbackSpeed = 0.0f;

	/**
	 *  How long the attacker and the target freeze when the hit lands, for impact; 0 for none.
	 *  Off by default, to keep combos fluid. Real time, unaffected by the freeze itself. At 60 fps, 0.05 s is 3 frames
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit", meta = (ClampMin = 0, Units = "s"))
	float HitStop = 0.0f;
};

/** Per-hit overrides of an ability's FCombatHitSettings. Each value only applies with its checkbox ticked */
USTRUCT(BlueprintType)
struct FCombatHitSettingsOverride
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Hit", meta = (InlineEditConditionToggle))
	bool bOverrideDamage = false;

	/** Damage dealt to the target */
	UPROPERTY(EditAnywhere, Category = "Hit", meta = (EditCondition = "bOverrideDamage", ClampMin = 0))
	float Damage = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Hit", meta = (InlineEditConditionToggle))
	bool bOverrideHitStun = false;

	/** How long the target is stunned (can't act) after the hit. At 60 fps, 0.1 s is 6 frames */
	UPROPERTY(EditAnywhere, Category = "Hit", meta = (EditCondition = "bOverrideHitStun", ClampMin = 0, Units = "s"))
	float HitStun = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Hit", meta = (InlineEditConditionToggle))
	bool bOverrideLaunchSpeed = false;

	/** Vertical speed the hit gives the target: above 0 launches up, below 0 slams down, 0 leaves it alone */
	UPROPERTY(EditAnywhere, Category = "Hit", meta = (EditCondition = "bOverrideLaunchSpeed", Units = "cm/s"))
	float LaunchSpeed = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Hit", meta = (InlineEditConditionToggle))
	bool bOverrideKnockbackSpeed = false;

	/** Horizontal speed the hit pushes the target with, along the attacker's facing; 0 means no push */
	UPROPERTY(EditAnywhere, Category = "Hit", meta = (EditCondition = "bOverrideKnockbackSpeed", ClampMin = 0, Units = "cm/s"))
	float KnockbackSpeed = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Hit", meta = (InlineEditConditionToggle))
	bool bOverrideHitStop = false;

	/** How long the attacker and the target freeze when the hit lands; 0 for none */
	UPROPERTY(EditAnywhere, Category = "Hit", meta = (EditCondition = "bOverrideHitStop", ClampMin = 0, Units = "s"))
	float HitStop = 0.05f;

	/** Defaults with the ticked values replaced */
	FCombatHitSettings ApplyTo(const FCombatHitSettings& Defaults) const
	{
		FCombatHitSettings Result = Defaults;

		if (bOverrideDamage)
		{
			Result.Damage = Damage;
		}

		if (bOverrideHitStun)
		{
			Result.HitStun = HitStun;
		}

		if (bOverrideLaunchSpeed)
		{
			Result.LaunchSpeed = LaunchSpeed;
		}

		if (bOverrideKnockbackSpeed)
		{
			Result.KnockbackSpeed = KnockbackSpeed;
		}

		if (bOverrideHitStop)
		{
			Result.HitStop = HitStop;
		}

		return Result;
	}
};
