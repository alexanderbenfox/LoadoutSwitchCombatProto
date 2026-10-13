#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "CombatResolverComponent.generated.h"

class UCombatAbilityDefinition;
class UCombatContextComponent;
class UCombatLoadoutComponent;
class UCombatResolverRules;

/**
 *  Turns the current context into a slot, using the priority order in a rules asset,
 *  then into the ability the active loadout has in that slot.
 *  Later this is where the buffered attack input lands and the ability gets activated.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class UCombatResolverComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UCombatResolverComponent();

	/** The slot an attack press would resolve to right now */
	UFUNCTION(BlueprintCallable, Category = "Combat|Resolver")
	FGameplayTag ResolveSlot() const;

	/** The ability an attack press would use right now, or null if there is no loadout */
	UFUNCTION(BlueprintCallable, Category = "Combat|Resolver")
	UCombatAbilityDefinition* ResolveAbility(FGameplayTag slot) const;

	/** Called by the character when the attack button goes down */
	void HandleAttackPressed();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	virtual void BeginPlay() override;

	/** Priority order to resolve with */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resolver")
	TObjectPtr<UCombatResolverRules> Rules;

	/** Prints the slot and ability the next attack press would resolve to, and a message on each press */
	UPROPERTY(EditAnywhere, Category = "Resolver|Debug")
	bool bShowDebug = true;

private:

	/** The owner's context component, found at BeginPlay */
	UPROPERTY(Transient)
	TObjectPtr<UCombatContextComponent> ContextComponent;

	/** The owner's loadout component, found at BeginPlay */
	UPROPERTY(Transient)
	TObjectPtr<UCombatLoadoutComponent> LoadoutComponent;
};
