#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CombatAbilityDefinition.generated.h"

/**
 *  One ability a loadout can slot.
 *  The ability class, montage, warp settings, damage and stagger get added here once the base attack ability exists.
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

	/** Colour of the on-screen message when this ability is used, so each one reads differently before there are animations */
	UPROPERTY(EditAnywhere, Category = "Ability|Debug")
	FColor DebugColor = FColor::White;
};
