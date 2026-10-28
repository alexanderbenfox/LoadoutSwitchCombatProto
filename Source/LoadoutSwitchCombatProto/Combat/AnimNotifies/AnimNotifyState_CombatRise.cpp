#include "Combat/AnimNotifies/AnimNotifyState_CombatRise.h"
#include "Combat/CombatResolverComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotifyState_CombatRise::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	const AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (UCombatResolverComponent* Resolver = Owner ? Owner->FindComponentByClass<UCombatResolverComponent>() : nullptr)
	{
		Resolver->HandleRiseWindowBegin(Animation, TotalDuration);
	}
}

FString UAnimNotifyState_CombatRise::GetNotifyName_Implementation() const
{
	return TEXT("Combat Rise");
}
