#include "Combat/CombatTargetingComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Combat/CombatContextComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LoadoutSwitchCombatProto.h"

UCombatTargetingComponent::UCombatTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatTargetingComponent::BeginPlay()
{
	Super::BeginPlay();

	ContextComponent = GetOwner()->FindComponentByClass<UCombatContextComponent>();
	CameraBoom = GetOwner()->FindComponentByClass<USpringArmComponent>();
	if (CameraBoom)
	{
		BaseCameraOffset = CameraBoom->SocketOffset;
	}
}

APlayerController* UCombatTargetingComponent::GetPlayerController() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	return Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
}

void UCombatTargetingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// a lock ends when its target is gone or has got too far away
	if (const AActor* Target = LockedTarget.Get())
	{
		if (FVector::Dist2D(Target->GetActorLocation(), GetOwner()->GetActorLocation()) > LockBreakDistance)
		{
			UE_LOG(LogCombatProto, Log, TEXT("LOCK BROKEN: %s out of range"), *GetNameSafe(Target));
			SetLockedTarget(nullptr);
		}
	}
	else if (!LockedTarget.IsExplicitlyNull())
	{
		SetLockedTarget(nullptr);
	}

	if (IsLocked())
	{
		ContextComponent->SetTarget(LockedTarget.Get());
		UpdateLockCamera(DeltaTime);
	}
	else
	{
		UpdateSoftTarget();
		UpdateRecentre(DeltaTime);
	}
	UpdateCameraOffset(DeltaTime);

	if (bShowDebug && IsLocked())
	{
		const AActor* Target = LockedTarget.Get();
		FVector Origin, Extent;
		Target->GetActorBounds(true, Origin, Extent);
		DrawDebugCircle(GetWorld(), Origin - FVector(0.0, 0.0, Extent.Z - 2.0), FMath::Max(Extent.X, Extent.Y) + 20.0f, 32, FColor::Red,
			false, -1.0f, 0, 3.0f, FVector::ForwardVector, FVector::RightVector, false);
		DrawDebugString(GetWorld(), Origin + FVector(0.0, 0.0, Extent.Z + 70.0), TEXT("LOCKED"), nullptr, FColor::Red, 0.0f, true, 1.2f);
	}
}

void UCombatTargetingComponent::ToggleLock()
{
	if (IsLocked())
	{
		UE_LOG(LogCombatProto, Log, TEXT("LOCK OFF: %s"), *GetNameSafe(LockedTarget.Get()));
		SetLockedTarget(nullptr);
		return;
	}

	// lock onto what soft targeting already picked: the press goes where the reticle is
	AActor* Target = ContextComponent ? ContextComponent->GetTarget() : nullptr;
	if (Target)
	{
		UE_LOG(LogCombatProto, Log, TEXT("LOCK ON: %s"), *GetNameSafe(Target));
		SetLockedTarget(Target);
		return;
	}

	// nothing to lock onto: swing the camera behind the character instead (Dark Souls)
	bRecentring = true;
	RecentreYaw = GetOwner()->GetActorRotation().Yaw;
	UE_LOG(LogCombatProto, Log, TEXT("LOCK: no target, recentring camera"));
}

void UCombatTargetingComponent::SetLockedTarget(AActor* NewTarget)
{
	LockedTarget = NewTarget;
	bRecentring = false;
	bStickArmed = false;    // a stick held during the lock press shouldn't switch straight away
	MouseSwipe = 0.0f;
}

void UCombatTargetingComponent::HandleStickInput(const FVector2D& Stick)
{
	const float Amount = Stick.Size();
	const double Now = GetWorld()->GetTimeSeconds();

	// back near the middle: arm the next switch, and start the flick clock when it leaves again
	if (Amount <= SwitchStickRearm)
	{
		bStickArmed = true;
		bStickAtRest = true;
		return;
	}
	if (bStickAtRest)
	{
		bStickAtRest = false;
		StickLeftRestTime = Now;
	}

	if (!IsLocked() || !bStickArmed || Amount < SwitchStickThreshold)
	{
		return;
	}

	// pushed to the edge, but slowly: not a flick, so it doesn't count (and won't until the stick goes back to rest)
	bStickArmed = false;
	if (SwitchFlickTime > 0.0f && Now - StickLeftRestTime > SwitchFlickTime)
	{
		UE_LOG(LogCombatProto, Verbose, TEXT("LOCK SWITCH ignored: stick pushed too slowly (%.2f s)"), Now - StickLeftRestTime);
		return;
	}

	// stick up is screen up, which is screen Y down
	SwitchTarget(FVector2D(Stick.X, -Stick.Y));
}

void UCombatTargetingComponent::HandleMouseInput(const FVector2D& MouseDelta)
{
	if (!IsLocked())
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - MouseSwipeStartTime > SwitchMouseWindow || FMath::Sign(MouseDelta.X) != FMath::Sign(MouseSwipe))
	{
		MouseSwipe = 0.0f;
		MouseSwipeStartTime = Now;
	}
	MouseSwipe += MouseDelta.X;

	if (FMath::Abs(MouseSwipe) >= SwitchMouseDistance)
	{
		SwitchTarget(FVector2D(FMath::Sign(MouseSwipe), 0.0f));
		MouseSwipe = 0.0f;
		// the next swipe has to start fresh
		MouseSwipeStartTime = Now + SwitchMouseWindow;
	}
}

void UCombatTargetingComponent::SwitchTarget(const FVector2D& ScreenDirection)
{
	if (AActor* NewTarget = FindSwitchTarget(ScreenDirection))
	{
		UE_LOG(LogCombatProto, Log, TEXT("LOCK SWITCH: %s -> %s"), *GetNameSafe(LockedTarget.Get()), *GetNameSafe(NewTarget));
		LockedTarget = NewTarget;
	}
	else
	{
		UE_LOG(LogCombatProto, Log, TEXT("LOCK SWITCH: nothing that way"));
	}
}

AActor* UCombatTargetingComponent::FindSwitchTarget(const FVector2D& ScreenDirection) const
{
	const APlayerController* PlayerController = GetPlayerController();
	const AActor* Current = LockedTarget.Get();
	if (!PlayerController || !Current || ScreenDirection.IsNearlyZero())
	{
		return nullptr;
	}

	FVector2D CurrentScreen;
	if (!PlayerController->ProjectWorldLocationToScreen(Current->GetActorLocation(), CurrentScreen, true))
	{
		return nullptr;
	}

	int32 ViewportX = 1, ViewportY = 1;
	PlayerController->GetViewportSize(ViewportX, ViewportY);
	const float ScreenSize = FMath::Max(1, FMath::Max(ViewportX, ViewportY));
	const FVector2D Direction = ScreenDirection.GetSafeNormal();

	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsWithTag(this, TEXT("Enemy"), Enemies);

	AActor* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	for (AActor* Enemy : Enemies)
	{
		if (Enemy == Current || FVector::Dist2D(Enemy->GetActorLocation(), GetOwner()->GetActorLocation()) > LockBreakDistance)
		{
			continue;
		}

		// behind the camera doesn't project; it isn't "that way" on screen anyway
		FVector2D EnemyScreen;
		if (!PlayerController->ProjectWorldLocationToScreen(Enemy->GetActorLocation(), EnemyScreen, true))
		{
			continue;
		}

		const FVector2D Offset = EnemyScreen - CurrentScreen;
		const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector2D::DotProduct(Offset.GetSafeNormal(), Direction), -1.0f, 1.0f)));
		if (Offset.IsNearlyZero() || Angle > SwitchMaxAngle)
		{
			continue;
		}

		// closest to the flick direction first, then nearest on screen
		const float Score = Angle / SwitchMaxAngle + Offset.Size() / ScreenSize;
		if (Score < BestScore)
		{
			BestScore = Score;
			Best = Enemy;
		}
	}
	return Best;
}

void UCombatTargetingComponent::UpdateLockCamera(float DeltaTime)
{
	APlayerController* PlayerController = GetPlayerController();
	const AActor* Target = LockedTarget.Get();
	if (!PlayerController || !PlayerController->PlayerCameraManager || !Target)
	{
		return;
	}

	// sideways: face a point between the player and the target so both stay in frame; 1 would centre the target
	const FVector PlayerLocation = GetOwner()->GetActorLocation();
	const FVector TargetLocation = Target->GetActorLocation();
	const FVector Focus = FMath::Lerp(PlayerLocation, TargetLocation, LockFocusBlend);
	const FVector CameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
	const float Yaw = (Focus - CameraLocation).Rotation().Yaw;

	// tilt: not from the aim point, which gets close to the camera as the player closes in and tilts it sharply. A base tilt,
	// plus some of the target's height above or below the player, measured as if it were at least LockPitchMinDistance away
	const FVector ToTarget = TargetLocation - PlayerLocation;
	const float Elevation = FMath::RadiansToDegrees(FMath::Atan2(ToTarget.Z, FMath::Max(ToTarget.Size2D(), LockPitchMinDistance)));
	const float Pitch = FMath::Clamp(LockPitchOffset + Elevation * LockPitchFollow, LockPitchMin, LockPitchMax);

	const FRotator Desired(Pitch, Yaw, 0.0);

	const FRotator Current = PlayerController->GetControlRotation();
	PlayerController->SetControlRotation(FMath::RInterpTo(Current, Desired, DeltaTime, LockCameraSpeed));
}

void UCombatTargetingComponent::UpdateCameraOffset(float DeltaTime)
{
	if (!CameraBoom)
	{
		return;
	}
	const FVector Wanted = IsLocked() ? LockCameraOffset : BaseCameraOffset;
	CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, Wanted, DeltaTime, LockOffsetSpeed);
}

void UCombatTargetingComponent::UpdateRecentre(float DeltaTime)
{
	APlayerController* PlayerController = bRecentring ? GetPlayerController() : nullptr;
	if (!PlayerController)
	{
		bRecentring = false;
		return;
	}

	FRotator Rotation = PlayerController->GetControlRotation();
	const float Remaining = FMath::FindDeltaAngleDegrees(Rotation.Yaw, RecentreYaw);
	const float Step = RecentreSpeed * DeltaTime;
	if (FMath::Abs(Remaining) <= Step)
	{
		Rotation.Yaw = RecentreYaw;
		bRecentring = false;
	}
	else
	{
		Rotation.Yaw += FMath::Sign(Remaining) * Step;
	}
	PlayerController->SetControlRotation(Rotation);
}

void UCombatTargetingComponent::UpdateSoftTarget()
{
	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsWithTag(this, TEXT("Enemy"), Enemies);

	// where the camera looks, flattened: an enemy above or below the view line isn't "off to the side"
	const APlayerController* PlayerController = GetPlayerController();
	const FRotator ViewRotation = PlayerController ? PlayerController->GetControlRotation() : GetOwner()->GetActorRotation();
	const FVector ViewForward = FRotator(0.0, ViewRotation.Yaw, 0.0).Vector();

	const float Unreachable = TNumericLimits<float>::Max();
	AActor* CurrentTarget = ContextComponent->GetTarget();
	float CurrentScore = Unreachable;
	AActor* BestTarget = nullptr;
	float BestScore = Unreachable;

	for (AActor* Enemy : Enemies)
	{
		const float Score = ScoreCandidate(Enemy, ViewForward);
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

	ContextComponent->SetTarget(BestTarget);
}

float UCombatTargetingComponent::ScoreCandidate(const AActor* Candidate, const FVector& ViewForward) const
{
	// horizontal, like the context component's range bands
	const FVector ToCandidate = Candidate->GetActorLocation() - GetOwner()->GetActorLocation();
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
