#include "LoadoutSwitchCombatProtoEditor.h"
#include "Combat/CombatWeaponComponent.h"
#include "CombatWeaponComponentVisualizer.h"
#include "Editor/UnrealEdEngine.h"
#include "UnrealEdGlobals.h"

IMPLEMENT_MODULE(FLoadoutSwitchCombatProtoEditorModule, LoadoutSwitchCombatProtoEditor)

void FLoadoutSwitchCombatProtoEditorModule::StartupModule()
{
	// GUnrealEd exists from PostEngineInit, the loading phase this module uses (see the .uproject)
	if (GUnrealEd)
	{
		GUnrealEd->RegisterComponentVisualizer(UCombatWeaponComponent::StaticClass()->GetFName(), MakeShared<FCombatWeaponComponentVisualizer>());
	}
}

void FLoadoutSwitchCombatProtoEditorModule::ShutdownModule()
{
	if (GUnrealEd)
	{
		GUnrealEd->UnregisterComponentVisualizer(UCombatWeaponComponent::StaticClass()->GetFName());
	}
}
