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
	}
}
