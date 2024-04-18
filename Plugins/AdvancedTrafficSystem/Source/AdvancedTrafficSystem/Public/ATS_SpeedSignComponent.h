// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ATS_TrafficAwarenessComponent.h"
#include "ATS_SpeedSignComponent.generated.h"

/*
	This will set the max speed of passing vehicles to a certain speed assigned by this actorComponent	
*/

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ADVANCEDTRAFFICSYSTEM_API UATS_SpeedSignComponent : public UATS_TrafficAwarenessComponent
{
	GENERATED_BODY()

public:
	UATS_SpeedSignComponent() = default;

protected:
	virtual bool AdjustAgent(AActor* pAgent) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed Sign", DisplayName = "Speed Limit")
	float _SpeedLimit{ 30.0f };
};
