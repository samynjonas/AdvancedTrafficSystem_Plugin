// Fill out your copyright notice in the Description page of Project Settings.
#include "ATS_LaneSpline.h"

#include "Components/SplineComponent.h"

AATS_LaneSpline::AATS_LaneSpline()
{
 	PrimaryActorTick.bCanEverTick = false;

	_pSpline = CreateDefaultSubobject<USplineComponent>(TEXT("LaneSpline"));
	_pSpline->SetupAttachment(RootComponent);

	_pSpline->SetSplinePointType(0, ESplinePointType::Linear, true);
	_pSpline->SetSplinePointType(1, ESplinePointType::Linear, true);

	//Change color depending on the type of lane
	switch (_LaneType)
	{
		case ELaneType::ATS_Car:
			_pSpline->EditorUnselectedSplineSegmentColor = FColor::Green;
			_pSpline->UpdateSpline();
			break;										 
		case ELaneType::ATS_Pedestrian:					  
			_pSpline->EditorUnselectedSplineSegmentColor = FColor::Yellow;
			_pSpline->UpdateSpline();
			break;										  
		case ELaneType::ATS_Bicycle:					  
			_pSpline->EditorUnselectedSplineSegmentColor = FColor::Blue;
			_pSpline->UpdateSpline();
			break;
	}

}

bool AATS_LaneSpline::RegisterNextLane(AATS_LaneSpline* pNextLane)
{
	if (pNextLane == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("ATS_LaneSpline::RegisterNextLane() -- pNextLane is nullptr!"));
		return false;
	}

	_pNextLanes.Add(pNextLane);
	return true;
}
bool AATS_LaneSpline::RegisterPreviousLane(AATS_LaneSpline* pPreviousLane)
{
	if (pPreviousLane == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("ATS_LaneSpline::RegisterPreviousLane() -- pPreviousLane is nullptr!"));
		return false;
	}

	_pPreviousLanes.Add(pPreviousLane);
	return true;
}

bool AATS_LaneSpline::RegisterLaneModifier(UATS_TrafficAwarenessComponent* pLaneModifier)
{
	if (pLaneModifier == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("ATS_LaneSpline::RegisterLaneModifier() -- pLaneModifier is nullptr!"));
		return false;
	}

	_pArrLaneModifiers.Add(pLaneModifier);
	return true;
}
bool AATS_LaneSpline::UnregisterLaneModifier(UATS_TrafficAwarenessComponent* pLaneModifier)
{
	if (pLaneModifier == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("ATS_LaneSpline::UnregisterLaneModifier() -- pLaneModifier is nullptr!"));
		return false;
	}

	_pArrLaneModifiers.Remove(pLaneModifier);
	return true;
}

float AATS_LaneSpline::GetDistanceAlongSpline(const FVector& location) const
{
	float distance{};
	if (_pSpline == nullptr)
	{
		return distance;
	}

	auto closestkey = _pSpline->FindInputKeyClosestToWorldLocation(location);
	distance = _pSpline->GetDistanceAlongSplineAtSplineInputKey(closestkey);
	return distance;
}

FVector AATS_LaneSpline::GetPositionOnSpline(const FVector& location) const
{
	FVector position{};
	if (_pSpline == nullptr)
	{
		return position;
	}

	//Get all actors with a certain tag
	//TArray<AActor*> FoundActors;
	//UGameplayStatics::GetAllActorsWithTag(GetWorld(), TEXT("Lane"), FoundActors);

	auto closestkey = _pSpline->FindInputKeyClosestToWorldLocation(location);
	position		= _pSpline->GetLocationAtSplineInputKey(closestkey, ESplineCoordinateSpace::World);
	return position;
}

void AATS_LaneSpline::BeginPlay()
{
	Super::BeginPlay();
}
void AATS_LaneSpline::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

