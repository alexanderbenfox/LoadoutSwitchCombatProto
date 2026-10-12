#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CombatResolverRules.generated.h"

/** One line of the resolver's priority list: "if the context satisfies Requirement, use Slot" */
USTRUCT(BlueprintType)
struct FCombatSlotRule
{
	GENERATED_BODY()

	/** The slot this rule resolves to */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rule", meta = (Categories = "Slot"))
	FGameplayTag Slot;

	/** What the context has to look like for this rule to apply */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rule")
	FGameplayTagQuery Requirement;
};

/**
 *  The resolver's priority order as designer-editable data.
 *  Rules are checked top to bottom and the first one whose requirement the context satisfies wins.
 */
UCLASS(BlueprintType)
class UCombatResolverRules : public UDataAsset
{
	GENERATED_BODY()

public:

	/** Picks the slot for a context snapshot */
	UFUNCTION(BlueprintPure, Category = "Combat|Resolver")
	FGameplayTag ResolveSlot(const FGameplayTagContainer& Context) const;

protected:

	/** Highest priority first */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resolver", meta = (TitleProperty = "Slot"))
	TArray<FCombatSlotRule> Rules;

	/** Used when no rule matches: the weapon's basic attack */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resolver", meta = (Categories = "Slot"))
	FGameplayTag FallbackSlot;
};
