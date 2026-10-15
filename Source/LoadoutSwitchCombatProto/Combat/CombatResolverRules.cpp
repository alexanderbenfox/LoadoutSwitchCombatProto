#include "Combat/CombatResolverRules.h"

FGameplayTag UCombatResolverRules::ResolveSlot(const FGameplayTagContainer& Context) const
{
	// walk Rules in order and return the Slot of the first rule the context satisfies
	for (int32 i = 0; i < Rules.Num(); i++)
	{
		const FCombatSlotRule& Rule = Rules[i];
		if (Rule.Requirement.Matches(Context))
		{
			return Rule.Slot;
		}
	}

	return FallbackSlot;
}

TArray<FGameplayTag> UCombatResolverRules::GetMatchingSlots(const FGameplayTagContainer& Context) const
{
	TArray<FGameplayTag> MatchingSlots;

	// rules array in order of priority, so output array is in order of priority
	for (int32 i = 0; i < Rules.Num(); i++)
	{
		const FCombatSlotRule& Rule = Rules[i];
		if (Rule.Requirement.Matches(Context))
		{
			MatchingSlots.Add( Rule.Slot );
		}
	}

	return MatchingSlots;
}
