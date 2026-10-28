#include "Character/CombatCharacter.h"
#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "Combat/CombatContextComponent.h"
#include "Combat/CombatResolverComponent.h"
#include "Combat/CombatWeaponComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
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

	Weapon = CreateDefaultSubobject<UCombatWeaponComponent>(TEXT("Weapon"));
	Weapon->SetupAttachment(GetMesh(), TEXT("weapon_r_socket"));
}

void ACombatCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateSoftTarget();
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
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			for (const UInputMappingContext* MappingContext : DefaultMappingContexts)
			{
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

		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACombatCharacter::Look);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ACombatCharacter::Look);

		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ACombatCharacter::Attack);
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

void ACombatCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (GetController() != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ACombatCharacter::Attack()
{
	CombatResolver->HandleAttackPressed();
}

void ACombatCharacter::UpdateSoftTarget()
{
	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsWithTag(this, TEXT("Enemy"), Enemies);

	// where the camera looks, flattened: an enemy above or below the view line isn't "off to the side"
	const FRotator ViewRotation = Controller ? Controller->GetControlRotation() : GetActorRotation();
	const FVector ViewForward = FRotator(0.0, ViewRotation.Yaw, 0.0).Vector();

	const float Unreachable = TNumericLimits<float>::Max();
	AActor* CurrentTarget = CombatContext->GetTarget();
	float CurrentScore = Unreachable;
	AActor* BestTarget = nullptr;
	float BestScore = Unreachable;

	for (AActor* Enemy : Enemies)
	{
		const float Score = ScoreTargetCandidate(Enemy, ViewForward);
		if (Enemy == CurrentTarget)
		{
			CurrentScore = Score;
		}
		if (Score < BestScore)
		{
			BestScore = Score;
			BestTarget = Enemy;
		}
	}

	// keep the current target unless the best one is clearly better; a destroyed or out-of-range target scores Unreachable and always loses
	if (CurrentScore < Unreachable && BestScore > CurrentScore - TargetSwitchMargin)
	{
		BestTarget = CurrentTarget;
	}

	CombatContext->SetTarget(BestTarget);
}

float ACombatCharacter::ScoreTargetCandidate(const AActor* Candidate, const FVector& ViewForward) const
{
	// horizontal, like the context component's range bands
	const FVector ToCandidate = Candidate->GetActorLocation() - GetActorLocation();
	const float Distance = ToCandidate.Size2D();
	if (Distance > TargetMaxDistance)
	{
		return TNumericLimits<float>::Max();
	}

	// 0 straight ahead, 0.5 square to the side, 1 directly behind; an enemy on top of the player counts as ahead
	const float AngleCost = Distance > KINDA_SMALL_NUMBER
		? (1.0f - FVector::DotProduct(ToCandidate.GetSafeNormal2D(), ViewForward)) * 0.5f
		: 0.0f;

	return Distance / TargetMaxDistance + TargetAngleWeight * AngleCost;
}
