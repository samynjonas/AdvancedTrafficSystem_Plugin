// Fill out your copyright notice in the Description page of Project Settings.


#include "../Public/ATS_TrafficAwarenessComponent.h"
#include "../Public/ATS_NavigationManager.h"
#include "../Public/ATS_LaneSpline.h"
#include "Components/SplineComponent.h"

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
	FVector currentLocation{ GetOffsetPosition() }; // Offset the location
	
	if (_bDrawDebug)
	{
		DrawDebugLine( GetWorld(),  currentLocation,   _ConnectionPoint,	_DebugColor, false, _DebugDrawTime, 0, 10.0f);
		DrawDebugPoint(GetWorld(), _ConnectionPoint,  10,				    _DebugColor, false, _DebugDrawTime);
	}

	if(FVector::DistSquared(currentLocation, _LastLocation) < 0.01f)
	{
		//Object has not moved
		if (_bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("TrafficAwarenessComponent::UpdateLocation() -- Object has not moved!"));
		}
		return false;
	}

	//Update location
	if (_bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("TrafficAwarenessComponent::UpdateLocation() -- Object has moved!"));
	}

	_LastLocation = currentLocation;
	_bIsDirty = true;
	return true;
}

bool UATS_TrafficAwarenessComponent::Initialize()
{
	if (_bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("TrafficAwarenessComponent::Initialize() -- Initializing..."));
	}

	if (_bIsInitialized)
	{
		return true;
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

	FVector currentLocation{ GetOffsetPosition() };

	_LastLocation = currentLocation;

	//-------------------------------------------------------------------------------------------------
	// Register this actor to the NavigationManager
	//-------------------------------------------------------------------------------------------------
	
	_bIsConnectedToLane = true;
	if (_bDrawDebug)
	{
		DrawDebugLine(GetWorld(), currentLocation, _ConnectionPoint, _DebugColor, false, _DebugDrawTime, 0, 10.0f);
		DrawDebugPoint(GetWorld(), _ConnectionPoint, 10, _DebugColor, false, _DebugDrawTime);
	}

	//Find the navigationManager
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AATS_NavigationManager::StaticClass(), FoundActors);
	for (AActor* pActor : FoundActors)
	{
		AATS_NavigationManager* pNavManager = Cast<AATS_NavigationManager>(pActor);
		if (pNavManager)
		{
			pNavManager->RegisterTrafficObject(this);
		}
	}


	if (_bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("TrafficAwarenessComponent::Initialize() -- Initialized succesfully!"));
	}
	_bIsInitialized = true;
	return true;
}

void UATS_TrafficAwarenessComponent::SetDistanceAlongLane(float distanceAlongLane)
{
	_bIsDirty = true;
	_DistanceAlongLane = distanceAlongLane;
}

void UATS_TrafficAwarenessComponent::SetLanePoint(FVector position)
{
	_bIsDirty = true;
	_ConnectionPoint = position;
}

void UATS_TrafficAwarenessComponent::SetIsDirty(bool bIsDirty)
{
	if (_bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("TrafficAwarenessComponent::SetIsDirty() -- Setting is dirty to %s"), bIsDirty ? TEXT("true") : TEXT("false"));
	}

	_bIsDirty = bIsDirty;
}

void UATS_TrafficAwarenessComponent::SetCanAgentPass(bool bCanAgentPass)
{
	_bCanAgentPass = bCanAgentPass;
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

bool UATS_TrafficAwarenessComponent::CanAgentPass(bool respectOverride) const
{
	if (_bOverrideCanAgentPassGetter && respectOverride)
	{
		return true;
	}
	return _bCanAgentPass;
}

FVector UATS_TrafficAwarenessComponent::GetOffset() const
{
	FRotator rotation = GetOwner()->GetActorRotation();
	FVector transformedOffset = rotation.RotateVector(_TransformOffset.GetLocation());

	return transformedOffset;
}

FVector UATS_TrafficAwarenessComponent::GetOffsetPosition() const
{
	return GetOwner()->GetActorLocation() + GetOffset();
}