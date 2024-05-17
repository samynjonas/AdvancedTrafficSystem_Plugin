// Fill out your copyright notice in the Description page of Project Settings.
#include "../Public/ATS_PedestrianNavigation.h"
#include "../Public/ATS_TrafficManager.h"
#include "../Public/ATS_SpatialPartitioning.h"
#include "../Public/ATS_LaneSpline.h"
#include "../Public/ATS_TrafficAwarenessComponent.h"

#include "Components/SplineComponent.h"

void UATS_PedestrianNavigation::BeginPlay()
{
	Super::BeginPlay();

	// Variables
	_CurrentVelocity	= FVector::ZeroVector;
	_Position			= GetOwner()->GetActorLocation();
	_pSteeringManager	= MakeUnique<steeringManager>(this);
	
	// Traffic manager
	if (m_pTrafficManager == nullptr)
	{
		m_pTrafficManager = GetWorld()->SpawnActor<AATS_TrafficManager>();
	}

	if (m_pTrafficManager->Initialize() == false)
	{
		return;
	}

	// Spatial partitioning
	if (RetrieveSpatialUnit())
	{
		m_pSpatialUnit->SetIsMoveable(true);
	}
	
	RetrieveNextLanePoints();
}

TArray<FVector> UATS_PedestrianNavigation::GetPathPoints() const
{
	TArray<FVector> pathPoints;
	if (m_pTrafficManager == nullptr)
	{
		return pathPoints;
	}

	TArray<FLanePoint> lanePoints{};
	lanePoints = m_pTrafficManager->GetLanePoints(GetOwner());
	if (lanePoints.IsEmpty())
	{
		return pathPoints;
	}

	for (FLanePoint lanePoint : lanePoints)
	{
		pathPoints.Add(lanePoint.position);
	}

	return pathPoints;
}

bool UATS_PedestrianNavigation::MoveToNextPointSimple(float deltaTime)
{
	/*
		This is starting to look like a behavior tree so ideally we convert to using it from Unreal Engine
	*/
	
	if (_pSteeringManager == nullptr)
	{
		return false;
	}
	TArray<AActor*> nearbyActors = GetNearbyActors<UATS_PedestrianNavigation>();

	// First we have to check if we can continue on the path - the way to do this is by checking the current lane
	// -- An example is with modifier, if there is a modifier and it has closed the lane the agent will have to arrive on that points and wait until it can continue
	if (UATS_TrafficAwarenessComponent* laneModifier = WillPassLaneModifier())
	{
		UE_LOG(LogTemp, Warning, TEXT("UATS_PedestrianNavigation::MoveToNextPointSimple -- Lane modifier found"));
		//Check if the lane modfier is open or closed -- if it is closed, it will be the target for an arrive state
		if (laneModifier->CanAgentPass() == false)
		{
			// We have to arrive at the lane modifier
			_pSteeringManager->Seek(laneModifier->GetLanePoint(), 250.f, 1.f);
		}	
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UATS_PedestrianNavigation::MoveToNextPointSimple -- No lane modifier found"));
		if (_bUseSeek)
	{
		_pSteeringManager->Seek(RetrieveTarget(), (_SeekImpact / 100.f));
	}
		if (_bUseFlee)
	{
		_pSteeringManager->Flee(RetrieveTarget(), (_FleeImpact / 100.f));
	}
		if (_bUseWander)
	{
		_pSteeringManager->Wander(_WanderAngle, _WanderAngleChange, _WanderRadius, (_WanderImpact / 100.f));
	}
		if (_bUseFollowPath)
	{
		// Check if the currentPath point is the end of the lane -- if this is the case we take the next lane
		_pSteeringManager->FollowPath(_PathPoints, _CurrentPathPointIndex, _PathPointCheckDistance, (_FollowPathImpact / 100.f));
		if (_CurrentPathPointIndex >= _PathPoints.Num())
		{
			if (RetrieveNextLanePoints() == false)
			{
				return false;
			}
		}
	}
	}
	
	if (_bUseSeperation)
	{
		_pSteeringManager->Seperation(nearbyActors, _SeperationDistance, (_SeperationImpact / 100.f));
	}
	if (_bUseCohesion)
	{
		_pSteeringManager->Cohesion(nearbyActors, _CohesionDistance, (_CohesionImpact / 100.f));
	}
	if (_bUseAlignment)
	{
		_pSteeringManager->Alignment(nearbyActors, _AlignmentDistance, (_AlignmentImpact / 100.f));
	}


	// Update
	_pSteeringManager->Update(deltaTime);

	// Update position
	GetOwner()->SetActorLocation(_Position);
	GetOwner()->SetActorRotation(_CurrentRotation);

	//Draw the first one
	if (_PathPoints.Num() > 0)
	{
		DrawDebugSphere(GetWorld(), _PathPoints[_CurrentPathPointIndex], 150.f, 12, FColor::Red, false, 0.1f);
	}
	return true;
}

bool UATS_PedestrianNavigation::GetPathPointsFromNavigationSystem(AATS_LaneSpline* pLane)
{
	// Traffic manager has knowledge about the navigationsystem points
	if (m_pTrafficManager == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("UATS_PedestrianNavigation::GetPathPointsFromNavigationSystem -- Traffic manager is null"));
		return false;
	}

	if (m_pTrafficManager->Initialize() == false)
	{
		UE_LOG(LogTemp, Error, TEXT("UATS_PedestrianNavigation::GetPathPointsFromNavigationSystem -- Traffic manager failed to initialize"));
		return false;
	}

	if (pLane == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("UATS_PedestrianNavigation::GetPathPointsFromNavigationSystem -- Current lane is null"));
		return false;
	}

	USplineComponent* pSpline = pLane->GetSpline();
	if (pSpline == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("UATS_PedestrianNavigation::GetPathPointsFromNavigationSystem -- Spline is null"));
		return false;
	}
	
	TArray<FVector> pathPoints;	

	//Find actor distance along spline
	float actorInputKey = pSpline->FindInputKeyClosestToWorldLocation(GetOwner()->GetActorLocation());
	float actorDistance = pSpline->GetDistanceAlongSplineAtSplinePoint(actorInputKey);

	int pointsCount = pSpline->GetNumberOfSplinePoints();
	int closestPointIndex{ -1 };
	for (int i = 0; i < pointsCount; ++i)
	{
		FVector point		= pSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
		float pointDistance = pSpline->GetDistanceAlongSplineAtSplinePoint(i);

		if (closestPointIndex == -1 && pointDistance > actorDistance)
		{
			closestPointIndex = i;
		}

		pathPoints.Add(point);
	}

	if (pathPoints.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("UATS_PedestrianNavigation::GetPathPointsFromNavigationSystem -- No path points found"));
		return false;
	}

	if (closestPointIndex == -1)
	{
		closestPointIndex = 0;
	}

	_CurrentPathPointIndex = closestPointIndex;
	_PathPoints = pathPoints;
	return true;
}

bool UATS_PedestrianNavigation::RetrieveNextLanePoints()
{
	if (_pCurrentLane == nullptr)
	{
		_pCurrentLane = m_pTrafficManager->GetClosestLane(GetOwner()->GetActorLocation(), ELaneType::ATS_Pedestrian);	
	}
	else
	{
		// The current lane is set, we can get the next lane
		TArray<AATS_LaneSpline*> nextLanes = _pCurrentLane->GetNextLanes();

		//Pick a random lane
		if (nextLanes.Num() > 0)
		{
			int randomIndex = FMath::RandRange(0, nextLanes.Num() - 1);
			_pCurrentLane = nextLanes[randomIndex];
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("UATS_PedestrianNavigation::RetrieveNextLanePoints -- No next lanes found"));
			return false;
		}
	}

	if (_pCurrentLane == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("UATS_PedestrianNavigation::RetrieveNextLanePoints -- Current lane is null"));
		return false;
	}

	return GetPathPointsFromNavigationSystem(_pCurrentLane);
}

UATS_TrafficAwarenessComponent* UATS_PedestrianNavigation::WillPassLaneModifier()
{
	if (_pCurrentLane == nullptr)
	{
		return nullptr;
	}

	TArray<UATS_TrafficAwarenessComponent*> laneModifiers = _pCurrentLane->GetLaneModifiers();
	if (laneModifiers.IsEmpty())
	{
		return nullptr;
	}

	// Check if the agent will pass a lane modifier before it will reach the next point
	float pointDistance{ 0.f };
	int localPointIndex{ _CurrentPathPointIndex };
	if (localPointIndex > _PathPoints.Num() - 1)
	{
		localPointIndex = _PathPoints.Num() - 1;
	}
	pointDistance = _pCurrentLane->GetSpline()->GetDistanceAlongSplineAtSplinePoint(localPointIndex);

	float agentDistance{ 0.f };
	agentDistance = _pCurrentLane->GetDistanceAlongSpline(GetOwner()->GetActorLocation());

	//Get the next closest lane modifier
	UATS_TrafficAwarenessComponent* pClosestLaneModifier = nullptr;
	float laneModifierDistance{ 0.f };
	for (UATS_TrafficAwarenessComponent* laneModifier : laneModifiers)
	{
		float distance = _pCurrentLane->GetDistanceAlongSpline(laneModifier->GetOwner()->GetActorLocation());
		if (distance > agentDistance)
		{
			if (pClosestLaneModifier == nullptr)
			{
				pClosestLaneModifier = laneModifier;
				laneModifierDistance = distance;
			}
			else
			{
				if (distance < laneModifierDistance)
				{
					pClosestLaneModifier = laneModifier;
					laneModifierDistance = distance;
				}
			}
		}
	}

	if (pointDistance < laneModifierDistance)
	{
		return nullptr;
	}
	return pClosestLaneModifier;
}

FVector UATS_PedestrianNavigation::RetrieveTarget()
{
	if (_GoalActor)
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("UATS_PedestrianNavigation::MoveToNextPointPhysics -- Goal actor set as target"));
		}
		return _GoalActor->GetActorLocation();
	}

	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("UATS_PedestrianNavigation::MoveToNextPointPhysics -- No valid target found"));
	}
	return GetOwner()->GetActorLocation();
}

//-----------------------------------------------------------------------------------------------
// AGENT DATA GETTERS & SETTERS
//-----------------------------------------------------------------------------------------------

FVector UATS_PedestrianNavigation::GetCurrentVelocity() const
{
	return _CurrentVelocity;
}
FVector UATS_PedestrianNavigation::GetPosition() const
{
	return _Position;
}
FRotator UATS_PedestrianNavigation::GetCurrentRotation() const
{
	return _CurrentRotation;
}

float UATS_PedestrianNavigation::GetMaxSpeed() const
{
	return _MaxSpeed;
}
float UATS_PedestrianNavigation::GetMaxForce() const
{
	return _MaxForce;
}
float UATS_PedestrianNavigation::GetMass() const
{
	return _Mass;
}

void UATS_PedestrianNavigation::SetPosition(const FVector& newPosition)
{
	_Position = newPosition;
}
void UATS_PedestrianNavigation::SetCurrentVelocity(const FVector& newVelocity)
{
	_CurrentVelocity = newVelocity;
}
void UATS_PedestrianNavigation::SetCurrentRotation(const FRotator& newRotation)
{
	_CurrentRotation = newRotation;
}

//-----------------------------------------------------------------------------------------------
// Steering manager
//-----------------------------------------------------------------------------------------------

steeringManager::steeringManager(UATS_PedestrianNavigation* pHost)
	: _pHost{ pHost }
	, _Steering{ FVector::ZeroVector }
{

}

void steeringManager::Update(float deltaTime)
{
	FVector velocity{ FVector::ZeroVector };
	FVector position{ FVector::ZeroVector };

	velocity = _pHost->GetCurrentVelocity();
	position = _pHost->GetPosition();

	_Steering = _Steering.GetClampedToMaxSize(_pHost->GetMaxForce());
	_Steering *= 1 / _pHost->GetMass();

	//velocity += _Steering;
	velocity += _Steering.GetSafeNormal();
	velocity = velocity.GetClampedToMaxSize(_pHost->GetMaxSpeed());

	position += velocity * deltaTime;
	position.Z = _pHost->GetPosition().Z;

	_pHost->SetCurrentVelocity(velocity);
	_pHost->SetPosition(position);

	// Rotate the actor to face the direction of movement
	if (!velocity.IsNearlyZero()) // Check that velocity is not zero
	{
		FRotator newRotation = FRotationMatrix::MakeFromX(velocity).Rotator();
		newRotation.Roll	= 0;
		newRotation.Pitch	= 0;

		//UE_LOG(LogTemp, Warning, TEXT("steeringManager::Update -- New rotation: %s"), *newRotation.ToString());
		_pHost->SetCurrentRotation(newRotation);
	}
}

//-----------------------------------------------------------------------------------------------
// Steering behaviors
//-----------------------------------------------------------------------------------------------

// Seek behavior
void steeringManager::Seek(const FVector& target, float slowingRadius, float impactScale)
{
	_Steering += DoSeek(target, slowingRadius, impactScale);
}

FVector steeringManager::DoSeek(const FVector& target, float slowingRadius, float impactScale)
{
	// Variables
	FVector force{ FVector::ZeroVector };
	FVector desiredVelocity{ FVector::ZeroVector };

	float distance{ 0.f };

	// Data
	desiredVelocity = target - _pHost->GetPosition();
	distance		= desiredVelocity.Length();

	desiredVelocity = desiredVelocity.GetSafeNormal();

	if(distance < slowingRadius)
	{
		desiredVelocity *= _pHost->GetMaxSpeed() * (distance / slowingRadius);
	}
	else
	{
		desiredVelocity *= _pHost->GetMaxSpeed();
	}

	force = desiredVelocity - _pHost->GetCurrentVelocity();
	return (force * impactScale);
}


// Flee behavior
void steeringManager::Flee(const FVector& target, float impactScale)
{
	_Steering += DoFlee(target, impactScale);
}

FVector steeringManager::DoFlee(const FVector& target, float impactScale)
{
	// Variables
	FVector force{ FVector::ZeroVector };
	FVector desiredVelocity{ FVector::ZeroVector };

	// Data
	desiredVelocity = target - _pHost->GetPosition();
	desiredVelocity = desiredVelocity.GetSafeNormal();
	desiredVelocity *= _pHost->GetMaxSpeed();

	force = -desiredVelocity - _pHost->GetCurrentVelocity();
	return (force * impactScale);
}


// Wander behavior
void steeringManager::Wander(float& agentWanderAngle, float maxAngleChange, float circleRadius, float impactScale)
{
	_Steering += DoWander(agentWanderAngle, maxAngleChange, circleRadius, impactScale);
}

FVector steeringManager::DoWander(float& agentWanderAngle, float maxAngleChange, float circleRadius, float impactScale)
{
	// Variables
	FVector steering{ FVector::ZeroVector };
	FVector circleCenter{ FVector::ZeroVector };
	FVector displacement{ FVector::ZeroVector };

	// Calculate circle center
	circleCenter = _pHost->GetCurrentVelocity().GetSafeNormal() * circleRadius;

	// Calculate displacement
	displacement = FVector(0, -1, 0);
	displacement *= circleRadius;

	// Randomly change the direction
	float halfAngleChange = FMath::DegreesToRadians(maxAngleChange) * 0.5f;
	agentWanderAngle += FMath::FRandRange(-halfAngleChange, halfAngleChange);  // Incrementally update angle
	agentWanderAngle = FMath::Fmod(agentWanderAngle, 360.0f);

	float length = displacement.Length();
	displacement.X = FMath::Cos(agentWanderAngle) * length;
	displacement.Y = FMath::Sin(agentWanderAngle) * length;

	FVector wanderForce = circleCenter + displacement;

	steering = wanderForce.GetClampedToMaxSize(_pHost->GetMaxForce());
	return (steering * impactScale);
}


// Follow path behavior
void steeringManager::FollowPath(const TArray<FVector>& path, int& currentPoint, float switchPointDistance, float impactScale)
{
	_Steering += DoFollowPath(path, currentPoint, switchPointDistance, impactScale);
}

FVector steeringManager::DoFollowPath(const TArray<FVector>& path, int& currentPoint, float switchPointDistance, float impactScale)
{
	// Variables
	FVector force{ FVector::ZeroVector };
	FVector desiredVelocity{ FVector::ZeroVector };
	FVector currentTarget{ FVector::ZeroVector };

	// Data
	if (path.IsEmpty())
	{
		return force;
	}

	if(currentPoint >= path.Num() || currentPoint < 0)
	{
		currentPoint = 0;
	}

	if (path.IsValidIndex(currentPoint) == false)
	{
		return force;
	}
	currentTarget = path[currentPoint];

	//Check if we can move to the next point
	if((currentTarget - _pHost->GetPosition()).Size() < switchPointDistance)
	{
		currentPoint++;
	}

	return DoSeek(currentTarget, switchPointDistance, impactScale);
}


// Seperation behavior
void steeringManager::Seperation(TArray<AActor*> agents, float minSeperationDistance, float impactScale)
{
	_Steering += DoSeperation(agents, minSeperationDistance, impactScale);
}

FVector steeringManager::DoSeperation(TArray<AActor*> agents, float minSeperationDistance, float impactScale)
{
	FVector steering{ FVector::ZeroVector };
	FVector force{ FVector::ZeroVector };
	float neighboursCount{ 0 };

	if (!_pHost || !_pHost->GetOwner()) return FVector::ZeroVector; // Safety check

	for (AActor* agent : agents)
	{
		if (agent == _pHost->GetOwner())
		{
			continue; // Skip the host itself
		}

		FVector distanceVec = agent->GetActorLocation() - _pHost->GetPosition();
		float distance = distanceVec.Length();

		if (distance > 0 && distance < minSeperationDistance)
		{
			FVector pushForce = _pHost->GetPosition() - agent->GetActorLocation();
			force += pushForce;
			neighboursCount++;
		}
	}

	if (neighboursCount > 0)
	{
		force /= neighboursCount;										// Average the force
		steering = force.GetClampedToMaxSize(_pHost->GetMaxForce());	// Clamp to max force
	}

	return steering * impactScale; // Scale the steering impact
}


// Cohesion behavior
void steeringManager::Cohesion(TArray<AActor*> agents, float maxCohesion, float impactScale)
{
	_Steering += DoCohesion(agents, maxCohesion, impactScale);
}

FVector steeringManager::DoCohesion(TArray<AActor*> agents, float maxCohesion, float impactScale)
{
	FVector steering{ FVector::ZeroVector };
	FVector force{ FVector::ZeroVector };
	FVector centerOfMass{  };
	float neighboursCount{ 1 };

	if (!_pHost || !_pHost->GetOwner()) return FVector::ZeroVector; // Safety check

	centerOfMass = _pHost->GetPosition();

	for (AActor* agent : agents)
	{
		if (agent == _pHost->GetOwner())
		{
			continue; // Skip the host itself
		}

		float distance = FVector::Distance(agent->GetActorLocation(), _pHost->GetPosition());
		if (distance < maxCohesion)
		{
			centerOfMass += agent->GetActorLocation();
			neighboursCount++;
		}
	}

	if (neighboursCount == 1)
	{
		return steering;
	}


	centerOfMass /= neighboursCount; // Average the position

	return DoSeek(centerOfMass, maxCohesion, impactScale); // Seek the center of mass
}


// Alignment behavior
void steeringManager::Alignment(TArray<AActor*> agents, float maxCohesion, float impactScale)
{
	_Steering += DoAlignment(agents, maxCohesion, impactScale);
}

FVector steeringManager::DoAlignment(TArray<AActor*> agents, float maxCohesion, float impactScale)
{
	FVector steering{ FVector::ZeroVector };
	FVector averageHeading{ FVector::ZeroVector };
	FVector desired{ FVector::ZeroVector };
	float neighboursCount{ 0 };

	if (!_pHost || !_pHost->GetOwner()) return FVector::ZeroVector; // Safety check

	for (AActor* agent : agents)
	{
		if (agent == _pHost->GetOwner())
		{
			continue; // Skip the host itself
		}

		float distance = FVector::Distance(agent->GetActorLocation(), _pHost->GetPosition());
		if (UATS_PedestrianNavigation* pedestrianNav = agent->FindComponentByClass<UATS_PedestrianNavigation>())
		{
			if (pedestrianNav->GetCurrentVelocity().Length() > 0)
			{
				averageHeading += pedestrianNav->GetCurrentVelocity().GetSafeNormal();
				neighboursCount++;
			}
		}
	}

	if (neighboursCount == 0)
	{
		return steering;
	}

	averageHeading /= neighboursCount; // Average the heading

	desired = averageHeading * _pHost->GetMaxSpeed();				// Desired velocity
	steering = desired - _pHost->GetCurrentVelocity();				// Calculate steering
	steering = steering.GetClampedToMaxSize(_pHost->GetMaxForce()); // Clamp to max force

	return steering * impactScale; // Scale the steering impact
}