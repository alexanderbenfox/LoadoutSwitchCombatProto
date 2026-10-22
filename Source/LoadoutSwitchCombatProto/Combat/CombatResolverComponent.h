#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "CombatResolverComponent.generated.h"

class UAnimSequenceBase;
class UCombatAbilityDefinition;
class UCombatContextComponent;
class UCombatLoadoutComponent;
class UCombatResolverRules;
class UCombatWeaponComponent;

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

/** One weapon an open Hit Window is sweeping, and where its hit segment was at the end of last frame's sweep, in world space **/
struct FWeaponSweep
{
	TWeakObjectPtr<UCombatWeaponComponent> Weapon;

	/** False until the window's first tick has recorded a segment to sweep from */
	bool bHasLastHitSegment = false;
	FVector LastHitStart = FVector::ZeroVector;
	FVector LastHitEnd = FVector::ZeroVector;
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

	/** True while an attack montage this resolver started is playing, recovery included */
	bool IsAttacking() const { return bAbilityActive; }

	/** True unless an attack is playing whose ability isn't Jump Cancelable */
	bool CanJumpNow() const;

	/** Stops the attack in progress, if any: its montage, its lunge and any kept press. Used by cancels such as jumping */
	void CancelAttack();

	/** Called by a Combat Lunge notify window on the playing montage; lunges for the ability that montage belongs to */
	void HandleLungeWindowBegin(float Duration);

	/** Called by a Combo Window notify on Animation. Presses from here on are kept for the next attack */
	void HandleComboWindowBegin(const UAnimSequenceBase* Animation);

	/** Called when that window closes. From here until the attack ends, a kept or new press chains into the next attack */
	void HandleComboWindowEnd(const UAnimSequenceBase* Animation);

	/**
	 *  Called by a Hit Window notify on Animation when it opens: forgets who the last window hit, and picks the weapons to sweep,
	 *  the Combat Weapon components named in WeaponNames, or all of them if it's empty. RadiusScale multiplies each weapon's Hit Radius
	 */
	void HandleHitWindowBegin(const UAnimSequenceBase* Animation, float RadiusScale, const TArray<FName>& WeaponNames);

	/** Called every frame the window is open: sweeps each of its weapons from last frame's position to this frame's */
	void HandleHitWindowTick(const UAnimSequenceBase* Animation);

	/** Called when that window closes */
	void HandleHitWindowEnd(const UAnimSequenceBase* Animation);

	/** True once the current attack has hit something. Swap cancel and the combo counter will read this */
	bool HasAttackConnected() const { return bAttackConnected; }

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
	 *  Starts at StartSection, or the montage's beginning with NAME_None. Returns false if nothing was played
	 */
	bool PlayAbilityMontage(const UCombatAbilityDefinition* Ability, FName StartSection = NAME_None) const;

	/**
	 *  Which section of Next's montage a chained attack starts at: the section after the one playing now
	 *  if Next is the ability already playing, else NAME_None (from the start, e.g. a cancel into a different move).
	 */
	FName GetChainSection(const UCombatAbilityDefinition* Next) const;

	/** True if Animation is the montage of the ability we're playing; filters out windows on montages we didn't start, or stale ones */
	bool IsActiveAbilityMontage(const UAnimSequenceBase* Animation) const;

	/**
	 *  Turns the owner to face the context target and moves it towards the target with a root motion source,
	 *  using the ability's lunge settings, over Duration seconds. Does nothing if the ability doesn't lunge or there's no target.
	 *  Stand-in for the GAS ApplyRootMotionMoveToActorForce task.
	 */
	void StartLunge(const UCombatAbilityDefinition* Ability, float Duration) const;

	/**
	 *  Sweeps Sweep's weapon hit segment from Sweep.LastHitStart/LastHitEnd to where it is now, with the weapon's Hit Radius times HitRadiusScale,
	 *  calls ApplyHit on each enemy it touches, then stores this frame's segment in Sweep as the last one.
	 */
	void SweepWeapon(FWeaponSweep& Sweep);

	/** Hits Victim with the active ability, unless this window already hit it */
	void ApplyHit(AActor* Victim, const FHitResult& Hit);

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

	/** Every Combat Weapon component on the owner, found at BeginPlay; Hit Windows sweep these */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UCombatWeaponComponent>> WeaponComponents;

	bool bAbilityQueued = false;
	bool bAbilityActive = false;

	/** True from a Combo Window opening until it closes: presses are kept for the next attack */
	bool bComboWindowOpen = false;

	/** True from a Combo Window closing until the attack ends or the next one starts: the attack can be cancelled into the next */
	bool bCanChain = false;

	/** The ability whose montage was last played, so notifies on that montage know which ability they belong to */
	UPROPERTY(Transient)
	TObjectPtr<const UCombatAbilityDefinition> ActiveAbility;

	/** True while a Hit Window on the active ability's montage is open */
	bool bHitWindowOpen = false;

	/** The open Hit Window's multiplier on each weapon's Hit Radius */
	float HitRadiusScale = 1.0f;

	/** The weapons the open Hit Window is sweeping */
	TArray<FWeaponSweep> ActiveSweeps;

	/** Who the open Hit Window has already hit, with any of its weapons, so each enemy takes one hit per window */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> HitActors;

	/** True once the current attack has hit something; reset when the next attack starts */
	bool bAttackConnected = false;

};
