#include "Loadout/CombatLoadoutComponent.h"
#include "Loadout/CombatLoadout.h"

UCombatLoadoutComponent::UCombatLoadoutComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UCombatLoadout* UCombatLoadoutComponent::GetActiveLoadout() const
{
	return Loadouts.IsValidIndex(ActiveIndex) ? Loadouts[ActiveIndex] : nullptr;
}
