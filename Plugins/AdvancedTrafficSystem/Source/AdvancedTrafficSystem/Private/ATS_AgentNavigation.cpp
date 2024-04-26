// Fill out your copyright notice in the Description page of Project Settings.
#include "../Public/ATS_AgentNavigation.h"
#include "../Public/ATS_TrafficManager.h"

#include "ChaosVehicleMovementComponent.h"
#include "ZoneShapeComponent.h"

#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

//--------------------------------------------------------------------------------------------
// Default Unreal Engine Functions
//--------------------------------------------------------------------------------------------

UATS_AgentNavigation::UATS_AgentNavigation()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UATS_AgentNavigation::BeginPlay()
{
	Super::BeginPlay();

	//Get TrafficManager and make sure it's initialzed already
	TArray<AActor*> pTrafficManagers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AATS_TrafficManager::StaticClass(), pTrafficManagers);
	for (AActor* pActor : pTrafficManagers)
	{
		AATS_TrafficManager* pTrafficManager = Cast<AATS_TrafficManager>(pActor);
		if (pTrafficManager)
		{
			m_pTrafficManager = pTrafficManager;
			break;
		}
	}

	if (m_pTrafficManager)
	{
		if (m_pTrafficManager->IsInitialized() == false)
		{
			m_pTrafficManager->Initialize();
		}
	}

	//Try to get the ChaosVehicleMovementComponent
	if (bIsPhysicsBased)
	{
		m_pVehicleComponent = GetOwner()->FindComponentByClass<UChaosVehicleMovementComponent>();
		if (m_pVehicleComponent == nullptr)
		{
			bIsPhysicsBased = false;
		}
	}

	//Set the max speed
	SetMaxSpeed(m_MaxSpeedkmph);
	
	//Initialize the agent data
	m_AgentData.bFollowsSpline			= true;
	m_AgentData.agentTransform			= GetOwner()->GetActorTransform();
	m_AgentData.agentSpeed				= 0.f;
	m_AgentData.agentDistanceOnSpline	= 0.f;
	m_AgentData.agentBox				= FVector(100.f, 100.f, 100.f); //Should be changed -- depedends on the vehicle
	bIsDisabled = true;

	//Register the agent
	bool bTempBool{ false };
	FVector NextPoint = m_pTrafficManager->GetNextNavigationPoint(GetOwner(), MaxPointDistance, bTempBool, m_AgentData);

	//THESE POINTS CAN BE USED TO DETERMINE WHERE THE CORNERS ARE
	RetrieveLanePoints();
}

void UATS_AgentNavigation::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bDrawDebugPath)
	{
		VisualizePath(true);
	}

	//if (bIsDisabled)
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::Tick -- Agent is disabled"));
	//	return;
	//}

	if (bFollowPath)
	{
		if (m_NavigationPath.bIsFollowingPath)
		{
			if (m_NavigationPath.bHasReachedEnd == true)
			{
				return;
			}
			FollowPath(DeltaTime);
		}
		else
		{
			RetrievePath();
		}
	}
	else
	{
		if (bIsParked == false)
		{
			MoveToNextPoint(DeltaTime);
		}
	}	
	UpdateAgentData();
}

void UATS_AgentNavigation::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	Super::OnComponentDestroyed(bDestroyingHierarchy);
	if (m_pTrafficManager)
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::Unregistering agent %s"), *GetOwner()->GetName());
		}
		m_pTrafficManager->UnregisterAgent(GetOwner(), this);
	}
}

//--------------------------------------------------------------------------------------------
// Navigation functions
//--------------------------------------------------------------------------------------------

void UATS_AgentNavigation::SetFollowPath(bool isEnabled)
{
	bFollowPath = isEnabled;
}

void UATS_AgentNavigation::SetNavGoal(const FVector& destination)
{
	m_Destination	= destination;
	bFollowPath		= true;

	m_NavigationPath.bIsFollowingPath	= false;
	m_NavigationPath.bHasReachedEnd		= false;

	EnableAgent();
}

bool UATS_AgentNavigation::MoveToNextPoint(float deltaTime)
{
	//Check if TrafficManager is initialized
	if (m_pTrafficManager && m_pTrafficManager->IsInitialized())
	{
		//If the vehicle is physics based
		if (bIsPhysicsBased)
		{
			return MoveToNextPointPhysics(deltaTime);
		}
		else
		{
			return MoveToNextPointSimple(deltaTime);
		}
	}
	return false;
}

bool UATS_AgentNavigation::MoveToNextPointPhysics(float deltaTime)
{
	return false;
}

bool UATS_AgentNavigation::MoveToNextPointSimple(float deltaTime)
{
	return true;
}

bool UATS_AgentNavigation::FollowPath(float deltaTime)
{
	/*
	//Loop over path
	if (bIsPhysicsBased)
	{
		if (m_pTrafficManager && m_pTrafficManager->IsInitialized())
		{
			//--------------------------------------------------------
			//----------------NEXT POINT CALCULATION------------------
			//--------------------------------------------------------
			float CurrentSpeed = m_pVehicleComponent->GetForwardSpeed(); // Assuming VehicleMovement is your vehicle's movement component
			float SpeedFactor = FMath::Clamp(CurrentSpeed / m_MaxSpeedUnrealunits, 0.0f, 1.0f);
			float desiredSpeedFactor = FMath::Clamp(m_DesiredSpeed / m_MaxSpeedUnrealunits, 0.0f, 1.0f);

			float DynamicPointDistance = FMath::Lerp(MinPointDistance, MaxPointDistance, SpeedFactor);
			float desiredPointDistance = FMath::Lerp(MinPointDistance, MaxPointDistance, desiredSpeedFactor);

			bool	tempBool{ false };
			bool	bStopDueTrafficRule{ false };

			FVector NextPoint			= m_pTrafficManager->GetNextNavigationPathPoint(GetOwner(), DynamicPointDistance, tempBool, m_AgentData, m_NavigationPath);
			FVector SecondNextPoint		= m_pTrafficManager->GetNextNavigationPathPoint(GetOwner(), DynamicPointDistance * 2.f, tempBool, m_AgentData, m_NavigationPath);

			FVector DesiredSpeedPoint	= m_pTrafficManager->GetNextNavigationPathPoint(GetOwner(), desiredPointDistance, bStopDueTrafficRule, m_AgentData, m_NavigationPath); //This will be used to calculate the desired speed of the car - depending on traffic it could be faster or slower
			FVector carInfront			= m_pTrafficManager->GetTrafficAwareNavigationPoint(this, m_AgentData, desiredPointDistance * 1.5f);
			FVector pathEndPoint		= m_pTrafficManager->GetPathEndNavigationPoint(this, m_AgentData, m_NavigationPath, desiredPointDistance * 1.5f);

			if (carInfront.IsZero() == false)
			{
				DesiredSpeedPoint = carInfront;
				bStopDueTrafficRule = true;
			}
			else if (pathEndPoint.IsZero() == false)
			{
				DesiredSpeedPoint = pathEndPoint;
				bStopDueTrafficRule = true;

				if (IsGoalPointReached(pathEndPoint))
				{
					m_NavigationPath.bHasReachedEnd = true;
					m_NavigationPath.bIsFollowingPath = true;
				}
			}

			if (bDebug)
			{
				const float THICKNESS = 5.f;
				DrawDebugSphere(GetWorld(), NextPoint, 100.f, 12, FColor::Red, false, 0.f, 0, THICKNESS);
				DrawDebugSphere(GetWorld(), SecondNextPoint, 100.f, 12, FColor::Red, false, 0.f, 0, THICKNESS);
				DrawDebugSphere(GetWorld(), DesiredSpeedPoint, 100.f, 12, FColor::Green, false, 0.f, 0, THICKNESS);
			}


			//--------------------------------------------------------
			//----------------VECTOR CALCULATIONS---------------------
			//--------------------------------------------------------
			FVector CurrentPosition = GetOwner()->GetActorLocation();
			FVector CurrentForward	= GetOwner()->GetActorForwardVector();
			FVector ToNextPoint		= (NextPoint - CurrentPosition).GetSafeNormal();

			float DistanceToNextPoint = FVector::Dist(CurrentPosition, NextPoint);

			//--------------------------------------------------------
			//----------------TURNING CALCULATIONS--------------------
			//--------------------------------------------------------
			float TurnSharpness = CalculateTurnSharpness(CurrentForward, ToNextPoint, DistanceToNextPoint);
			
			//float dryAsphaltFriction = 0.8f;
			float turnRadius = CalculateTurnRadius(TurnSharpness, CurrentSpeed);
			float MaxCorneringSpeed = CalculateMaxCorneringSpeed(_DryAsphaltFriction, CurrentPosition, NextPoint, SecondNextPoint);

			//--------------------------------------------------------
			//----------------STEERING CALCULATIONS-------------------
			//--------------------------------------------------------
			float SteeringInput;
			SteeringInput = CalculateSteeringInput(CurrentForward, ToNextPoint);

			//--------------------------------------------------------
			//----------------BRAKING AND THROTTLE--------------------
			//--------------------------------------------------------
			bool bApplyBrake = ShouldBrake(TurnSharpness, CurrentSpeed, MaxCorneringSpeed);
			float ThrottleInput, BrakeInput;
			if (bApplyBrake || bStopDueTrafficRule)
			{
				ThrottleInput = 0.0f; // No throttle when braking
				BrakeInput = 0.5f; // Maximum brake
			}
			else
			{
				float ThrottleReduction = FMath::Clamp(TurnSharpness, 0.0f, 1.0f);
				ThrottleInput = CalculateThrottleInput(CurrentPosition, NextPoint) * (1.0f - ThrottleReduction);
				BrakeInput = 0.0f; // No brake
			}


			//--------------------------------------------------------
			//----------------APPLY ALL-------------------------------
			//--------------------------------------------------------
			if (bDebug)
			{
				Debugging(CurrentSpeed, SteeringInput, ThrottleInput, BrakeInput, 0);
			}

			return ApplyVehicleControl(SteeringInput, ThrottleInput, BrakeInput);
		}
	}
	else
	{
		if (m_pTrafficManager && m_pTrafficManager->IsInitialized())
		{
			//--------------------------------------------------------
			//----------------NEXT POINT CALCULATION------------------
			//--------------------------------------------------------
			float desiredSpeedFactor = FMath::Clamp(m_DesiredSpeed / m_MaxSpeedUnrealunits, 0.0f, 1.0f);
			float desiredPointDistance = FMath::Lerp(MinPointDistance, MaxPointDistance, desiredSpeedFactor);

			bool	tempBool{ false };
			bool	bStopDueTrafficRule{ false };

			FVector DesiredSpeedPoint = m_pTrafficManager->GetNextNavigationPathPoint(GetOwner(), desiredPointDistance, bStopDueTrafficRule, m_AgentData, m_NavigationPath); //This will be used to calculate the desired speed of the car - depending on traffic it could be faster or slower
			FVector carInfront = m_pTrafficManager->GetTrafficAwareNavigationPoint(this, m_AgentData, desiredPointDistance * 1.5f);
			FVector pathEndPoint = m_pTrafficManager->GetPathEndNavigationPoint(this, m_AgentData, m_NavigationPath, desiredPointDistance * 1.5f);

			if (carInfront.IsZero() == false)
			{
				if (bDebug)
				{
					UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::Car infront"));
				}

				DesiredSpeedPoint = carInfront;
				bStopDueTrafficRule = true;
			}
			else if (pathEndPoint.IsZero() == false)
			{
				if (bDebug)
				{
					UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::Path end in sight"));
				}


				DesiredSpeedPoint = pathEndPoint;
				bStopDueTrafficRule = true;

				if (IsGoalPointReached(pathEndPoint))
				{
					m_NavigationPath.bHasReachedEnd = true;
					m_NavigationPath.bIsFollowingPath = false;
				}
			}

			if (bDebug)
			{
				const float THICKNESS = 5.f;
				DrawDebugSphere(GetWorld(), DesiredSpeedPoint, 100.f, 12, FColor::Green, false, 0.f, 0, THICKNESS);
			}

			//--------------------------------------------------------
			//----------------VECTOR CALCULATIONS---------------------
			//--------------------------------------------------------
			FVector CurrentPosition = GetOwner()->GetActorLocation();
			FVector CurrentForward = GetOwner()->GetActorForwardVector();

			// Calculate the direction vector towards the desired point
			FVector DirectionToPoint = (DesiredSpeedPoint - CurrentPosition).GetSafeNormal();
			float DistanceToNextPoint = FVector::Dist(CurrentPosition, DesiredSpeedPoint);


			//--------------------------------------------------------
			//----------------APPLY ALL-------------------------------
			//--------------------------------------------------------
			// Calculate the new position by moving the current position towards the desired point
			FVector NewPosition = FMath::Lerp(CurrentPosition, DesiredSpeedPoint, deltaTime * desiredSpeedFactor);

			// Set the vehicle's position to the new position
			GetOwner()->SetActorLocation(NewPosition);

			// Optionally, update the vehicle's rotation to face the direction of movement
			FRotator NewRotation = DirectionToPoint.Rotation();
			GetOwner()->SetActorRotation(NewRotation);

			return true;
		}
	}
	*/
	return false;
}

void UATS_AgentNavigation::RetrievePath()
{
	if (m_pTrafficManager == nullptr)
	{
		return;
	}

	if (m_pTrafficManager->IsInitialized() == false)
	{
		m_pTrafficManager->Initialize();
	}

	FTrafficNavigationPath navigationPath = m_pTrafficManager->FindPath(GetOwner()->GetActorLocation(), m_Destination);
	if (navigationPath.path.IsEmpty() == false)
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::Path found"));
		}

		m_NavigationPath = navigationPath;
		m_NavigationPath.bIsFollowingPath = true;
		bIsDisabled = false;
	}
}

bool UATS_AgentNavigation::IsGoalPointReached(const FVector& point) const
{
	FVector CurrentPosition = GetOwner()->GetActorLocation();
	float DistanceToNextPoint = FVector::DistSquared(CurrentPosition, point);

	return DistanceToNextPoint < 100.f;
}

bool UATS_AgentNavigation::RetrieveLanePoints()
{
	/*
		Lane points are the zonegraphs points on the lane of the vehicle they will return the current lane points and the points for the next lane if they are known

		A way we can use this is for corner detection and maybe steering?
		For corner detected we can just say that each point is a corner point
		For steering we can do a bit more, we could use the points to generate a spline that the vehicle will follow
			To achieve this we will need to have the previous point also and than we can generate a spline path between all the known points, 
			the diffilcult part will be adding extra points near corners so that the corner spline is looking realistic
	*/

	if (m_pTrafficManager == nullptr)
	{
		return false;
	}

	m_LanePoints = m_pTrafficManager->GetLanePoints(GetOwner());
	if (m_LanePoints.Num() == 0)
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("AgentNavigation::RetrieveLanePoints() - Lane points are empty"));
		}
		return false;
	}

	if (bDrawDebugObjects)
	{
		for (FLanePoint point : m_LanePoints)
		{
			//UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::RetrieveLanePoints() - Point %s"), *point.ToString());
			DrawDebugSphere(GetWorld(), point.position, 100.f, 12, FColor::Green, false, 0.f, 0, 5.f);
		}
	}

	m_CurrentPointIndex = 0;
	return true;
}

//--------------------------------------------------------------------------------------------
// agent calculation functions
//--------------------------------------------------------------------------------------------

void UATS_AgentNavigation::SetMaxSpeed(float speedLimit)
{
	//Based on the previous and new speed - scale the pointDistance values
	float prevSpeed = m_MaxSpeedkmph;

	m_MaxSpeedkmph = speedLimit;
	m_MaxSpeedUnrealunits = m_MaxSpeedkmph / 0.036f;
	m_DesiredSpeed = m_MaxSpeedUnrealunits;

	float speedScale = m_MaxSpeedkmph / prevSpeed;

	MinPointDistance *= speedScale;
	MaxPointDistance *= speedScale;
}

//--------------------------------------------------------------------------------------------
// Debugging functions
//--------------------------------------------------------------------------------------------

void UATS_AgentNavigation::VisualizePath(bool isVisible)
{
	if (m_pTrafficManager == nullptr)
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("AgentNavigation::VisualizePath() - m_pTrafficManager is nullptr"));
		}
		return;
	}

	if (m_NavigationPath.path.Num() == 0)
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("AgentNavigation::VisualizePath() - m_NavigationPath.path is empty"));
		}
		return;
	}

	m_pTrafficManager->DrawPath(m_NavigationPath.path);
}

void UATS_AgentNavigation::Debugging(float currentSpeed, float steeringInput, float throttleInput, float brakingInput, float cornerAngle, bool isEnabled)
{
	if (!isEnabled)
	{
		return;
	}

	//Debugging on viewport - nicely formatted

	bool printToScreen{ false };
	const int32 key{ -1 };
	const float TimeToDisplay{ 0.001f };

	if (printToScreen)
	{
		FString speedMsg = FString::Printf(TEXT("AgentNavigation::Current Speed %d"), static_cast<int>(currentSpeed * 0.036f));
		GEngine->AddOnScreenDebugMessage(key, TimeToDisplay, FColor::Blue, speedMsg);

		FString steeringMsg = FString::Printf(TEXT("AgentNavigation::Steering input %d"), static_cast<int>(steeringInput * 100));
		GEngine->AddOnScreenDebugMessage(key, TimeToDisplay, FColor::Blue, steeringMsg);

		FString throttleMsg = FString::Printf(TEXT("AgentNavigation::Throttle input %d"), static_cast<int>(throttleInput * 100));
		GEngine->AddOnScreenDebugMessage(key, TimeToDisplay, FColor::Blue, throttleMsg);

		FString brakingMsg = FString::Printf(TEXT("AgentNavigation::Braking input %d"), static_cast<int>(brakingInput * 100));
		GEngine->AddOnScreenDebugMessage(key, TimeToDisplay, FColor::Blue, brakingMsg);

		FString cornerAngleMsg = FString::Printf(TEXT("AgentNavigation::Corner Angle %d"), static_cast<int>(cornerAngle));
		GEngine->AddOnScreenDebugMessage(key, TimeToDisplay, FColor::Blue, cornerAngleMsg);

		FString cornerMsg = FString::Printf(TEXT("AgentNavigation::In Corner %d"), bIsCornering);
		GEngine->AddOnScreenDebugMessage(key, TimeToDisplay, FColor::Blue, cornerMsg);
	}

	UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::Current Speed %d"), static_cast<int>(currentSpeed * 0.036f));

	UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::Steering input %d"), static_cast<int>(steeringInput * 100));

	UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::Throttle input %d"), static_cast<int>(throttleInput * 100));

	UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::Braking input %d"), static_cast<int>(brakingInput * 100));

	UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::Corner Angle %d"), static_cast<int>(cornerAngle));

	UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::In Corner %d"), bIsCornering);
}


//--------------------------------------------------------------------------------------------
// Other functions
//--------------------------------------------------------------------------------------------

void UATS_AgentNavigation::UpdateAgentData()
{
	m_AgentData.agentTransform = GetOwner()->GetActorTransform();
	if (m_pVehicleComponent)
	{
		m_AgentData.agentSpeed = m_pVehicleComponent->GetForwardSpeed();
	}	
}

void UATS_AgentNavigation::DissableAgent()
{
	if (bIsDisabled)
	{
		return;
	}

	bIsDisabled = true;
	m_AgentData.bIsEnabled = false;
}

void UATS_AgentNavigation::EnableAgent()
{
	if (bIsDisabled == false)
	{
		return;
	}

	bIsDisabled = false;
	m_AgentData.bIsEnabled = true;
}

