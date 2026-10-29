#pragma once

#include "CoreMinimal.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "CombatInputUserSettings.generated.h"

class APlayerController;

/**
 *  The player's input settings: their key rebinds (from Enhanced Input) plus the look options, saved together to
 *  Saved/SaveGames. Set as the User Settings Class in Project Settings > Enhanced Input.
 *
 *  Keyboard-and-mouse and gamepad bindings are kept apart, so each action can be rebound on one without touching the other.
 *  Until there's a menu, the Combat.Input.* console commands list, rebind and reset them.
 */
UCLASS()
class UCombatInputUserSettings : public UEnhancedInputUserSettings
{
	GENERATED_BODY()

public:

	/** The settings of the player behind this controller, or null when user settings are off in the project */
	static UCombatInputUserSettings* Get(const APlayerController* PlayerController);

	/** Camera speed on the right stick, as a multiple of the character's base turn rates */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Look", meta = (ClampMin = 0.1, ClampMax = 5))
	float StickLookSensitivity = 1.0f;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Look")
	bool bInvertStickY = false;

	/** Camera speed with the mouse, as a multiple of the raw mouse movement */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Look", meta = (ClampMin = 0.1, ClampMax = 5))
	float MouseLookSensitivity = 1.0f;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Look")
	bool bInvertMouseY = false;

	/** The camera leans towards where the player is moving when they aren't steering it. Off, it only moves when they move it */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Look")
	bool bCameraFollow = true;

protected:

	/** Gamepad keys belong to the gamepad, every other key to keyboard and mouse; each device gets its own key slots */
	virtual FHardwareDeviceIdentifier DetermineHardwareDeviceForActionMapping(const FEnhancedActionKeyMapping& ActionMapping, const UInputMappingContext* IMC = nullptr) const override;
};
