#include "Combat/CombatContextComponent.h"
#include "Combat/CombatTags.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	bool IsAirborne(const AActor* Actor)
	{
		const ACharacter* Character = Cast<ACharacter>(Actor);
		return Character && Character->GetCharacterMovement()->IsFalling();
	}
}

UCombatContextComponent::UCombatContextComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

FGameplayTagContainer UCombatContextComponent::BuildContext() const
{
	FGameplayTagContainer Context;

	// Stance: what is the player's current state?
	const bool bIsPlayerAirborne = IsAirborne(GetOwner());
	if (bIsPlayerAirborne)
	{
		Context.AddTag(CombatTags::Context_Stance_Air);
	}
	else
	{
		Context.AddTag(CombatTags::Context_Stance_Ground);
	}
	

	// Range: cached range band for target
	if (Target != nullptr)
	{
		if (RangeBand == ECombatRangeBand::Close)
		{
			Context.AddTag(CombatTags::Context_Range_Close);
		}
		else
		{
			Context.AddTag(CombatTags::Context_Range_Far);
		}
	}

	// target state
	const bool bIsTargetAirborne = IsAirborne(Target.Get());
	if (bIsTargetAirborne)
	{
		Context.AddTag(CombatTags::Context_Target_Airborne);
	}

	// TODO: Add context for crowd/single target - from the Target get something like UEnemyAIComponent and get the crowd proximity state from that

	// recent action
	const double currTime = GetWorld()->GetTimeSeconds();
	auto fEvalRecentAction = [&currTime](double lastActionTime, double actionWindow) {return lastActionTime >= 0 && (currTime - lastActionTime) <= actionWindow; };
	if (fEvalRecentAction(LastDodgeTime, PostDodgeWindow ) )
	{
		Context.AddTag(CombatTags::Context_Recent_Dodge);
	}
	if (fEvalRecentAction(LastSwapTime, SwapInWindow))
	{
		Context.AddTag(CombatTags::Context_Recent_Swap);
	}

	// TODO: Add combo counter + query the combo count data from weapon data (or weapon ability data) to see if we are on the finisher

	return Context;
}

ECombatRangeBand UCombatContextComponent::ComputeRangeBand(float Distance, ECombatRangeBand Previous, bool UseHysteresis ) const
{
	if (!UseHysteresis)
	{
		return Distance <= CloseRange ? ECombatRangeBand::Close : ECombatRangeBand::Far;
	}
	else
	{
		if (Previous == ECombatRangeBand::Close)
		{
			if (Distance >= CloseRange + (RangeHysteresis / 2.f))
			{
				return ECombatRangeBand::Far;
			}
			return ECombatRangeBand::Close;
		}
		else
		{
			if (Distance <= CloseRange - (RangeHysteresis / 2.f))
			{
				return ECombatRangeBand::Close;
			}
			return ECombatRangeBand::Far;
		}
	}
}

void UCombatContextComponent::SetTarget(AActor* NewTarget)
{
	const bool bTargetChange = Target != NewTarget;
	Target = NewTarget;

	if (bTargetChange)
	{
		const float Distance = GetDistanceToTarget();
		RangeBand = Distance >= 0.0f ? ComputeRangeBand(Distance, RangeBand, false) : ECombatRangeBand::Far;
	}
}

void UCombatContextComponent::NotifyDodged()
{
	LastDodgeTime = GetWorld()->GetTimeSeconds();
}

void UCombatContextComponent::NotifySwapped()
{
	LastSwapTime = GetWorld()->GetTimeSeconds();
}

float UCombatContextComponent::GetDistanceToTarget() const
{
	const AActor* TargetActor = Target.Get();
	return TargetActor ? FVector::Dist2D(GetOwner()->GetActorLocation(), TargetActor->GetActorLocation()) : -1.0f;
}

void UCombatContextComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float Distance = GetDistanceToTarget();
	RangeBand = Distance >= 0.0f ? ComputeRangeBand(Distance, RangeBand, true) : ECombatRangeBand::Far;

	if (bShowDebug)
	{
		DrawDebug();
	}
}

void UCombatContextComponent::DrawDebug() const
{
	const AActor* TargetActor = Target.Get();

	if (GEngine)
	{
		const FString Text = FString::Printf(TEXT("Target: %s  (%.0f cm, %s)\nContext: %s"),
			*GetNameSafe(TargetActor), GetDistanceToTarget(), *UEnum::GetDisplayValueAsText(RangeBand).ToString(),
			*BuildContext().ToStringSimple());

		// a fixed key replaces last frame's message instead of stacking a new line every tick
		GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 0.0f, FColor::Cyan, Text);
	}

	if (TargetActor)
	{
		// one ring per flip distance; the bright one is the ring that has to be crossed to change band
		const bool bClose = RangeBand == ECombatRangeBand::Close;
		const float EnterCloseRadius = CloseRange - RangeHysteresis * 0.5f;
		const float LeaveCloseRadius = CloseRange + RangeHysteresis * 0.5f;
		const FColor BandColor = bClose ? FColor::Green : FColor::Yellow;
		const FColor IdleColor(96, 96, 96);

		DrawDebugCircle(GetWorld(), TargetActor->GetActorLocation(), EnterCloseRadius, 48, bClose ? IdleColor : BandColor,
			false, -1.0f, 0, bClose ? 1.0f : 2.0f, FVector::ForwardVector, FVector::RightVector, false);
		DrawDebugCircle(GetWorld(), TargetActor->GetActorLocation(), LeaveCloseRadius, 48, bClose ? BandColor : IdleColor,
			false, -1.0f, 0, bClose ? 2.0f : 1.0f, FVector::ForwardVector, FVector::RightVector, false);
	}
}
