#include "Character/CombatCharacter.h"
#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Combat/CombatContextComponent.h"
#include "Combat/CombatResolverComponent.h"
#include "Combat/CombatTargetingComponent.h"
#include "Combat/CombatWeaponComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Input/CombatInputUserSettings.h"
#include "InputActionValue.h"
#include "Loadout/CombatLoadoutComponent.h"
#include "LoadoutSwitchCombatProto.h"
#include "MotionWarpingComponent.h"

ACombatCharacter::ACombatCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	MotionWarping = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));
	CombatContext = CreateDefaultSubobject<UCombatContextComponent>(TEXT("CombatContext"));
	CombatLoadout = CreateDefaultSubobject<UCombatLoadoutComponent>(TEXT("CombatLoadout"));
	CombatResolver = CreateDefaultSubobject<UCombatResolverComponent>(TEXT("CombatResolver"));
	Targeting = CreateDefaultSubobject<UCombatTargetingComponent>(TEXT("Targeting"));

	Weapon = CreateDefaultSubobject<UCombatWeaponComponent>(TEXT("Weapon"));
	Weapon->SetupAttachment(GetMesh(), TEXT("weapon_r_socket"));
}

void ACombatCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateFollowCamera(DeltaSeconds);
}

void ACombatCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// the ability system needs to know who owns it and which actor it acts through; here both are this character
	AbilitySystem->InitAbilityActorInfo(this, this);
}

void ACombatCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		// the camera manager clamps the view pitch every frame, whatever turned it
		if (APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager)
		{
			CameraManager->ViewPitchMin = CameraPitchMin;
			CameraManager->ViewPitchMax = CameraPitchMax;
		}

		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			UEnhancedInputUserSettings* UserSettings = Subsystem->GetUserSettings();
			for (const UInputMappingContext* MappingContext : DefaultMappingContexts)
			{
				// registering makes the context's rebindable keys show up in the player's settings, with their saved rebinds applied
				if (UserSettings)
				{
					UserSettings->RegisterInputMappingContext(MappingContext);
				}
				Subsystem->AddMappingContext(MappingContext, 0);
			}
		}
	}
}

void ACombatCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACombatCharacter::Move);

		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACombatCharacter::StickLook);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Completed, this, &ACombatCharacter::StickLookReleased);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ACombatCharacter::MouseLook);

		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ACombatCharacter::Attack);
		EnhancedInputComponent->BindAction(LockOnAction, ETriggerEvent::Started, this, &ACombatCharacter::ToggleLockOn);
	}
	else
	{
		UE_LOG(LogCombatProto, Error, TEXT("'%s' failed to find an Enhanced Input component"), *GetNameSafe(this));
	}
}

bool ACombatCharacter::CanJumpInternal_Implementation() const
{
	return Super::CanJumpInternal_Implementation() && CombatResolver->CanJumpNow();
}

void ACombatCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	CombatResolver->CancelAttack();
}

void ACombatCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	CombatResolver->HandleLanded();
}

void ACombatCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	// moving out of an attack's recovery stops it
	if (!MovementVector.IsNearlyZero())
	{
		CombatResolver->HandleMovementInput();
	}

	if (GetController() != nullptr)
	{
		// move relative to where the camera is facing, ignoring its pitch
		const FRotator YawRotation(0, GetController()->GetControlRotation().Yaw, 0);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ACombatCharacter::StickLook(const FInputActionValue& Value)
{
	const FVector2D Stick = Value.Get<FVector2D>();

	Targeting->HandleStickInput(Stick);
	if (Targeting->IsLocked())
	{
		return;
	}
	Targeting->NotifyManualLook();

	const UCombatInputUserSettings* Settings = UCombatInputUserSettings::Get(Cast<APlayerController>(GetController()));
	const float Sensitivity = Settings ? Settings->StickLookSensitivity : 1.0f;
	const float InvertY = Settings && Settings->bInvertStickY ? -1.0f : 1.0f;
	const float DeltaSeconds = GetWorld()->GetDeltaSeconds();
	LastManualLookTime = GetWorld()->GetTimeSeconds();

	if (GetController() != nullptr)
	{
		AddControllerYawInput(Stick.X * StickYawRate * Sensitivity * DeltaSeconds);
		AddControllerPitchInput(Stick.Y * StickPitchRate * Sensitivity * InvertY * DeltaSeconds);
	}
}

void ACombatCharacter::StickLookReleased(const FInputActionValue& Value)
{
	Targeting->HandleStickInput(FVector2D::ZeroVector);
}

void ACombatCharacter::MouseLook(const FInputActionValue& Value)
{
	const FVector2D Mouse = Value.Get<FVector2D>();

	if (Targeting->IsLocked())
	{
		Targeting->HandleMouseInput(Mouse);
		return;
	}
	Targeting->NotifyManualLook();

	const UCombatInputUserSettings* Settings = UCombatInputUserSettings::Get(Cast<APlayerController>(GetController()));
	const float Sensitivity = Settings ? Settings->MouseLookSensitivity : 1.0f;
	const float InvertY = Settings && Settings->bInvertMouseY ? -1.0f : 1.0f;
	LastManualLookTime = GetWorld()->GetTimeSeconds();

	if (GetController() != nullptr)
	{
		AddControllerYawInput(Mouse.X * MouseLookScale * Sensitivity);
		AddControllerPitchInput(Mouse.Y * MouseLookScale * Sensitivity * InvertY);
	}
}

void ACombatCharacter::UpdateFollowCamera(float DeltaSeconds)
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || FollowRate <= 0.0f || Targeting->IsDrivingCamera())
	{
		return;
	}

	const UCombatInputUserSettings* Settings = UCombatInputUserSettings::Get(PlayerController);
	if (Settings && !Settings->bCameraFollow)
	{
		return;
	}

	// the player steering the camera always wins; the follow only comes back after they've let go for a moment
	const double SinceManualLook = GetWorld()->GetTimeSeconds() - LastManualLookTime;
	if (SinceManualLook < FollowDelay)
	{
		return;
	}

	// driven by the movement stick (the acceleration it asks for), not velocity, so lunges and knockback don't swing the camera
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	const FVector Acceleration = Movement->GetCurrentAcceleration();
	const float StickAmount = Movement->GetMaxAcceleration() > 0.0f ? FMath::Clamp(Acceleration.Size2D() / Movement->GetMaxAcceleration(), 0.0f, 1.0f) : 0.0f;
	if (StickAmount <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	// how far round the movement is from where the camera points: positive is to the right
	const float CameraYaw = PlayerController->GetControlRotation().Yaw;
	const float OffAngle = FMath::FindDeltaAngleDegrees(CameraYaw, Acceleration.Rotation().Yaw);

	// sideways movement turns the camera most; straight away from it, not at all
	float Sideways = FMath::Sin(FMath::DegreesToRadians(OffAngle));
	if (FMath::Abs(OffAngle) > 90.0f)
	{
		Sideways *= FollowTowardsCameraScale;
	}

	const float Ramp = FollowRampTime > 0.0f ? FMath::Clamp(static_cast<float>(SinceManualLook - FollowDelay) / FollowRampTime, 0.0f, 1.0f) : 1.0f;
	AddControllerYawInput(Sideways * FollowRate * StickAmount * Ramp * DeltaSeconds);
}

void ACombatCharacter::Attack()
{
	CombatResolver->HandleAttackPressed();
}

void ACombatCharacter::ToggleLockOn()
{
	Targeting->ToggleLock();
}
