// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATS_TrafficHelper.h"
#include "ATS_NavigationManager.generated.h"

UCLASS()
class ADVANCEDTRAFFICSYSTEM_API AATS_NavigationManager : public AActor
{
	GENERATED_BODY()
	
public:	
	AATS_NavigationManager();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	bool InitializeNavigationPaths();

public:	
	FLaneNavigationPath* GetNavigationPath(UINT32 index) const;
	USplineComponent* GetSpline(UINT32 index) const;

	float GetLaneLength(UINT32 index) const;

	bool Initialize();

	UINT32 GetClosestPath(const FVector& location, TArray<ELaneType> tags) const;
	UINT32 GetRandomNextPath(UINT32 pathIndex, bool isReversed = false) const;
	
	FTransform	GetTransformOnPath(UINT32 pathIndex, const FVector& location, float offsetDistance = 0.f) const;
	FTransform	GetTransformOnPath(UINT32 pathIndex, float distanceAlongPath, float offsetDistance = 0.f) const;

	FVector		GetLocationOnPath(UINT32 pathIndex, const FVector& location, float offsetDistance = 0.f) const;
	float		GetDistanceOnPath(UINT32 pathIndex, const FVector& location) const;

	float GetSpeedLimit(UINT32 pathIndex) const;

protected:
	TArray<TSharedPtr<FLaneNavigationPath>> _ArrNavigationPath{};

	TArray<UINT32> _ArrPedestrianLanes{};
	TArray<UINT32> _ArrBicycleLanes{};
	TArray<UINT32> _ArrRoadLanes{};
	TArray<UINT32> _ArrAllLanes{};
	
protected:
	bool _bInitialized{ false };
	float _MaxConnectionDistance{ 100.f };

};
