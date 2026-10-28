#include "Enemy/CombatDummy.h"
#include "AbilitySystemComponent.h"
#include "Combat/CombatTags.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LoadoutSwitchCombatProto.h"

ACombatDummy::ACombatDummy()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));

	PoiseTier = CombatTags::Enemy_Poise_Unarmored;

	// the player's placeholder targeting looks for this actor tag
	Tags.Add(TEXT("Enemy"));

	// character movement only simulates for a possessed pawn, so make sure spawned dummies get an AI controller too
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ACombatDummy::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystem->InitAbilityActorInfo(this, this);

	Health = MaxHealth;
	DefaultGravityScale = GetCharacterMovement()->GravityScale;

	// the dummy never steers, so this only ever slows knockback; falling movement keeps the vertical speed separate, so launches are unaffected
	GetCharacterMovement()->BrakingDecelerationFalling = AirBrakingDeceleration;
}

void ACombatDummy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bShowHitDebug)
	{
		DrawHitDebug();
	}

	if (LaunchPeakHold == ELaunchPeakHold::None)
	{
		return;
	}

	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement->IsFalling())
	{
		// the launch hasn't been applied yet, or the dummy has already landed (Landed clears this too)
		return;
	}

	if (LaunchPeakHold == ELaunchPeakHold::Pending)
	{
		if (Movement->Velocity.Z > 0.0)
		{
			LaunchPeakHold = ELaunchPeakHold::Rising;
		}
		return;
	}

	if (Movement->Velocity.Z <= 0.0)
	{
		LaunchPeakHold = ELaunchPeakHold::None;

		// float at the peak for whatever is left of the launching hit's stun
		const double Remaining = HitStunEndTime - GetWorld()->GetTimeSeconds();
		if (bLogHits)
		{
			UE_LOG(LogCombatProto, Log, TEXT("[Dummy] PEAK  stun left %.2fs  %s"), Remaining, *DescribeStateForLog());
		}
		if (Remaining > 0.0)
		{
			StartAirHold(static_cast<float>(Remaining));
		}
	}
}

void ACombatDummy::DebugLaunch()
{
	Launch(DebugLaunchSpeed, FVector::ZeroVector);
}

void ACombatDummy::ReceiveHit(AActor* Attacker, const FCombatHitSettings& Settings, const FHitResult& Hit, const FString& DebugLabel)
{
	if (bShowHitDebug)
	{
		const bool bAirborne = bLaunched || GetCharacterMovement()->IsFalling();
		FDebugHit& Entry = DebugHits.AddDefaulted_GetRef();
		Entry.Time = GetWorld()->GetTimeSeconds();
		Entry.Text = FString::Printf(TEXT("%s  [%s]  launch %.0f  kb %.0f  stun %.2f"),
			DebugLabel.IsEmpty() ? TEXT("hit") : *DebugLabel, bAirborne ? TEXT("air") : TEXT("ground"),
			Settings.LaunchSpeed, Settings.KnockbackSpeed, Settings.HitStun);

		while (DebugHits.Num() > DebugHitCount)
		{
			DebugHits.RemoveAt(0);
		}
	}

	if (bLogHits)
	{
		UE_LOG(LogCombatProto, Log, TEXT("[Dummy] HIT by %s  dmg %.0f stun %.2f launch %.0f kb %.0f  before: %s"),
			DebugLabel.IsEmpty() ? TEXT("hit") : *DebugLabel, Settings.Damage, Settings.HitStun, Settings.LaunchSpeed, Settings.KnockbackSpeed, *DescribeStateForLog());
	}

	Health -= Settings.Damage;
	if (Health <= 0.f)
	{
		Health = MaxHealth;
	}

	// a new hit restarts the stun rather than adding to what's left, so a fast combo doesn't stack it up
	HitStunEndTime = GetWorld()->GetTimeSeconds() + Settings.HitStun;

	// the attacker's facing, flattened: the lunge has turned it to the target, and a crowd hit pushes everyone the same way.
	// not Hit.ImpactNormal, which is unreliable when the sweep starts inside the capsule
	const FVector KnockbackDirection = Attacker ? Attacker->GetActorForwardVector().GetSafeNormal2D() : FVector::ZeroVector;
	const bool bWasInAir = bLaunched || GetCharacterMovement()->IsFalling();

	// juggling: an air hit that doesn't launch holds the dummy where it is for the hit stun. A launch or slam runs on normal gravity,
	// so a launcher's height doesn't depend on whether the dummy was being held.
	// before the launch: LaunchCharacter keeps the vertical speed it's called with when it isn't overriding Z, so the hold has to stop the fall first
	if (FMath::IsNearlyZero(Settings.LaunchSpeed))
	{
		if (bWasInAir)
		{
			StartAirHold(Settings.HitStun);
		}
	}
	else
	{
		EndAirHold();
	}

	Launch(Settings.LaunchSpeed, KnockbackDirection * Settings.KnockbackSpeed);

	// the launch only takes effect on the next movement update, so a dummy launched this frame isn't falling yet; bLaunched covers it
	const bool bInAir = bLaunched || GetCharacterMovement()->IsFalling();

	if (bInAir)
	{
		// a ground flinch would play out early in the air time and leave the plain fall pose, so the air has its own (or none)
		if (FlinchMontage)
		{
			StopAnimMontage(FlinchMontage);
		}

		if (AirFlinchMontage)
		{
			PlayAnimMontage(AirFlinchMontage, 1.f);
		}
	}
	else if (FlinchMontage)
	{
		PlayAnimMontage(FlinchMontage, 1.f);
	}
}

bool ACombatDummy::IsInHitStun() const
{
	const UWorld* World = GetWorld();
	return World && World->GetTimeSeconds() < HitStunEndTime;
}

void ACombatDummy::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (bLogHits)
	{
		UE_LOG(LogCombatProto, Log, TEXT("[Dummy] LANDED  %s"), *DescribeStateForLog());
	}

	bLaunched = false;
	LaunchPeakHold = ELaunchPeakHold::None;
	EndAirHold();

	if (AirFlinchMontage)
	{
		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			AnimInstance->Montage_Stop(AirFlinchLandBlendOut, AirFlinchMontage);
		}
	}
}

void ACombatDummy::Launch(float UpSpeed, const FVector& HorizontalVelocity)
{
	const bool bVertical = !FMath::IsNearlyZero(UpSpeed);
	const bool bPushAcross = !HorizontalVelocity.IsNearlyZero();

	if (!bVertical && !bPushAcross)
	{
		return;
	}

	// a slam on a dummy standing on the ground just lands it again next frame, so it doesn't count as launched
	const bool bGoesAirborne = UpSpeed > 0.f || (UpSpeed < 0.f && GetCharacterMovement()->IsFalling());

	// overriding rather than adding, so a juggle hit on a falling dummy pops it back up instead of just slowing the fall,
	// and repeated pushes don't stack. A part left at 0 keeps the dummy's current velocity there
	LaunchCharacter(FVector(HorizontalVelocity.X, HorizontalVelocity.Y, UpSpeed), bPushAcross, bVertical);

	if (bLogHits)
	{
		UE_LOG(LogCombatProto, Log, TEXT("[Dummy] LAUNCH  up %.0f  across (%.0f, %.0f)  %s"), UpSpeed, HorizontalVelocity.X, HorizontalVelocity.Y,
			bGoesAirborne ? TEXT("goes airborne") : TEXT("stays put vertically"));
	}

	if (bGoesAirborne)
	{
		bLaunched = true;
	}

	// an upward launch floats at its peak; a slam or a push across doesn't
	if (bVertical)
	{
		LaunchPeakHold = UpSpeed > 0.f ? ELaunchPeakHold::Pending : ELaunchPeakHold::None;
	}
}

void ACombatDummy::DrawHitDebug() const
{
	const UWorld* World = GetWorld();
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	const double Now = World->GetTimeSeconds();

	const bool bHeld = GetWorldTimerManager().IsTimerActive(AirHoldTimer);
	const TCHAR* PeakText = LaunchPeakHold == ELaunchPeakHold::Pending ? TEXT("  launch pending")
		: LaunchPeakHold == ELaunchPeakHold::Rising ? TEXT("  rising to peak") : TEXT("");

	FString Text = FString::Printf(TEXT("%s%s%s  vz %.0f  grav %.2f  stun %.2f  hp %.0f"),
		Movement->IsFalling() ? TEXT("AIR") : TEXT("GROUND"),
		bHeld ? *FString::Printf(TEXT("  HELD %.2fs"), GetWorldTimerManager().GetTimerRemaining(AirHoldTimer)) : TEXT(""),
		PeakText, Movement->Velocity.Z, Movement->GravityScale, FMath::Max(0.0, HitStunEndTime - Now), Health);

	// newest first, with how long ago each landed
	for (int32 i = DebugHits.Num() - 1; i >= 0; i--)
	{
		Text += FString::Printf(TEXT("\n%.1fs ago  %s"), Now - DebugHits[i].Time, *DebugHits[i].Text);
	}

	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	DrawDebugString(World, FVector(0.0, 0.0, HalfHeight + 40.0), Text, const_cast<ACombatDummy*>(this), FColor::White, 0.0f, true, 1.0f);
}

void ACombatDummy::StartAirHold(float Duration)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->Velocity.Z = 0.0;
	Movement->GravityScale = DefaultGravityScale * AirHitGravityScale;

	// a later hit restarts the hold with its own hit stun
	GetWorldTimerManager().SetTimer(AirHoldTimer, this, &ACombatDummy::OnAirHoldTimeUp, FMath::Max(Duration, KINDA_SMALL_NUMBER), false);

	if (bLogHits)
	{
		UE_LOG(LogCombatProto, Log, TEXT("[Dummy] AIR HOLD for %.2fs  %s"), Duration, *DescribeStateForLog());
	}
}

void ACombatDummy::EndAirHold()
{
	// only log a hold that was actually running; this is also called on every launch and landing
	if (bLogHits && GetWorldTimerManager().IsTimerActive(AirHoldTimer))
	{
		UE_LOG(LogCombatProto, Log, TEXT("[Dummy] AIR HOLD ended early  %s"), *DescribeStateForLog());
	}
	else if (bLogHits && !FMath::IsNearlyEqual(GetCharacterMovement()->GravityScale, DefaultGravityScale))
	{
		UE_LOG(LogCombatProto, Log, TEXT("[Dummy] AIR HOLD ended  %s"), *DescribeStateForLog());
	}

	GetWorldTimerManager().ClearTimer(AirHoldTimer);
	GetCharacterMovement()->GravityScale = DefaultGravityScale;
}

void ACombatDummy::OnAirHoldTimeUp()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement->IsFalling())
	{
		EndAirHold();
		return;
	}

	// the timer has fired, so this isn't an early end; Landed or the next hit's hold or launch takes it from here
	Movement->GravityScale = DefaultGravityScale * JuggleFallGravityScale;

	if (bLogHits)
	{
		UE_LOG(LogCombatProto, Log, TEXT("[Dummy] AIR HOLD ended, floating down  %s"), *DescribeStateForLog());
	}
}

FString ACombatDummy::DescribeStateForLog() const
{
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	const FVector Location = GetActorLocation();
	const FVector Velocity = Movement->Velocity;
	return FString::Printf(TEXT("at (%.0f, %.0f, %.0f) vel (%.0f, %.0f, %.0f) grav %.2f %s%s"),
		Location.X, Location.Y, Location.Z, Velocity.X, Velocity.Y, Velocity.Z, Movement->GravityScale,
		Movement->IsFalling() ? TEXT("in air") : TEXT("on ground"), bLaunched ? TEXT(" launched") : TEXT(""));
}
