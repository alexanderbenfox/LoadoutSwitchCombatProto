#include "Enemy/CombatDummy.h"
#include "AbilitySystemComponent.h"
#include "Combat/CombatTags.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Loadout/CombatAbilityDefinition.h"

ACombatDummy::ACombatDummy()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));

	PoiseTier = CombatTags::Enemy_Poise_Unarmored;

	// the player's placeholder targeting looks for this actor tag
	Tags.Add(TEXT("Enemy"));

	// character movement only simulates for a possessed pawn, so make sure spawned dummies get an AI controller too
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ACombatDummy::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystem->InitAbilityActorInfo(this, this);
}

void ACombatDummy::DebugLaunch()
{
	Launch(DebugLaunchSpeed, FVector::ZeroVector);
}

void ACombatDummy::ReceiveHit(AActor* Attacker, const UCombatAbilityDefinition* Ability, const FHitResult& Hit)
{
	if (Ability)
	{
		// the attacker's facing, flattened: the lunge has turned it to the target, and a crowd hit pushes everyone the same way.
		// not Hit.ImpactNormal, which is unreliable when the sweep starts inside the capsule
		const FVector KnockbackDirection = Attacker ? Attacker->GetActorForwardVector().GetSafeNormal2D() : FVector::ZeroVector;
		Launch(Ability->HitLaunchSpeed, KnockbackDirection * Ability->HitKnockbackSpeed);
	}

	// the launch only takes effect on the next movement update, so a dummy launched this frame isn't falling yet; bLaunched covers it
	const bool bInAir = bLaunched || GetCharacterMovement()->IsFalling();

	if (bInAir)
	{
		// a ground flinch would play out early in the air time and leave the plain fall pose, so the air has its own (or none)
		if (FlinchMontage)
		{
			StopAnimMontage(FlinchMontage);
		}

		if (AirFlinchMontage)
		{
			PlayAnimMontage(AirFlinchMontage, 1.f);
		}
	}
	else if (FlinchMontage)
	{
		PlayAnimMontage(FlinchMontage, 1.f);
	}
}

void ACombatDummy::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	bLaunched = false;

	if (AirFlinchMontage)
	{
		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			AnimInstance->Montage_Stop(AirFlinchLandBlendOut, AirFlinchMontage);
		}
	}
}

void ACombatDummy::Launch(float UpSpeed, const FVector& HorizontalVelocity)
{
	const bool bLaunchUp = UpSpeed > 0.f;
	const bool bPushAcross = !HorizontalVelocity.IsNearlyZero();

	if (!bLaunchUp && !bPushAcross)
	{
		return;
	}

	// overriding rather than adding, so a juggle hit on a falling dummy pops it back up instead of just slowing the fall,
	// and repeated pushes don't stack. A part left at 0 keeps the dummy's current velocity there
	LaunchCharacter(FVector(HorizontalVelocity.X, HorizontalVelocity.Y, UpSpeed), bPushAcross, bLaunchUp);

	if (bLaunchUp)
	{
		bLaunched = true;
	}
}
