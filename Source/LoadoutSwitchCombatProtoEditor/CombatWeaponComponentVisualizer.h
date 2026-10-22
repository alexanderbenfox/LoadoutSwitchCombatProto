#pragma once

#include "CoreMinimal.h"
#include "ComponentVisualizer.h"

/**
 *  Draws a Combat Weapon component's hit segment in the editor viewports (Blueprint editor and level)
 *  while the component is selected, so Hit Direction / Hit Start / Hit Length can be checked against the mesh.
 */
class FCombatWeaponComponentVisualizer : public FComponentVisualizer
{
public:

	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
};
