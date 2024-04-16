// Fill out your copyright notice in the Description page of Project Settings.


#include "../Public/ATS_AgentNavigation.h"
#include "../Public/ATS_TrafficManager.h"

#include "ChaosVehicleMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ZoneShapeComponent.h"
#include "EngineUtils.h"

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
		// Do something with each actor
		AATS_TrafficManager* pTrafficManager = Cast<AATS_TrafficManager>(pActor);
		if (pTrafficManager)
		{
			m_pTrafficManager = pTrafficManager;
			//Break out of loop - found trafficManager
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

	m_pVehicleComponent = GetOwner()->FindComponentByClass<UChaosVehicleMovementComponent>();
	if (m_pVehicleComponent == nullptr)
	{
		bIsPhysicsBased = false;
	}

	m_MaxSpeedUnrealunits	= m_MaxSpeedkmph / 0.036f;
	m_DesiredSpeed			= m_MaxSpeedUnrealunits;

	m_AgentData.bFollowsSpline			= true;
	m_AgentData.agentTransform			= GetOwner()->GetActorTransform();
	m_AgentData.agentSpeed				= 0.f;
	m_AgentData.agentDistanceOnSpline	= 0.f;
	m_AgentData.agentBox				= FVector(100.f, 100.f, 100.f); //Should be changed -- depedends on the vehicle

	bIsDisabled = true;

	bool bTempBool{ false };
	FVector NextPoint = m_pTrafficManager->GetNextNavigationPoint(GetOwner(), MaxPointDistance, bTempBool, m_AgentData);
}

void UATS_AgentNavigation::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bDrawDebugPath)
	{
		VisualizePath(true);
	}

	if (bIsDisabled)
	{
		UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::Tick -- Agent is disabled"));
		return;
	}

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
	//--------------------------------------------------------
	//----------------NEXT POINT CALCULATION------------------
	//--------------------------------------------------------
	float CurrentSpeed			= m_pVehicleComponent->GetForwardSpeed(); // Assuming VehicleMovement is your vehicle's movement component
	float SpeedFactor			= FMath::Clamp(CurrentSpeed / m_MaxSpeedUnrealunits, 0.0f, 1.0f);
	float desiredSpeedFactor	= FMath::Clamp(m_DesiredSpeed / m_MaxSpeedUnrealunits, 0.0f, 1.0f);

	float DynamicPointDistance  = FMath::Lerp(MinPointDistance, MaxPointDistance, SpeedFactor);
	float desiredPointDistance  = FMath::Lerp(MinPointDistance, MaxPointDistance, desiredSpeedFactor);

	bool	tempBool{ false };
	bool	bStopDueTrafficRule{ false };

	FVector NextPoint		= m_pTrafficManager->GetNextNavigationPoint(GetOwner(), DynamicPointDistance, tempBool, m_AgentData);
	FVector SecondNextPoint = m_pTrafficManager->GetNextNavigationPoint(GetOwner(), DynamicPointDistance * 2.f, tempBool, m_AgentData);

	FVector DesiredSpeedPoint	= m_pTrafficManager->GetNextNavigationPoint(GetOwner(), desiredPointDistance, bStopDueTrafficRule, m_AgentData); //This will be used to calculate the desired speed of the car - depending on traffic it could be faster or slower
	FVector carInfront			= m_pTrafficManager->GetTrafficAwareNavigationPoint(this, m_AgentData, desiredPointDistance * 1.5f);
	if (carInfront.IsZero() == false)
	{
		DesiredSpeedPoint = carInfront;
		bStopDueTrafficRule = true;
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
	FVector CurrentForward = GetOwner()->GetActorForwardVector();
	FVector ToNextPoint = (NextPoint - CurrentPosition).GetSafeNormal();

	float DistanceToNextPoint = FVector::Dist(CurrentPosition, NextPoint);

	//--------------------------------------------------------
	//----------------TURNING CALCULATIONS--------------------
	//--------------------------------------------------------
	float TurnSharpness = CalculateTurnSharpness(CurrentForward, ToNextPoint, DistanceToNextPoint);

	float dryAsphaltFriction = 0.8f;
	float turnRadius = CalculateTurnRadius(TurnSharpness, CurrentSpeed);
	float MaxCorneringSpeed = CalculateMaxCorneringSpeed(dryAsphaltFriction, CurrentPosition, NextPoint, SecondNextPoint);

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

bool UATS_AgentNavigation::MoveToNextPointSimple(float deltaTime)
{
	//--------------------------------------------------------
	//----------------NEXT POINT CALCULATION------------------
	//--------------------------------------------------------
	float desiredSpeedFactor	= FMath::Clamp(m_DesiredSpeed / m_MaxSpeedUnrealunits, 0.0f, 1.0f);
	float desiredPointDistance	= FMath::Lerp(MinPointDistance, MaxPointDistance, desiredSpeedFactor);

	bool	tempBool{ false };
	bool	bStopDueTrafficRule{ false };

	FVector DesiredSpeedPoint	= m_pTrafficManager->GetNextNavigationPoint(GetOwner(), desiredPointDistance, bStopDueTrafficRule, m_AgentData); //This will be used to calculate the desired speed of the car - depending on traffic it could be faster or slower
	FVector carInfront			= m_pTrafficManager->GetTrafficAwareNavigationPoint(this, m_AgentData, desiredPointDistance * 1.5f);
	if (carInfront.IsZero() == false)
	{
		DesiredSpeedPoint = carInfront;
		bStopDueTrafficRule = true;
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
	FVector DirectionToPoint	= (DesiredSpeedPoint - CurrentPosition).GetSafeNormal();
	float DistanceToNextPoint	= FVector::Dist(CurrentPosition, DesiredSpeedPoint);


	//--------------------------------------------------------
	//----------------APPLY ALL-------------------------------
	//--------------------------------------------------------
	// Calculate the new position by moving the current position towards the desired point
	FVector NewPosition = FMath::Lerp(CurrentPosition, DesiredSpeedPoint, deltaTime * desiredSpeedFactor);

	// Set the vehicle's position to the new position using teleport
	GetOwner()->SetActorLocation(NewPosition, false, { nullptr }, ETeleportType::ResetPhysics);


	//GetOwner()->SetActorLocation(NewPosition);

	// Optionally, update the vehicle's rotation to face the direction of movement
	FRotator NewRotation = DirectionToPoint.Rotation();
	GetOwner()->SetActorRotation(NewRotation);

	return true;
}

float UATS_AgentNavigation::CalculateSteeringInput(const FVector& CurrentForward, const FVector& ToNextPoint)
{
	// Calculate the angle between the current forward direction and the direction to the next point
	float DotProduct = FVector::DotProduct(CurrentForward, ToNextPoint);
	float Angle = FMath::Acos(DotProduct);

	// Determine if the angle is to the left or right
	float SteeringDirection = FMath::Sign(FVector::CrossProduct(CurrentForward, ToNextPoint).Z);
	float SteeringInput = FMath::RadiansToDegrees(Angle) * SteeringDirection;

	float HighSpeedThreshold = 1000.0f; // Adjust this value based on testing
	float HighSpeedSensitivity = 0.5f;	// Adjust this value based on testing
	float LowSpeedSensitivity = 1.0f;	// Adjust this value based on testing

	// Adjust steering sensitivity based on speed
	float CurrentSpeed = m_pVehicleComponent->GetForwardSpeed();
	float SpeedFactor = FMath::Clamp(CurrentSpeed / HighSpeedThreshold, 0.0f, 1.0f);
	float SensitivityAdjustment = FMath::Lerp(HighSpeedSensitivity, LowSpeedSensitivity, SpeedFactor);

	SteeringInput *= SensitivityAdjustment;

	// Normalize the steering input to be between -1 and 1
	SteeringInput = FMath::Clamp(SteeringInput / m_MaxSteeringAngle, -1.f, 1.f);
	return SteeringInput;
}

float UATS_AgentNavigation::CalculateThrottleInput(const FVector& CurrentPosition, const FVector& NextPoint)
{
	float Distance = (NextPoint - CurrentPosition).Size();
	float ThrottleInput = FMath::Clamp(Distance / m_MaxThrottleDistance, 0.f, 1.f);

	float CurrentSpeed = m_pVehicleComponent->GetForwardSpeed();
	if (CurrentSpeed > m_MaxSpeedUnrealunits)
	{
		return 0.f;
	}

	return ThrottleInput;
}

bool UATS_AgentNavigation::ApplyVehicleControl(float SteeringInput, float ThrottleInput, float BrakeInput)
{
	if (m_pVehicleComponent)
	{
		m_pVehicleComponent->SetSteeringInput(SteeringInput);
		m_pVehicleComponent->SetThrottleInput(ThrottleInput);
		m_pVehicleComponent->SetBrakeInput(BrakeInput);
		return true;
	}
	return false;
}

float UATS_AgentNavigation::CalculateTurnSharpness(const FVector& CurrentDirection, const FVector& DirectionToNextPoint, float DistanceToNextPoint)
{
	// Normalize the vectors
	FVector NormCurrentDirection = CurrentDirection.GetSafeNormal();
	FVector NormDirectionToNextPoint = DirectionToNextPoint.GetSafeNormal();

	// Calculate the angle between the current direction and the direction to the next point
	float DotProduct = FVector::DotProduct(NormCurrentDirection, NormDirectionToNextPoint);
	float AngleRadians = acosf(FMath::Clamp(DotProduct, -1.0f, 1.0f)); // Clamp to avoid numerical inaccuracies
	float AngleDegrees = FMath::RadiansToDegrees(AngleRadians);

	// Factor in the distance to the next point (the closer the point, the more immediate the turn)
	float CurrentSpeed = m_pVehicleComponent->GetForwardSpeed();
	float SpeedFactor = FMath::Clamp(CurrentSpeed / m_MaxSpeedUnrealunits, 0.0f, 1.0f);

	float MaxViewDistance = FMath::Lerp(MinPointDistance, MaxPointDistance, SpeedFactor);

	float DistanceFactor = FMath::Clamp(1.0f - (DistanceToNextPoint / MaxViewDistance), 0.0f, 1.0f);

	// Combine angle and distance factors for sharpness
	float Sharpness = (AngleDegrees / m_MaxSteeringAngle) * DistanceFactor;

	return Sharpness;
}

bool UATS_AgentNavigation::ShouldBrake(float TurnSharpness, float currentSpeed, float maxCornerSpeed)
{
	return currentSpeed > maxCornerSpeed;
}

float UATS_AgentNavigation::CalculateCornerAngle(const FVector& currentPoint, const FVector& nextPoint, const FVector& secondNextPoint)
{
	// Lengths of sides of the triangle formed by the points
	float a = (nextPoint	- secondNextPoint).Size();
	float b = (currentPoint - secondNextPoint).Size();
	float c = (currentPoint - nextPoint).Size();

	// Using the Law of Cosines to find the angle at B
	float angleB = acosf(FMath::Clamp((a * a + c * c - b * b) / (2 * a * c), -1.0f, 1.0f));

	return FMath::RadiansToDegrees(angleB);
}

float UATS_AgentNavigation::CalculateMaxCorneringSpeed(float TurnRadius, float FrictionCoefficient)
{
	const float Gravity = 981.0f; // cm/s^2
	float MaxSpeed = FMath::Sqrt(TurnRadius * Gravity * FrictionCoefficient); // in cm/s
	return MaxSpeed;
}

float UATS_AgentNavigation::CalculateMaxCorneringSpeed(float frictionCoefficient, FVector agentPoint, FVector nextPoint, FVector secondNextPoint)
{
	const bool bIsDebugging{ bDebug };
	const float GRAVITY{ 981.0f }; // cm/s^2
	if (m_pVehicleComponent == nullptr)
	{
		if (bIsDebugging)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::VehicleComponent is null, defaulting curvature."));
		}
		return m_MaxSpeedUnrealunits;
	}
	float agentMass = m_pVehicleComponent->Mass;

	// Calculate the side lengths of the triangle formed by the points
	float a = (nextPoint - agentPoint).Size();
	float b = (secondNextPoint - nextPoint).Size();
	float c = (secondNextPoint - agentPoint).Size();
	float s = (a + b + c) / 2.0f; // Semi-perimeter

	// Check for nearly collinear points to avoid division by zero
	if (FMath::Abs(s * (s - a) * (s - b) * (s - c)) < SMALL_NUMBER)
	{
		if (bIsDebugging)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::Points are nearly collinear, defaulting curvature."));
		}
		return m_MaxSpeedUnrealunits;
	}

	// Calculate the radius of the circumscribed circle
	float radius = (a * b * c) / (4.0f * sqrt(s * (s - a) * (s - b) * (s - c)));

	// Ensure radius is positive to avoid invalid curvature
	if (radius <= 0.0f)
	{
		if (bIsDebugging)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::Invalid radius, defaulting curvature."));
		}
		return m_MaxSpeedUnrealunits;
	}

	// Curvature calculation
	float curvature = 1.0f / radius;
	if (agentMass <= 0)
	{
		agentMass = 1.f;
	}
	else
	{
		agentMass /= 10.f;
	}

	// Lateral acceleration calculation
	float lateralAcceleration = sqrt(GRAVITY * frictionCoefficient * agentMass);

	// Calculate the maximum cornering speed
	float maxSpeed = sqrt(lateralAcceleration / curvature);

	if (bIsDebugging)
	{
		UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::curvature: %f"), curvature);
		UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::agentMass: %f"), );
		UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::maxSpeed: %f"), maxSpeed * 0.036f); // Explain or define this constant
		UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::lateralAcceleration: %f"), lateralAcceleration);
	}

	// Check speed boundaries
	if (maxSpeed > m_MaxSpeedUnrealunits || maxSpeed < 0)
	{
		return m_MaxSpeedUnrealunits;
	}

	return maxSpeed;
}

float UATS_AgentNavigation::CalculateTurnRadius(float TurnSharpness, float CurrentSpeed)
{
	// Determine the speed factor based on current speed
	float SpeedFactor = FMath::Clamp(1.0f - (CurrentSpeed / m_MaxSpeedUnrealunits), 0.0f, 1.0f);

	float TireWidth = 50.f;

	// Calculate the turn radius
	float TurnRadius = m_pVehicleComponent->ChassisWidth / FMath::Sin(TurnSharpness) + TireWidth / 2 * (1.0f - SpeedFactor);

	return TurnRadius;
}

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

bool UATS_AgentNavigation::FollowPath(float deltaTime)
{
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

			FVector NextPoint		= m_pTrafficManager->GetNextNavigationPathPoint(GetOwner(), DynamicPointDistance, tempBool, m_AgentData, m_NavigationPath);
			FVector SecondNextPoint = m_pTrafficManager->GetNextNavigationPathPoint(GetOwner(), DynamicPointDistance * 2.f, tempBool, m_AgentData, m_NavigationPath);

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
			FVector CurrentForward = GetOwner()->GetActorForwardVector();
			FVector ToNextPoint = (NextPoint - CurrentPosition).GetSafeNormal();

			float DistanceToNextPoint = FVector::Dist(CurrentPosition, NextPoint);

			//--------------------------------------------------------
			//----------------TURNING CALCULATIONS--------------------
			//--------------------------------------------------------
			float TurnSharpness = CalculateTurnSharpness(CurrentForward, ToNextPoint, DistanceToNextPoint);

			float dryAsphaltFriction = 0.8f;
			float turnRadius = CalculateTurnRadius(TurnSharpness, CurrentSpeed);
			float MaxCorneringSpeed = CalculateMaxCorneringSpeed(dryAsphaltFriction, CurrentPosition, NextPoint, SecondNextPoint);

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
			float desiredSpeedFactor	= FMath::Clamp(m_DesiredSpeed / m_MaxSpeedUnrealunits, 0.0f, 1.0f);
			float desiredPointDistance	= FMath::Lerp(MinPointDistance, MaxPointDistance, desiredSpeedFactor);

			bool	tempBool{ false };
			bool	bStopDueTrafficRule{ false };

			FVector DesiredSpeedPoint	= m_pTrafficManager->GetNextNavigationPathPoint(GetOwner(), desiredPointDistance, bStopDueTrafficRule, m_AgentData, m_NavigationPath); //This will be used to calculate the desired speed of the car - depending on traffic it could be faster or slower
			FVector carInfront			= m_pTrafficManager->GetTrafficAwareNavigationPoint(this, m_AgentData, desiredPointDistance * 1.5f);
			FVector pathEndPoint		= m_pTrafficManager->GetPathEndNavigationPoint(this, m_AgentData, m_NavigationPath, desiredPointDistance * 1.5f);

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
			FVector CurrentForward	= GetOwner()->GetActorForwardVector();

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

	return false;
}

void UATS_AgentNavigation::RetrievePath()
{
	if (m_pTrafficManager == nullptr)
	{
		return;
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

void UATS_AgentNavigation::SetNavGoal(const FVector& destination)
{
	m_Destination = destination;
	bFollowPath = true;
	m_NavigationPath.bIsFollowingPath = false;
	m_NavigationPath.bHasReachedEnd = false;

	EnableAgent();
}

bool UATS_AgentNavigation::IsGoalPointReached(const FVector& point) const
{
	FVector CurrentPosition = GetOwner()->GetActorLocation();
	float DistanceToNextPoint = FVector::DistSquared(CurrentPosition, point);

	return DistanceToNextPoint < 100.f;
}

void UATS_AgentNavigation::SetFollowPath(bool isEnabled)
{
	bFollowPath = isEnabled;
}

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