#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "Combat/CombatHitSettings.h"
#include "CombatDummy.generated.h"

class UAbilitySystemComponent;
class UAnimMontage;

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

	/**
	 *  Gravity multiplier while the dummy is held up in the air, which happens for the rest of a hit's Hit Stun:
	 *  from an air hit with no Launch Speed (each one also stops its fall, so an air combo juggles it),
	 *  and from the top of an upward launch (so a launched dummy floats at its peak, KH style). 1 = no hold
	 */
	UPROPERTY(EditAnywhere, Category = "Enemy|Reactions", meta = (ClampMin = 0, ClampMax = 1))
	float AirHitGravityScale = 0.15f;

	/**
	 *  Gravity multiplier for the fall after an air hold runs out, until the dummy lands or is hit again: it floats down KH style
	 *  rather than dropping, leaving time to land and jump back up to it. A launch, slam or knock-away falls on normal gravity. 1 = drops
	 */
	UPROPERTY(EditAnywhere, Category = "Enemy|Reactions", meta = (ClampMin = 0, ClampMax = 1))
	float JuggleFallGravityScale = 0.5f;

	/**
	 *  How quickly the dummy's sideways speed dies off in the air (the movement component's Braking Deceleration Falling, set at BeginPlay).
	 *  Without it a launch's knockback carries the dummy across for its whole flight, out of reach of a follow-up.
	 *  0 = keeps drifting; 1000 stops a 300 cm/s push in 0.3 s, about 45 cm
	 */
	UPROPERTY(EditAnywhere, Category = "Enemy|Reactions", meta = (ClampMin = 0, Units = "cm/s"))
	float AirBrakingDeceleration = 1000.0f;

	/** True from being launched until landing. The anim blueprint reads this to pick its launched / tumble state over the plain fall */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy|Reactions")
	bool bLaunched = false;

	/** Health the dummy starts with, and refills to when a hit takes it to 0 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Health", meta = (ClampMin = 1))
	float MaxHealth = 100.0f;

	/** Current health. A training dummy never dies: reaching 0 refills it */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy|Health")
	float Health = 0.0f;

	/** Shows the dummy's air state and its last few hits above its head */
	UPROPERTY(EditAnywhere, Category = "Enemy|Debug")
	bool bShowHitDebug = true;

	/** How many recent hits the debug readout lists */
	UPROPERTY(EditAnywhere, Category = "Enemy|Debug", meta = (ClampMin = 1, EditCondition = "bShowHitDebug"))
	int32 DebugHitCount = 6;

	/** Writes the dummy's hits, launches, air holds and landings to the output log (LogCombatProto) */
	UPROPERTY(EditAnywhere, Category = "Enemy|Debug")
	bool bLogHits = true;

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
	 *  Reacts to a hit from Attacker with Settings: takes the damage, is stunned for the hit stun, launches up or slams down
	 *  by the launch speed and pushes along Attacker's facing by the knockback speed (either or both).
	 *  Hit in the air with no launch speed, it's held up for the hit stun instead (see AirHitGravityScale).
	 *  Then plays the air flinch if it's launched or already in the air, else the ground flinch.
	 *  DebugLabel names the hit in the debug readout, e.g. the ability and combo section.
	 *  Stand-in until damage and stagger come in as Gameplay Effects.
	 */
	void ReceiveHit(AActor* Attacker, const FCombatHitSettings& Settings, const FHitResult& Hit, const FString& DebugLabel = FString());

	/** True from being launched until landing. For the anim blueprint, which can't read the protected bLaunched directly */
	UFUNCTION(BlueprintPure, Category = "Enemy|Reactions")
	bool IsLaunched() const { return bLaunched; }

	/** True until the last hit's hit stun runs out. Enemy AI will check this before acting */
	UFUNCTION(BlueprintPure, Category = "Enemy|Reactions")
	bool IsInHitStun() const;

	UFUNCTION(BlueprintPure, Category = "Enemy|Health")
	float GetHealth() const { return Health; }

protected:

	virtual void BeginPlay() override;

	/** Starts the air hold at the top of an upward launch */
	virtual void Tick(float DeltaSeconds) override;

	/** Clears bLaunched, ends any air hold and fades out the air flinch */
	virtual void Landed(const FHitResult& Hit) override;

private:

	/**
	 *  Sets the dummy's vertical velocity to UpSpeed (negative is down) and its horizontal velocity to HorizontalVelocity (Z ignored),
	 *  overriding only the parts that are non-zero. Marks the dummy launched if it goes up, or is slammed while in the air
	 */
	void Launch(float UpSpeed, const FVector& HorizontalVelocity);

	/** Stops the dummy's fall and lowers its gravity to AirHitGravityScale for Duration seconds */
	void StartAirHold(float Duration);

	/** Puts gravity back to normal */
	void EndAirHold();

	/** The air hold's timer ran out: falls on JuggleFallGravityScale if still in the air */
	void OnAirHoldTimeUp();

	/** World time the current hit stun runs out */
	double HitStunEndTime = 0.0;

	/** The movement component's gravity scale at BeginPlay, which an air hold returns to */
	float DefaultGravityScale = 1.0f;

	FTimerHandle AirHoldTimer;

	/**
	 *  Where an upward launch is on its way to the peak hold. Pending until the launch has actually moved the dummy up
	 *  (it's applied on the next movement update), Rising while it goes up; the hold starts when it stops rising
	 */
	enum class ELaunchPeakHold : uint8 { None, Pending, Rising };
	ELaunchPeakHold LaunchPeakHold = ELaunchPeakHold::None;

	/** One line of the debug readout's hit list */
	struct FDebugHit
	{
		double Time = 0.0;
		FString Text;
	};

	/** The last DebugHitCount hits, newest last */
	TArray<FDebugHit> DebugHits;

	/** Draws the debug readout above the dummy for this frame */
	void DrawHitDebug() const;

	/** The dummy's location, velocity, gravity and air state, for the log */
	FString DescribeStateForLog() const;
};
