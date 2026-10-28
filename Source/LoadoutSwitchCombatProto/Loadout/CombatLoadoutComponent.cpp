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

int32 UCombatLoadoutComponent::GetComboLength(bool bAir) const
{
	const UCombatLoadout* Loadout = GetActiveLoadout();
	return Loadout ? Loadout->GetComboLength(bAir) : 0;
}

int32 UCombatLoadoutComponent::GetNextComboIndex(bool bAir) const
{
	// the length comes from the loadout active now, so after a swap the string finishes on the new loadout's length
	const int32 Length = GetComboLength(bAir);
	const bool bStringGoing = ComboCount > 0 && bComboInAir == bAir && (Length <= 0 || ComboCount < Length);
	return bStringGoing ? ComboCount + 1 : 1;
}

bool UCombatLoadoutComponent::IsNextAttackFinisher(bool bAir) const
{
	const int32 Length = GetComboLength(bAir);
	return Length > 0 && GetNextComboIndex(bAir) == Length;
}

int32 UCombatLoadoutComponent::AdvanceCombo(bool bAir)
{
	ComboCount = GetNextComboIndex(bAir);
	bComboInAir = bAir;
	return ComboCount;
}

void UCombatLoadoutComponent::ResetCombo()
{
	ComboCount = 0;
}
