#include "Combat/AnimNotifies/AnimNotifyState_HitWindow.h"
#include "Combat/CombatResolverComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotifyState_HitWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (UCombatResolverComponent* Resolver = FindResolver(MeshComp))
	{
		Resolver->HandleHitWindowBegin(Animation, HitRadiusScale, Weapons, HitOverrides);
	}
}

void UAnimNotifyState_HitWindow::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (UCombatResolverComponent* Resolver = FindResolver(MeshComp))
	{
		Resolver->HandleHitWindowTick(Animation);
	}
}

void UAnimNotifyState_HitWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (UCombatResolverComponent* Resolver = FindResolver(MeshComp))
	{
		Resolver->HandleHitWindowEnd(Animation);
	}
}

FString UAnimNotifyState_HitWindow::GetNotifyName_Implementation() const
{
	return TEXT("Hit Window");
}

UCombatResolverComponent* UAnimNotifyState_HitWindow::FindResolver(const USkeletalMeshComponent* MeshComp)
{
	const AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	return Owner ? Owner->FindComponentByClass<UCombatResolverComponent>() : nullptr;
}
