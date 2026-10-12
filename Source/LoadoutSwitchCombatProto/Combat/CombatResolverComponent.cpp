#include "Combat/CombatResolverComponent.h"
#include "Combat/CombatContextComponent.h"
#include "Combat/CombatResolverRules.h"
#include "Engine/Engine.h"

UCombatResolverComponent::UCombatResolverComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatResolverComponent::BeginPlay()
{
	Super::BeginPlay();

	ContextComponent = GetOwner()->FindComponentByClass<UCombatContextComponent>();
}

FGameplayTag UCombatResolverComponent::ResolveSlot() const
{
	if (!Rules || !ContextComponent)
	{
		return FGameplayTag();
	}

	return Rules->ResolveSlot(ContextComponent->BuildContext());
}

void UCombatResolverComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bShowDebug && GEngine)
	{
		const FGameplayTag Slot = ResolveSlot();
		const FString Text = FString::Printf(TEXT("Slot: %s"), Slot.IsValid() ? *Slot.ToString() : TEXT("(none)"));

		// a fixed key replaces last frame's message instead of stacking a new line every tick
		GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 0.0f, FColor::Orange, Text);
	}
}
