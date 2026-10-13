#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatLoadoutComponent.generated.h"

class UCombatLoadout;

/**
 *  The loadouts a character has equipped and which one is active.
 *  Swapping, the combo counter and off-hand charge get added here later.
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

protected:

	/** Equipped loadouts. Players start with two slots and unlock a third. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loadout")
	TArray<TObjectPtr<UCombatLoadout>> Loadouts;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loadout", meta = (ClampMin = 0))
	int32 ActiveIndex = 0;
};
