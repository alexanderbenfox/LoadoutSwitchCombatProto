#pragma once

#include "NativeGameplayTags.h"

/** Gameplay tags referenced from C++. Defined natively so a typo is a compile error, not a silent mismatch. */
namespace CombatTags
{
	// Context tags read by the resolver (design doc, "Context system")
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Context_Stance_Ground);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Context_Stance_Air);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Context_Range_Close);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Context_Range_Far);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Context_Target_Single);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Context_Target_Crowd);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Context_Target_Airborne);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Context_Recent_Dodge);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Context_Recent_Swap);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Context_Combo_Finisher);

	// Enemy poise tiers (design doc, "Stagger tiers")
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Poise_Unarmored);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Poise_Armored);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Poise_Juggernaut);
}
