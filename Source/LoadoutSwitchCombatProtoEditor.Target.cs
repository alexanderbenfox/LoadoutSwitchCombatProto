using UnrealBuildTool;
using System.Collections.Generic;

public class LoadoutSwitchCombatProtoEditorTarget : TargetRules
{
	public LoadoutSwitchCombatProtoEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("LoadoutSwitchCombatProto");
		ExtraModuleNames.Add("LoadoutSwitchCombatProtoEditor");
	}
}
