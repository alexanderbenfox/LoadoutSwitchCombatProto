#include "Combat/CombatTags.h"

namespace CombatTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Context_Stance_Ground, "Context.Stance.Ground", "Player is grounded");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Context_Stance_Air, "Context.Stance.Air", "Player is airborne");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Context_Range_Close, "Context.Range.Close", "Target within close band");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Context_Range_Far, "Context.Range.Far", "Target in far band");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Context_Target_Single, "Context.Target.Single", "Fewer than crowd threshold near target");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Context_Target_Crowd, "Context.Target.Crowd", "Crowd threshold met near target");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Context_Target_Airborne, "Context.Target.Airborne", "Target is airborne (Riser)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Context_Recent_Dodge, "Context.Recent.Dodge", "Inside post-dodge window");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Context_Recent_Swap, "Context.Recent.Swap", "Inside swap-in window");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Context_Combo_Finisher, "Context.Combo.Finisher", "Next hit is the combo's last");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_Poise_Unarmored, "Enemy.Poise.Unarmored", "Staggered by everything");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_Poise_Armored, "Enemy.Poise.Armored", "Ignores light hits");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_Poise_Juggernaut, "Enemy.Poise.Juggernaut", "Only charged shots stagger");
}
