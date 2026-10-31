#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CombatMontageLibrary.generated.h"

class UAnimMontage;
class UAnimSequence;

/** Editor helpers for building attack montages, callable from editor Python or Editor Utility Blueprints */
UCLASS()
class UCombatMontageLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 *  Builds a combo montage the way the README's combo checklist does by hand: one hit sequence after another on SlotName,
	 *  a section at the start of each (A, B, C, ...) with the links between them cleared, and Blend Out Trigger Time 0.
	 *  Creates the montage at AssetPath (e.g. /Game/Combat/Animations/Manny_AirCombo_Montage), or rebuilds the one there,
	 *  replacing its tracks and sections; its notifies are kept. Doesn't save. Returns null if Hits is empty or holds a null.
	 *  PlayRates sets each hit's playback speed (0.8 plays it at 80%, so it takes longer); a missing or non-positive entry keeps
	 *  the rate that hit's animation already had in the montage, or 1.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Editor", meta = (AutoCreateRefTerm = "PlayRates"))
	static UAnimMontage* BuildComboMontage(const FString& AssetPath, const TArray<UAnimSequence*>& Hits, FName SlotName, const TArray<float>& PlayRates);
};
