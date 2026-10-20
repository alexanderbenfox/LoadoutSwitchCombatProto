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

	/** Called by a Combat Lunge notify window on the playing montage; lunges for the ability that montage belongs to */
	void HandleLungeWindowBegin(float Duration);

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

	/** Buffer time before ability time has finished to buffer another ability **/
	UPROPERTY(EditAnywhere, Category = "Resolver|Buffer", meta = (Units = "s"))
	float BufferTime = 0.1f;

private:

	/**
	 *  Plays the ability's montage on the owning character. Stand-in for activating a GAS ability,
	 *  which will take over montage playback once the base context-attack ability exists.
	 *  Returns false if nothing was played
	 */
	bool PlayAbilityMontage(const UCombatAbilityDefinition* Ability) const;

	/**
	 *  Turns the owner to face the context target and moves it towards the target with a root motion source,
	 *  using the ability's lunge settings, over Duration seconds. Does nothing if the ability doesn't lunge or there's no target.
	 *  Stand-in for the GAS ApplyRootMotionMoveToActorForce task.
	 */
	void StartLunge(const UCombatAbilityDefinition* Ability, float Duration) const;

	/** Draws the reticle on Target for the predicted ability; Ability can be null */
	void DrawReticle(const AActor* Target, const UCombatAbilityDefinition* Ability) const;

	/** Returns 0.f when no ability active **/
	float GetCurrentAbilityTimeRemaining() const;

	/** The owner's context component, found at BeginPlay */
	UPROPERTY(Transient)
	TObjectPtr<UCombatContextComponent> ContextComponent;

	/** The owner's loadout component, found at BeginPlay */
	UPROPERTY(Transient)
	TObjectPtr<UCombatLoadoutComponent> LoadoutComponent;

	bool bAbilityQueued = false;
	bool bAbilityActive = false;

	/** The ability whose montage was last played, so notifies on that montage know which ability they belong to */
	UPROPERTY(Transient)
	TObjectPtr<const UCombatAbilityDefinition> ActiveAbility;

};
