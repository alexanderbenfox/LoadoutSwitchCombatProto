#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "CombatResolverComponent.generated.h"

class UCombatAbilityDefinition;
class UCombatContextComponent;
class UCombatLoadoutComponent;
class UCombatResolverRules;

/** What one attack press resolves to: the slot that won and the ability in it, taken from a single context snapshot */
USTRUCT(BlueprintType)
struct FCombatResolution
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Resolution")
	FGameplayTag Slot;

	/** Null when there is no loadout to resolve against */
	UPROPERTY(BlueprintReadOnly, Category = "Resolution")
	TObjectPtr<UCombatAbilityDefinition> Ability;
};

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

	/**
	 *  What an attack press would resolve to right now. The one place slot and ability get decided:
	 *  the highest-priority matching rule whose slot the active loadout has filled, else the fallback slot.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Resolver")
	FCombatResolution Resolve() const;

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

	/** Marks the target with the ability the next attack press would use */
	UPROPERTY(EditAnywhere, Category = "Resolver|Reticle")
	bool bShowReticle = true;

	/** How far above the target's origin the reticle sits */
	UPROPERTY(EditAnywhere, Category = "Resolver|Reticle", meta = (Units = "cm"))
	float ReticleHeight = 120.0f;

private:

	/** Draws the reticle on Target for the predicted ability; Ability can be null */
	void DrawReticle(const AActor* Target, const UCombatAbilityDefinition* Ability) const;

	/** The owner's context component, found at BeginPlay */
	UPROPERTY(Transient)
	TObjectPtr<UCombatContextComponent> ContextComponent;

	/** The owner's loadout component, found at BeginPlay */
	UPROPERTY(Transient)
	TObjectPtr<UCombatLoadoutComponent> LoadoutComponent;
};
