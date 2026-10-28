#include "Combat/CombatContextComponent.h"
#include "Combat/CombatTags.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Loadout/CombatLoadoutComponent.h"

#include "Kismet/GameplayStatics.h"

namespace
{
	bool IsAirborne_Internal(const AActor* Actor)
	{
		const ACharacter* Character = Cast<ACharacter>(Actor);
		return Character && Character->GetCharacterMovement()->IsFalling();
	}

	bool IsCrowdNearTarget(const UObject* WorldObj, const AActor* Target, float CrowdRadius, int32 CrowdThreshold)
	{
		// eventually replace this (or just the counting part) with querying an enemy registry and cache the result once per tick

		if (CrowdThreshold == 0)
		{
			return true;
		}

		TArray<AActor*> Enemies;
		UGameplayStatics::GetAllActorsWithTag(
			WorldObj,           // world context: any object that lives in the world
			TEXT("Enemy"),  // the actor tag to look for
			Enemies);       // filled with every actor carrying that tag

		int32 iCrowdNumber = 0;
		for (const auto& Enemy : Enemies)
		{
			// count 2d distance for horizontal check only (can be very far apart Z, but thats fine for now we will fix that later)
			const float fDist = FVector::Dist2D(Enemy->GetActorLocation(), Target->GetActorLocation());

			// will want to add hysteresis for this later
			if (fDist <= CrowdRadius)
			{
				iCrowdNumber++;
				if (iCrowdNumber >= CrowdThreshold)
				{
					return true;
				}
			}
		}

		return false;
	}
}

UCombatContextComponent::UCombatContextComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatContextComponent::BeginPlay()
{
	Super::BeginPlay();

	LoadoutComponent = GetOwner()->FindComponentByClass<UCombatLoadoutComponent>();
}

FGameplayTagContainer UCombatContextComponent::BuildContext() const
{
	FGameplayTagContainer Context;

	// Stance: what is the player's current state?
	const bool bIsPlayerAirborne = IsAirborne();
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

	// target state: only a target properly up in the air counts, not one popped up a little by a hit
	if (IsTargetAirborne())
	{
		Context.AddTag(CombatTags::Context_Target_Airborne);
	}


	// target crowd density
	if (Target != nullptr)
	{
		if (IsCrowdNearTarget( GetWorld(), Target.Get(), CrowdRadius, CrowdThreshold))
		{
			Context.AddTag(CombatTags::Context_Target_Crowd);
		}
		else
		{
			Context.AddTag(CombatTags::Context_Target_Single);
		}
	}

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

	// combo position: the next attack is the last of the string in this stance
	if (LoadoutComponent && LoadoutComponent->IsNextAttackFinisher(bIsPlayerAirborne))
	{
		Context.AddTag(CombatTags::Context_Combo_Finisher);
	}

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

bool UCombatContextComponent::IsAirborne() const
{
	return IsAirborne_Internal(GetOwner());
}

bool UCombatContextComponent::IsTargetAirborne() const
{
	if (!IsAirborne_Internal(Target.Get()))
	{
		return false;
	}
	return TargetAirborneMinHeight <= 0.0f || GetTargetHeightAboveGround(TargetAirborneMinHeight) >= TargetAirborneMinHeight;
}

float UCombatContextComponent::GetTargetHeightAboveGround(float MaxDistance) const
{
	const ACharacter* TargetCharacter = Cast<ACharacter>(Target.Get());
	if (!TargetCharacter)
	{
		return MaxDistance;
	}

	const FVector Feet = TargetCharacter->GetActorLocation() - FVector(0.0f, 0.0f, TargetCharacter->GetSimpleCollisionHalfHeight());
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TargetHeightAboveGround), false, TargetCharacter);
	Params.AddIgnoredActor(GetOwner());

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Feet, Feet - FVector(0.0f, 0.0f, MaxDistance), ECC_Visibility, Params))
	{
		return Hit.Distance;
	}
	return MaxDistance;
}

float UCombatContextComponent::GetDistanceToTarget() const
{
	const AActor* TargetActor = Target.Get();
	return TargetActor ? FVector::Dist2D(GetOwner()->GetActorLocation(), TargetActor->GetActorLocation()) : -1.0f;
}

int32 UCombatContextComponent::CountEnemiesNearTarget() const
{
	if( !Target.IsValid() )
	{
		return 0;
	}

	// eventually replace this (or just the counting part) with querying an enemy registry and cache the result once per tick

	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsWithTag(
		GetWorld(),           // world context: any object that lives in the world
		TEXT("Enemy"),  // the actor tag to look for
		Enemies);       // filled with every actor carrying that tag

	int32 iCrowdNumber = 0;
	for (const auto& Enemy : Enemies)
	{
		// count 2d distance for horizontal check only (can be very far apart Z, but thats fine for now we will fix that later)
		const float fDist = FVector::Dist2D(Enemy->GetActorLocation(), Target->GetActorLocation());

		// will want to add hysteresis for this later
		if (fDist <= CrowdRadius)
		{
			iCrowdNumber++;
		}
	}

	return iCrowdNumber;
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
		const FString Text = FString::Printf(TEXT("Target: %s  (%.0f cm, %s, %d nearby, %.0f cm up / airborne from %.0f)\nContext: %s"),
			*GetNameSafe(TargetActor), GetDistanceToTarget(), *UEnum::GetDisplayValueAsText(RangeBand).ToString(),
			CountEnemiesNearTarget(), TargetActor ? GetTargetHeightAboveGround(1000.0f) : 0.0f, TargetAirborneMinHeight,
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

		// crowd radius: enemies inside this ring count toward Context.Target.Crowd
		DrawDebugCircle(GetWorld(), TargetActor->GetActorLocation(), CrowdRadius, 48, FColor::Magenta,
			false, -1.0f, 0, 1.0f, FVector::ForwardVector, FVector::RightVector, false);
	}
}
