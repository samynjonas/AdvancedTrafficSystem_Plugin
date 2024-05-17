// Fill out your copyright notice in the Description page of Project Settings.
#include "../Public/ATS_NavigationManager.h"
#include "../Public/ATS_LaneSpline.h"
#include "../Public/ATS_NavigationLane.h"
#include "../Public/ATS_TrafficAwarenessComponent.h"

#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SplineComponent.h"

#include "ZoneGraphAStar.h"
#include "ZoneShapeComponent.h"
#include <ZoneGraphRenderingUtilities.h>

AATS_NavigationManager::AATS_NavigationManager()
{
 	PrimaryActorTick.bCanEverTick = true;
}

void AATS_NavigationManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateTrafficObjects();
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

	if (InitializeNavigationPaths())
	{
		if (_bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AATS_NavigationManager::Initialize() -- Navigation paths initialized!"));
		}
	}

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

			for (const auto& laneType : pLaneSpline->GetTags())
			{
				path->laneTags.Add(laneType);
			}

			_ArrNavigationPath.Add(path);
			_ArrAllLanes.AddUnique(path->laneIndex);

			for (const auto& laneType : path->laneTags)
			{
				_MapTagsToLanes.FindOrAdd(laneType).AddUnique(path->laneIndex);
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

		FVector startLocation	= path->pSpline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
		FVector endLocation		= path->pSpline->GetLocationAtSplinePoint(path->pSpline->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World);

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

			if (FVector::Distance(startLocation, otherStartLocation) < _MaxConnectionDistance)
			{
				path->previousPaths.Add(otherPath->laneIndex);
				otherPath->previousPaths.Add(path->laneIndex);
			}
			else if (FVector::Distance(startLocation, otherEndLocation) < _MaxConnectionDistance)
			{
				path->previousPaths.Add(otherPath->laneIndex);
				otherPath->nextPaths.Add(path->laneIndex);
			}
			else if (FVector::Distance(endLocation, otherStartLocation) < _MaxConnectionDistance)
			{
				path->nextPaths.Add(otherPath->laneIndex);
				otherPath->previousPaths.Add(path->laneIndex);
			}
			else if (FVector::Distance(endLocation, otherEndLocation) < _MaxConnectionDistance)
			{
				path->nextPaths.Add(otherPath->laneIndex);
				otherPath->nextPaths.Add(path->laneIndex);
			}
		}
	}

	//Initialize all the traffic objects
	if (InitializeTrafficObjects())
	{
		if (_bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AATS_NavigationManager::Initialize() -- Traffic objects initialized!"));
		}
		_bInitialized = true;
		return true;
	}

	return false;
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

	if (_bDebug)
	{
		UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::GetNavigationPath() -- path is invalid!"));
	}
	return nullptr;
}

USplineComponent* AATS_NavigationManager::GetSpline(UINT32 index) const
{
	FLaneNavigationPath* pPath = GetNavigationPath(index);
	if (pPath == nullptr || pPath->IsValid() == false)
	{
		if (_bDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::GetSpline() -- path is invalid!"));
		}
		return nullptr;
	}

	USplineComponent* pSpline = pPath->pSpline;
	return pSpline;
}

UATS_TrafficAwarenessComponent* AATS_NavigationManager::GetNextTrafficObject(UINT32 currentPath, float currentDistanceAlongPath, AActor* askingActor)
{
	FLaneNavigationPath* pPath = GetNavigationPath(currentPath);
	if (pPath == nullptr)
	{
		if (_bDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::GetNextTrafficObject() -- Path is nullptr!"));
		}
		return nullptr;
	}

	UATS_TrafficAwarenessComponent* closestTrafficObject{ nullptr };
	float minDistance = FLT_MAX;

	for (UATS_TrafficAwarenessComponent* pTrafficObject : pPath->arrTrafficObjects)
	{
		if (pTrafficObject == nullptr)
		{
			continue;
		}

		if (pTrafficObject->GetOwner() == askingActor)
		{
			continue;
		}

		if (pTrafficObject->GetDistanceAlongLane() > currentDistanceAlongPath && pTrafficObject->GetDistanceAlongLane() < minDistance)
		{
			minDistance = pTrafficObject->GetDistanceAlongLane();
			closestTrafficObject = pTrafficObject;
		}
	}

	return closestTrafficObject;
}

float AATS_NavigationManager::GetLaneLength(UINT32 index) const
{
	USplineComponent* pSpline = GetSpline(index);
	if (pSpline == nullptr)
	{
		if (_bDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::GetLaneLength() -- pSpline is nullptr!"));
		}
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
			if (_bDebug)
			{
				UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::GetSpline() -- path is invalid!"));
			}
			continue;
		}
		
		if (path->HasOverlappingTags(tags) == false)
		{
			if (_bDebug)
			{
				UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::GetSpline() -- path has no overlapping tags!"));
			}
			continue;
		}

		auto inputKey				= path->pSpline->FindInputKeyClosestToWorldLocation(location);
		FVector locationOnSpline	= path->pSpline->GetLocationAtSplineInputKey(inputKey, ESplineCoordinateSpace::World);
		float distance				= FVector::DistSquared(location, locationOnSpline);
		
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
		if (_bDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::GetRandomNextPath() -- Path is nullptr!"));
		}
		return 0;
	}
	
	TArray<UINT32> nextPaths = isReversed ? pPath->previousPaths : pPath->nextPaths;

	if (nextPaths.IsEmpty())
	{
		if (_bDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::GetRandomNextPath() -- Path has no next paths!"));
		}
		return 0;
	}

	int randomIndex = FMath::RandRange(0, nextPaths.Num() - 1);
	if (_bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("AATS_NavigationManager::GetRandomNextPath() -- Random index: %d -> path %d"), randomIndex, nextPaths[randomIndex]);
	}
	return nextPaths[randomIndex];
}

FTransform AATS_NavigationManager::GetTransformOnPath(UINT32 pathIndex, const FVector& location, float offsetDistance) const
{
	USplineComponent* pSpline = GetSpline(pathIndex);
	if (pSpline == nullptr)
	{
		if (_bDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::GetTransformOnPath() -- pSpline is nullptr!"));
		}
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
		if (_bDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::GetTransformOnPath() -- pSpline is nullptr!"));
		}
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
		if (_bDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::GetDistanceOnPath() -- pSpline is nullptr!"));
		}
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

	TArray<TArray<FVector>>	  arrLanePoints{};
	TArray<TArray<ELaneType>> arrLaneTypes{};

	auto data = pZoneGraphSubsystem->GetRegisteredZoneGraphData();
	for (auto thing : data)
	{
		auto storage = thing.ZoneGraphData->GetStorage();

		TArray<int32> laneEndPoints{};

		auto lanes = storage.Lanes;
		for (auto lane : lanes)
		{
			laneEndPoints.Add(lane.GetLastPoint());

			TArray<ELaneType> laneTags{};
			if (lane.Tags.Contains(_PedestrianTag))
			{
				laneTags.Add(ELaneType::ATS_Pedestrian);
			}
			if (lane.Tags.Contains(_VehicleTag))
			{
				laneTags.Add(ELaneType::ATS_Car);
			}
			if (lane.Tags.Contains(_BikeTag))
			{
				laneTags.Add(ELaneType::ATS_Bicycle);
			}
			if (lane.Tags.Contains(_TruckTag))
			{
				laneTags.Add(ELaneType::ATS_Truck);
			}
			if (lane.Tags.Contains(_CrosswalkTag))
			{
				laneTags.Add(ELaneType::ATS_Crosswalk);
			}
			arrLaneTypes.Add(laneTags);
		}

		auto points = storage.LanePoints;
		TArray<FVector> lanePoints{};

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
	for (int index{ 0 }; index < arrLanePoints.Num(); ++index)
	{
		TArray<FVector> lane{ arrLanePoints[index] };
		TArray<ELaneType> tags{ arrLaneTypes[index] };

		AATS_NavigationLane* pLane = GetWorld()->SpawnActor<AATS_NavigationLane>();
		if (pLane == nullptr)
		{
			if (_bDebug)
			{
				UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::InitializeNavigationPaths() -- Failed to spawn lane!"));
			}
			return false;
		}

		pLane->SetPoints(lane);
		pLane->SetTags(tags);

		if (pLane->Initialize())
		{
			if (_bDebug)
			{
				UE_LOG(LogTemp, Warning, TEXT("AATS_NavigationManager::InitializeNavigationPaths() -- Lane initialized!"));
			}
		}
	}

	if (_bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("AATS_NavigationManager::InitializeNavigationPaths() -- All lanes initialized!"));
	}
	return true;
}

bool AATS_NavigationManager::InitializeTrafficObjects()
{
	if (_bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("AATS_NavigationManager::InitializeTrafficObjects() -- Initializing traffic objects"));
	}

	//Find all actors in the world that contain a traffic awareness component
	TArray<AActor*> foundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), foundActors);

	for (AActor* actor : foundActors)
	{
		TArray<UATS_TrafficAwarenessComponent*> arrTrafficAwarenessComponent;
		actor->GetComponents<UATS_TrafficAwarenessComponent>(arrTrafficAwarenessComponent);
		for (auto pTrafficObject : arrTrafficAwarenessComponent)
		{
			if (pTrafficObject == nullptr)
			{
				if (_bDebug)
				{
					UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::InitializeTrafficObjects() -- Traffic object is nullptr!"));
				}
				continue;
			}

			FVector actorLocation{ pTrafficObject->GetOffsetPosition() };

			UINT32 closestPath		= GetClosestPath(actorLocation, { ELaneType::ATS_Car, ELaneType::ATS_Truck, ELaneType::ATS_Bicycle, ELaneType::ATS_Pedestrian });
			FVector connectionPoint = GetLocationOnPath(closestPath, actorLocation, 0.f);
			float distanceAlongPath = GetDistanceOnPath(closestPath, actorLocation);

			pTrafficObject->SetDistanceAlongLane(distanceAlongPath);
			pTrafficObject->SetLanePoint(connectionPoint);
			pTrafficObject->SetIsDirty(false);

			FLaneNavigationPath* pPath = GetNavigationPath(closestPath);
			if (pPath)
			{
				if (_bDebug && pTrafficObject->HasDebuggingEnabled())
				{
					UE_LOG(LogTemp, Warning, TEXT("AATS_NavigationManager::InitializeTrafficObjects() -- Traffic object added to path %d"), closestPath);
				}
				pPath->arrTrafficObjects.Add(pTrafficObject);
			}
			else
			{
				if (_bDebug)
				{
					UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::InitializeTrafficObjects() -- Path is nullptr!"));
				}
			}
		}
	}

	return true;
}
void AATS_NavigationManager::UpdateTrafficObjects()
{
	/*
		Update dirty traffic objects by rechecking their position and lane
	*/

	if (_bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("AATS_NavigationManager::UpdateTrafficObjects() -- Updating traffic objects"));
	}

	for (const TSharedPtr<FLaneNavigationPath>& path : _ArrNavigationPath)
	{
		if (path->IsValid() == false)
		{
			if (_bDebug)
			{
				UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::UpdateTrafficObjects() -- path is invalid!"));
			}
			continue;
		}

		for (UATS_TrafficAwarenessComponent* pTrafficObject : path->arrTrafficObjects)
		{
			if (pTrafficObject->GetIsDirty() == false)
			{
				if (_bDebug && pTrafficObject->HasDebuggingEnabled())
				{
					UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::UpdateTrafficObjects() -- Object hasn't changed"));
				}

				continue;
			}

			FVector actorLocation{ pTrafficObject->GetOffsetPosition() };

			UINT32 closestPath		= GetClosestPath(actorLocation, { ELaneType::ATS_Car, ELaneType::ATS_Truck, ELaneType::ATS_Bicycle, ELaneType::ATS_Pedestrian });
			FVector connectionPoint = GetLocationOnPath(closestPath, actorLocation, 0.f);
			float distanceAlongPath = GetDistanceOnPath(closestPath, actorLocation);

			if (_bDebug && pTrafficObject->HasDebuggingEnabled())
			{
				UE_LOG(LogTemp, Warning, TEXT("AATS_NavigationManager::UpdateTrafficObjects() -- Object has changed!"));
			}

			pTrafficObject->SetDistanceAlongLane(distanceAlongPath);
			pTrafficObject->SetLanePoint(connectionPoint);
			pTrafficObject->SetIsDirty(false);

			FLaneNavigationPath* pPath = GetNavigationPath(closestPath);
			if (pPath && pPath->laneIndex !=  path->laneIndex)
			{
				if (_bDebug && pTrafficObject->HasDebuggingEnabled())
				{
					UE_LOG(LogTemp, Warning, TEXT("AATS_NavigationManager::UpdateTrafficObjects() -- Traffic object changed lanes!"));
				}
				pPath->arrTrafficObjects.Add(pTrafficObject);
				path->arrTrafficObjects.Remove(pTrafficObject);
			}
		}
	}
}
bool AATS_NavigationManager::RegisterTrafficObject(UATS_TrafficAwarenessComponent* pTrafficObject)
{
	if (pTrafficObject == nullptr)
	{
		if (_bDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("AATS_NavigationManager::InitializeTrafficObjects() -- Traffic object is nullptr!"));
		}
		return false;
	}

	FVector actorLocation{ pTrafficObject->GetOffsetPosition() };

	UINT32 closestPath = GetClosestPath(actorLocation, { ELaneType::ATS_Car, ELaneType::ATS_Truck, ELaneType::ATS_Bicycle, ELaneType::ATS_Pedestrian });
	FVector connectionPoint = GetLocationOnPath(closestPath, actorLocation, 0.f);
	float distanceAlongPath = GetDistanceOnPath(closestPath, actorLocation);

	pTrafficObject->SetDistanceAlongLane(distanceAlongPath);
	pTrafficObject->SetLanePoint(connectionPoint);
	pTrafficObject->SetIsDirty(false);

	FLaneNavigationPath* pPath = GetNavigationPath(closestPath);
	if (pPath)
	{
		if (_bDebug && pTrafficObject->HasDebuggingEnabled())
		{
			UE_LOG(LogTemp, Warning, TEXT("AATS_NavigationManager::InitializeTrafficObjects() -- Traffic object added to path %d"), closestPath);
		}
		pPath->arrTrafficObjects.Add(pTrafficObject);
		return true;
	}
	return false;
}