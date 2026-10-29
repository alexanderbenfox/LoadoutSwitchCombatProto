#include "Combat/AnimNotifies/AnimNotifyState_CombatStall.h"
#include "Combat/CombatResolverComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotifyState_CombatStall::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	const AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (UCombatResolverComponent* Resolver = Owner ? Owner->FindComponentByClass<UCombatResolverComponent>() : nullptr)
	{
		Resolver->HandleStallBegin(Animation, this, GravityScale, bStopFall);
	}
}

void UAnimNotifyState_CombatStall::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	const AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (UCombatResolverComponent* Resolver = Owner ? Owner->FindComponentByClass<UCombatResolverComponent>() : nullptr)
	{
		Resolver->HandleAirMotionEnd(this);
	}
}

FString UAnimNotifyState_CombatStall::GetNotifyName_Implementation() const
{
	return TEXT("Combat Stall");
}
