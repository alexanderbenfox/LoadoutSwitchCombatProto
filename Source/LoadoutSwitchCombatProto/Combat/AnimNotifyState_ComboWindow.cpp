#include "Combat/AnimNotifyState_ComboWindow.h"
#include "Combat/CombatResolverComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotifyState_ComboWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (UCombatResolverComponent* Resolver = FindResolver(MeshComp))
	{
		Resolver->HandleComboWindowBegin(Animation);
	}
}

void UAnimNotifyState_ComboWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (UCombatResolverComponent* Resolver = FindResolver(MeshComp))
	{
		Resolver->HandleComboWindowEnd(Animation);
	}
}

FString UAnimNotifyState_ComboWindow::GetNotifyName_Implementation() const
{
	return TEXT("Combo Window");
}

UCombatResolverComponent* UAnimNotifyState_ComboWindow::FindResolver(const USkeletalMeshComponent* MeshComp)
{
	const AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	return Owner ? Owner->FindComponentByClass<UCombatResolverComponent>() : nullptr;
}
