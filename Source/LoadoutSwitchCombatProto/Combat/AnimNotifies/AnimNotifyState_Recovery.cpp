#include "Combat/AnimNotifies/AnimNotifyState_Recovery.h"
#include "Combat/CombatResolverComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotifyState_Recovery::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	const AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (UCombatResolverComponent* Resolver = Owner ? Owner->FindComponentByClass<UCombatResolverComponent>() : nullptr)
	{
		Resolver->HandleRecoveryBegin(Animation);
	}
}

FString UAnimNotifyState_Recovery::GetNotifyName_Implementation() const
{
	return TEXT("Recovery");
}
