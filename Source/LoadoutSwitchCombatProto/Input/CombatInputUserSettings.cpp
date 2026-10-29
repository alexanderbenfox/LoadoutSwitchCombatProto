#include "Input/CombatInputUserSettings.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedActionKeyMapping.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "LoadoutSwitchCombatProto.h"

UCombatInputUserSettings* UCombatInputUserSettings::Get(const APlayerController* PlayerController)
{
	const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	const UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
	return Subsystem ? Subsystem->GetUserSettings<UCombatInputUserSettings>() : nullptr;
}

FHardwareDeviceIdentifier UCombatInputUserSettings::DetermineHardwareDeviceForActionMapping(const FEnhancedActionKeyMapping& ActionMapping, const UInputMappingContext* IMC) const
{
	return ActionMapping.Key.IsGamepadKey() ? FHardwareDeviceIdentifier::DefaultGamepad : FHardwareDeviceIdentifier::DefaultKeyboardAndMouse;
}

// console commands for testing rebinds before there's a menu; they act on the first local player
namespace
{
	UCombatInputUserSettings* GetSettingsForCommand(const UWorld* World)
	{
		UCombatInputUserSettings* Settings = World ? UCombatInputUserSettings::Get(World->GetFirstPlayerController()) : nullptr;
		if (!Settings)
		{
			UE_LOG(LogCombatProto, Warning, TEXT("Combat.Input: no input user settings (start PIE, and check Enable User Settings and the User Settings Class in Project Settings > Enhanced Input)"));
		}
		return Settings;
	}

	void ApplyAndSave(UCombatInputUserSettings* Settings)
	{
		Settings->ApplySettings();
		Settings->SaveSettings();
	}

	const TCHAR* DeviceLabel(const FHardwareDeviceIdentifier& Device)
	{
		return Device == FHardwareDeviceIdentifier::DefaultGamepad ? TEXT("pad") : TEXT("kbm");
	}

	void ListBindings(const TArray<FString>& Args, UWorld* World)
	{
		UCombatInputUserSettings* Settings = GetSettingsForCommand(World);
		const UEnhancedPlayerMappableKeyProfile* Profile = Settings ? Settings->GetActiveKeyProfile() : nullptr;
		if (!Profile)
		{
			return;
		}

		TArray<FString> ProfileIds;
		Settings->GetAllAvailableKeyProfiles().GetKeys(ProfileIds);
		UE_LOG(LogCombatProto, Display, TEXT("Combat.Input: profile '%s' (available: %s)"), *Profile->GetProfileIdString(), *FString::Join(ProfileIds, TEXT(", ")));

		TArray<FName> MappingNames;
		Profile->GetPlayerMappingRows().GetKeys(MappingNames);
		MappingNames.Sort(FNameLexicalLess());
		for (const FName& MappingName : MappingNames)
		{
			TArray<FString> Keys;
			for (const FPlayerKeyMapping& Mapping : Profile->GetPlayerMappingRows()[MappingName].Mappings)
			{
				Keys.Add(FString::Printf(TEXT("%s %d: %s%s"), DeviceLabel(Mapping.GetHardwareDeviceId()), static_cast<int32>(Mapping.GetSlot()) + 1,
					*Mapping.GetCurrentKey().ToString(), Mapping.IsCustomized() ? *FString::Printf(TEXT(" (default %s)"), *Mapping.GetDefaultKey().ToString()) : TEXT("")));
			}
			Keys.Sort();
			UE_LOG(LogCombatProto, Display, TEXT("  %-14s %s"), *MappingName.ToString(), *FString::Join(Keys, TEXT("   ")));
		}
		UE_LOG(LogCombatProto, Display, TEXT("Look: stick x%.2f%s, mouse x%.2f%s, follow camera %s"), Settings->StickLookSensitivity, Settings->bInvertStickY ? TEXT(" inverted") : TEXT(""),
			Settings->MouseLookSensitivity, Settings->bInvertMouseY ? TEXT(" inverted") : TEXT(""), Settings->bCameraFollow ? TEXT("on") : TEXT("off"));
	}

	void RebindKey(const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() < 2)
		{
			UE_LOG(LogCombatProto, Warning, TEXT("Usage: Combat.Input.Rebind <Mapping> <Key> [Slot]  e.g. Combat.Input.Rebind Attack Gamepad_RightShoulder. The key decides pad or keyboard; slot defaults to 1"));
			return;
		}
		UCombatInputUserSettings* Settings = GetSettingsForCommand(World);
		if (!Settings)
		{
			return;
		}

		const FKey NewKey(*Args[1]);
		if (!NewKey.IsValid())
		{
			UE_LOG(LogCombatProto, Warning, TEXT("Combat.Input.Rebind: '%s' isn't a key name (they look like SpaceBar, LeftMouseButton, Gamepad_FaceButton_Left)"), *Args[1]);
			return;
		}

		const FHardwareDeviceIdentifier& Device = NewKey.IsGamepadKey() ? FHardwareDeviceIdentifier::DefaultGamepad : FHardwareDeviceIdentifier::DefaultKeyboardAndMouse;
		const int32 SlotNumber = Args.Num() > 2 ? FMath::Clamp(FCString::Atoi(*Args[2]), 1, 7) : 1;

		// a key already on another action keeps working for both; say so rather than quietly doubling up
		TArray<FName> Existing;
		if (const UEnhancedPlayerMappableKeyProfile* Profile = Settings->GetActiveKeyProfile())
		{
			Profile->GetMappingNamesForKey(NewKey, Existing);
			Existing.Remove(FName(*Args[0]));
		}

		FMapPlayerKeyArgs MapArgs;
		MapArgs.MappingName = FName(*Args[0]);
		MapArgs.NewKey = NewKey;
		MapArgs.Slot = static_cast<EPlayerMappableKeySlot>(SlotNumber - 1);
		MapArgs.HardwareDeviceId = Device.HardwareDeviceIdentifier;
		MapArgs.bCreateMatchingSlotIfNeeded = true;

		FGameplayTagContainer Failure;
		Settings->MapPlayerKey(MapArgs, Failure);
		if (!Failure.IsEmpty())
		{
			UE_LOG(LogCombatProto, Warning, TEXT("Combat.Input.Rebind failed: %s (Combat.Input.List shows the mapping names)"), *Failure.ToStringSimple());
			return;
		}

		ApplyAndSave(Settings);
		UE_LOG(LogCombatProto, Display, TEXT("Combat.Input: %s %s %d -> %s%s"), *Args[0], DeviceLabel(Device), SlotNumber, *NewKey.ToString(),
			Existing.IsEmpty() ? TEXT("") : *FString::Printf(TEXT("  (also bound to %s)"), *FString::JoinBy(Existing, TEXT(", "), [](const FName& Name) { return Name.ToString(); })));
	}

	void ResetBindings(const TArray<FString>& Args, UWorld* World)
	{
		UCombatInputUserSettings* Settings = GetSettingsForCommand(World);
		UEnhancedPlayerMappableKeyProfile* Profile = Settings ? Settings->GetActiveKeyProfile() : nullptr;
		if (!Profile)
		{
			return;
		}

		if (Args.IsEmpty())
		{
			Profile->ResetToDefault();
		}
		else
		{
			Profile->ResetMappingToDefault(FName(*Args[0]));
		}
		ApplyAndSave(Settings);
		UE_LOG(LogCombatProto, Display, TEXT("Combat.Input: reset %s to defaults"), Args.IsEmpty() ? TEXT("every binding") : *Args[0]);
	}

	void SetProfile(const TArray<FString>& Args, UWorld* World)
	{
		UCombatInputUserSettings* Settings = GetSettingsForCommand(World);
		if (!Settings)
		{
			return;
		}
		if (Args.IsEmpty())
		{
			ListBindings(Args, World);
			return;
		}
		// Enhanced Input names the built-in profile with a tag-like id
		const FString ProfileId = Args[0] == TEXT("Default") ? TEXT("InputUserSettings.Profiles.Default") : Args[0];
		if (!Settings->SetActiveKeyProfile(ProfileId))
		{
			UE_LOG(LogCombatProto, Warning, TEXT("Combat.Input.Profile: no profile '%s' (Combat.Input.List shows the available ones)"), *Args[0]);
			return;
		}
		ApplyAndSave(Settings);
		UE_LOG(LogCombatProto, Display, TEXT("Combat.Input: profile '%s'"), *ProfileId);
	}

	void SetLook(const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() < 2 || (Args[0] != TEXT("stick") && Args[0] != TEXT("mouse")))
		{
			UE_LOG(LogCombatProto, Warning, TEXT("Usage: Combat.Input.Look <stick|mouse> <Sensitivity> [InvertY 0|1]  e.g. Combat.Input.Look stick 1.5 1"));
			return;
		}
		UCombatInputUserSettings* Settings = GetSettingsForCommand(World);
		if (!Settings)
		{
			return;
		}

		const bool bStick = Args[0] == TEXT("stick");
		float& Sensitivity = bStick ? Settings->StickLookSensitivity : Settings->MouseLookSensitivity;
		bool& bInvert = bStick ? Settings->bInvertStickY : Settings->bInvertMouseY;
		Sensitivity = FMath::Clamp(FCString::Atof(*Args[1]), 0.1f, 5.0f);
		if (Args.Num() > 2)
		{
			bInvert = FCString::ToBool(*Args[2]);
		}
		ApplyAndSave(Settings);
		UE_LOG(LogCombatProto, Display, TEXT("Combat.Input: %s look x%.2f%s"), *Args[0], Sensitivity, bInvert ? TEXT(" inverted") : TEXT(""));
	}

	void SetCameraFollow(const TArray<FString>& Args, UWorld* World)
	{
		UCombatInputUserSettings* Settings = GetSettingsForCommand(World);
		if (!Settings)
		{
			return;
		}
		Settings->bCameraFollow = Args.IsEmpty() ? !Settings->bCameraFollow : FCString::ToBool(*Args[0]);
		ApplyAndSave(Settings);
		UE_LOG(LogCombatProto, Display, TEXT("Combat.Input: follow camera %s"), Settings->bCameraFollow ? TEXT("on") : TEXT("off"));
	}

	FAutoConsoleCommandWithWorldAndArgs FollowCommand(TEXT("Combat.Input.CameraFollow"),
		TEXT("Combat.Input.CameraFollow [0|1]: turns the follow camera off or on (no argument toggles). Saved"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SetCameraFollow));

	FAutoConsoleCommandWithWorldAndArgs ListCommand(TEXT("Combat.Input.List"),
		TEXT("Lists every rebindable action with its keyboard/mouse and pad keys, the active profile and the look settings"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ListBindings));

	FAutoConsoleCommandWithWorldAndArgs RebindCommand(TEXT("Combat.Input.Rebind"),
		TEXT("Combat.Input.Rebind <Mapping> <Key> [Slot]: binds a key to an action; a pad key rebinds the pad binding, any other key the keyboard/mouse one. Saved"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RebindKey));

	FAutoConsoleCommandWithWorldAndArgs ResetCommand(TEXT("Combat.Input.Reset"),
		TEXT("Combat.Input.Reset [Mapping]: puts one action, or every action, back on its default keys. Saved"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ResetBindings));

	FAutoConsoleCommandWithWorldAndArgs ProfileCommand(TEXT("Combat.Input.Profile"),
		TEXT("Combat.Input.Profile <Id>: switches the control preset, e.g. ShoulderAttack or Default. Saved"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SetProfile));

	FAutoConsoleCommandWithWorldAndArgs LookCommand(TEXT("Combat.Input.Look"),
		TEXT("Combat.Input.Look <stick|mouse> <Sensitivity> [InvertY 0|1]: camera speed and inverted Y. Saved"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SetLook));
}
