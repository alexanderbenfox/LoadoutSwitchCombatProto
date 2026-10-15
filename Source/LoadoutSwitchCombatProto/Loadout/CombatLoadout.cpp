#include "Loadout/CombatLoadout.h"
#include "Loadout/CombatAbilityDefinition.h"

UCombatAbilityDefinition* UCombatLoadout::GetAbilityForSlot(FGameplayTag Slot) const
{
	UCombatAbilityDefinition* assignedAbility = GetAssignedAbilityForSlot(Slot);
	if (assignedAbility)
	{
		return assignedAbility;
	}

	return BasicAttack;
}


UCombatAbilityDefinition* UCombatLoadout::GetAssignedAbilityForSlot(FGameplayTag Slot) const
{
	const TObjectPtr<UCombatAbilityDefinition>* slotDefinition = SlotAbilities.Find(Slot);
	if (slotDefinition && *slotDefinition)
	{
		return *slotDefinition;
	}

	return nullptr;
}


int32 UCombatLoadout::GetSpentAP() const
{
	int spent = 0;
	for (const auto& ability : SlotAbilities )
	{
		const TObjectPtr<UCombatAbilityDefinition> definition = ability.Value;
		if (definition != nullptr)
		{
			spent += definition->APCost;
		}
	}

	return spent;
}
