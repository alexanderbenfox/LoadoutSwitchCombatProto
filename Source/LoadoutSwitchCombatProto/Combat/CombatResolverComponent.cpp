#include "Combat/CombatResolverComponent.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Combat/CombatContextComponent.h"
#include "Combat/CombatHitStopSubsystem.h"
#include "Combat/CombatResolverRules.h"
#include "Combat/CombatWeaponComponent.h"
#include "Components/CapsuleComponent.h"
#include "Curves/CurveFloat.h"
#include "DrawDebugHelpers.h"
#include "Enemy/CombatDummy.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "Loadout/CombatAbilityDefinition.h"
#include "Loadout/CombatLoadout.h"
#include "Loadout/CombatLoadoutComponent.h"
#include "LoadoutSwitchCombatProto.h"

namespace
{
	/** An actor's location, velocity and whether it's in the air, for the hit log */
	FString DescribeForLog(const AActor* Actor)
	{
		if (!Actor)
		{
			return TEXT("(none)");
		}

		const ACharacter* Char = Cast<ACharacter>(Actor);
		const UCharacterMovementComponent* Movement = Char ? Char->GetCharacterMovement() : nullptr;
		const FVector Location = Actor->GetActorLocation();
		const FVector Velocity = Actor->GetVelocity();
		return FString::Printf(TEXT("%s at (%.0f, %.0f, %.0f) vel (%.0f, %.0f, %.0f) %s"), *Actor->GetName(),
			Location.X, Location.Y, Location.Z, Velocity.X, Velocity.Y, Velocity.Z,
			Movement && Movement->IsFalling() ? TEXT("in air") : TEXT("on ground"));
	}

	/** Where To is from From: how far across, and how far above or below */
	FString DescribeOffset(const AActor* From, const AActor* To)
	{
		if (!From || !To)
		{
			return TEXT("no target");
		}

		const FVector Offset = To->GetActorLocation() - From->GetActorLocation();
		return FString::Printf(TEXT("target %.0f cm across, %.0f cm %s"), Offset.Size2D(), FMath::Abs(Offset.Z), Offset.Z >= 0.0 ? TEXT("above") : TEXT("below"));
	}
}

UCombatResolverComponent::UCombatResolverComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatResolverComponent::BeginPlay()
{
	Super::BeginPlay();

	ContextComponent = GetOwner()->FindComponentByClass<UCombatContextComponent>();
	LoadoutComponent = GetOwner()->FindComponentByClass<UCombatLoadoutComponent>();
	GetOwner()->GetComponents<UCombatWeaponComponent>(WeaponComponents);

	// a Glide sets the owner's speed here; the movement component has to move with it the same frame, not before
	if (const ACharacter* Char = Cast<ACharacter>(GetOwner()))
	{
		Char->GetCharacterMovement()->PrimaryComponentTick.AddPrerequisite(this, PrimaryComponentTick);
	}

	// 1 - (1 - t)^2 as a cubic key pair: slope 2 at the start, 0 at the top, so the rise hands over to gravity with no jolt
	DefaultRiseCurve = NewObject<UCurveFloat>(this, TEXT("DefaultRiseCurve"), RF_Transient);
	const FKeyHandle StartKey = DefaultRiseCurve->FloatCurve.AddKey(0.0f, 0.0f);
	const FKeyHandle EndKey = DefaultRiseCurve->FloatCurve.AddKey(1.0f, 1.0f);
	for (const FKeyHandle Key : { StartKey, EndKey })
	{
		FRichCurveKey& CurveKey = DefaultRiseCurve->FloatCurve.GetKey(Key);
		CurveKey.InterpMode = RCIM_Cubic;
		CurveKey.TangentMode = RCTM_User;
	}
	DefaultRiseCurve->FloatCurve.GetKey(StartKey).LeaveTangent = 2.0f;
	DefaultRiseCurve->FloatCurve.GetKey(EndKey).ArriveTangent = 0.0f;
}

FCombatResolution UCombatResolverComponent::Resolve() const
{
	FCombatResolution Resolution;

	if (!Rules || !ContextComponent)
	{
		return Resolution;
	}

	// one snapshot; everything below reads from it
	const FGameplayTagContainer Context = ContextComponent->BuildContext();
	const UCombatLoadout* ActiveLoadout = LoadoutComponent ? LoadoutComponent->GetActiveLoadout() : nullptr;

	if (ActiveLoadout != nullptr)
	{
		Resolution.Slot = Rules->GetFallbackSlot();
		Resolution.Ability = ActiveLoadout->GetAbilityForSlot(Resolution.Slot);

		TArray<FGameplayTag> MatchingSlots = Rules->GetMatchingSlots(Context);
		for (int32 i = 0; i < MatchingSlots.Num(); i++)
		{
			// get assigned ability does not return fallback ability like GetAbilityForSlot does
			UCombatAbilityDefinition* Ability = ActiveLoadout->GetAssignedAbilityForSlot(MatchingSlots[i]);
			if (Ability)
			{
				Resolution.Slot = MatchingSlots[i];
				Resolution.Ability = Ability;
				break;
			}
		}
	}
	else
	{
		Resolution.Slot = Rules->ResolveSlot(Context);
		Resolution.Ability = nullptr;
	}

	return Resolution;
}

void UCombatResolverComponent::HandleAttackPressed()
{
	// the air string has been finished: nothing more until the ground
	if (bAirAttacksSpent && ContextComponent && ContextComponent->IsAirborne())
	{
		if (bLogHits)
		{
			UE_LOG(LogCombatProto, Log, TEXT("[Hit] PRESS IGNORED: air attacks spent until landing"));
		}
		return;
	}

	float AbilityTimeRemaining = GetCurrentAbilityTimeRemaining();
	const bool bComboPending = bComboWindowOpen || bCanChain || bRecoveryOpen;
	if (AbilityTimeRemaining >= BufferTime && !bComboPending )
	{
		return;
	}

	bAbilityQueued = true;
}

void UCombatResolverComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Get the resolution state for the tick - drives queued ability press
	const FCombatResolution Resolution = Resolve();
	const FGameplayTag Slot = Resolution.Slot;
	const UCombatAbilityDefinition* Ability = Resolution.Ability;

	float AbilityTimeRemaining = GetCurrentAbilityTimeRemaining();
	bool bAttackStarted = false;
	if (( bCanChain || bRecoveryOpen || AbilityTimeRemaining <= 0.f ) && bAbilityQueued )
	{
		bAttackStarted = true;

		// the stance the attack resolved in, read before it plays: a rise or lunge moves the character afterwards
		const bool bAirAttack = ContextComponent && ContextComponent->IsAirborne();

		// the last attack's Stall or Glide belongs to it; this one opens its own
		EndAirMotion();

		// carry on the combo if it's the same ability; otherwise a fresh start, which may open differently (rising opener)
		FName ChainName = GetChainSection(Ability);
		if (ChainName.IsNone())
		{
			ChainName = GetOpenerSection(Ability);
		}
		bAbilityActive = PlayAbilityMontage(Ability, ChainName);
		ActiveAbility = bAbilityActive ? Ability : nullptr;
		AttackStartTime = GetWorld()->GetTimeSeconds();

		bAbilityQueued = false;
		bComboWindowOpen = false;
		bCanChain = false;
		bRecoveryOpen = false;
		bHitWindowOpen = false;
		bAttackConnected = false;

		// count it into the string; one that didn't play doesn't count
		if (bAbilityActive && LoadoutComponent)
		{
			const int32 ComboIndex = LoadoutComponent->AdvanceCombo(bAirAttack);

			// the air string's last hit: no more air attacks until landing
			const int32 AirLength = LoadoutComponent->GetComboLength(true);
			if (bAirAttack && AirLength > 0 && ComboIndex == AirLength)
			{
				bAirAttacksSpent = true;
			}
		}

		// say which ability the press resolved to, so it still reads before every ability has a montage
		if (bShowDebug && GEngine)
		{
			const FString Text = Ability ? FString::Printf(TEXT("ATTACK: %s  (%s)"), *Ability->DisplayName.ToString(), *DescribeCombo()) : FString(TEXT("ATTACK: (nothing resolved)"));

			// key -1 adds a new line each press, so a string of attacks reads as a list
			GEngine->AddOnScreenDebugMessage(-1, 1.5f, Ability ? Ability->DebugColor : FColor::Red, Text);
		}

		if (bLogHits)
		{
			// the context it resolved from (range is measured across only, so height doesn't change Close/Far), and where everyone was
			const AActor* Target = ContextComponent ? ContextComponent->GetTarget() : nullptr;
			const FString Context = ContextComponent ? ContextComponent->BuildContext().ToStringSimple() : FString();
			const FString Name = bAbilityActive ? GetHitDebugLabel() : Ability ? Ability->GetName() + TEXT(" (no montage played)") : FString(TEXT("(nothing resolved)"));
			UE_LOG(LogCombatProto, Log, TEXT("[Hit] ATTACK %s  slot %s  combo %s  context [%s]  %s | player %s | target %s"),
				*Name, *Slot.ToString(), *DescribeCombo(), *Context, *DescribeOffset(GetOwner(), Target), *DescribeForLog(GetOwner()), *DescribeForLog(Target));
		}
	}
	else if (AbilityTimeRemaining <= 0.f)
	{
		// ended with nothing chained into it: the string is over, and the next press starts a new one
		if (bAbilityActive)
		{
			EndComboString(TEXT("attack ended"));
		}

		bAbilityActive = false;
	}

	if (!bAbilityActive)
	{
		bComboWindowOpen = false;
		bCanChain = false;
		bRecoveryOpen = false;
	}

	UpdateAirMotion(DeltaTime);

	if (!bShowDebug && !bShowReticle)
	{
		return;
	}

	if (bShowDebug && GEngine)
	{
		const FString Text = FString::Printf(TEXT("Slot: %s\nAbility: %s\nCombo: %s%s"),
			Slot.IsValid() ? *Slot.ToString() : TEXT("(none)"),
			Ability ? *Ability->DisplayName.ToString() : TEXT("(none)"),
			*DescribeCombo(), bAirAttacksSpent ? TEXT("  (air attacks spent until landing)") : TEXT(""));

		// a fixed key replaces last frame's message instead of stacking a new line every tick
		GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 0.0f, FColor::Orange, Text);
	}

	const AActor* Target = ContextComponent ? ContextComponent->GetTarget() : nullptr;
	if (bShowReticle && Target)
	{
		DrawReticle(Target, Ability);
	}
}

bool UCombatResolverComponent::CanJumpNow() const
{
	return !bAbilityActive || IsInRecovery() || (ActiveAbility && ActiveAbility->bJumpCancelable);
}

void UCombatResolverComponent::HandleMovementInput()
{
	// a kept press is about to chain; let it, rather than the held stick ending the combo
	if (!IsInRecovery() || bAbilityQueued || GetWorld()->GetTimeSeconds() - RecoveryStartTime < MovementCancelDelay)
	{
		return;
	}

	if (bLogHits)
	{
		UE_LOG(LogCombatProto, Log, TEXT("[Hit] MOVE CANCEL %s  t %.3f"), *GetHitDebugLabel(), GetActiveMontagePosition());
	}

	CancelAttack();
}

void UCombatResolverComponent::HandleLanded()
{
	bAirAttacksSpent = false;
	EndAirMotion();

	// a kept press still chains: it resolves on the ground now, e.g. into the Riser under a juggled target
	if (!IsInRecovery() || bAbilityQueued)
	{
		return;
	}

	if (bLogHits)
	{
		UE_LOG(LogCombatProto, Log, TEXT("[Hit] LAND CANCEL %s  t %.3f"), *GetHitDebugLabel(), GetActiveMontagePosition());
	}

	CancelAttack();
}

void UCombatResolverComponent::CancelAttack()
{
	if (!bAbilityActive)
	{
		return;
	}

	if (ACharacter* Char = Cast<ACharacter>(GetOwner()))
	{
		if (ActiveAbility && ActiveAbility->Montage)
		{
			Char->StopAnimMontage(ActiveAbility->Montage);
		}

		// the names StartLunge and StartRise give their root motion sources
		Char->GetCharacterMovement()->RemoveRootMotionSource(TEXT("CombatLunge"));
		Char->GetCharacterMovement()->RemoveRootMotionSource(TEXT("CombatRise"));
	}

	bAbilityActive = false;
	bAbilityQueued = false;
	bComboWindowOpen = false;
	bCanChain = false;
	bRecoveryOpen = false;
	bHitWindowOpen = false;
	ActiveAbility = nullptr;
	EndComboString(TEXT("attack cancelled"));

	// put gravity and air control back now rather than next tick, so a jump cancel out of an air attack jumps normally
	EndAirMotion();
}

void UCombatResolverComponent::EndComboString(const TCHAR* Why)
{
	if (!LoadoutComponent || LoadoutComponent->GetComboCount() == 0)
	{
		return;
	}

	if (bLogHits)
	{
		UE_LOG(LogCombatProto, Log, TEXT("[Hit] COMBO END after %s: %s"), *DescribeCombo(), Why);
	}

	LoadoutComponent->ResetCombo();
}

FString UCombatResolverComponent::DescribeCombo() const
{
	if (!LoadoutComponent || LoadoutComponent->GetComboCount() == 0)
	{
		return TEXT("-");
	}

	const bool bAir = LoadoutComponent->IsComboInAir();
	const int32 Length = LoadoutComponent->GetComboLength(bAir);
	const FString LengthText = Length > 0 ? FString::FromInt(Length) : FString(TEXT("-"));
	return FString::Printf(TEXT("%d/%s %s%s"), LoadoutComponent->GetComboCount(), *LengthText, bAir ? TEXT("air") : TEXT("ground"),
		Length > 0 && LoadoutComponent->GetComboCount() == Length ? TEXT(" finisher") : TEXT(""));
}

void UCombatResolverComponent::HandleStallBegin(const UAnimSequenceBase* Animation, const UAnimNotifyState* Window, float GravityScale, bool bStopFall)
{
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (!Char || !IsActiveAbilityMontage(Animation) || !Char->GetCharacterMovement()->IsFalling())
	{
		return;
	}

	EndAirMotion();

	// a rise always stops; a fall only with bStopFall
	UCharacterMovementComponent* Movement = Char->GetCharacterMovement();
	Movement->Velocity.Z = bStopFall ? 0.0 : FMath::Min(Movement->Velocity.Z, 0.0);

	AirMotion = EAirMotion::Stall;
	AirMotionWindow = Window;
	AttackGravityScale = GravityScale;

	if (bLogHits)
	{
		UE_LOG(LogCombatProto, Log, TEXT("[Hit] STALL %s  gravity x%.2f%s  t %.3f"), *GetHitDebugLabel(), GravityScale, bStopFall ? TEXT(", fall stopped") : TEXT(""),
			GetActiveMontagePosition());
	}
}

void UCombatResolverComponent::HandleGlideBegin(const UAnimSequenceBase* Animation, const UAnimNotifyState* Window, float GravityScale, bool bStopFall, float MaxSpeed, float Stiffness)
{
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (!Char || !IsActiveAbilityMontage(Animation) || !Char->GetCharacterMovement()->IsFalling())
	{
		return;
	}

	EndAirMotion();

	UCharacterMovementComponent* Movement = Char->GetCharacterMovement();
	const AActor* Target = ContextComponent ? ContextComponent->GetTarget() : nullptr;

	AirMotionWindow = Window;
	AttackGravityScale = GravityScale;

	// nothing to glide towards: hang where it is instead
	if (!Target)
	{
		Movement->Velocity.Z = 0.0;
		AirMotion = EAirMotion::Stall;
		if (bLogHits)
		{
			UE_LOG(LogCombatProto, Log, TEXT("[Hit] GLIDE %s  no target, stalling  gravity x%.2f  t %.3f"), *GetHitDebugLabel(), GravityScale, GetActiveMontagePosition());
		}
		return;
	}

	// a rise still going always stops; the fall too with bStopFall, so the glide starts level and lingers
	Movement->Velocity.Z = bStopFall ? 0.0 : FMath::Min(Movement->Velocity.Z, 0.0);

	// air control and air braking would fight the spring; they come back when the glide ends
	SavedAirControl = Movement->AirControl;
	SavedBrakingDecelerationFalling = Movement->BrakingDecelerationFalling;
	Movement->AirControl = 0.0f;
	Movement->BrakingDecelerationFalling = 0.0f;

	AirMotion = EAirMotion::Glide;
	GlideMaxSpeed = MaxSpeed;
	GlideStiffness = FMath::Max(Stiffness, 0.1f);

	if (bLogHits)
	{
		UE_LOG(LogCombatProto, Log, TEXT("[Hit] GLIDE %s  gravity x%.2f%s  max %.0f cm/s  stiffness %.1f  speed in (%.0f, %.0f, %.0f)  %s  t %.3f"), *GetHitDebugLabel(),
			GravityScale, bStopFall ? TEXT(", fall stopped") : TEXT(""), MaxSpeed, Stiffness, Movement->Velocity.X, Movement->Velocity.Y, Movement->Velocity.Z, *DescribeOffset(GetOwner(), Target),
			GetActiveMontagePosition());
	}
}

void UCombatResolverComponent::HandleAirMotionEnd(const UAnimNotifyState* Window)
{
	if (AirMotion != EAirMotion::None && AirMotionWindow.Get() == Window)
	{
		EndAirMotion();
	}
}

void UCombatResolverComponent::EndAirMotion()
{
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (AirMotion == EAirMotion::Glide && Char)
	{
		UCharacterMovementComponent* Movement = Char->GetCharacterMovement();
		Movement->AirControl = SavedAirControl;
		Movement->BrakingDecelerationFalling = SavedBrakingDecelerationFalling;

		if (bLogHits)
		{
			UE_LOG(LogCombatProto, Log, TEXT("[Hit] GLIDE END %s  speed out (%.0f, %.0f, %.0f)"), *GetHitDebugLabel(), Movement->Velocity.X, Movement->Velocity.Y, Movement->Velocity.Z);
		}
	}

	AirMotion = EAirMotion::None;
	AirMotionWindow.Reset();
	AttackGravityScale = 1.0f;
}

void UCombatResolverComponent::UpdateAirMotion(float DeltaTime)
{
	if (AirMotion == EAirMotion::None)
	{
		return;
	}

	ACharacter* Char = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent* Movement = Char ? Char->GetCharacterMovement() : nullptr;
	if (!Movement || !bAbilityActive || !Movement->IsFalling())
	{
		EndAirMotion();
		return;
	}

	const AActor* Target = ContextComponent ? ContextComponent->GetTarget() : nullptr;
	if (AirMotion != EAirMotion::Glide || !Target || DeltaTime <= 0.0f)
	{
		return;
	}

	// where the glide settles: Lunge Stop Distance in front of the target, across only (gravity has the height)
	const FVector OwnerLocation = Char->GetActorLocation();
	const FVector ToTarget2D = (Target->GetActorLocation() - OwnerLocation) * FVector(1.0, 1.0, 0.0);
	const float Distance2D = ToTarget2D.Size();
	const FVector Direction2D = Distance2D > KINDA_SMALL_NUMBER ? ToTarget2D / Distance2D : Char->GetActorForwardVector();
	const float StopDistance = ActiveAbility ? ActiveAbility->LungeStopDistance : 100.0f;
	const FVector Offset = ToTarget2D - Direction2D * FMath::Min(StopDistance, Distance2D);

	// a critically damped spring on the speed across, towards a point that moves with the target: it builds on the speed it has,
	// eases in, keeps pace with a target being knocked away (damping towards the target's speed, not zero) and settles without overshooting
	FVector Velocity2D(Movement->Velocity.X, Movement->Velocity.Y, 0.0);
	const FVector TargetVelocity2D = Target->GetVelocity() * FVector(1.0, 1.0, 0.0);
	const float W = GlideStiffness;
	Velocity2D += (Offset * (W * W) + (TargetVelocity2D - Velocity2D) * (2.0f * W)) * DeltaTime;
	Velocity2D = Velocity2D.GetClampedToMaxSize(GlideMaxSpeed);

	Movement->Velocity.X = Velocity2D.X;
	Movement->Velocity.Y = Velocity2D.Y;

	if (Distance2D > KINDA_SMALL_NUMBER)
	{
		Char->SetActorRotation(FRotator(0.0, Direction2D.Rotation().Yaw, 0.0));
	}
}

bool UCombatResolverComponent::PlayAbilityMontage(const UCombatAbilityDefinition* Ability, FName StartSection) const
{
	if (!Ability || !Ability->Montage)
	{
		return false;
	}

	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (!Char)
	{
		return false;
	}

	return (Char->PlayAnimMontage(Ability->Montage, 1.0f, StartSection) > 0.f);
}

FName UCombatResolverComponent::GetChainSection(const UCombatAbilityDefinition* Next) const
{
	if (!Next)
	{
		return NAME_None;
	}

	// only allow if we are continuing current ability
	if (!bAbilityActive || Next != ActiveAbility || !Next->Montage)
	{
		return NAME_None;
	}

	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (!Char )
	{
		return NAME_None;
	}

	UAnimMontage* Montage = ActiveAbility->Montage;
	UAnimInstance* CurrAnim = Char->GetMesh()->GetAnimInstance();
	if (!CurrAnim)
	{
		return NAME_None;
	}

	FName CurrSelectionName = CurrAnim->Montage_GetCurrentSection(Montage);

	// the rising opener stands in for the first section, so it carries on into the second
	const FName RisingOpener = ActiveAbility->RisingOpenerSection;
	if (!RisingOpener.IsNone() && CurrSelectionName == RisingOpener)
	{
		return Montage->CompositeSections.Num() > 1 ? Montage->GetSectionName(1) : NAME_None;
	}

	int32 iAnimIndex = Montage->GetSectionIndex(CurrSelectionName);

	if ( ( iAnimIndex != INDEX_NONE ) && ( iAnimIndex + 1 ) < Montage->CompositeSections.Num())
	{
		// the string's last section doesn't run on into the rising opener after it
		const FName NextSection = Montage->GetSectionName(iAnimIndex + 1);
		return NextSection == RisingOpener ? NAME_None : NextSection;
	}
	else
	{
		return NAME_None;
	}
}

FName UCombatResolverComponent::GetOpenerSection(const UCombatAbilityDefinition* Ability) const
{
	if (!Ability || Ability->RisingOpenerSection.IsNone() || !Ability->Montage || Ability->Montage->GetSectionIndex(Ability->RisingOpenerSection) == INDEX_NONE)
	{
		return NAME_None;
	}

	const AActor* Target = ContextComponent ? ContextComponent->GetTarget() : nullptr;
	if (!Target)
	{
		return NAME_None;
	}

	// KH: an air combo opens upwards at a target above, downwards at one level or below
	const float Above = Target->GetActorLocation().Z - GetOwner()->GetActorLocation().Z;
	const bool bRising = Above >= Ability->RisingOpenerMinHeight;
	if (bLogHits)
	{
		UE_LOG(LogCombatProto, Log, TEXT("[Hit] OPENER %s: target %.0f cm %s (rising from %.0f)"), bRising ? TEXT("rising") : TEXT("usual"),
			FMath::Abs(Above), Above >= 0.0f ? TEXT("above") : TEXT("below"), Ability->RisingOpenerMinHeight);
	}
	return bRising ? Ability->RisingOpenerSection : NAME_None;
}

bool UCombatResolverComponent::IsActiveAbilityMontage(const UAnimSequenceBase* Animation) const
{
	return bAbilityActive && ActiveAbility && Animation && Animation == ActiveAbility->Montage.Get();
}

void UCombatResolverComponent::HandleComboWindowBegin(const UAnimSequenceBase* Animation)
{
	if (!IsActiveAbilityMontage(Animation))
	{
		return;
	}

	bComboWindowOpen = true;
}

void UCombatResolverComponent::HandleComboWindowEnd(const UAnimSequenceBase* Animation)
{
	// a window also ends when its montage is interrupted; only a window we saw open counts
	if (!bComboWindowOpen || !IsActiveAbilityMontage(Animation))
	{
		return;
	}

	bComboWindowOpen = false;
	bCanChain = true;
	RecoveryStartTime = GetWorld()->GetTimeSeconds();
}

void UCombatResolverComponent::HandleRecoveryBegin(const UAnimSequenceBase* Animation)
{
	if (!IsActiveAbilityMontage(Animation))
	{
		return;
	}

	// a recovery never starts on an attack's first frame: that's the previous section's window ending right where this one
	// starts (jumping to a section lands on its edge), and opening it would let a buffered press cut the new swing off
	if (GetWorld()->GetTimeSeconds() - AttackStartTime <= KINDA_SMALL_NUMBER)
	{
		if (bLogHits)
		{
			UE_LOG(LogCombatProto, Log, TEXT("[Hit] RECOVERY IGNORED %s  t %.3f: opened on the attack's first frame"), *GetHitDebugLabel(), GetActiveMontagePosition());
		}
		return;
	}

	bRecoveryOpen = true;
	RecoveryStartTime = GetWorld()->GetTimeSeconds();

	if (bLogHits)
	{
		UE_LOG(LogCombatProto, Log, TEXT("[Hit] RECOVERY %s  t %.3f  hit %d"), *GetHitDebugLabel(), GetActiveMontagePosition(), bAttackConnected);
	}
}

void UCombatResolverComponent::HandleHitWindowBegin(const UAnimSequenceBase* Animation, float RadiusScale, const TArray<FName>& WeaponNames, const FCombatHitSettingsOverride& HitOverrides)
{
	if (!IsActiveAbilityMontage(Animation))
	{
		if (bLogHits)
		{
			UE_LOG(LogCombatProto, Log, TEXT("[Hit] WINDOW IGNORED on %s: not the active ability's montage (active %s, attacking %d)"),
				*GetNameSafe(Animation), ActiveAbility ? *GetNameSafe(ActiveAbility->Montage) : TEXT("(none)"), bAbilityActive);
		}
		return;
	}

	bHitWindowOpen = true;
	HitRadiusScale = RadiusScale;

	// IsActiveAbilityMontage passing means ActiveAbility is set
	ActiveHitSettings = HitOverrides.ApplyTo(ActiveAbility->Hit);
	HitActors.Reset();

	// the window's weapons: the ones it names, or all of them if it names none.
	// each starts with no last frame to sweep from; its first tick only records where it is
	ActiveSweeps.Reset();
	for (UCombatWeaponComponent* Weapon : WeaponComponents)
	{
		if (Weapon && (WeaponNames.IsEmpty() || WeaponNames.Contains(Weapon->GetFName())))
		{
			FWeaponSweep& Sweep = ActiveSweeps.AddDefaulted_GetRef();
			Sweep.Weapon = Weapon;
		}
	}

	if (ActiveSweeps.IsEmpty())
	{
		UE_LOG(LogCombatProto, Warning, TEXT("Hit Window on '%s' matched no Combat Weapon components on '%s'; check its Weapons names"),
			*GetNameSafe(Animation), *GetNameSafe(GetOwner()));
	}

	if (bLogHits)
	{
		const AActor* Target = ContextComponent ? ContextComponent->GetTarget() : nullptr;
		UE_LOG(LogCombatProto, Log, TEXT("[Hit] WINDOW OPEN %s  t %.3f  weapons %d  radius x%.2f  dmg %.0f stun %.2f launch %.0f kb %.0f  %s | player %s | target %s"),
			*GetHitDebugLabel(), GetActiveMontagePosition(), ActiveSweeps.Num(), HitRadiusScale,
			ActiveHitSettings.Damage, ActiveHitSettings.HitStun, ActiveHitSettings.LaunchSpeed, ActiveHitSettings.KnockbackSpeed,
			*DescribeOffset(GetOwner(), Target), *DescribeForLog(GetOwner()), *DescribeForLog(Target));
	}
}

void UCombatResolverComponent::HandleHitWindowTick(const UAnimSequenceBase* Animation)
{
	if (!bHitWindowOpen || !IsActiveAbilityMontage(Animation))
	{
		return;
	}

	for (FWeaponSweep& Sweep : ActiveSweeps)
	{
		SweepWeapon(Sweep);
	}
}

void UCombatResolverComponent::HandleHitWindowEnd(const UAnimSequenceBase* Animation)
{
	if (!IsActiveAbilityMontage(Animation))
	{
		return;
	}

	// where the target was relative to us, to tell a target out of reach across from one above or below the swing
	const AActor* Target = ContextComponent ? ContextComponent->GetTarget() : nullptr;

	// a window that closes having hit nothing is the usual reason a combo or juggle doesn't carry on
	if (bShowDebug && bHitWindowOpen && HitActors.IsEmpty() && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, FString::Printf(TEXT("WHIFF: %s  (%s)"), *GetHitDebugLabel(), *DescribeOffset(GetOwner(), Target)));
	}

	if (bLogHits)
	{
		UE_LOG(LogCombatProto, Log, TEXT("[Hit] WINDOW CLOSE %s  t %.3f  %s  hits %d%s  %s | player %s | target %s"),
			*GetHitDebugLabel(), GetActiveMontagePosition(), bHitWindowOpen ? TEXT("") : TEXT("(was not open)"), HitActors.Num(),
			bHitWindowOpen && HitActors.IsEmpty() ? TEXT(" WHIFF") : TEXT(""),
			*DescribeOffset(GetOwner(), Target), *DescribeForLog(GetOwner()), *DescribeForLog(Target));
	}

	bHitWindowOpen = false;
	HitActors.Reset();
	ActiveSweeps.Reset();
}

FString UCombatResolverComponent::GetHitDebugLabel() const
{
	if (!ActiveAbility)
	{
		return TEXT("(no ability)");
	}

	FString Label = ActiveAbility->DisplayName.IsEmpty() ? ActiveAbility->GetName() : ActiveAbility->DisplayName.ToString();

	// the combo section, so each hit of a combo reads separately
	const ACharacter* Char = Cast<ACharacter>(GetOwner());
	const UAnimInstance* AnimInstance = Char && Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr;
	if (AnimInstance && ActiveAbility->Montage && ActiveAbility->Montage->CompositeSections.Num() > 1)
	{
		Label += FString::Printf(TEXT(" %s"), *AnimInstance->Montage_GetCurrentSection(ActiveAbility->Montage).ToString());
	}

	return Label;
}

float UCombatResolverComponent::GetActiveMontagePosition() const
{
	const ACharacter* Char = Cast<ACharacter>(GetOwner());
	const UAnimInstance* AnimInstance = Char && Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance || !ActiveAbility || !ActiveAbility->Montage || !AnimInstance->Montage_IsPlaying(ActiveAbility->Montage))
	{
		return -1.0f;
	}

	return AnimInstance->Montage_GetPosition(ActiveAbility->Montage);
}

void UCombatResolverComponent::SweepWeapon(FWeaponSweep& Sweep)
{
	const UCombatWeaponComponent* Weapon = Sweep.Weapon.Get();
	if (!Weapon)
	{
		return;
	}

	FVector HitStart;
	FVector HitEnd;
	Weapon->GetHitSegment(HitStart, HitEnd);

	if (bLogHits)
	{
		// how close this frame's hit segment came to the target's capsule: a gap of 0 or less means it's touching
		FString TargetText = TEXT("no target");
		const ACharacter* TargetChar = ContextComponent ? Cast<ACharacter>(ContextComponent->GetTarget()) : nullptr;
		if (const UCapsuleComponent* Capsule = TargetChar ? TargetChar->GetCapsuleComponent() : nullptr)
		{
			const float Radius = Capsule->GetScaledCapsuleRadius();
			const FVector Center = Capsule->GetComponentLocation();
			const FVector Axis = Capsule->GetUpVector() * (Capsule->GetScaledCapsuleHalfHeight() - Radius);

			FVector OnSegment;
			FVector OnAxis;
			FMath::SegmentDistToSegmentSafe(HitStart, HitEnd, Center - Axis, Center + Axis, OnSegment, OnAxis);
			const float Gap = FVector::Dist(OnSegment, OnAxis) - Radius - Weapon->GetHitRadius() * HitRadiusScale;
			const FVector FromCenter = OnSegment - Center;

			TargetText = FString::Printf(TEXT("gap to target %.0f cm (nearest weapon point %.0f across, %.0f up from its centre)"),
				Gap, FromCenter.Size2D(), FromCenter.Z);
		}

		UE_LOG(LogCombatProto, Log, TEXT("[Hit]   SWEEP %s  t %.3f  %s  segment (%.0f, %.0f, %.0f) -> (%.0f, %.0f, %.0f)%s  %s"),
			*GetHitDebugLabel(), GetActiveMontagePosition(), *Weapon->GetName(), HitStart.X, HitStart.Y, HitStart.Z, HitEnd.X, HitEnd.Y, HitEnd.Z,
			Sweep.bHasLastHitSegment ? TEXT("") : TEXT(" (first frame, not swept)"), *TargetText);
	}

	// if has last hit segment, we can sweep from that to this frame's segment
	if (Sweep.bHasLastHitSegment)
	{
		const float HitRadius = Weapon->GetHitRadius() * HitRadiusScale;
		const int32 HitProbeNum = Weapon->GetHitProbeCount(HitRadius);
		const FCollisionQueryParams Params(SCENE_QUERY_STAT(WeaponSweep), false, GetOwner());

		// an object-type query reports every pawn along the sweep; a Pawn channel sweep would stop at the first capsule, missing enemies behind it
		const FCollisionObjectQueryParams ObjectParams(ECC_Pawn);

		for (int32 i = 0; i < HitProbeNum; i++)
		{
			FVector StartProbe = FMath::Lerp(Sweep.LastHitStart, Sweep.LastHitEnd, static_cast<float>(i) / (HitProbeNum - 1));
			FVector EndProbe = FMath::Lerp(HitStart, HitEnd, static_cast<float>(i) / (HitProbeNum - 1));

			TArray<FHitResult> HitResults;
			GetWorld()->SweepMultiByObjectType(HitResults, StartProbe, EndProbe, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(HitRadius), Params);

			for (const FHitResult& Hit : HitResults)
			{
				if (!Hit.GetActor())
				{
					continue;
				}

				if (bLogHits && !HitActors.Contains(Hit.GetActor()))
				{
					UE_LOG(LogCombatProto, Log, TEXT("[Hit]   SWEEP touched %s (%s)"), *Hit.GetActor()->GetName(), *GetNameSafe(Hit.GetComponent()));
				}

				ApplyHit(Hit.GetActor(), Hit);
			}

			// draw hit sphere
			if (bShowDebug)
			{
				DrawDebugSphere(GetWorld(), EndProbe, HitRadius, 20, FColor::Red);
			}
		}
	}

	// the hit segment itself, so a wrong HitDirection / HitStart / HitLength is obvious
	if (bShowDebug)
	{
		DrawDebugLine(GetWorld(), HitStart, HitEnd, FColor::Yellow);
	}

	Sweep.bHasLastHitSegment = true;
	Sweep.LastHitStart = HitStart;
	Sweep.LastHitEnd = HitEnd;
}

void UCombatResolverComponent::ApplyHit(AActor* Victim, const FHitResult& Hit)
{
	if (!Victim || HitActors.Contains(Victim))
	{
		return;
	}

	// stand-in for applying a Gameplay Effect to the victim's ability system
	ACombatDummy* Dummy = Cast<ACombatDummy>(Victim);
	if (!Dummy)
	{
		return;
	}

	HitActors.Add(Victim);
	bAttackConnected = true;

	const FString Label = GetHitDebugLabel();
	if (bLogHits)
	{
		UE_LOG(LogCombatProto, Log, TEXT("[Hit] HIT %s  t %.3f  on %s  dmg %.0f stun %.2f launch %.0f kb %.0f stop %.2f"), *Label, GetActiveMontagePosition(),
			*DescribeForLog(Victim), ActiveHitSettings.Damage, ActiveHitSettings.HitStun, ActiveHitSettings.LaunchSpeed, ActiveHitSettings.KnockbackSpeed,
			ActiveHitSettings.HitStop);
	}

	Dummy->ReceiveHit(GetOwner(), ActiveHitSettings, Hit, Label);

	// after the reaction, so the target freezes already flinching, with its launch or knockback waiting to play out.
	// a hit on several enemies restarts the attacker's stop with each, rather than stacking
	if (UCombatHitStopSubsystem* HitStop = GetWorld()->GetSubsystem<UCombatHitStopSubsystem>())
	{
		HitStop->StartHitStop(Victim, ActiveHitSettings.HitStop, HitStopTimeDilation);
		HitStop->StartHitStop(GetOwner(), ActiveHitSettings.HitStop, HitStopTimeDilation);
	}

	if (bShowDebug && GEngine)
	{
		const FString Text = FString::Printf(TEXT("HIT (%s): %s  dmg %.0f  stun %.2fs  launch %.0f  knockback %.0f"), *Label, *GetNameSafe(Victim),
			ActiveHitSettings.Damage, ActiveHitSettings.HitStun, ActiveHitSettings.LaunchSpeed, ActiveHitSettings.KnockbackSpeed);
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Yellow, Text);
	}
}

void UCombatResolverComponent::HandleLungeWindowBegin(float Duration)
{
	// a window on a montage we didn't start (or one that has since ended) has no ability to lunge for
	if (!bAbilityActive || !ActiveAbility)
	{
		return;
	}

	StartLunge(ActiveAbility, Duration);
}

void UCombatResolverComponent::HandleRiseWindowBegin(const UAnimSequenceBase* Animation, float Duration)
{
	if (!IsActiveAbilityMontage(Animation))
	{
		return;
	}

	StartRise(ActiveAbility, Duration);
}

void UCombatResolverComponent::StartRise(const UCombatAbilityDefinition* Ability, float Duration) const
{
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (!Ability || Ability->RiseHeight <= 0.f || !Char)
	{
		return;
	}

	UCharacterMovementComponent* Movement = Char->GetCharacterMovement();
	const FVector Start = Char->GetActorLocation();
	FVector Target = Start + FVector::UpVector * Ability->RiseHeight;

	// soft move to target: face it and drift across towards it on the way up, at most RiseReach, stopping LungeStopDistance short
	if (const AActor* EnemyTarget = ContextComponent ? ContextComponent->GetTarget() : nullptr)
	{
		const FVector ToTarget2D = (EnemyTarget->GetActorLocation() - Start) * FVector(1.0, 1.0, 0.0);
		const float Distance2D = ToTarget2D.Size();

		if (Distance2D > KINDA_SMALL_NUMBER)
		{
			const FVector Direction2D = ToTarget2D / Distance2D;
			Char->SetActorRotation(Direction2D.Rotation());

			const float Drift = FMath::Clamp(Distance2D - Ability->LungeStopDistance, 0.0f, Ability->RiseReach);
			Target += Direction2D * Drift;
		}
	}

	// in Walking the movement component keeps the character on the floor, so the upward root motion would go nowhere
	Movement->SetMovementMode(MOVE_Falling);

	// gravity doesn't apply while an Override source runs, so the climb takes exactly Duration.
	// to meet a launched target at its peak: launched at v, it tops out at v * v / (2 * g) after v / g seconds
	// (g = 980 at default gravity, so 1000 cm/s peaks at ~510 cm after ~1 s)
	// the dynamic variant only for its time mapping curve, which eases the move; the target itself doesn't move
	TSharedPtr<FRootMotionSource_MoveToDynamicForce> Rise = MakeShared<FRootMotionSource_MoveToDynamicForce>();
	Rise->InstanceName = TEXT("CombatRise");
	Rise->AccumulateMode = ERootMotionAccumulateMode::Override;
	Rise->Priority = 6;   // above the lunge; the two shouldn't overlap, but if they do the rise wins
	Rise->StartLocation = Start;
	Rise->InitialTargetLocation = Target;
	Rise->TargetLocation = Target;
	Rise->Duration = Duration;
	Rise->TimeMappingCurve = Ability->RiseCurve ? Ability->RiseCurve.Get() : DefaultRiseCurve.Get();
	Rise->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	Rise->FinishVelocityParams.SetVelocity = FVector::ZeroVector;  // the eased rise is already at ~0 speed at the top; gravity takes it from there

	Movement->ApplyRootMotionSource(Rise);

	if (bLogHits)
	{
		UE_LOG(LogCombatProto, Log, TEXT("[Hit] RISE %s  from (%.0f, %.0f, %.0f) to (%.0f, %.0f, %.0f) over %.3fs"), *GetHitDebugLabel(),
			Start.X, Start.Y, Start.Z, Target.X, Target.Y, Target.Z, Duration);
	}

	if (bShowDebug)
	{
		DrawDebugLine(GetWorld(), Start, Target, Ability->DebugColor, false, Duration);
	}
}

void UCombatResolverComponent::StartLunge(const UCombatAbilityDefinition* Ability, float Duration) const
{
	if (!Ability || Ability->LungeReach <= 0.f)
	{
		return;
	}

	if (!ContextComponent)
	{
		return;
	}

	const AActor* Target = ContextComponent->GetTarget();
	const FVector OwnerLocation = GetOwner()->GetActorLocation();
	const bool bAirborne = ContextComponent->IsAirborne();

	FVector FinalLocation;
	if (!Target)
	{
		// nothing to home in on: step straight ahead, so a combo into empty space still moves
		if (Ability->LungeStepWithoutTarget <= 0.f)
		{
			if (bLogHits)
			{
				UE_LOG(LogCombatProto, Log, TEXT("[Hit] LUNGE %s skipped: no target"), *GetHitDebugLabel());
			}
			return;
		}

		FinalLocation = OwnerLocation + FRotator(0.0, GetOwner()->GetActorRotation().Yaw, 0.0).Vector() * Ability->LungeStepWithoutTarget;
	}
	else
	{
		const FVector TargetLocation = Target->GetActorLocation();

		// set facing direction
		FVector TargetVector = TargetLocation - OwnerLocation;

		FRotator LungeRotator = TargetVector.Rotation();
		LungeRotator.Pitch = 0;

		GetOwner()->SetActorRotation(LungeRotator);

		if (bAirborne && Ability->bLungeMatchesHeight)
		{
			// homing: in the air, line up with the target's height and stop LungeStopDistance short across it (staying put across if already
			// closer), so the swing meets it whether it's above, below or level. Stopping short along the straight line instead left a
			// target above or below out of the swing's reach
			const FVector ToTarget2D = TargetVector * FVector(1.0, 1.0, 0.0);
			const float Distance2D = ToTarget2D.Size();
			const FVector Direction2D = Distance2D > KINDA_SMALL_NUMBER ? ToTarget2D / Distance2D : LungeRotator.Vector();
			const FVector StopPoint = TargetLocation - Direction2D * FMath::Min(Ability->LungeStopDistance, Distance2D);

			const FVector Move = (StopPoint - OwnerLocation).GetClampedToMaxSize(Ability->LungeReach);
			if (Move.IsNearlyZero(1.0))
			{
				if (bLogHits)
				{
					UE_LOG(LogCombatProto, Log, TEXT("[Hit] LUNGE %s skipped: already in place (air, %s)"), *GetHitDebugLabel(), *DescribeOffset(GetOwner(), Target));
				}
				return;
			}

			FinalLocation = OwnerLocation + Move;
		}
		else
		{
			// across only, keeping the current height: on the ground, or in the air without height matching
			const float LungeLength = FMath::Min(FVector::Dist2D(OwnerLocation, TargetLocation) - Ability->LungeStopDistance, Ability->LungeReach);
			if (LungeLength <= 0.f)
			{
				if (bLogHits)
				{
					UE_LOG(LogCombatProto, Log, TEXT("[Hit] LUNGE %s skipped: already in place (across, %s)"), *GetHitDebugLabel(), *DescribeOffset(GetOwner(), Target));
				}
				return;
			}

			FinalLocation = OwnerLocation + LungeRotator.Vector() * LungeLength;
		}
	}

	// apply root motion
	TSharedPtr<FRootMotionSource_MoveToForce> Move = MakeShared<FRootMotionSource_MoveToForce>();
	Move->InstanceName = TEXT("CombatLunge");
	Move->AccumulateMode = ERootMotionAccumulateMode::Override;   // replaces normal movement while it runs
	Move->Priority = 5;
	Move->StartLocation = OwnerLocation;
	Move->TargetLocation = FinalLocation;
	Move->Duration = Duration;
	Move->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	Move->FinishVelocityParams.SetVelocity = FVector::ZeroVector;  // stop dead instead of sliding past

	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (!Char)
	{
		return;
	}
	Char->GetCharacterMovement()->ApplyRootMotionSource(Move);

	if (bLogHits)
	{
		UE_LOG(LogCombatProto, Log, TEXT("[Hit] LUNGE %s (%s) from (%.0f, %.0f, %.0f) to (%.0f, %.0f, %.0f) over %.3fs  %s"), *GetHitDebugLabel(),
			bAirborne ? TEXT("air") : TEXT("ground"), OwnerLocation.X, OwnerLocation.Y, OwnerLocation.Z, FinalLocation.X, FinalLocation.Y, FinalLocation.Z,
			Duration, *DescribeOffset(GetOwner(), Target));
	}

	if (bShowDebug && GEngine)
	{
		DrawDebugLine(GetWorld(), OwnerLocation, FinalLocation, Ability->DebugColor, false, Duration);
	}
}

void UCombatResolverComponent::DrawReticle(const AActor* Target, const UCombatAbilityDefinition* Ability) const
{
	const FVector TargetLocation = Target->GetActorLocation();
	const FVector ReticleLocation = TargetLocation + FVector{0.0, 0.0, ReticleHeight};

	FString displayName = TEXT("No Ability For Slot");
	FColor displayColor = FColor::White;
	if (Ability != nullptr)
	{
		displayName = Ability->DisplayName.ToString();
		displayColor = Ability->DebugColor;
	}

	UWorld* World = GetWorld();
	DrawDebugString(World, ReticleLocation, displayName, nullptr, displayColor, 0);
	DrawDebugSphere(World, ReticleLocation, 10.f, 20, FColor::Green);
}

float UCombatResolverComponent::GetCurrentAbilityTimeRemaining() const
{
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (!Char || !bAbilityActive )
	{
		return 0.f;
	}

	UAnimInstance* Anim = Char->GetMesh()->GetAnimInstance();
	if (!Anim)
	{
		return 0.f;
	}
	UAnimMontage* Current = Anim->GetCurrentActiveMontage();
	if (!Current)
	{
		return 0.f;
	}

	FName CurrSelectionName = Anim->Montage_GetCurrentSection(Current);
	int32 iAnimIndex = Current->GetSectionIndex(CurrSelectionName);

	float fStartTime;
	float fEndTime;
	Current->GetSectionStartAndEndTime(iAnimIndex, fStartTime, fEndTime);

	const float Rate = Anim->Montage_GetPlayRate(Current) * Current->RateScale;
	return Rate > 0.f ? ( fEndTime - Anim->Montage_GetPosition(Current)) / Rate : 0.f;
}
