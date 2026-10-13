#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CombatLoadout.generated.h"

class UCombatAbilityDefinition;

/** A weapon plus the abilities slotted on it. Each loadout has its own AP budget. */
UCLASS(BlueprintType)
class UCombatLoadout : public UDataAsset
{
	GENERATED_BODY()

public:

	/** The ability to use for a slot, or the basic attack if nothing is slotted there */
	UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
	UCombatAbilityDefinition* GetAbilityForSlot(FGameplayTag Slot) const;

	/** Total AP cost of everything slotted */
	UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
	int32 GetSpentAP() const;

	int32 GetAPBudget() const { return APBudget; }

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loadout")
	FText WeaponName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loadout", meta = (ClampMin = 0))
	int32 APBudget = 10;

	/** Used for any slot with nothing in it. Free; not counted against the budget. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loadout")
	TObjectPtr<UCombatAbilityDefinition> BasicAttack;

	/** Which ability sits in which slot */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loadout", meta = (Categories = "Slot"))
	TMap<FGameplayTag, TObjectPtr<UCombatAbilityDefinition>> SlotAbilities;
};
