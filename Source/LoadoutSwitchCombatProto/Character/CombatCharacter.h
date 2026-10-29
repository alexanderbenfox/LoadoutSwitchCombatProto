#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "CombatCharacter.generated.h"

class UAbilitySystemComponent;
class UCameraComponent;
class UCombatContextComponent;
class UCombatLoadoutComponent;
class UCombatResolverComponent;
class UCombatTargetingComponent;
class UInputAction;
class UInputMappingContext;
class UMotionWarpingComponent;
class USpringArmComponent;
class UCombatWeaponComponent;
struct FInputActionValue;

/**
 *  The player character: third person camera, Enhanced Input, and the components the combat system hangs off.
 *  Abstract; the mesh, anim blueprint and input assets are assigned in a Blueprint child.
 */
UCLASS(abstract)
class ACombatCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	/** Owns this character's gameplay abilities, attributes and tags */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	/** Bends attack root motion toward the target */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMotionWarpingComponent> MotionWarping;

	/** Builds the context tags the resolver reads */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatContextComponent> CombatContext;

	/** Equipped loadouts and which one is active */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatLoadoutComponent> CombatLoadout;

	/** Picks a slot from the context using the priority rules */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatResolverComponent> CombatResolver;

	/** Soft targeting and hard lock: picks the target for the context component and frames it with the camera when locked */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatTargetingComponent> Targeting;

	/**
	 *  The weapon in the character's hand; its hit segment is set on the component.
	 *  Attached to weapon_r_socket (on hand_r); change Parent Socket on the component in the Blueprint if the socket is renamed.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatWeaponComponent> Weapon;

protected:

	/** Mapping contexts added when a local player takes control */
	UPROPERTY(EditAnywhere, Category = "Input")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MouseLookAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> AttackAction;

	/** Toggles hard lock (R3); while locked, the camera stick switches target instead of turning the camera */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LockOnAction;

	/** How fast a full right-stick push turns the camera sideways, before the player's stick sensitivity */
	UPROPERTY(EditAnywhere, Category = "Input|Look", meta = (ClampMin = 0, Units = "DegreesPerSecond"))
	float StickYawRate = 200.0f;

	/** How fast a full right-stick push tilts the camera up or down, before the player's stick sensitivity */
	UPROPERTY(EditAnywhere, Category = "Input|Look", meta = (ClampMin = 0, Units = "DegreesPerSecond"))
	float StickPitchRate = 130.0f;

	/**
	 *  Gravity while falling (going down), as a multiple of the movement component's Gravity Scale: a lighter rise and a quicker
	 *  drop. 1 = the same both ways. An attack's Stall or Glide window multiplies on top
	 */
	UPROPERTY(EditAnywhere, Category = "Movement|Air", meta = (ClampMin = 1))
	float FallGravityMultiplier = 1.3f;

	/** How far the camera turns per unit of mouse movement, before the player's mouse sensitivity (2.5 matches the engine's old built-in look scale) */
	UPROPERTY(EditAnywhere, Category = "Input|Look", meta = (ClampMin = 0, Units = "Degrees"))
	float MouseLookScale = 2.5f;

	/**
	 *  Furthest the camera can tilt down to look at the character from above (negative pitch). Applied to the player's
	 *  camera manager when the character is possessed, so it limits the stick, the mouse and anything else that turns the camera
	 */
	UPROPERTY(EditAnywhere, Category = "Camera|Limits", meta = (ClampMin = -89, ClampMax = 0, Units = "Degrees"))
	float CameraPitchMin = -60.0f;

	/** Furthest the camera can tilt up to look from below (positive pitch); low keeps it from dropping under the floor */
	UPROPERTY(EditAnywhere, Category = "Camera|Limits", meta = (ClampMin = 0, ClampMax = 89, Units = "Degrees"))
	float CameraPitchMax = 30.0f;

	/**
	 *  Follow camera: while the player moves without touching the camera, it leans towards where they're going.
	 *  How long after the last manual camera input (stick or mouse) before it starts; manual input always wins
	 */
	UPROPERTY(EditAnywhere, Category = "Camera|Follow", meta = (ClampMin = 0, Units = "s"))
	float FollowDelay = 0.5f;

	/** How long the follow takes to build up to full strength once it starts, so it eases in instead of kicking in */
	UPROPERTY(EditAnywhere, Category = "Camera|Follow", meta = (ClampMin = 0, Units = "s"))
	float FollowRampTime = 0.3f;

	/**
	 *  How fast the camera turns at full stick moving straight across the screen. It scales with how sideways the movement is,
	 *  so running away from the camera holds it still, and with how far the stick is pushed. 0 = no follow
	 */
	UPROPERTY(EditAnywhere, Category = "Camera|Follow", meta = (ClampMin = 0, Units = "DegreesPerSecond"))
	float FollowRate = 90.0f;

	/**
	 *  Strength of the follow while moving back towards the camera, as a fraction of the normal follow. Low keeps the camera
	 *  from swinging round when the player turns to run at it (the Dark Souls way); 1 treats it like any other direction
	 */
	UPROPERTY(EditAnywhere, Category = "Camera|Follow", meta = (ClampMin = 0, ClampMax = 1))
	float FollowTowardsCameraScale = 0.2f;

public:

	ACombatCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }

	virtual void Tick(float DeltaSeconds) override;

protected:

	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** No jumping out of an attack unless it's in its recovery or its ability is Jump Cancelable. A blocked jump press is dropped, not saved for when the attack ends */
	virtual bool CanJumpInternal_Implementation() const override;

	/** A jump that went ahead during an attack cancels the attack */
	virtual void OnJumped_Implementation() override;

	/** Landing in an air attack's recovery ends the attack, so the character lands instead of finishing the air swing on the ground */
	virtual void Landed(const FHitResult& Hit) override;

	void Move(const FInputActionValue& Value);

	/** Right stick: turns at the stick rates, so the camera speed doesn't depend on the frame rate. While locked, it's for switching target */
	void StickLook(const FInputActionValue& Value);

	/** Right stick back in its dead zone; the lock's flick detection needs to see it return to rest */
	void StickLookReleased(const FInputActionValue& Value);

	/** Mouse: turns by however far the mouse moved. While locked, a quick swipe switches target */
	void MouseLook(const FInputActionValue& Value);

	void Attack();
	void ToggleLockOn();

private:

	/** Sets the movement component's gravity each tick: the base Gravity Scale, times FallGravityMultiplier when going down, times the attack's Stall or Glide gravity */
	void UpdateGravity();

	/** The movement component's Gravity Scale at BeginPlay; everything UpdateGravity sets is relative to it */
	float BaseGravityScale = 1.0f;

	/** Turns the camera towards the movement direction when the player isn't steering it (see Camera|Follow) */
	void UpdateFollowCamera(float DeltaSeconds);

	/** World time of the last stick or mouse camera input; the follow camera waits FollowDelay after it */
	double LastManualLookTime = -1.0e9;
};
