// Fill out your copyright notice in the Description page of Project Settings.
#include "../Public/ATS_NavigationPathFollower.h"
#include "../Public/ATS_NavigationManager.h"
#include "../Public/ATS_TrafficAwarenessComponent.h"

#include "Kismet/GameplayStatics.h"

UATS_NavigationPathFollower::UATS_NavigationPathFollower()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UATS_NavigationPathFollower::BeginPlay()
{
	Super::BeginPlay();
	
	// Find the navigation manager
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AATS_NavigationManager::StaticClass(), FoundActors);
	for (AActor* pActor : FoundActors)
	{
		_pNavigationManager = Cast<AATS_NavigationManager>(pActor);
		if (_pNavigationManager)
		{
			break;
		}
	}

	if (_pNavigationManager == nullptr)
	{
		_pNavigationManager = GetWorld()->SpawnActor<AATS_NavigationManager>();
		if (_pNavigationManager == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to receive/spawn Navigation Manager"));
			return;
		}	
	}
	_pNavigationManager->Initialize();

	_LaneTags.Add(ELaneType::ATS_Car);

	_CurrentPathIndex	= _pNavigationManager->GetClosestPath(GetOwner()->GetActorLocation(), _LaneTags);
	_DistanceAlongPath	= _pNavigationManager->GetDistanceOnPath(_CurrentPathIndex, GetOwner()->GetActorLocation());
	_SpeedLimit			= _pNavigationManager->GetSpeedLimit(_CurrentPathIndex);
}

FTransform UATS_NavigationPathFollower::GetNewPosition(float deltaTime, FVector position, float speed, bool loopOnEnd)
{
	FTransform splineTransform{ FTransform::Identity };
	if (_pNavigationManager == nullptr || _pNavigationManager->Initialize() == false)
	{
		return splineTransform;
	}
	const float offsetDistance{ speed * _Direction * deltaTime };

	splineTransform			= _pNavigationManager->GetTransformOnPath(_CurrentPathIndex, position, offsetDistance);
	float distanceOnPath	= _pNavigationManager->GetDistanceOnPath(_CurrentPathIndex, position);
	float pathLength		= _pNavigationManager->GetLaneLength(_CurrentPathIndex);

	if (distanceOnPath >= pathLength - offsetDistance)
	{
		float overlap = distanceOnPath + offsetDistance - pathLength;
		if (loopOnEnd)
		{
			splineTransform = _pNavigationManager->GetTransformOnPath(_CurrentPathIndex, overlap, 0);
		}
		else
		{
			_CurrentPathIndex  = _pNavigationManager->GetRandomNextPath(_CurrentPathIndex);
			_DistanceAlongPath = overlap;

			splineTransform = _pNavigationManager->GetTransformOnPath(_CurrentPathIndex, position, 0);

			_SpeedLimit = _pNavigationManager->GetSpeedLimit(_CurrentPathIndex);

		}
	}

	return splineTransform;
}
FTransform UATS_NavigationPathFollower::GetNewPostionBasedOn2Points(float deltaTime, FVector frontPoint, FVector backPoint, float speed, bool loopOnEnd)
{
	FTransform splineTransform{ FTransform::Identity };
	if (_pNavigationManager == nullptr || _pNavigationManager->Initialize() == false)
	{
		return splineTransform;
	}
	
	FTransform frontTransform	= GetNewPosition(deltaTime, frontPoint, speed, loopOnEnd);
	FTransform backTransform	= GetNewPosition(deltaTime, backPoint, speed, loopOnEnd);

	FTransform averageTransform{ FTransform::Identity };
	averageTransform.SetLocation((frontTransform.GetLocation() + backTransform.GetLocation()) / 2);
	averageTransform.SetRotation(FQuat::Slerp(frontTransform.GetRotation(), backTransform.GetRotation(), 0.5f));

	return averageTransform;
}

bool UATS_NavigationPathFollower::GetObjectOnPath(FVector location, float distanceOffset, bool& canAgentPass, FVector& outLocation, float& distanceToObject, AActor* askingActor)
{
	if (_pNavigationManager == nullptr || _pNavigationManager->Initialize() == false)
	{
		return false;
	}
	float distanceOnPath = _pNavigationManager->GetDistanceOnPath(_CurrentPathIndex, location);

	if (askingActor == nullptr)
	{
		askingActor = GetOwner();
	}

	UATS_TrafficAwarenessComponent* pTrafficObject = _pNavigationManager->GetNextTrafficObject(_CurrentPathIndex, distanceOnPath + distanceOffset, askingActor);
	if (pTrafficObject == nullptr)
	{
		return false;
	}

	canAgentPass		= pTrafficObject->CanAgentPass();
	outLocation			= pTrafficObject->GetLanePoint();
	distanceToObject	= FMath::Abs(pTrafficObject->GetDistanceAlongLane() - (distanceOnPath + distanceOffset));

	return true;
}

void UATS_NavigationPathFollower::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

