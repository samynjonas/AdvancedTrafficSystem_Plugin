// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ATS_LaneModifierComponent.generated.h"

/*
	These are components that can be added to a traffic sign, to modify the lane behavior.
	It will not be able to open or close lanes but modify the behavior of agents on those lanes
*/

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ADVANCEDTRAFFICSYSTEM_API UATS_LaneModifierComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UATS_LaneModifierComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

public:	
	virtual void ModifyLane();
};
