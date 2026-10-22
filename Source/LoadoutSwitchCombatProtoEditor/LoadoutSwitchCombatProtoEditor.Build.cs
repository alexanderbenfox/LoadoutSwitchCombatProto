using UnrealBuildTool;

/** Editor-only tooling for the game module: viewport visualizers and the like. Never part of a packaged game */
public class LoadoutSwitchCombatProtoEditor : ModuleRules
{
	public LoadoutSwitchCombatProtoEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"LoadoutSwitchCombatProto",
			"UnrealEd"
		});

		PublicIncludePaths.Add(ModuleDirectory);
	}
}
