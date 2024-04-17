// Fill out your copyright notice in the Description page of Project Settings.


#include "../Public/ATS_TrafficAwarenessComponent.h"
#include "../Public/ATS_TrafficManager.h"

#include "Kismet/GameplayStatics.h"

UATS_TrafficAwarenessComponent::UATS_TrafficAwarenessComponent()
{
	PrimaryComponentTick.bCanEverTick = _bIsMoveable;
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
	// Register this actor to the TrafficManager
	//-------------------------------------------------------------------------------------------------

	_bIsConnectedToLane = _pTrafficManager->RegisterTrafficObject(this, _ConnectionPoint);
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
			DrawDebugLine(GetWorld(), GetOwner()->GetActorLocation(), _ConnectionPoint, _DebugColor, false, 10.0f);
			DrawDebugPoint(GetWorld(), _ConnectionPoint, 10, _DebugColor, false, 15.0f);
		}
	}

	if (_bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("TrafficAwarenessComponent::Initialize() -- Initialized succesfully!"));
	}
	return true;
}