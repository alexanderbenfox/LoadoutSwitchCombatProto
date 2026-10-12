#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "CombatDummy.generated.h"

class UAbilitySystemComponent;

/**
 *  A training dummy: an enemy that stands still, can be targeted, and can be put in the air.
 *  Abstract; the mesh and anim blueprint are assigned in a Blueprint child.
 */
UCLASS(abstract)
class ACombatDummy : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

	/** Owns this enemy's attributes and state tags */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

protected:

	/** Which attacks can stagger this enemy */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (Categories = "Enemy.Poise"))
	FGameplayTag PoiseTier;

	/** Upward speed used by DebugLaunch */
	UPROPERTY(EditAnywhere, Category = "Enemy|Debug", meta = (ClampMin = 0, Units = "cm/s"))
	float DebugLaunchSpeed = 900.0f;

public:

	ACombatDummy();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }

	/** Pops the dummy into the air so airborne contexts can be tested. Shows as a button in the Details panel. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Enemy|Debug")
	void DebugLaunch();

protected:

	virtual void BeginPlay() override;
};
