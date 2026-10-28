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

	/** Enemies further away than this are never picked. Also scales the distance part of a candidate's score */
	UPROPERTY(EditAnywhere, Category = "Targeting", meta = (ClampMin = 1, Units = "cm"))
	float TargetMaxDistance = 2000.0f;

	/**
	 *  How much being off to the side of the camera's view counts against a candidate, compared with distance.
	 *  0 picks the nearest enemy wherever it is; higher favours whatever the camera is pointing at.
	 *  At 1, an enemy directly behind costs as much as one at TargetMaxDistance straight ahead.
	 */
	UPROPERTY(EditAnywhere, Category = "Targeting", meta = (ClampMin = 0))
	float TargetAngleWeight = 1.0f;

	/** A candidate has to beat the current target's score by this much to take over, so the target doesn't flicker between two similar enemies */
	UPROPERTY(EditAnywhere, Category = "Targeting", meta = (ClampMin = 0))
	float TargetSwitchMargin = 0.1f;

public:

	ACombatCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }

	virtual void Tick(float DeltaSeconds) override;

protected:

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
	void Look(const FInputActionValue& Value);
	void Attack();

private:

	/**
	 *  Soft targeting, until the Targeting component and hard lock exist: of the actors carrying the "Enemy" actor tag,
	 *  targets the one with the lowest score, preferring to keep the current target.
	 */
	void UpdateSoftTarget();

	/** Lower is better: distance as a fraction of TargetMaxDistance plus the weighted angle off ViewForward. Max float if out of range */
	float ScoreTargetCandidate(const AActor* Candidate, const FVector& ViewForward) const;
};
