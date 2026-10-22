#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "CombatDummy.generated.h"

class UAbilitySystemComponent;
class UAnimMontage;
class UCombatAbilityDefinition;

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

	/** Played when a hit lands on the dummy while it's on the ground and doesn't launch it */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Reactions")
	TObjectPtr<UAnimMontage> FlinchMontage;

	/**
	 *  Played when a hit lands on the dummy in the air, or launches it. Stopped on landing.
	 *  Leave empty to play nothing, leaving the anim blueprint's launched state to show
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Reactions")
	TObjectPtr<UAnimMontage> AirFlinchMontage;

	/** How long the air flinch takes to fade out when the dummy lands */
	UPROPERTY(EditAnywhere, Category = "Enemy|Reactions", meta = (ClampMin = 0, Units = "s"))
	float AirFlinchLandBlendOut = 0.15f;

	/** True from being launched until landing. The anim blueprint reads this to pick its launched / tumble state over the plain fall */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy|Reactions")
	bool bLaunched = false;

	/** Upward speed used by DebugLaunch */
	UPROPERTY(EditAnywhere, Category = "Enemy|Debug", meta = (ClampMin = 0, Units = "cm/s"))
	float DebugLaunchSpeed = 900.0f;

public:

	ACombatDummy();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }

	/** Pops the dummy into the air so airborne contexts can be tested. Shows as a button in the Details panel. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Enemy|Debug")
	void DebugLaunch();

	/**
	 *  Reacts to being hit by Attacker using Ability (null if the attack has no ability): launches up by the ability's
	 *  HitLaunchSpeed and pushes along Attacker's facing by its HitKnockbackSpeed, either or both,
	 *  then plays the air flinch if it's launched or already in the air, else the ground flinch.
	 *  Stand-in until damage and stagger come in as Gameplay Effects.
	 */
	void ReceiveHit(AActor* Attacker, const UCombatAbilityDefinition* Ability, const FHitResult& Hit);

	/** True from being launched until landing. For the anim blueprint, which can't read the protected bLaunched directly */
	UFUNCTION(BlueprintPure, Category = "Enemy|Reactions")
	bool IsLaunched() const { return bLaunched; }

protected:

	virtual void BeginPlay() override;

	/** Clears bLaunched and fades out the air flinch */
	virtual void Landed(const FHitResult& Hit) override;

private:

	/**
	 *  Sets the dummy's velocity to UpSpeed straight up and HorizontalVelocity across (Z ignored), overriding only the parts that
	 *  are non-zero. Marks the dummy launched if UpSpeed is above 0
	 */
	void Launch(float UpSpeed, const FVector& HorizontalVelocity);
};
