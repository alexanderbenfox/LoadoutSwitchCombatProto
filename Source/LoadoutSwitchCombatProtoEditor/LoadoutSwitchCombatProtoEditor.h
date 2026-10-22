#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

/** Registers the game's component visualizers with the editor on startup, and removes them on shutdown */
class FLoadoutSwitchCombatProtoEditorModule : public IModuleInterface
{
public:

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
