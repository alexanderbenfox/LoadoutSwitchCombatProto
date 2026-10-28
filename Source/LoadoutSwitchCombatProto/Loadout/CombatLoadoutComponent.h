#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatLoadoutComponent.generated.h"

class UCombatLoadout;

/**
 *  The loadouts a character has equipped and which one is active, and the combo counter.
 *  The counter lives here rather than in an ability so it carries over a swap. Swapping and off-hand charge get added here later.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class UCombatLoadoutComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UCombatLoadoutComponent();

	/** The loadout attacks currently resolve against, or null if none is equipped */
	UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
	UCombatLoadout* GetActiveLoadout() const;

	/** How many attacks a string has in that stance, the finisher included, from the active loadout; 0 = no finisher, or no loadout */
	UFUNCTION(BlueprintPure, Category = "Combat|Combo")
	int32 GetComboLength(bool bAir) const;

	/**
	 *  Which attack of the string the next one in that stance would be, 1 for the first. A string carries on until an attack ends with
	 *  no follow-up (ResetCombo), its finisher has been used, or the stance changes: an air attack after ground ones starts an air string
	 */
	UFUNCTION(BlueprintPure, Category = "Combat|Combo")
	int32 GetNextComboIndex(bool bAir) const;

	/** True if the next attack in that stance is the string's last: the context's Context.Combo.Finisher */
	UFUNCTION(BlueprintPure, Category = "Combat|Combo")
	bool IsNextAttackFinisher(bool bAir) const;

	/** Counts an attack that just started in that stance into the string, as GetNextComboIndex said it would be. Returns its index */
	int32 AdvanceCombo(bool bAir);

	/** Ends the string; the next attack is its first */
	void ResetCombo();

	/** How many attacks the current string has had; 0 with no string going */
	UFUNCTION(BlueprintPure, Category = "Combat|Combo")
	int32 GetComboCount() const { return ComboCount; }

	/** Whether the current string is in the air */
	bool IsComboInAir() const { return bComboInAir; }

protected:

	/** Equipped loadouts. Players start with two slots and unlock a third. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loadout")
	TArray<TObjectPtr<UCombatLoadout>> Loadouts;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loadout", meta = (ClampMin = 0))
	int32 ActiveIndex = 0;

private:

	int32 ComboCount = 0;
	bool bComboInAir = false;
};
