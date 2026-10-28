#include "Combat/AnimNotifies/AnimNotifyState_CombatLunge.h"
#include "Combat/CombatResolverComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotifyState_CombatLunge::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	UCombatResolverComponent* Resolver = Owner->GetComponentByClass<UCombatResolverComponent>();
	if (!Resolver)
	{
		return;
	}

	Resolver->HandleLungeWindowBegin(TotalDuration);
}

FString UAnimNotifyState_CombatLunge::GetNotifyName_Implementation() const
{
	return TEXT("Combat Lunge");
}
