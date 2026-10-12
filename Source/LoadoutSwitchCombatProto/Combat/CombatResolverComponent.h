#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "CombatResolverComponent.generated.h"

class UCombatContextComponent;
class UCombatResolverRules;

/**
 *  Turns the current context into a slot, using the priority order in a rules asset.
 *  Later this is where the buffered attack input lands and the slot's ability gets activated.
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

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	virtual void BeginPlay() override;

	/** Priority order to resolve with */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resolver")
	TObjectPtr<UCombatResolverRules> Rules;

	/** Prints the slot the next attack press would resolve to */
	UPROPERTY(EditAnywhere, Category = "Resolver|Debug")
	bool bShowDebug = true;

private:

	/** The owner's context component, found at BeginPlay */
	UPROPERTY(Transient)
	TObjectPtr<UCombatContextComponent> ContextComponent;
};
