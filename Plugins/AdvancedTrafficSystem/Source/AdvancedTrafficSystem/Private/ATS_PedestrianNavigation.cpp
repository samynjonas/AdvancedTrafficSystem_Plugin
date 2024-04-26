// Fill out your copyright notice in the Description page of Project Settings.
#include "../Public/ATS_PedestrianNavigation.h"
#include "../Public/ATS_TrafficManager.h"

void UATS_PedestrianNavigation::BeginPlay()
{
	Super::BeginPlay();

	// Variables
	_CurrentVelocity	= FVector::ZeroVector;
	_Position			= GetOwner()->GetActorLocation();
	_pSteeringManager	= MakeUnique<steeringManager>(this);

	GetPathPointsFromNavigationSystem();
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
	if (_pSteeringManager == nullptr)
	{
		return false;
	}

	// Behaviors to add
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
		_pSteeringManager->FollowPath(_PathPoints, _CurrentPathPointIndex, _PathPointCheckDistance, (_FollowPathImpact / 100.f));
	}
	//_pSteeringManager->Flee(RetrieveTarget());
	//_pSteeringManager->Wander(_WanderAngle, _WanderAngleChange, _WanderRadius);
	//_pSteeringManager->FollowPath(_PathPoints, _CurrentPathPointIndex, _PathPointCheckDistance);

	// Update
	_pSteeringManager->Update(deltaTime);

	// Update position
	GetOwner()->SetActorLocation(_Position);

	//Draw the first one
	if (_PathPoints.Num() > 0)
	{
		DrawDebugSphere(GetWorld(), _PathPoints[_CurrentPathPointIndex], 150.f, 12, FColor::Red, false, 0.1f);
	}
	return true;
}

bool UATS_PedestrianNavigation::GetPathPointsFromNavigationSystem()
{
	// Traffic manager has knowledge about the navigationsystem points
	if (m_pTrafficManager == nullptr)
	{
		return false;
	}

	if (m_pTrafficManager->IsInitialized() == false)
	{
		m_pTrafficManager->Initialize();
	}

	//Retrieve point from navigation system
	TArray<FLanePoint> lanePoints = m_pTrafficManager->GetAllLanePoints(GetOwner(), m_pTrafficManager->GetPedestrianTag());

	if (lanePoints.IsEmpty())
	{
		return false;
	}

	//compare the points with the current distance along lane to know what point is the next one
	
	bool hasFoundNextPoint{ false };
	float agentLaneDistance{ m_AgentData.agentDistanceOnSpline };
	
	TArray<FVector> pathPoints;
	for (int index{ 0 }; index < lanePoints.Num(); ++index)
	{
		FLanePoint lanePoint = lanePoints[index];
		float pointLaneDistance = lanePoint.distanceAlongLane;
		if (hasFoundNextPoint == false && pointLaneDistance > agentLaneDistance)
		{
			_CurrentPathPointIndex = index;
			hasFoundNextPoint = true;
		}
		pathPoints.Add(lanePoint.position);
	}
	_PathPoints = pathPoints;

	return true;
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

	//if (Retriel())
	//{
	//	if (bDebug)
	//	{
	//		UE_LOG(LogTemp, Warning, TEXT("UATS_PedestrianNavigation::MoveToNextPointPhysics -- Path point set as target"));
	//	}
	//	return _PathPoints[_CurrentPathPointIndex];
	//}

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

	velocity += _Steering;
	velocity = velocity.GetClampedToMaxSize(_pHost->GetMaxSpeed());

	position += velocity * deltaTime;

	_pHost->SetCurrentVelocity(velocity);
	_pHost->SetPosition(position);
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

	if(currentPoint >= path.Num())
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
		if (currentPoint >= path.Num())
		{
			currentPoint = 0;
		}
	}

	return DoSeek(currentTarget, switchPointDistance, impactScale);
}