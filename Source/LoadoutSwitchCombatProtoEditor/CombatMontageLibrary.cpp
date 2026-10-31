#include "CombatMontageLibrary.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/Package.h"

UAnimMontage* UCombatMontageLibrary::BuildComboMontage(const FString& AssetPath, const TArray<UAnimSequence*>& Hits, FName SlotName, const TArray<float>& PlayRates)
{
	if (Hits.IsEmpty() || Hits.Contains(nullptr))
	{
		return nullptr;
	}

	const FString AssetName = FPackageName::GetLongPackageAssetName(AssetPath);

	UAnimMontage* Montage = LoadObject<UAnimMontage>(nullptr, *FString::Printf(TEXT("%s.%s"), *AssetPath, *AssetName), nullptr, LOAD_NoWarn | LOAD_Quiet);
	const bool bCreated = Montage == nullptr;
	if (bCreated)
	{
		UPackage* Package = CreatePackage(*AssetPath);
		Montage = NewObject<UAnimMontage>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
	}

	Montage->Modify();
	Montage->SetSkeleton(Hits[0]->GetSkeleton());

	// the rates hits already play at, so a rebuild that doesn't pass PlayRates doesn't undo a slowed or sped up hit
	TMap<const UAnimSequenceBase*, float> PreviousRates;
	for (const FSlotAnimationTrack& OldTrack : Montage->SlotAnimTracks)
	{
		for (const FAnimSegment& OldSegment : OldTrack.AnimTrack.AnimSegments)
		{
			PreviousRates.Add(OldSegment.GetAnimReference(), OldSegment.AnimPlayRate);
		}
	}

	// one slot track holding every hit back to back
	Montage->SlotAnimTracks.Reset();
	FSlotAnimationTrack& Track = Montage->SlotAnimTracks.AddDefaulted_GetRef();
	Track.SlotName = SlotName;

	TArray<float> HitStartTimes;
	float Position = 0.0f;
	for (int32 i = 0; i < Hits.Num(); i++)
	{
		UAnimSequence* Hit = Hits[i];
		FAnimSegment Segment;
		Segment.SetAnimReference(Hit, true);
		Segment.StartPos = Position;
		if (PlayRates.IsValidIndex(i) && PlayRates[i] > 0.0f)
		{
			Segment.AnimPlayRate = PlayRates[i];
		}
		else if (const float* PreviousRate = PreviousRates.Find(Hit))
		{
			Segment.AnimPlayRate = *PreviousRate;
		}
		Track.AnimTrack.AnimSegments.Add(Segment);

		HitStartTimes.Add(Position);
		Position += Segment.GetLength();
	}

	Montage->SetCompositeLength(Montage->CalculateSequenceLength());

	// a section per hit, unlinked so each stops on its own and the combo code chains them
	Montage->CompositeSections.Reset();
	for (int32 i = 0; i < HitStartTimes.Num(); i++)
	{
		const FName SectionName(FString::Chr(TEXT('A') + i));
		Montage->AddAnimCompositeSection(SectionName, HitStartTimes[i]);
	}
	for (FCompositeSection& Section : Montage->CompositeSections)
	{
		Section.NextSectionName = NAME_None;
	}

	// stay active to the last frame; see the README's montage checklist
	Montage->BlendOutTriggerTime = 0.0f;

	Montage->PostEditChange();
	Montage->MarkPackageDirty();

	if (bCreated)
	{
		FAssetRegistryModule::AssetCreated(Montage);
	}

	return Montage;
}
