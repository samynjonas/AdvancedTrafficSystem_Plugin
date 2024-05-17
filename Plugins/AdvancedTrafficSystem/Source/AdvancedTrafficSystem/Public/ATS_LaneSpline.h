// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ATS_TrafficHelper.h"
#include "GameFramework/Actor.h"
#include "ATS_LaneSpline.generated.h"


class USplineComponent;
class UATS_TrafficAwarenessComponent;

UCLASS()
class ADVANCEDTRAFFICSYSTEM_API AATS_LaneSpline : public AActor
{
	GENERATED_BODY()
	
public:	
	AATS_LaneSpline();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

public:	
	ELaneType GetLaneType() const { return _LaneType; }
	TArray<AATS_LaneSpline*> GetNextLanes() const { return _pNextLanes; }
	TArray<AATS_LaneSpline*> GetPreviousLanes() const { return _pPreviousLanes; }
	TArray<UATS_TrafficAwarenessComponent*> GetLaneModifiers() const { return _pArrLaneModifiers; }

	USplineComponent* GetSpline() const { return _pSpline; }

	// Lane modifier will be able to register themself to these lanes
	bool RegisterLaneModifier(UATS_TrafficAwarenessComponent* pLaneModifier);

	// Lane modifier will be able to unregister themself to these lanes
	bool UnregisterLaneModifier(UATS_TrafficAwarenessComponent* pLaneModifier);

	// Auto connect lanes where points are overlapping
	bool RegisterNextLane(AATS_LaneSpline* pNextLane);
	bool RegisterPreviousLane(AATS_LaneSpline* pPreviousLane);

	float GetDistanceAlongSpline(const FVector& Location) const;
	FVector GetPositionOnSpline(const FVector& location) const;

protected:
	USplineComponent* _pSpline{ nullptr };

	// This should be used to know where the lane is going
	UPROPERTY(EditAnywhere, Category = "ATS")
	TArray<AATS_LaneSpline*> _pNextLanes;

	// This should also be used so that reversing the lanes is possible
	UPROPERTY(EditAnywhere, Category = "ATS")
	TArray<AATS_LaneSpline*> _pPreviousLanes;
	
	// This should be used to know what type of lane it is
	UPROPERTY(EditAnywhere, Category = "ATS")
	ELaneType _LaneType{ ELaneType::ATS_Car };
	
	// Array of objects that are on the lane(traffic lights, speed signs, etc.)
	UPROPERTY(EditAnywhere, Category = "ATS")
	TArray<UATS_TrafficAwarenessComponent*> _pArrLaneModifiers;

};
