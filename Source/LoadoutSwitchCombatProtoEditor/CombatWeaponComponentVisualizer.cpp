#include "CombatWeaponComponentVisualizer.h"
#include "Combat/CombatWeaponComponent.h"
#include "SceneManagement.h"

void FCombatWeaponComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	const UCombatWeaponComponent* Weapon = Cast<UCombatWeaponComponent>(Component);
	if (!Weapon)
	{
		return;
	}

	FVector HitStart;
	FVector HitEnd;
	Weapon->GetHitSegment(HitStart, HitEnd);

	// at a Hit Window's default scale of 1; a window with a bigger scale reaches further than drawn
	const float HitRadius = Weapon->GetHitRadius();
	const int32 HitProbeNum = Weapon->GetHitProbeCount(HitRadius);

	const FLinearColor ProbeColor(1.0f, 1.0f, 0.0f, 0.5f);
	const FLinearColor SegmentColor = FLinearColor::Yellow;
	const FLinearColor StartColor = FLinearColor::Green;
	const FLinearColor EndColor = FLinearColor::Red;
	const FLinearColor OffsetColor(0.4f, 0.4f, 0.4f);

	// foreground so the lines stay visible inside the mesh
	PDI->DrawLine(Weapon->GetComponentLocation(), HitStart, OffsetColor, SDPG_Foreground, 0.5f);
	PDI->DrawLine(HitStart, HitEnd, SegmentColor, SDPG_Foreground, 1.5f);

	// different colours at each end show which way HitDirection runs
	PDI->DrawPoint(HitStart, StartColor, 10.0f, SDPG_Foreground);
	PDI->DrawPoint(HitEnd, EndColor, 10.0f, SDPG_Foreground);

	// one sphere per probe, where the sweep puts them: shows how much space around the weapon can hit
	for (int32 i = 0; i < HitProbeNum; i++)
	{
		const float Alpha = static_cast<float>(i) / (HitProbeNum - 1);
		const FLinearColor Color = i == 0 ? StartColor : (i == HitProbeNum - 1 ? EndColor : ProbeColor);
		DrawWireSphere(PDI, FMath::Lerp(HitStart, HitEnd, Alpha), Color, HitRadius, 16, SDPG_Foreground);
	}
}
