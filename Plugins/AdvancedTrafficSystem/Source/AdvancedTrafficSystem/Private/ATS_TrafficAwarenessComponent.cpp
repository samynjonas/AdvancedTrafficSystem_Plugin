// Fill out your copyright notice in the Description page of Project Settings.


#include "../Public/ATS_TrafficAwarenessComponent.h"
#include "../Public/ATS_TrafficManager.h"

#include "Kismet/GameplayStatics.h"

UATS_TrafficAwarenessComponent::UATS_TrafficAwarenessComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UATS_TrafficAwarenessComponent::BeginPlay()
{
	Super::BeginPlay();
	Initialize();
}

void UATS_TrafficAwarenessComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//No need to tick if the actor is not moveable
	if (_bIsMoveable == false)
	{
		return;
	}

	UpdateLocation();
}

bool UATS_TrafficAwarenessComponent::UpdateLocation()
{
	//Check if the actor has moved
	FVector currentLocation = GetOwner()->GetActorLocation();
	if(FVector::DistSquared(currentLocation, _LastLocation) < 0.01f)
	{
		//Object has not moved
		if (_bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("TrafficAwarenessComponent::UpdateLocation() -- Object has not moved!"));
		}
		return false;
	}

	if(_pTrafficManager == nullptr)
	{
		if(_bDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("TrafficAwarenessComponent::UpdateLocation() -- TrafficManager not found!"));
		}
		return false;
	}

	if (_pTrafficManager->IsInitialized() == false)
	{
		_pTrafficManager->Initialize();
	}

	if (_bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("TrafficAwarenessComponent::UpdateLocation() -- Updating location..."));
	}
	_pTrafficManager->RegisterTrafficObject(this, _ConnectionPoint);

	if (_bDrawDebug)
	{
		DrawDebugLine(GetWorld(), GetOwner()->GetActorLocation(), _ConnectionPoint, _DebugColor, false, _DebugDrawTime, 0, 10.0f);
		DrawDebugPoint(GetWorld(), _ConnectionPoint, 10, _DebugColor, false, _DebugDrawTime);
	}

	//Update location
	_LastLocation = currentLocation;
	return true;
}

bool UATS_TrafficAwarenessComponent::Initialize()
{
	if (_bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("TrafficAwarenessComponent::Initialize() -- Initializing..."));
	}

	//-------------------------------------------------------------------------------------------------
	// Get TrafficManager and make sure it's initialzed already
	//-------------------------------------------------------------------------------------------------

	TArray<AActor*> pTrafficManagers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AATS_TrafficManager::StaticClass(), pTrafficManagers);

	for (AActor* pActor : pTrafficManagers)
	{
		AATS_TrafficManager* pTrafficManager = Cast<AATS_TrafficManager>(pActor);
		if (pTrafficManager)
		{
			_pTrafficManager = pTrafficManager;
			//Break out of loop - found trafficManager
			break;
		}
	}

	if (_pTrafficManager)
	{
		if (_pTrafficManager->IsInitialized() == false)
		{
			_pTrafficManager->Initialize();
		}
	}
	else
	{
		if (_bDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("TrafficAwarenessComponent::Initialize() -- TrafficManager not found!"));
		}
		return false;
	}

	if (_bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("TrafficAwarenessComponent::Initialize() -- TrafficManager connected!"));
	}

	//-------------------------------------------------------------------------------------------------
	// Configure object
	//-------------------------------------------------------------------------------------------------

	if(_AwarenessType == EATS_AwarenessType::Lane)
	{
		_bCanAgentPass = false;
	}
	else
	{
		_bCanAgentPass = true;
	}

	_LastLocation = GetOwner()->GetActorLocation();

	//-------------------------------------------------------------------------------------------------
	// Register this actor to the TrafficManager
	//-------------------------------------------------------------------------------------------------

	_bIsConnectedToLane = _pTrafficManager->RegisterTrafficObject(this, _ConnectionPoint, _SearchRadius);
	if (_bIsConnectedToLane == false)
	{
		if (_bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("TrafficAwarenessComponent::Initialize() -- Failed to register to TrafficManager!"));
		}
		return false;
	}
	else
	{
		if (_bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("TrafficAwarenessComponent::Initialize() -- Connected to lane at position: %s"), *_ConnectionPoint.ToString());

			
		}

		if (_bDrawDebug)
		{
			DrawDebugLine(GetWorld(), GetOwner()->GetActorLocation(), _ConnectionPoint, _DebugColor, false, _DebugDrawTime, 0, 10.0f);
			DrawDebugPoint(GetWorld(), _ConnectionPoint, 10, _DebugColor, false, _DebugDrawTime);
		}
	}

	if (_bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("TrafficAwarenessComponent::Initialize() -- Initialized succesfully!"));
	}
	return true;
}

void UATS_TrafficAwarenessComponent::SetDistanceAlongLane(float distanceAlongLane)
{
	_DistanceAlongLane = distanceAlongLane;
}

bool UATS_TrafficAwarenessComponent::AdjustAgent(AActor* pAgent)
{
	//This function can be used to adjust to agent depending on this object
	if(pAgent == nullptr)
	{
		return false;
	}

	return true;
}