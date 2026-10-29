#include "Combat/AnimNotifies/AnimNotifyState_CombatGlide.h"
#include "Combat/CombatResolverComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotifyState_CombatGlide::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	const AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (UCombatResolverComponent* Resolver = Owner ? Owner->FindComponentByClass<UCombatResolverComponent>() : nullptr)
	{
		Resolver->HandleGlideBegin(Animation, this, GravityScale, bStopFall, MaxSpeed, Stiffness);
	}
}

void UAnimNotifyState_CombatGlide::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	const AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (UCombatResolverComponent* Resolver = Owner ? Owner->FindComponentByClass<UCombatResolverComponent>() : nullptr)
	{
		Resolver->HandleAirMotionEnd(this);
	}
}

FString UAnimNotifyState_CombatGlide::GetNotifyName_Implementation() const
{
	return TEXT("Combat Glide");
}
