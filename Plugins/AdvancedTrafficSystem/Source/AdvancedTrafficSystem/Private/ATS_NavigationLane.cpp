// Fill out your copyright notice in the Description page of Project Settings.
#include "../Public/ATS_NavigationLane.h"
#include "Components/SplineComponent.h"

AATS_NavigationLane::AATS_NavigationLane()
{
 	PrimaryActorTick.bCanEverTick = false;

	_pSpline = CreateDefaultSubobject<USplineComponent>(TEXT("NavigationLane"));
}

void AATS_NavigationLane::BeginPlay()
{
	Super::BeginPlay();

	//Initialize();
}

bool AATS_NavigationLane::Initialize()
{
	if (_bInitialized)
	{
		return true;
	}
	
	if (_pSplineActor)
	{
		USplineComponent* pSplineComponent = _pSplineActor->FindComponentByClass<USplineComponent>();
		if (pSplineComponent)
		{
			_SplinePoints = GetSplinePoints(pSplineComponent);
		}
	}
	ReplaceSplinePoints(_SplinePoints, _pSpline);
	SmoothenSplineCorners(_pSpline);

	_bInitialized = true;
	return true;
}

void AATS_NavigationLane::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AATS_NavigationLane::ReplaceSplinePoints(TArray<FVector> points, USplineComponent* pSplineToAlter)
{
	if (pSplineToAlter == nullptr)
	{
		return;
	}

	pSplineToAlter->SetSplinePoints(points, ESplineCoordinateSpace::World);
}

TArray<FVector> AATS_NavigationLane::GetSplinePoints(USplineComponent* pSpline) const
{
	TArray<FVector> points;
	
	if (pSpline == nullptr)
	{
		return points;
	}

	for (int i = 0; i < pSpline->GetNumberOfSplinePoints(); i++)
	{
		points.Add(pSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World));
	}

	return points;
}

bool AATS_NavigationLane::SmoothenSplineCorners(USplineComponent* pSpline)
{
	TArray<FVector> points;
	TArray<FVector> tangentPoints;

	if (pSpline == nullptr)
	{
		return false;
	}

	SetSplinePointTypeLinear(pSpline);

	/*
		-----------------------------------------
			Calculate new points and tangents
		-----------------------------------------
	*/
	for (int index = 0; index < pSpline->GetNumberOfSplinePoints(); index++)
	{
		int prevIndex = index - 1;
		int nextIndex = index + 1;

		if (pSpline->IsClosedLoop())
		{
			if (index == 0)
			{
				prevIndex = pSpline->GetNumberOfSplinePoints() - 1;
			}
			else if (index == pSpline->GetNumberOfSplinePoints() - 1)
			{
				nextIndex = 0;
			}
		}
		else
		{
			if (prevIndex < 0 || nextIndex >= pSpline->GetNumberOfSplinePoints())
			{
				points.Add(pSpline->GetLocationAtSplinePoint(index, ESplineCoordinateSpace::World));
				tangentPoints.Add(pSpline->GetLocationAtSplinePoint(index, ESplineCoordinateSpace::World));
				continue;
			}
		}
		

		float prevDist		= pSpline->GetDistanceAlongSplineAtSplinePoint(prevIndex);
		float currentDist	= pSpline->GetDistanceAlongSplineAtSplinePoint(index);
		float nextDist		= pSpline->GetDistanceAlongSplineAtSplinePoint(nextIndex);

		prevDist = currentDist - DistanceFromCorner;
		nextDist = currentDist + DistanceFromCorner;

		FVector prevPoint		= pSpline->GetLocationAtDistanceAlongSpline(prevDist,		ESplineCoordinateSpace::World);
		FVector currentPoint	= pSpline->GetLocationAtDistanceAlongSpline(currentDist,	ESplineCoordinateSpace::World);
		FVector nextPoint		= pSpline->GetLocationAtDistanceAlongSpline(nextDist,		ESplineCoordinateSpace::World);

		points.Add(prevPoint);
		points.Add(nextPoint);

		tangentPoints.Add(currentPoint);
		tangentPoints.Add(currentPoint);
	}

	/*
		-----------------------------------------
			Apply new points and tangents
		-----------------------------------------
	*/

	SetSplinePointTypeCurve(pSpline);
	pSpline->SetSplinePoints(points, ESplineCoordinateSpace::World);

	for (int index = 0; index < pSpline->GetNumberOfSplinePoints(); index++)
	{
		FVector inTangent{};
		FVector outTangent{};
		FVector locationAtSplinePoint{};

		float distance{};

		if (tangentPoints.IsValidIndex(index) == false)
		{
			continue;
		}

		locationAtSplinePoint = pSpline->GetLocationAtSplinePoint(index, ESplineCoordinateSpace::World);
		inTangent	= tangentPoints[index];
		outTangent	= tangentPoints[index];

		distance = FVector::Distance(locationAtSplinePoint, inTangent) * 2;

		inTangent = inTangent - locationAtSplinePoint;
		inTangent.Normalize();
		inTangent *= distance;

		outTangent = locationAtSplinePoint - outTangent;
		outTangent.Normalize();
		outTangent *= distance;

		if (index % 2 == 1)
		{
			//Is even
			pSpline->SetTangentAtSplinePoint(index, inTangent, ESplineCoordinateSpace::World);
		}
		else
		{
			//Is odd
			pSpline->SetTangentAtSplinePoint(index, outTangent, ESplineCoordinateSpace::World);
		}
	}

	return true;
}

void AATS_NavigationLane::SetSplinePointTypeLinear(USplineComponent* pSpline)
{
	if (pSpline == nullptr)
	{
		return;
	}

	for (int i = 0; i < pSpline->GetNumberOfSplinePoints(); i++)
	{
		pSpline->SetSplinePointType(i, ESplinePointType::Linear);
	}
}

void AATS_NavigationLane::SetSplinePointTypeCurve(USplineComponent* pSpline)
{
	if (pSpline == nullptr)
	{
		return;
	}

	for (int i = 0; i < pSpline->GetNumberOfSplinePoints(); i++)
	{
		pSpline->SetSplinePointType(i, ESplinePointType::Curve);
	}
}

void AATS_NavigationLane::SetPoints(TArray<FVector> points)
{
	_SplinePoints = points;
}