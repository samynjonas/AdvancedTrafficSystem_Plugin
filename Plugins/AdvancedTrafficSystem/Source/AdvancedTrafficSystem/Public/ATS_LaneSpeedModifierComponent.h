// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../Public/ATS_LaneModifierComponent.h"
#include "ATS_LaneSpeedModifierComponent.generated.h"

/* 
	Set the speedlimit of the lane
 */
UCLASS()
class ADVANCEDTRAFFICSYSTEM_API UATS_LaneSpeedModifierComponent : public UATS_LaneModifierComponent
{
	GENERATED_BODY()
	
public:
	virtual void ModifyLane() override;

	float GetSpeedLimit() const { return _SpeedLimit; }
	void  SetSpeedLimit(float SpeedLimit) { _SpeedLimit = SpeedLimit; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed Modifier")
	float _SpeedLimit{ 30.f };

};
