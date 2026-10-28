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

	UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
	UCombatAbilityDefinition* GetAssignedAbilityForSlot(FGameplayTag Slot) const;

	/** Total AP cost of everything slotted */
	UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
	int32 GetSpentAP() const;

	int32 GetAPBudget() const { return APBudget; }

	/** GroundComboLength or AirComboLength */
	int32 GetComboLength(bool bAir) const { return bAir ? AirComboLength : GroundComboLength; }

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

	/**
	 *  How many attacks a string on the ground has. The last one is the finisher: the context gets Context.Combo.Finisher for it,
	 *  so the Finisher rule replaces whatever it would have been, and the attack after it starts a new string. 0 = no finisher, strings never end
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loadout|Combo", meta = (ClampMin = 0))
	int32 GroundComboLength = 4;

	/** The same for a string in the air */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loadout|Combo", meta = (ClampMin = 0))
	int32 AirComboLength = 3;
};
