#include "Combat/CombatResolverComponent.h"
#include "Combat/CombatContextComponent.h"
#include "Combat/CombatResolverRules.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Loadout/CombatAbilityDefinition.h"
#include "Loadout/CombatLoadout.h"
#include "Loadout/CombatLoadoutComponent.h"

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
	const FCombatResolution Resolution = Resolve();
	const UCombatAbilityDefinition* Ability = Resolution.Ability;

	// stand-in for activating the ability: say which one it would have been
	if (bShowDebug && GEngine)
	{
		const FString Text = Ability ? FString::Printf(TEXT("ATTACK: %s"), *Ability->DisplayName.ToString()) : FString(TEXT("ATTACK: (nothing resolved)"));

		// key -1 adds a new line each press, so a string of attacks reads as a list
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, Ability ? Ability->DebugColor : FColor::Red, Text);
	}
}

void UCombatResolverComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bShowDebug && !bShowReticle)
	{
		return;
	}

	// a prediction for display only; the press resolves again for itself
	const FCombatResolution Resolution = Resolve();
	const FGameplayTag Slot = Resolution.Slot;
	const UCombatAbilityDefinition* Ability = Resolution.Ability;

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
