#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CombatHitStopSubsystem.generated.h"

/**
 *  Hit-stop: briefly slows individual actors (the attacker and whoever it hit) to near zero, for impact.
 *  Uses each actor's Custom Time Dilation, which scales its own tick and its components' (animation, movement, root motion),
 *  and times the stop on the world clock, which the slowed actors don't affect.
 */
UCLASS()
class UCombatHitStopSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	/**
	 *  Slows Actor to TimeDilation for Duration seconds, then puts its time dilation back.
	 *  Stopping an actor that's already stopped restarts its stop, still returning it to the dilation it had before the first one
	 */
	void StartHitStop(AActor* Actor, float Duration, float TimeDilation);

	/** True while Actor is in a hit-stop */
	bool IsHitStopped(const AActor* Actor) const { return ActiveStops.Contains(Actor); }

	virtual void Deinitialize() override;

private:

	void EndHitStop(TWeakObjectPtr<AActor> Actor);

	struct FActiveStop
	{
		/** The actor's time dilation before the stop */
		float RestoreDilation = 1.0f;

		FTimerHandle Timer;
	};

	TMap<TWeakObjectPtr<AActor>, FActiveStop> ActiveStops;
};
