#include "Combat/CombatResolverComponent.h"
#include "Animation/AnimMontage.h"
#include "Combat/CombatContextComponent.h"
#include "Combat/CombatResolverRules.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "Loadout/CombatAbilityDefinition.h"
#include "Loadout/CombatLoadout.h"
#include "Loadout/CombatLoadoutComponent.h"
#include "LoadoutSwitchCombatProto.h"

UCombatResolverComponent::UCombatResolverComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatResolverComponent::BeginPlay()
{
	Super::BeginPlay();

	ContextComponent = GetOwner()->FindComponentByClass<UCombatContextComponent>();
	LoadoutComponent = GetOwner()->FindComponentByClass<UCombatLoadoutComponent>();
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
	float AbilityTimeRemaining = GetCurrentAbilityTimeRemaining();
	const bool bComboPending = bComboWindowOpen || bCanChain;
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
	if (( bCanChain || AbilityTimeRemaining <= 0.f ) && bAbilityQueued )
	{
		FName ChainName = GetChainSection(Ability);
		bAbilityActive = PlayAbilityMontage(Ability, ChainName);
		ActiveAbility = bAbilityActive ? Ability : nullptr;
		
		bAbilityQueued = false;
		bComboWindowOpen = false;
		bCanChain = false;

		// say which ability the press resolved to, so it still reads before every ability has a montage
		if (bShowDebug && GEngine)
		{
			const FString Text = Ability ? FString::Printf(TEXT("ATTACK: %s"), *Ability->DisplayName.ToString()) : FString(TEXT("ATTACK: (nothing resolved)"));

			// key -1 adds a new line each press, so a string of attacks reads as a list
			GEngine->AddOnScreenDebugMessage(-1, 1.5f, Ability ? Ability->DebugColor : FColor::Red, Text);
		}
	}
	else if (AbilityTimeRemaining <= 0.f)
	{
		bAbilityActive = false;
	}

	if (!bAbilityActive)
	{
		bComboWindowOpen = false;
		bCanChain = false;
	}

	if (!bShowDebug && !bShowReticle)
	{
		return;
	}

	if (bShowDebug && GEngine)
	{
		const FString Text = FString::Printf(TEXT("Slot: %s\nAbility: %s"),
			Slot.IsValid() ? *Slot.ToString() : TEXT("(none)"),
			Ability ? *Ability->DisplayName.ToString() : TEXT("(none)"));

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
	return !bAbilityActive || (ActiveAbility && ActiveAbility->bJumpCancelable);
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

		// the name StartLunge gives its root motion source
		Char->GetCharacterMovement()->RemoveRootMotionSource(TEXT("CombatLunge"));
	}

	bAbilityActive = false;
	bAbilityQueued = false;
	bComboWindowOpen = false;
	bCanChain = false;
	ActiveAbility = nullptr;
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

	int32 iAnimIndex = Montage->GetSectionIndex(CurrSelectionName);

	if ( ( iAnimIndex != INDEX_NONE ) && ( iAnimIndex + 1 ) < Montage->CompositeSections.Num())
	{
		return Montage->GetSectionName(iAnimIndex + 1);
	}
	else
	{
		return NAME_None;
	}
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
	if (!Target)
	{
		return;
	}

	const FVector OwnerLocation = GetOwner()->GetActorLocation();
	const FVector TargetLocation = Target->GetActorLocation();

	// set facing direction
	FVector TargetVector = TargetLocation - OwnerLocation;

	FRotator LungeRotator = TargetVector.Rotation();
	LungeRotator.Pitch = 0;

	GetOwner()->SetActorRotation(LungeRotator);

	// calculate allowed distance of lunge
	const bool bIsAirborne = ContextComponent->IsAirborne();
	const float Length = bIsAirborne ? FVector::Dist(OwnerLocation, TargetLocation) : FVector::Dist2D(OwnerLocation, TargetLocation);
	const float LungeLength = FMath::Min(Length - Ability->LungeStopDistance, Ability->LungeReach);
	if (LungeLength <= 0.f)
	{
		return;
	}

	// use 2D distance on the ground, but 3D space in air
	FVector FinalLocation;
	if (bIsAirborne)
	{
		FVector Direction;
		float TempLength;
		TargetVector.ToDirectionAndLength(Direction, TempLength);

		FinalLocation = Direction * LungeLength;
	}
	else
	{
		FinalLocation = LungeRotator.Vector() * LungeLength;
	}

	FinalLocation = OwnerLocation + FinalLocation;

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
