#include "Combat/CombatResolverComponent.h"
#include "Combat/CombatContextComponent.h"
#include "Combat/CombatResolverRules.h"
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

FGameplayTag UCombatResolverComponent::ResolveSlot() const
{
	if (!Rules || !ContextComponent)
	{
		return FGameplayTag();
	}

	return Rules->ResolveSlot(ContextComponent->BuildContext());
}

UCombatAbilityDefinition* UCombatResolverComponent::ResolveAbility(FGameplayTag slot) const
{
	if (LoadoutComponent == nullptr)
	{
		return nullptr;
	}

	const UCombatLoadout* activeLoadout = LoadoutComponent->GetActiveLoadout();
	if (activeLoadout == nullptr)
	{
		return nullptr;
	}

	return activeLoadout->GetAbilityForSlot(slot);
}

void UCombatResolverComponent::HandleAttackPressed()
{
	const FGameplayTag Slot = ResolveSlot();
	const UCombatAbilityDefinition* Ability = ResolveAbility(Slot);

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

	if (bShowDebug && GEngine)
	{
		const FGameplayTag Slot = ResolveSlot();
		const UCombatAbilityDefinition* Ability = ResolveAbility(Slot);

		const FString Text = FString::Printf(TEXT("Slot: %s\nAbility: %s"),
			Slot.IsValid() ? *Slot.ToString() : TEXT("(none)"),
			Ability ? *Ability->DisplayName.ToString() : TEXT("(none)"));

		// a fixed key replaces last frame's message instead of stacking a new line every tick
		GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 0.0f, FColor::Orange, Text);
	}
}
