#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CombatAbilityDefinition.generated.h"

class UAnimMontage;

/**
 *  One ability a loadout can slot.
 *  The ability class, warp settings, damage and stagger get added here once the base attack ability exists.
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

	// when the lunge happens and how long it takes come from a Combat Lunge notify window on the montage

	/** Colour of the on-screen message when this ability is used, so each one reads differently before there are animations */
	UPROPERTY(EditAnywhere, Category = "Ability|Debug")
	FColor DebugColor = FColor::White;
};
