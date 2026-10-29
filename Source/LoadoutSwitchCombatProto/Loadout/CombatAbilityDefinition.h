#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/CombatHitSettings.h"
#include "CombatAbilityDefinition.generated.h"

class UAnimMontage;
class UCurveFloat;

/**
 *  One ability a loadout can slot.
 *  The ability class and warp settings get added here once the base attack ability exists.
 */
UCLASS(BlueprintType)
class UCombatAbilityDefinition : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FText DisplayName;

	/** What slotting this ability costs out of the loadout's AP budget */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability", meta = (ClampMin = 0))
	int32 APCost = 1;

	/** What the character plays when this ability is used */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Animation")
	TObjectPtr<UAnimMontage> Montage;

	/**
	 *  Furthest this ability can carry the character towards its target; 0 means it doesn't lunge.
	 *  The lunge stops LungeStopDistance short of the target, so a target is out of reach (a miss) beyond LungeReach + LungeStopDistance.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Lunge", meta = (ClampMin = 0, Units = "cm"))
	float LungeReach = 0.0f;

	/** How far short of the target the lunge stops, so the character ends up in striking distance rather than inside it */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Lunge", meta = (ClampMin = 0, Units = "cm"))
	float LungeStopDistance = 100.0f;

	/**
	 *  In the air, the lunge also moves to the target's height, homing in on it wherever it is. Off, it moves across only and keeps
	 *  the character's height, like on the ground: forward momentum without being pulled up or down to the target
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Lunge")
	bool bLungeMatchesHeight = false;

	/** With no target, the lunge steps this far straight ahead instead, so the attack still carries forward; 0 = stays put */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Lunge", meta = (ClampMin = 0, Units = "cm"))
	float LungeStepWithoutTarget = 0.0f;

	// when the lunge happens and how long it takes come from a Combat Lunge notify window on the montage

	/** How high this ability carries the character, e.g. a riser going up with the enemy it launches; 0 means it doesn't rise */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Rise", meta = (ClampMin = 0, Units = "cm"))
	float RiseHeight = 0.0f;

	/**
	 *  How far the rise can also drift across towards the target, so the character stays in front of what it launched;
	 *  0 means straight up. Stops Lunge Stop Distance short of the target, like the lunge
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Rise", meta = (ClampMin = 0, Units = "cm"))
	float RiseReach = 0.0f;

	/**
	 *  Optional shape of the rise: how far along it is (0 to 1, Y) at each point of the Combat Rise window (0 to 1, X).
	 *  Empty uses a jump-like ease-out: fast off the ground, slowing to a stop at the top, the same shape as a launched target's arc.
	 *  End with a flat slope for a smooth hand-over to gravity
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Rise")
	TObjectPtr<UCurveFloat> RiseCurve;

	// when the rise happens and how long it takes come from a Combat Rise notify window on the montage

	// how an attack hangs or glides in the air comes from Combat Stall and Combat Glide notify windows on the montage;
	// a Glide stops Lunge Stop Distance short of the target

	/**
	 *  A montage section that replaces the first one when a string starts with the target at least Rising Opener Min Height
	 *  above the character, e.g. a rising slash instead of the usual downward opener of an air combo (KH). It chains into the
	 *  second section the way the first does, and the sections before it never chain into it, so put it after them.
	 *  None = always the first section
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Combo")
	FName RisingOpenerSection;

	/** How far above the character (origin to origin) the target has to be for the Rising Opener Section; level or below gets the usual opener */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Combo", meta = (ClampMin = 0, Units = "cm"))
	float RisingOpenerMinHeight = 60.0f;

	/** What each of this ability's hits does to its target. A Hit Window can override any of these for its own hit */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Hit", meta = (ShowOnlyInnerProperties))
	FCombatHitSettings Hit;

	// when the weapon can hit comes from Hit Window notifies on the montage

	/** A jump press cancels this attack at any point. Off, jumping is blocked until the attack ends */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Cancels")
	bool bJumpCancelable = false;

	/** Colour of the on-screen message when this ability is used, so each one reads differently before there are animations */
	UPROPERTY(EditAnywhere, Category = "Ability|Debug")
	FColor DebugColor = FColor::White;
};
