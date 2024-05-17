// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATS_TrafficHelper.h"
#include "ATS_NavigationLane.generated.h"

/*
	This spline will be generated from ZoneShapes but it will be altered in the corners, to make the turns smoother
*/


class USplineComponent;

UCLASS()
class ADVANCEDTRAFFICSYSTEM_API AATS_NavigationLane : public AActor
{
	GENERATED_BODY()
	
public:	
	AATS_NavigationLane();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	TArray<FVector> GetSplinePoints(USplineComponent* pSpline) const;

	void SetSplinePointTypeLinear(USplineComponent* pSpline);
	void SetSplinePointTypeCurve(USplineComponent* pSpline);

	void ReplaceSplinePoints(TArray<FVector> points, USplineComponent* pSplineToAlter);
	bool SmoothenSplineCorners(USplineComponent* pSpline);
	void CleanupSplineCorners(USplineComponent* pSpline);
	void FixTangents(USplineComponent* pSpline);

public:
	bool Initialize();
	void SetPoints(TArray<FVector> points);
	void SetTags(TArray<ELaneType> tags);

	TArray<ELaneType> GetTags() const { return _LaneTags; }

protected:
	USplineComponent* _pSpline{ nullptr };

	TArray<FVector>		_SplinePoints{};
	TArray<ELaneType>	_LaneTags{};

	UPROPERTY(EditAnywhere, Category = "Navigation Lane")
	AActor* _pSplineActor{ nullptr };

	UPROPERTY(EditAnywhere, Category = "Navigation Lane")
	float DistanceFromCorner{ 350.f };

	UPROPERTY(EditAnywhere, Category = "Navigation Lane")
	float _CornerCombineDistance{ 500.f };

	bool _bInitialized{ false };

};
