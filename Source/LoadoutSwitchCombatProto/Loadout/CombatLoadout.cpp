#include "Loadout/CombatLoadout.h"
#include "Loadout/CombatAbilityDefinition.h"

UCombatAbilityDefinition* UCombatLoadout::GetAbilityForSlot(FGameplayTag Slot) const
{
	const TObjectPtr<UCombatAbilityDefinition>* slotDefinition = SlotAbilities.Find(Slot);
	if ( slotDefinition && *slotDefinition )
	{
		return *slotDefinition;
	}

	return BasicAttack;
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
