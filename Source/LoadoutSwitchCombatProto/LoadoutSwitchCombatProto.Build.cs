using UnrealBuildTool;

public class LoadoutSwitchCombatProto : ModuleRules
{
	public LoadoutSwitchCombatProto(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"MotionWarping"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		// lets files include each other by path from the module root, e.g. "Combat/CombatTags.h"
		PublicIncludePaths.Add(ModuleDirectory);
	}
}
