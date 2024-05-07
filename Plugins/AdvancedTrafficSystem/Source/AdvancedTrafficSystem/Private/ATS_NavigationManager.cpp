// Fill out your copyright notice in the Description page of Project Settings.
#include "../Public/ATS_NavigationManager.h"
#include "../Public/ATS_LaneSpline.h"
#include "../Public/ATS_NavigationLane.h"

#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SplineComponent.h"

#include "ZoneGraphAStar.h"
#include "ZoneShapeComponent.h"
#include <ZoneGraphRenderingUtilities.h>

AATS_NavigationManager::AATS_NavigationManager()
{
 	PrimaryActorTick.bCanEverTick = false;
}

void AATS_NavigationManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AATS_NavigationManager::BeginPlay()
{
	Super::BeginPlay();
	Initialize();	
}

bool AATS_NavigationManager::Initialize()
{
	if (_bInitialized)
	{
		return true;
	}

	InitializeNavigationPaths();

	//Find all the splines from a certain class in the world
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AATS_NavigationLane::StaticClass(), FoundActors);
	UINT32 laneCount = 0;
	for (AActor* pActor : FoundActors)
	{
		AATS_NavigationLane* pLaneSpline	= Cast<AATS_NavigationLane>(pActor);
		USplineComponent* pSpline		    = pActor->GetComponentByClass<USplineComponent>();
		if (pLaneSpline && pLaneSpline->Initialize() && pSpline)
		{
			TSharedPtr<FLaneNavigationPath> path{ MakeShared<FLaneNavigationPath>() };
			path->pSpline		= pSpline;
			path->speedLimit	= 30.f;
			path->laneIndex		= laneCount++;

			path->laneTags.Add(ELaneType::ATS_Road);
			path->laneTags.Add(ELaneType::ATS_Pedestrian);
			path->laneTags.Add(ELaneType::ATS_Bicycle);

			_ArrNavigationPath.Add(path);
			_ArrAllLanes.AddUnique(path->laneIndex);

			for (const auto& laneType : path->laneTags)
			{
				switch (laneType)
				{
				case ELaneType::ATS_Road:
					_ArrRoadLanes.AddUnique(path->laneIndex);
					break;
				case ELaneType::ATS_Pedestrian:
					_ArrPedestrianLanes.AddUnique(path->laneIndex);
					break;
				case ELaneType::ATS_Bicycle:
					_ArrBicycleLanes.AddUnique(path->laneIndex);
					break;
				}
			}
		}
	}


	//Once all the splines are found we can start linking them together 
	// -- check just the start and end points of the splines, and check if it is on another splines start or end points
	// -- if it is, link them together
	for (const TSharedPtr<FLaneNavigationPath>& path : _ArrNavigationPath)
	{
		if (path->IsValid() == false)
		{
			continue;
		}

		FVector startLocation = path->pSpline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
		FVector endLocation = path->pSpline->GetLocationAtSplinePoint(path->pSpline->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World);

		for (const TSharedPtr<FLaneNavigationPath>& otherPath : _ArrNavigationPath)
		{
			if (otherPath->IsValid() == false)
			{
				continue;
			}
			if (path == otherPath)
			{
				continue;
			}

			FVector otherStartLocation	= otherPath->pSpline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
			FVector otherEndLocation	= otherPath->pSpline->GetLocationAtSplinePoint(otherPath->pSpline->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World);

			//UE_LOG(LogTemp, Warning, TEXT("AATS_NavigationManager::Initialize() -- start <-> start -- Distance: %f"),	FVector::Distance(startLocation, otherStartLocation));
			//UE_LOG(LogTemp, Warning, TEXT("AATS_NavigationManager::Initialize() -- start <-> end -- Distance: %f"),		FVector::Distance(startLocation, otherEndLocation));
			//UE_LOG(LogTemp, Warning, TEXT("AATS_NavigationManager::Initialize() -- end <-> start -- Distance: %f"),		FVector::Distance(endLocation, otherStartLocation));
			//UE_LOG(LogTemp, Warning, TEXT("AATS_NavigationManager::Initialize() -- end <-> end -- Distance: %f"),		FVector::Distance(endLocation, otherEndLocation));

			if (FVector::Distance(startLocation, otherStartLocation) < _MaxConnectionDistance)
			{
				UE_LOG(LogTemp, Warning, TEXT("AATS_NavigationManager::Initialize() -- FOUND CONNECTION"));
				
				path->previousPaths.Add(otherPath->laneIndex);
				otherPath->previousPaths.Add(path->laneIndex);
			}
			else if (FVector::Distance(startLocation, otherEndLocation) < _MaxConnectionDistance)
			{
				UE_LOG(LogTemp, Warning, TEXT("AATS_NavigationManager::Initialize() -- FOUND CONNECTION"));

				path->previousPaths.Add(otherPath->laneIndex);
				otherPath->nextPaths.Add(path->laneIndex);
			}
			else if (FVector::Distance(endLocation, otherStartLocation) < _MaxConnectionDistance)
			{
				UE_LOG(LogTemp, Warning, TEXT("AATS_NavigationManager::Initialize() -- FOUND CONNECTION"));

				path->nextPaths.Add(otherPath->laneIndex);
				otherPath->previousPaths.Add(path->laneIndex);
			}
			else if (FVector::Distance(endLocation, otherEndLocation) < _MaxConnectionDistance)
			{
				UE_LOG(LogTemp, Warning, TEXT("AATS_NavigationManager::Initialize() -- FOUND CONNECTION"));

				path->nextPaths.Add(otherPath->laneIndex);
				otherPath->nextPaths.Add(path->laneIndex);
			}
		}
	}

	//All connections have been set (:
	_bInitialized = true;
	return true;
}

FLaneNavigationPath* AATS_NavigationManager::GetNavigationPath(UINT32 index) const
{
	for (const TSharedPtr<FLaneNavigationPath>& path : _ArrNavigationPath)
	{
		if (path->laneIndex == index)
		{
			return path.Get();
		}
	}

	UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::GetNavigationPath() -- path is invalid!"));
	return nullptr;
}

USplineComponent* AATS_NavigationManager::GetSpline(UINT32 index) const
{
	FLaneNavigationPath* pPath = GetNavigationPath(index);
	if (pPath == nullptr || pPath->IsValid() == false)
	{
		UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::GetSpline() -- path is invalid!"));
		return nullptr;
	}

	USplineComponent* pSpline = pPath->pSpline;
	return pSpline;
}

float AATS_NavigationManager::GetLaneLength(UINT32 index) const
{
	USplineComponent* pSpline = GetSpline(index);
	if (pSpline == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::GetLaneLength() -- pSpline is nullptr!"));
		return 0.f;
	}

	float length = pSpline->GetSplineLength();
	return length;
}

UINT32 AATS_NavigationManager::GetClosestPath(const FVector& location, TArray<ELaneType> tags) const
{
	UINT32 closestPath = 0;
	float minDistance = FLT_MAX;

	for (const TSharedPtr<FLaneNavigationPath>& path : _ArrNavigationPath)
	{
		if (path->IsValid() == false)
		{
			UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::GetSpline() -- path is invalid!"));
			continue;
		}
		
		if (path->HasOverlappingTags(tags) == false)
		{
			UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::GetSpline() -- path has no overlapping tags!"));
			continue;
		}

		auto inputKey				= path->pSpline->FindInputKeyClosestToWorldLocation(location);
		FVector locationOnSpline	= path->pSpline->GetLocationAtSplineInputKey(inputKey, ESplineCoordinateSpace::World);
		float distance				= FVector::DistSquared(location, locationOnSpline);
		
		UE_LOG(LogTemp, Warning, TEXT("AATS_NavigationManager::GetSpline() -- Distance: %f"), distance);
		if (distance < minDistance)
		{
			minDistance = distance;
			closestPath = path->laneIndex;
		}
	}

	return closestPath;
}
UINT32 AATS_NavigationManager::GetRandomNextPath(UINT32 pathIndex, bool isReversed) const
{
	FLaneNavigationPath* pPath = GetNavigationPath(pathIndex);
	if (pPath == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::GetRandomNextPath() -- Path is nullptr!"));
		return 0;
	}
	
	TArray<UINT32> nextPaths = isReversed ? pPath->previousPaths : pPath->nextPaths;

	if (nextPaths.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::GetRandomNextPath() -- Path has no next paths!"));
		return 0;
	}

	int randomIndex = FMath::RandRange(0, nextPaths.Num() - 1);
	UE_LOG(LogTemp, Warning, TEXT("AATS_NavigationManager::GetRandomNextPath() -- Random index: %d -> path %d"), randomIndex, nextPaths[randomIndex]);
	return nextPaths[randomIndex];
}

FTransform AATS_NavigationManager::GetTransformOnPath(UINT32 pathIndex, const FVector& location, float offsetDistance) const
{
	USplineComponent* pSpline = GetSpline(pathIndex);
	if (pSpline == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::GetTransformOnPath() -- pSpline is nullptr!"));
		return FTransform::Identity;
	}

	float	   distanceOnSpline		= GetDistanceOnPath(pathIndex, location);
	FTransform transformOnSpline	= pSpline->GetTransformAtDistanceAlongSpline(distanceOnSpline + offsetDistance, ESplineCoordinateSpace::World);
	
	return transformOnSpline;
}
FTransform AATS_NavigationManager::GetTransformOnPath(UINT32 pathIndex, float distanceAlongPath, float offsetDistance) const
{
	USplineComponent* pSpline = GetSpline(pathIndex);
	if (pSpline == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::GetTransformOnPath() -- pSpline is nullptr!"));
		return FTransform::Identity;
	}

	FTransform transformOnSpline = pSpline->GetTransformAtDistanceAlongSpline(distanceAlongPath + offsetDistance, ESplineCoordinateSpace::World);

	return transformOnSpline;
}

FVector AATS_NavigationManager::GetLocationOnPath(UINT32 pathIndex, const FVector& location, float offsetDistance) const
{
	USplineComponent* pSpline = GetSpline(pathIndex);
	if (pSpline == nullptr)
	{
		return FVector::ZeroVector;
	}

	float	distanceOnSpline	= GetDistanceOnPath(pathIndex, location);
	FVector locationOnSpline	= pSpline->GetLocationAtDistanceAlongSpline(distanceOnSpline + offsetDistance, ESplineCoordinateSpace::World);

	return locationOnSpline;
}
float AATS_NavigationManager::GetDistanceOnPath(UINT32 pathIndex, const FVector& location) const
{
	USplineComponent* pSpline = GetSpline(pathIndex);
	if (pSpline == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::GetDistanceOnPath() -- pSpline is nullptr!"));
		return 0.f;
	}

	float	inputKey			= pSpline->FindInputKeyClosestToWorldLocation(location);
	float	distanceOnSpline	= pSpline->GetDistanceAlongSplineAtSplineInputKey(inputKey);

	return distanceOnSpline;
}

float AATS_NavigationManager::GetSpeedLimit(UINT32 pathIndex) const
{
	FLaneNavigationPath* pPath = GetNavigationPath(pathIndex);
	if (pPath == nullptr)
	{
		return 0.f;
	}

	return pPath->speedLimit;
}

bool AATS_NavigationManager::InitializeNavigationPaths()
{
	/*
		This funtion will initialize all the navigation paths -- to do so it will first receive all zoneshapes from the world
		It will than use the lanes on those zoneShapes to generate the different paths
	*/

	UWorld* pWorld = GetWorld();
	UZoneGraphSubsystem* pZoneGraphSubsystem = UWorld::GetSubsystem<UZoneGraphSubsystem>(pWorld);
	if (pZoneGraphSubsystem == nullptr)
	{
		return false;
	}

	TArray<TArray<FVector>> arrLanePoints{};

	auto data = pZoneGraphSubsystem->GetRegisteredZoneGraphData();
	for (auto thing : data)
	{
		auto storage = thing.ZoneGraphData->GetStorage();

		TArray<int32> laneEndPoints{};

		auto lanes = storage.Lanes;
		for (auto lane : lanes)
		{
			laneEndPoints.Add(lane.GetLastPoint());
		}

		auto points = storage.LanePoints;
		TArray<FVector> lanePoints{};

		//Draw the points
		FColor color = FColor::MakeRandomColor();
		for (int32 index{}; index < points.Num(); index++)
		{
			FVector location = points[index];
			lanePoints.Add(location);

			if (laneEndPoints.Contains(index))
			{
				if (lanePoints.Num() > 1)
				{
					arrLanePoints.Add(lanePoints);		
					lanePoints.Empty();
				}
			}
		}
	}

	//Now per lane we need to spawn in a AATS_NavigationLane
	for (auto lane : arrLanePoints)
	{
		AATS_NavigationLane* pLane = GetWorld()->SpawnActor<AATS_NavigationLane>();
		if (pLane == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::InitializeNavigationPaths() -- Failed to spawn lane!"));
			return false;
		}

		pLane->SetPoints(lane);
		if (pLane->Initialize())
		{
			UE_LOG(LogTemp, Warning, TEXT("AATS_NavigationManager::InitializeNavigationPaths() -- Lane initialized!"));
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("AATS_NavigationManager::InitializeNavigationPaths() -- All lanes initialized!"));
	return true;
}