#include "Combat/CombatHitStopSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

void UCombatHitStopSubsystem::StartHitStop(AActor* Actor, float Duration, float TimeDilation)
{
	if (!Actor || Duration <= 0.f)
	{
		return;
	}

	FActiveStop* Stop = ActiveStops.Find(Actor);
	if (!Stop)
	{
		Stop = &ActiveStops.Add(Actor);
		Stop->RestoreDilation = Actor->CustomTimeDilation;
	}

	// not 0: some ticks skip or divide by a zero delta. 0.02 is still effectively frozen
	Actor->CustomTimeDilation = FMath::Max(TimeDilation, KINDA_SMALL_NUMBER);

	// the world timer runs on world time, so the stop lasts Duration however slow the actor is
	GetWorld()->GetTimerManager().SetTimer(Stop->Timer,
		FTimerDelegate::CreateUObject(this, &UCombatHitStopSubsystem::EndHitStop, TWeakObjectPtr<AActor>(Actor)), Duration, false);
}

void UCombatHitStopSubsystem::EndHitStop(TWeakObjectPtr<AActor> Actor)
{
	FActiveStop Stop;
	if (!ActiveStops.RemoveAndCopyValue(Actor, Stop))
	{
		return;
	}

	if (AActor* StoppedActor = Actor.Get())
	{
		StoppedActor->CustomTimeDilation = Stop.RestoreDilation;
	}
}

void UCombatHitStopSubsystem::Deinitialize()
{
	for (TPair<TWeakObjectPtr<AActor>, FActiveStop>& Pair : ActiveStops)
	{
		if (AActor* StoppedActor = Pair.Key.Get())
		{
			StoppedActor->CustomTimeDilation = Pair.Value.RestoreDilation;
		}

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(Pair.Value.Timer);
		}
	}

	ActiveStops.Reset();

	Super::Deinitialize();
}
