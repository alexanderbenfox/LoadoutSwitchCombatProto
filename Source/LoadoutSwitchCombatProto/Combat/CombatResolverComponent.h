#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Combat/CombatHitSettings.h"
#include "CombatResolverComponent.generated.h"

class UAnimNotifyState;
class UAnimSequenceBase;
class UCombatAbilityDefinition;
class UCombatContextComponent;
class UCombatLoadoutComponent;
class UCombatResolverRules;
class UCombatWeaponComponent;
class UCurveFloat;

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

	/** Called by a Combat Stall window opening on Animation: in the air, stops the rise (and with bStopFall the fall) and hangs on GravityScale until Window closes */
	void HandleStallBegin(const UAnimSequenceBase* Animation, const UAnimNotifyState* Window, float GravityScale, bool bStopFall);

	/**
	 *  Called by a Combat Glide window opening on Animation: in the air, steers the owner towards the target with a spring (MaxSpeed,
	 *  Stiffness) on GravityScale until Window closes, stopping the fall first with bStopFall. With no target, stalls instead
	 */
	void HandleGlideBegin(const UAnimSequenceBase* Animation, const UAnimNotifyState* Window, float GravityScale, bool bStopFall, float MaxSpeed, float Stiffness);

	/** Called by a Combat Stall or Glide window closing; ends it if it's the one in effect */
	void HandleAirMotionEnd(const UAnimNotifyState* Window);

	/** Called by a Combat Rise notify window on Animation; rises for the ability that montage belongs to, over Duration seconds */
	void HandleRiseWindowBegin(const UAnimSequenceBase* Animation, float Duration);

	/** Called by a Combo Window notify on Animation. Presses from here on are kept for the next attack */
	void HandleComboWindowBegin(const UAnimSequenceBase* Animation);

	/** Called when that window closes. From here until the attack ends, a kept or new press chains into the next attack */
	void HandleComboWindowEnd(const UAnimSequenceBase* Animation);

	/** Called by a Recovery notify window on Animation: from here an attack press starts the next attack, and jumping or moving stops this one */
	void HandleRecoveryBegin(const UAnimSequenceBase* Animation);

	/** True in the recovery of the attack playing: after its Combo Window closed, or inside its Recovery window */
	bool IsInRecovery() const { return bAbilityActive && (bCanChain || bRecoveryOpen); }

	/** What the attack playing wants the owner's gravity multiplied by right now (an open Stall or Glide window); 1 when it doesn't. The character applies it */
	float GetAttackGravityScale() const { return AttackGravityScale; }

	/** Called by the character on movement input. Stops an attack in recovery, once it's been in it Movement Cancel Delay, unless a press is waiting to chain */
	void HandleMovementInput();

	/** Called by the character when it lands. Stops an attack in recovery, so an air attack's recovery doesn't play out on the ground */
	void HandleLanded();

	/**
	 *  Called by a Hit Window notify on Animation when it opens: forgets who the last window hit, and picks the weapons to sweep,
	 *  the Combat Weapon components named in WeaponNames, or all of them if it's empty. RadiusScale multiplies each weapon's Hit Radius.
	 *  The window's hits use the active ability's Hit settings with HitOverrides applied
	 */
	void HandleHitWindowBegin(const UAnimSequenceBase* Animation, float RadiusScale, const TArray<FName>& WeaponNames, const FCombatHitSettingsOverride& HitOverrides);

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

	/**
	 *  Writes each attack's events to the output log (LogCombatProto): the attack starting, its lunge and rise, and its Hit Windows,
	 *  with every frame's weapon sweep and how far it passed from the target. For working out why an attack misses
	 */
	UPROPERTY(EditAnywhere, Category = "Resolver|Debug")
	bool bLogHits = true;

	/** Marks the target with the ability the next attack press would use */
	UPROPERTY(EditAnywhere, Category = "Resolver|Reticle")
	bool bShowReticle = true;

	/** How far above the target's origin the reticle sits */
	UPROPERTY(EditAnywhere, Category = "Resolver|Reticle", meta = (Units = "cm"))
	float ReticleHeight = 120.0f;

	/** How slow the attacker and the target run during a hit's Hit Stop: 0.02 is effectively frozen, 1 turns hit-stop off */
	UPROPERTY(EditAnywhere, Category = "Resolver|Hit Stop", meta = (ClampMin = 0.01, ClampMax = 1))
	float HitStopTimeDilation = 0.02f;

	/** Buffer time before ability time has finished to buffer another ability **/
	UPROPERTY(EditAnywhere, Category = "Resolver|Buffer", meta = (Units = "s"))
	float BufferTime = 0.1f;

	/**
	 *  How long into an attack's recovery movement input waits before it stops the attack. Gives a press just after a Combo Window
	 *  closes time to chain while the stick is held, instead of the held stick ending the combo first
	 */
	UPROPERTY(EditAnywhere, Category = "Resolver|Cancels", meta = (ClampMin = 0, Units = "s"))
	float MovementCancelDelay = 0.1f;

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
	 *  Moves the owner up by the ability's Rise Height over Duration seconds with a root motion source named "CombatRise",
	 *  after which it falls normally. With a target, also faces it and drifts towards it by up to Rise Reach.
	 *  Does nothing if the ability doesn't rise.
	 */
	void StartRise(const UCombatAbilityDefinition* Ability, float Duration) const;

	/**
	 *  Sweeps Sweep's weapon hit segment from Sweep.LastHitStart/LastHitEnd to where it is now, with the weapon's Hit Radius times HitRadiusScale,
	 *  calls ApplyHit on each enemy it touches, then stores this frame's segment in Sweep as the last one.
	 */
	void SweepWeapon(FWeaponSweep& Sweep);

	/** The active ability's name plus its current montage section if it has several (e.g. "Air Combo B"), for debug messages */
	FString GetHitDebugLabel() const;

	/** Where the active ability's montage is playing, in seconds, or -1 if it isn't; for the hit log */
	float GetActiveMontagePosition() const;

	/** Hits Victim with ActiveHitSettings, unless this window already hit it */
	void ApplyHit(AActor* Victim, const FHitResult& Hit);

	/** Each tick: steers an open Glide, and ends a Stall or Glide once the attack is over or the owner isn't in the air */
	void UpdateAirMotion(float DeltaTime);

	/** Ends the Stall or Glide in effect, if any, and gives the owner back its air control and air braking */
	void EndAirMotion();

	/** The section a fresh (not chained) attack of Ability starts in: its Rising Opener Section with the target high enough above, else none (the first) */
	FName GetOpenerSection(const UCombatAbilityDefinition* Ability) const;

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

	/** The rise's shape when the ability has no Rise Curve: a quadratic ease-out, 1 - (1 - t)^2, made at BeginPlay */
	UPROPERTY(Transient)
	TObjectPtr<UCurveFloat> DefaultRiseCurve;

	/** Every Combat Weapon component on the owner, found at BeginPlay; Hit Windows sweep these */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UCombatWeaponComponent>> WeaponComponents;

	bool bAbilityQueued = false;
	bool bAbilityActive = false;

	/** True from a Combo Window opening until it closes: presses are kept for the next attack */
	bool bComboWindowOpen = false;

	/** True from a Combo Window closing until the attack ends or the next one starts: the attack can be cancelled into the next */
	bool bCanChain = false;

	/** True from a Recovery window opening until the attack ends or the next one starts */
	bool bRecoveryOpen = false;

	/** World time the current attack's recovery started (its Combo Window closing or its Recovery window opening) */
	double RecoveryStartTime = 0.0;

	/** The ability whose montage was last played, so notifies on that montage know which ability they belong to */
	UPROPERTY(Transient)
	TObjectPtr<const UCombatAbilityDefinition> ActiveAbility;

	/** True while a Hit Window on the active ability's montage is open */
	bool bHitWindowOpen = false;

	/** The open Hit Window's multiplier on each weapon's Hit Radius */
	float HitRadiusScale = 1.0f;

	/** What the open Hit Window's hits do: the active ability's Hit settings with the window's overrides applied */
	FCombatHitSettings ActiveHitSettings;

	/** The weapons the open Hit Window is sweeping */
	TArray<FWeaponSweep> ActiveSweeps;

	/** Who the open Hit Window has already hit, with any of its weapons, so each enemy takes one hit per window */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> HitActors;

	enum class EAirMotion : uint8 { None, Stall, Glide };

	/** The Stall or Glide window in effect, and which one it is; a window closing only ends the motion if it's this one */
	EAirMotion AirMotion = EAirMotion::None;
	TWeakObjectPtr<const UAnimNotifyState> AirMotionWindow;

	/** The open Glide's settings */
	float GlideMaxSpeed = 0.0f;
	float GlideStiffness = 1.0f;

	/** The owner's air control and air braking, put aside while a Glide steers it (they'd fight the spring) */
	float SavedAirControl = 0.0f;
	float SavedBrakingDecelerationFalling = 0.0f;

	/** See GetAttackGravityScale */
	float AttackGravityScale = 1.0f;

	/** True once the current attack has hit something; reset when the next attack starts */
	bool bAttackConnected = false;

	/** World time the current attack started */
	double AttackStartTime = 0.0;

	/**
	 *  True from the last hit of an air string (its finisher) starting until the character lands: attack presses in the air are ignored,
	 *  so the finisher's recovery lasts until the ground (KH style) instead of starting another air string
	 */
	bool bAirAttacksSpent = false;

	/** Ends the combo string, e.g. when an attack ends with nothing chained or is cancelled. Why is for the hit log */
	void EndComboString(const TCHAR* Why);

	/** "2/4 ground" for the debug text: the current string's count and length, or "-" with no string going */
	FString DescribeCombo() const;

};
