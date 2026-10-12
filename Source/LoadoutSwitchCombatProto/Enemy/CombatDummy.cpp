#include "Enemy/CombatDummy.h"
#include "AbilitySystemComponent.h"
#include "Combat/CombatTags.h"
#include "Components/CapsuleComponent.h"

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
	const FVector vLaunchVelocity = FVector(0.0, 0.0, DebugLaunchSpeed);

	// override z for constant launch velocity for debugging
	LaunchCharacter(vLaunchVelocity, false, true);
}
