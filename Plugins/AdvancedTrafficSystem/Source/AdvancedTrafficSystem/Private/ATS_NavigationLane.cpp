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
	CleanupSplineCorners(_pSpline);
	SmoothenSplineCorners(_pSpline);

	//_CornerCombineDistance = 100.f;
	//CleanupSplineCorners(_pSpline);
	//FixTangents(_pSpline);

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

	SetSplinePointTypeLinear(pSplineToAlter);
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

		//points.Add(nextPoint);

		/*
			Tangents will need another check
			If the distance from the new point to the current point is larger than the distance from the current point to the next point
			We take the next point as tangent instead
		*/

		//PREV POINT CHECK
		{
			float distanceToBeforePrev{};
			FVector pointBeforePrev{};
		
			if (prevIndex - 1 < 0)
			{
				tangentPoints.Add(currentPoint);
			}
			else
			{
				distanceToBeforePrev	= pSpline->GetDistanceAlongSplineAtSplinePoint(prevIndex - 1);

				if (abs(currentDist - distanceToBeforePrev) < DistanceFromCorner)
				{
					//New points should be from new distance
					pointBeforePrev = pSpline->GetLocationAtDistanceAlongSpline(distanceToBeforePrev, ESplineCoordinateSpace::World);
					tangentPoints.Add(pointBeforePrev);
				}
				else
				{
					tangentPoints.Add(currentPoint);
				}	
			}
		}
		points.Add(prevPoint);

		//NEXT POINT CHECK
		{
			float distanceToNext{};
			FVector pointNext{};

			if (nextIndex - 1 < 0)
			{
				tangentPoints.Add(currentPoint);
			}
			else
			{
				distanceToNext	  = pSpline->GetDistanceAlongSplineAtSplinePoint(nextIndex + 1);

				if (abs(currentDist - distanceToNext) < DistanceFromCorner)
				{
					//New points should be from new distance
					pointNext = pSpline->GetLocationAtDistanceAlongSpline(distanceToNext, ESplineCoordinateSpace::World);
					tangentPoints.Add(pointNext);
				}
				else
				{
					tangentPoints.Add(currentPoint);
				}
			}
		}
		points.Add(nextPoint);
		//tangentPoints.Add(currentPoint);

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

void AATS_NavigationLane::CleanupSplineCorners(USplineComponent* pSpline)
{
	/*
		On some corners(mostly present on intersection), there are a lot of point on the lane, 
		if these are too close to eachother the generated spline will look weird

		Clean these points up by removing some of them
	*/

	if (pSpline == nullptr)
	{
		return;
	}

	SetSplinePointTypeLinear(pSpline);

	FVector splineStartPoint{};
	FVector splineEndPoint{};

	splineStartPoint = pSpline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
	splineEndPoint	 = pSpline->GetLocationAtSplinePoint(pSpline->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World);

	TMap<int, float> points;
	TMap<int, float> pointToCombine;

	for (size_t index = 0; index < pSpline->GetNumberOfSplinePoints(); index++)
	{
		size_t compareIndex = index - 1;

		FVector point					= pSpline->GetLocationAtSplinePoint(index, ESplineCoordinateSpace::World);
		float distanceAlongSpline		= pSpline->GetDistanceAlongSplineAtSplinePoint(index);
		
		if (index == 0)
		{
			points.Add(index, distanceAlongSpline);
			continue;
		}		

		FVector prevPoint				= pSpline->GetLocationAtSplinePoint(compareIndex, ESplineCoordinateSpace::World);
		float	prevDistanceAlongSpline	= pSpline->GetDistanceAlongSplineAtSplinePoint(compareIndex);

		if (distanceAlongSpline - prevDistanceAlongSpline <= _CornerCombineDistance)
		{
			//We will combine these points
			if (index != pSpline->GetNumberOfSplinePoints() - 1)
			{
				pointToCombine.Add(index, distanceAlongSpline);	
				DrawDebugSphere(GetWorld(), point,		50.f, 12, FColor::Red, false, 5.f);
			}
			
			if (compareIndex != 0)
			{
				pointToCombine.Add(compareIndex, prevDistanceAlongSpline);
				DrawDebugSphere(GetWorld(), prevPoint,	50.f, 12, FColor::Red, false, 5.f);
			}

		}
		else
		{
			points.Add(index, distanceAlongSpline);
		}
	}


	//Combine the points to combine -- try to take the middle point based on the distance along spline
	int   firstindex{ -1 };
	float firstDistance{};
	float lastDistance{};
	float averageDistance{};
	float distanceDifference{};
	FVector averagePoint{};

	for (auto& point : pointToCombine)
	{
		if (firstindex == -1)
		{
			firstindex		= point.Key;
			firstDistance	= point.Value;
		}
		lastDistance = point.Value;
	}

	distanceDifference	= lastDistance  - firstDistance;
	averageDistance		= firstDistance + (distanceDifference / 2.f);
	averagePoint		= pSpline->GetLocationAtDistanceAlongSpline(averageDistance, ESplineCoordinateSpace::World);

	TMap<int, FVector> newPointsMap;
	for (auto& point : points)
	{
		if (pointToCombine.Contains(point.Key))
		{
			continue;
		}
		int pointIndex = point.Key;

		FVector pointLocation{};
		pointLocation = pSpline->GetLocationAtDistanceAlongSpline(point.Value, ESplineCoordinateSpace::World);
		
		newPointsMap.Add(pointIndex, pointLocation);
	}
	newPointsMap.Add(firstindex, averagePoint);
	
	//Sort the map based on the keys
	TArray<int32> SortedKeys;
	newPointsMap.GetKeys(SortedKeys);

	SortedKeys.Sort();

	TArray<FVector> finalPoints{};
	finalPoints.Add(splineStartPoint);
	for (int32 key : SortedKeys)
	{
		FVector pointLocation = newPointsMap[key];
		finalPoints.Add(pointLocation);
	}
	finalPoints.Add(splineEndPoint);

	TArray<FVector> cleanedPoints{};
	//Go over the points and check if there are doubles
	for (int index{}; index < finalPoints.Num(); index++)
	{
		FVector point		= finalPoints[index];
		int compareIndex	= index - 1;
		
		if (compareIndex < 0)
		{
			cleanedPoints.Add(point);
			continue;
		}

		FVector comparePoint	= finalPoints[compareIndex];

		if (FVector::Distance(point, comparePoint) > 0.1f)
		{
			cleanedPoints.Add(point);
		}
	}

	pSpline->SetSplinePoints(cleanedPoints, ESplineCoordinateSpace::World);
}

void AATS_NavigationLane::FixTangents(USplineComponent* pSpline)
{
	/*
		Tangents get calculated based on the distance between the points, but because some point are added later than other, some tangents may because wrong - this is where we will fix them
		Tangents should points towards the next point - if there is no next point, it should point towards their own point
	*/

	if (pSpline == nullptr)
	{
		return;
	}

	TArray<FVector> points;
	TArray<FVector> tangentPoints;

	points = GetSplinePoints(pSpline);

	for (size_t index = 0; index < points.Num(); index++)
	{
		int nextPointIndex = index + 1;
		if (points.IsValidIndex(nextPointIndex))
		{
			tangentPoints.Add(points[nextPointIndex]);
		}
		else
		{
			tangentPoints.Add(points[index]);
		}
	}

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


}

void AATS_NavigationLane::SetPoints(TArray<FVector> points)
{
	_SplinePoints = points;
}

void AATS_NavigationLane::SetTags(TArray<ELaneType> tags)
{
	_LaneTags = tags;
}