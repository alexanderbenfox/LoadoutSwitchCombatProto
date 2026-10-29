#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatTargetingComponent.generated.h"

class APlayerController;
class UCombatContextComponent;
class USpringArmComponent;

/**
 *  Picks what the player is fighting and hands it to the context component.
 *
 *  Soft targeting (the default): every tick, the enemy nearest the player and the camera's view wins, with a margin
 *  against flicker. Hard lock (Lock On, R3): holds one target until the player lets go or it gets too far away, and takes
 *  over the camera, framing the player and the target. While locked, a hard flick of the right stick (or a quick mouse
 *  swipe) switches to the next enemy in that direction; small camera movements do nothing.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class UCombatTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UCombatTargetingComponent();

	/** Lock press: locks onto the current soft target (or the best one in view), or lets go when already locked. With nothing to lock onto, swings the camera round behind the character */
	void ToggleLock();

	UFUNCTION(BlueprintPure, Category = "Combat|Targeting")
	bool IsLocked() const { return LockedTarget.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "Combat|Targeting")
	AActor* GetLockedTarget() const { return LockedTarget.Get(); }

	/** The lock or a recentre is turning the camera, so the follow camera should stay out of it */
	bool IsDrivingCamera() const { return IsLocked() || bRecentring; }

	/** The player turned the camera themselves (unlocked): stops a recentre so it doesn't fight them */
	void NotifyManualLook() { bRecentring = false; }

	/** Right stick while locked (already through its dead zone, -1 to 1). Only a hard flick to the edge of the stick switches target */
	void HandleStickInput(const FVector2D& Stick);

	/** Mouse movement while locked: a quick swipe sideways switches target */
	void HandleMouseInput(const FVector2D& MouseDelta);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	virtual void BeginPlay() override;

	/** Enemies further away than this are never picked. Also scales the distance part of a candidate's score */
	UPROPERTY(EditAnywhere, Category = "Targeting|Soft", meta = (ClampMin = 1, Units = "cm"))
	float TargetMaxDistance = 2000.0f;

	/**
	 *  How much being off to the side of the camera's view counts against a candidate, compared with distance.
	 *  0 picks the nearest enemy wherever it is; higher favours whatever the camera is pointing at.
	 *  At 1, an enemy directly behind costs as much as one at TargetMaxDistance straight ahead.
	 */
	UPROPERTY(EditAnywhere, Category = "Targeting|Soft", meta = (ClampMin = 0))
	float TargetAngleWeight = 1.0f;

	/** A candidate has to beat the current target's score by this much to take over, so the target doesn't flicker between two similar enemies */
	UPROPERTY(EditAnywhere, Category = "Targeting|Soft", meta = (ClampMin = 0))
	float TargetSwitchMargin = 0.1f;

	/** A locked target further away than this (horizontally) breaks the lock */
	UPROPERTY(EditAnywhere, Category = "Targeting|Lock", meta = (ClampMin = 1, Units = "cm"))
	float LockBreakDistance = 2500.0f;

	/**
	 *  How far the right stick has to be pushed to switch target, as a fraction of its full range: near 1 means all the way
	 *  to the edge. Anything less is ignored while locked
	 */
	UPROPERTY(EditAnywhere, Category = "Targeting|Switch", meta = (ClampMin = 0.1, ClampMax = 1))
	float SwitchStickThreshold = 0.9f;

	/** The stick has to come back inside this before it can switch again, so one held push switches once */
	UPROPERTY(EditAnywhere, Category = "Targeting|Switch", meta = (ClampMin = 0, ClampMax = 1))
	float SwitchStickRearm = 0.3f;

	/**
	 *  A switch also has to be a flick: the stick has to get from rest (inside the rearm zone) to the threshold within this long.
	 *  A slow push all the way out doesn't switch. 0 = any push to the edge counts, however slow
	 */
	UPROPERTY(EditAnywhere, Category = "Targeting|Switch", meta = (ClampMin = 0, Units = "s"))
	float SwitchFlickTime = 0.2f;

	/** How far the mouse has to move sideways within Switch Mouse Window to switch target */
	UPROPERTY(EditAnywhere, Category = "Targeting|Switch", meta = (ClampMin = 0))
	float SwitchMouseDistance = 40.0f;

	UPROPERTY(EditAnywhere, Category = "Targeting|Switch", meta = (ClampMin = 0.01, Units = "s"))
	float SwitchMouseWindow = 0.15f;

	/** A candidate more than this far round from the flick direction (as seen on screen, from the current target) isn't picked */
	UPROPERTY(EditAnywhere, Category = "Targeting|Switch", meta = (ClampMin = 1, ClampMax = 180, Units = "Degrees"))
	float SwitchMaxAngle = 70.0f;

	/** How quickly the camera turns to frame the locked target; higher is snappier */
	UPROPERTY(EditAnywhere, Category = "Targeting|Lock Camera", meta = (ClampMin = 0))
	float LockCameraSpeed = 6.0f;

	/**
	 *  Where the locked camera turns to face, from the player (0) to the target (1), sideways only. In between keeps both in
	 *  frame, the Kingdom Hearts way; 1 centres the target, the Dark Souls way. The tilt is worked out separately (below)
	 */
	UPROPERTY(EditAnywhere, Category = "Targeting|Lock Camera", meta = (ClampMin = 0, ClampMax = 1))
	float LockFocusBlend = 0.6f;

	/** The locked camera's tilt with the target level with the player: a little downward, looking over the shoulder */
	UPROPERTY(EditAnywhere, Category = "Targeting|Lock Camera", meta = (ClampMin = -45, ClampMax = 45, Units = "Degrees"))
	float LockPitchOffset = -10.0f;

	/**
	 *  How much of the target's height above or below the player the camera tilts to follow, e.g. up after a juggled target.
	 *  0 = always at Lock Pitch Offset, 1 = the full angle from the player to the target
	 */
	UPROPERTY(EditAnywhere, Category = "Targeting|Lock Camera", meta = (ClampMin = 0, ClampMax = 1))
	float LockPitchFollow = 0.5f;

	/**
	 *  The target's height is measured as if it were at least this far away, so a target right next to the player, a little
	 *  higher or lower, doesn't make the camera tilt sharply
	 */
	UPROPERTY(EditAnywhere, Category = "Targeting|Lock Camera", meta = (ClampMin = 1, Units = "cm"))
	float LockPitchMinDistance = 300.0f;

	/** The locked camera's tilt stays within these, inside the character's own pitch limits */
	UPROPERTY(EditAnywhere, Category = "Targeting|Lock Camera", meta = (ClampMin = -89, ClampMax = 0, Units = "Degrees"))
	float LockPitchMin = -35.0f;

	UPROPERTY(EditAnywhere, Category = "Targeting|Lock Camera", meta = (ClampMin = -45, ClampMax = 89, Units = "Degrees"))
	float LockPitchMax = 10.0f;

	/**
	 *  While locked, the camera boom's socket offset moves to this, so the camera sits a little over one shoulder and the target
	 *  isn't hidden behind the player. Y is sideways (positive = right shoulder), Z up. Eases back when the lock ends
	 */
	UPROPERTY(EditAnywhere, Category = "Targeting|Lock Camera")
	FVector LockCameraOffset = FVector(0.0, 50.0, 20.0);

	/** How quickly the shoulder offset eases in and out */
	UPROPERTY(EditAnywhere, Category = "Targeting|Lock Camera", meta = (ClampMin = 0))
	float LockOffsetSpeed = 4.0f;

	/** How quickly a lock press with nothing to lock onto swings the camera behind the character */
	UPROPERTY(EditAnywhere, Category = "Targeting|Lock Camera", meta = (ClampMin = 0, Units = "DegreesPerSecond"))
	float RecentreSpeed = 540.0f;

	/** Draws a marker on the locked target */
	UPROPERTY(EditAnywhere, Category = "Targeting|Debug")
	bool bShowDebug = true;

private:

	APlayerController* GetPlayerController() const;

	void UpdateSoftTarget();

	/** Lower is better: distance as a fraction of TargetMaxDistance plus the weighted angle off ViewForward. Max float if out of range */
	float ScoreCandidate(const AActor* Candidate, const FVector& ViewForward) const;

	/** The enemy nearest the flick direction from the current target, as seen on screen; null if none is within SwitchMaxAngle */
	AActor* FindSwitchTarget(const FVector2D& ScreenDirection) const;

	void SetLockedTarget(AActor* NewTarget);
	void SwitchTarget(const FVector2D& ScreenDirection);

	void UpdateLockCamera(float DeltaTime);
	void UpdateCameraOffset(float DeltaTime);
	void UpdateRecentre(float DeltaTime);

	UPROPERTY(Transient)
	TObjectPtr<UCombatContextComponent> ContextComponent;

	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> CameraBoom;

	TWeakObjectPtr<AActor> LockedTarget;

	/** The boom's socket offset when unlocked, to ease back to */
	FVector BaseCameraOffset = FVector::ZeroVector;

	/** Stick flick state: armed once the stick is back near rest; StickLeftRestTime is when it last left rest */
	bool bStickArmed = true;
	bool bStickAtRest = true;
	double StickLeftRestTime = 0.0;

	/** Mouse swipe: sideways movement summed over the last Switch Mouse Window */
	float MouseSwipe = 0.0f;
	double MouseSwipeStartTime = 0.0;

	/** Set by a lock press with no target: the yaw the camera is swinging round to */
	bool bRecentring = false;
	float RecentreYaw = 0.0f;
};
