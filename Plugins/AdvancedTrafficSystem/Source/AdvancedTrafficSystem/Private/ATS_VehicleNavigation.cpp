#include "../Public/ATS_VehicleNavigation.h"
#include "../Public/ATS_TrafficManager.h"

#include "Components/SplineComponent.h"
#include "ChaosVehicleMovementComponent.h"
#include "ZoneShapeComponent.h"

#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

UATS_VehicleNavigation::UATS_VehicleNavigation()
{
	PrimaryComponentTick.bCanEverTick = true;
}

/*
bool UATS_VehicleNavigation::MoveToNextPointPhysics(float deltaTime)
{
	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("\n-------------------------------------RETRIEVING AGENT DATA-------------------------------------"));
	}
	//Retrieve corner information
	RetrieveLanePoints();

	// Retrieve information about the lane
	float CurrentSpeed = m_pVehicleComponent->GetForwardSpeed(); // Get the current speed of the vehicle
	float brakingDistance = CalculateBrakingDistance(CurrentSpeed) * m_BrakingMultiplier + m_BrakingDistanceOffset; // Calculate the braking distance based on the current speed

	FVector CurrentPosition = GetOwner()->GetActorLocation();		//Vehicle actor position
	FVector CurrentForward = GetOwner()->GetActorForwardVector();	//Vehicle actor forward vector

	float desiredSpeedFactor = FMath::Clamp(m_DesiredSpeed / m_MaxSpeedUnrealunits, 0.0f, 1.0f);		// Get the desired speed factor based on the desired speed and speedLimit
	float desiredPointDistance = FMath::Lerp(MinPointDistance, MaxPointDistance, desiredSpeedFactor);	// Calculate the desired point distance based on the desired speed factor

	bool	brakeBool{ false };
	bool	bStopDueTrafficRule{ false };

	FAgentData brakeData{ m_AgentData };
	FAgentData secondBrakeData{ m_AgentData };

	FVector brakePoint = m_pTrafficManager->GetNextNavigationPoint(GetOwner(), brakingDistance, bStopDueTrafficRule, brakeData, false);
	FVector secondBrakePoint = m_pTrafficManager->GetNextNavigationPoint(GetOwner(), brakingDistance * 2.f, brakeBool, secondBrakeData, false);

	FVector cornerPoint = secondBrakePoint;
	if (m_LanePoints.IsValidIndex(m_CurrentPointIndex))
	{
		cornerPoint = m_LanePoints[m_CurrentPointIndex];
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::MoveToNextPointPhysics() -- LanePoints is not valid"));
	}

	float distanceToCorner = brakingDistance * 2.f;
	distanceToCorner = FVector::Dist(GetOwner()->GetActorLocation(), cornerPoint);	// Calculate the distance to the next corner

	float distanceToNextPoint{ FLT_MAX };	// Calculate the distance to the next point
	if (m_LanePoints.IsValidIndex(m_CurrentPointIndex + 1))
	{
		distanceToNextPoint = FVector::Dist(cornerPoint, m_LanePoints[m_CurrentPointIndex + 1]);	// Calculate the distance to the next point
	}

	bool tempBool{ false };
	FAgentData cornerData{ m_AgentData };
	FVector AfterCornerPoint{};
	float afterCornerDistance{ distanceToCorner * 2 };

	if (distanceToCorner > distanceToNextPoint)
	{
		afterCornerDistance = distanceToCorner + distanceToNextPoint;
	}
	AfterCornerPoint = m_pTrafficManager->GetNextNavigationPoint(GetOwner(), afterCornerDistance, tempBool, cornerData, false);

	//--------------------------------------------------------
	//----------------Corner CALCULATIONS---------------------
	//--------------------------------------------------------	
	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("\n-------------------------------------CORNER CALCULATIONS-------------------------------------"));
	}

	float brakeFromCornerInput{ 0.0f };
	float canTurn{ 0.0f };
	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::MoveToNextPointPhysics() -- brakingDistance %f"), brakingDistance);
	}

	brakeFromCornerInput = CalculateBrakingForCorner(CurrentSpeed, distanceToCorner, cornerPoint, AfterCornerPoint, canTurn);


	FVector desiredSpeedPoint = m_pTrafficManager->GetNextNavigationPoint(GetOwner(), desiredPointDistance, bStopDueTrafficRule, m_AgentData);

	FVector stopDueTraffic = m_pTrafficManager->GetTrafficAwareNavigationPoint(this, m_AgentData, brakingDistance);
	if (stopDueTraffic.IsZero() == false)
	{
		desiredSpeedPoint = stopDueTraffic;	// This point will be set as the desired speed point
		bStopDueTrafficRule = true;				// Set the stop due to traffic rule to true
	}


	if (bDrawDebugObjects)
	{
		DrawDebugSphere(GetWorld(), brakePoint, _DrawDebugCircleRadius, 12, FColor::Black, false, 0.f, 0, _DrawDebugThickness);
		DrawDebugSphere(GetWorld(), secondBrakePoint, _DrawDebugCircleRadius, 12, FColor::Black, false, 0.f, 0, _DrawDebugThickness);

		DrawDebugSphere(GetWorld(), desiredSpeedPoint, _DrawDebugCircleRadius, 12, _DrawDebugDesiredPointColor, false, 0.f, 0, _DrawDebugThickness);

		DrawDebugSphere(GetWorld(), cornerPoint, 100, 12, FColor::Magenta, false, 0.f, 0, _DrawDebugThickness);
		DrawDebugSphere(GetWorld(), AfterCornerPoint, _DrawDebugCircleRadius, 12, FColor::Orange, false, 0.f, 0, _DrawDebugThickness);
	}

	//--------------------------------------------------------
	//----------------STEERING CALCULATIONS-------------------
	//--------------------------------------------------------
	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("\n-------------------------------------STEERING CALCULATIONS-------------------------------------"));
	}

	float SteeringInput{ canTurn };

	FVector ToNextPoint = (desiredSpeedPoint - CurrentPosition).GetSafeNormal();	//Direction vector from actor towards the next point
	FVector ToCornerExit = (AfterCornerPoint - CurrentPosition).GetSafeNormal();		//Direction vector from actor towards the next point

	//Vehicle can turn if can turn is bigger than 0 or the braking distance is bigger than the dt to the corner
	if (canTurn != 0 || brakingDistance < distanceToCorner)
	{
		SteeringInput = CalculateSteeringInput(CurrentForward, ToNextPoint);
	}
	prevSteeringInput = SteeringInput;

	//--------------------------------------------------------
	//----------------BRAKING AND THROTTLE--------------------
	//--------------------------------------------------------
	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("\n-------------------------------------BRAKING AND THROTTLE CALCULATIONS-------------------------------------"));
	}

	float ThrottleInput{ 0.0f };
	float BrakeInput{ brakeFromCornerInput };

	if (BrakeInput > 0.1f || bStopDueTrafficRule || _bBrake)
	{
		ThrottleInput = 0.0f; // No throttle when braking
		SteeringInput = 0.0f;	// No steering when braking
	}
	else if (abs(SteeringInput) >= 0.5)
	{
		ThrottleInput = CalculateThrottleInput(CurrentPosition, desiredSpeedPoint);
		ThrottleInput = FMath::Clamp(ThrottleInput, 0.0f, 0.25f);
	}
	else
	{
		ThrottleInput = CalculateThrottleInput(CurrentPosition, desiredSpeedPoint);
		BrakeInput = 0.0f; // Make sure we are not braking
	}

	//--------------------------------------------------------
	//----------------APPLY ALL-------------------------------
	//--------------------------------------------------------
	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("\n-------------------------------------APPLYING CALCULATIONS-------------------------------------"));
	}

	if (bDebug)
	{
		Debugging(CurrentSpeed, SteeringInput, ThrottleInput, BrakeInput, 0);
	}

	return ApplyVehicleControl(SteeringInput, ThrottleInput, BrakeInput);
}
*/

bool UATS_VehicleNavigation::MoveToNextPointPhysics(float deltaTime)
{
	//--------------------------------------------------------
	//----------------LOCAL VARIABLES-------------------------
	//--------------------------------------------------------	
	FVector agentPosition{ FVector::ZeroVector }; //Position of the vehicle
	float currentSpeed{ 0.f };		//Current speed of the vehicle
	
	float steeringInput{ 0.f };		//Steering input of the vehicle
	float throttleInput{ 0.f };		//Throttle input of the vehicle
	float brakingInput{ 0.f };		//Braking input of the vehicle

	float desiredSpeed{ 0.f };		//Desired speed of the vehicle
	float brakingDistance{ 0.f };	//Braking distance of the vehicle

	//BRAKING POINT VARIABLES
	bool bBrake{ false };
	FAgentData brakeData{ m_AgentData };
	FVector brakePoint{ FVector::ZeroVector };

	//DESIRED SPEED POINT VARIABLES
	bool bDesiredSpeed{ false };
	FAgentData desiredSpeedData{ m_AgentData };
	FVector desiredSpeedPoint{ FVector::ZeroVector };
	float desiredSpeedFactor{ 0.f }; 	// Get the desired speed factor based on the desired speed and speedLimit
	float desiredPointDistance{ 0.f }; 	// Calculate the desired point distance based on the desired speed factor

	//CORNER POINT VARIABLES
	FVector cornerPoint{ FVector::ZeroVector };

	bool bCorner{ false };
	FAgentData cornerData{ m_AgentData };
	FVector cornerExitPoint{ FVector::ZeroVector };

	float distanceToCorner{ 0.f };		// Calculate the distance between the agent  and the corner
	float distanceToCornerExit{ 0.f };	// Calculate the distance between the corner and the corner exit


	//--------------------------------------------------------
	//----------------RETRIEVING AGENT DATA-------------------
	//--------------------------------------------------------	
	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("\n-------------------------------------RETRIEVING AGENT DATA-------------------------------------"));
	}
	
	agentPosition	= GetOwner()->GetActorLocation();
	currentSpeed	= m_pVehicleComponent->GetForwardSpeed();
	
	brakingDistance = CalculateBrakingDistance(currentSpeed) * m_BrakingMultiplier + m_BrakingDistanceOffset;
	desiredSpeed	= m_DesiredSpeed; // Add an offset to the desired speed to make traffic more dynamic

	desiredSpeedFactor		= FMath::Clamp(desiredSpeed / m_MaxSpeedUnrealunits, 0.0f, 1.0f);
	desiredPointDistance	= FMath::Lerp(MinPointDistance, MaxPointDistance, desiredSpeedFactor);
	
	if (RetrieveSpline() == false)
	{
		return false;
	}

	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::MoveToNextPointPhysics() -- currentSpeed %f"), currentSpeed);
		UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::MoveToNextPointPhysics() -- desiredSpeed %f"), desiredSpeed);
	}

	FVector stopDueTraffic = m_pTrafficManager->GetTrafficAwareNavigationPoint(this, m_AgentData, brakingDistance);
	if (stopDueTraffic.IsZero() == false)
	{
		desiredSpeedPoint = stopDueTraffic;	// This point will be set as the desired speed point
		brakingInput = 1.f;
	}

	//--------------------------------------------------------
	//----------------POINT CALCULATIONS----------------------
	//--------------------------------------------------------	
	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("\n-------------------------------------POINT CALCULATIONS-------------------------------------"));
	}

	//Brake point
	brakePoint			= m_pTrafficManager->GetNextNavigationPoint(GetOwner(), brakingDistance, bBrake, m_AgentData);							// Brake point of vehicle on his lane
	
	//Desired speed point
	desiredSpeedPoint	= m_pTrafficManager->GetNextNavigationPoint(GetOwner(), desiredPointDistance, bDesiredSpeed, desiredSpeedData, false);	// Desired speed point of vehicle on his lane

	//Corner point
	RetrieveLanePoints();
	if (m_LanePoints.IsValidIndex(m_CurrentPointIndex) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::MoveToNextPointPhysics() -- LanePoints is not valid"));
		return false;
	}
	cornerPoint = m_LanePoints[m_CurrentPointIndex].position;
	distanceToCorner = FVector::DistSquared(agentPosition,	cornerPoint);
	
	//Check if there is a next corner point
	if(m_LanePoints.IsValidIndex(m_CurrentPointIndex + 1))
	{
		cornerExitPoint = m_LanePoints[m_CurrentPointIndex + 1].position;
	}
	else
	{
		cornerExitPoint = m_pTrafficManager->GetNextNavigationPoint(GetOwner(), distanceToCorner + distanceToCorner, bCorner, cornerData, false);
	}
	distanceToCornerExit	= FVector::DistSquared(cornerPoint,	cornerExitPoint);

	//Compare the distance between the corner exit point and the distance from player to the corner
	if (distanceToCorner < distanceToCornerExit)
	{
		//The point will need to be changed
		distanceToCorner = FVector::Distance(agentPosition, cornerPoint);
		cornerExitPoint = m_pTrafficManager->GetNextNavigationPoint(GetOwner(), distanceToCorner + distanceToCorner, bCorner, cornerData, false);
	}
	distanceToCornerExit = FVector::Distance(cornerPoint, cornerExitPoint);


	//--------------------------------------------------------
	//----------------CORNER CALCULATIONS---------------------
	//--------------------------------------------------------	
	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("\n-------------------------------------CORNER CALCULATIONS-------------------------------------"));
	}

	float cornerAngle = CalculateCornerAngle(agentPosition, cornerPoint, cornerExitPoint);
	if(cornerAngle < 170.f && brakingInput == 0.f)
	{
		brakingInput = CalculateBrakingForCorner(currentSpeed, desiredSpeed, cornerPoint, cornerExitPoint, steeringInput);
		
		//--------------------------------------------------------
		//----------------SPLINE CALCULATIONS---------------------
		//--------------------------------------------------------
		if (m_pSplineComponent)
		{
			// Calculate direction vectors
			FVector InTangent  = cornerPoint - agentPosition;
			FVector OutTangent = cornerExitPoint - cornerPoint;

			// Normalize if necessary, depending on your needs for the spline's curvature
			InTangent.Normalize();
			OutTangent.Normalize();

			m_pSplineComponent->SetLocationAtSplinePoint(1, cornerExitPoint, ESplineCoordinateSpace::World);

			m_pSplineComponent->SetTangentAtSplinePoint(0, InTangent  *	(distanceToCorner * 2),		ESplineCoordinateSpace::World);
			m_pSplineComponent->SetTangentAtSplinePoint(1, OutTangent * (distanceToCornerExit * 2), ESplineCoordinateSpace::World);
		}

		if (brakingInput == 0.f)
		{
			desiredSpeedPoint = m_pTrafficManager->GetNextNavigationPoint(GetOwner(), desiredPointDistance + 400.f, bDesiredSpeed, desiredSpeedData, false);	// Desired speed point of vehicle on his lane
		}
	}

	//--------------------------------------------------------
	//----------------STEERING CALCULATIONS-------------------
	//--------------------------------------------------------
	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("\n-------------------------------------STEERING CALCULATIONS-------------------------------------"));
	}

	if (steeringInput != 0)
	{
		//Received steering from corner calculations -- move desired speed point?
		//desiredSpeedPoint = cornerExitPoint;
	}
	steeringInput = CalculateSteeringInput(GetOwner()->GetActorForwardVector(), (desiredSpeedPoint - agentPosition).GetSafeNormal());

	//--------------------------------------------------------
	//----------------BRAKING AND THROTTLE--------------------
	//--------------------------------------------------------
	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("\n-------------------------------------BRAKING AND THROTTLE CALCULATIONS-------------------------------------"));
	}

	throttleInput = 1.f;
	if (brakingInput > 0.01f)
	{
		throttleInput = 0.f;
	}
	else if (abs(steeringInput) > 0.5f)
	{
		throttleInput = 0.25f;
	}
	

	//--------------------------------------------------------
	//----------------APPLY ALL-------------------------------
	//--------------------------------------------------------
	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("\n-------------------------------------APPLYING CALCULATIONS-------------------------------------"));
	}

	if (bDrawDebugObjects)
	{
		//Draw debug the points
		DrawDebugSphere(GetWorld(), brakePoint,			_DrawDebugCircleRadius, 12, _DrawDebugBrakeColor,			false, 0.f, 0, _DrawDebugThickness);
		DrawDebugSphere(GetWorld(), desiredSpeedPoint,	_DrawDebugCircleRadius, 12, _DrawDebugDesiredPointColor,	false, 0.f, 0, _DrawDebugThickness);
		DrawDebugSphere(GetWorld(), cornerPoint,		_DrawDebugCircleRadius, 12, _DrawDebugCornerPointColor,		false, 0.f, 0, _DrawDebugThickness);
		DrawDebugSphere(GetWorld(), cornerExitPoint,	_DrawDebugCircleRadius, 12, _DrawDebugCornerExitColor,		false, 0.f, 0, _DrawDebugThickness);
	}

	if (bDebug)
	{
		//Rescale the brake input from 0-1 to 0-255
		int brakeColor = ( brakingInput * 255 );
		int throttleColor = ( throttleInput * 255 );

//		m_pSplineComponent->EditorUnselectedSplineSegmentColor = FColor(brakeColor, throttleColor, 255);

		Debugging(currentSpeed, steeringInput, throttleInput, brakingInput, 0);
	}

	return ApplyVehicleControl(steeringInput, throttleInput, brakingInput);
}


bool UATS_VehicleNavigation::MoveToNextPointSimple(float deltaTime)
{
	//--------------------------------------------------------
	//----------------NEXT POINT CALCULATION------------------
	//--------------------------------------------------------
	float desiredSpeedFactor = FMath::Clamp(m_DesiredSpeed / m_MaxSpeedUnrealunits, 0.0f, 1.0f);
	float desiredPointDistance = FMath::Lerp(MinPointDistance, MaxPointDistance, desiredSpeedFactor);

	bool	tempBool{ false };
	bool	bStopDueTrafficRule{ false };

	FVector DesiredSpeedPoint = m_pTrafficManager->GetNextNavigationPoint(GetOwner(), desiredPointDistance, bStopDueTrafficRule, m_AgentData); //This will be used to calculate the desired speed of the car - depending on traffic it could be faster or slower
	FVector carInfront = m_pTrafficManager->GetTrafficAwareNavigationPoint(this, m_AgentData, desiredPointDistance * 1.5f);
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
	FVector CurrentForward	= GetOwner()->GetActorForwardVector();

	// Calculate the direction vector towards the desired point
	FVector DirectionToPoint = (DesiredSpeedPoint - CurrentPosition).GetSafeNormal();
	float DistanceToNextPoint = FVector::Dist(CurrentPosition, DesiredSpeedPoint);


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

float UATS_VehicleNavigation::CalculateSteeringInput(const FVector& CurrentForward, const FVector& ToNextPoint)
{
	// Calculate the angle between the current forward direction and the direction to the next point
	float DotProduct			= FVector::DotProduct(CurrentForward, ToNextPoint);
	float Angle					= FMath::Acos(DotProduct);

	// Determine if the angle is to the left or right
	float SteeringDirection		= FMath::Sign(FVector::CrossProduct(CurrentForward, ToNextPoint).Z);
	float SteeringInput			= FMath::RadiansToDegrees(Angle) * SteeringDirection;

	float HighSpeedThreshold	= 1000.0f;	// Adjust this value based on testing
	float HighSpeedSensitivity	= 0.5f;		// Adjust this value based on testing
	float LowSpeedSensitivity	= 1.0f;		// Adjust this value based on testing

	// Adjust steering sensitivity based on speed
	float CurrentSpeed			= m_pVehicleComponent->GetForwardSpeed();
	float SpeedFactor			= FMath::Clamp(CurrentSpeed / HighSpeedThreshold, 0.0f, 1.0f);
	float SensitivityAdjustment = FMath::Lerp(HighSpeedSensitivity, LowSpeedSensitivity, SpeedFactor);

	//Scale the steeringInput
	SteeringInput *= SensitivityAdjustment;

	// Normalize the steering input to be between -1 and 1
	SteeringInput = FMath::Clamp(SteeringInput / m_MaxSteeringAngle, m_MinSteeringValue, m_MaxSteeringValue);
	return SteeringInput;
}

float UATS_VehicleNavigation::CalculateThrottleInput(const FVector& CurrentPosition, const FVector& NextPoint)
{
	float Distance		= (NextPoint - CurrentPosition).Size();
	float ThrottleInput = FMath::Clamp(Distance / m_MaxThrottleDistance, 0.f, 1.f);

	float CurrentSpeed = m_pVehicleComponent->GetForwardSpeed();
	if (CurrentSpeed > m_MaxSpeedUnrealunits)
	{
		return 0.f;
	}

	return ThrottleInput;
}

bool UATS_VehicleNavigation::ApplyVehicleControl(float SteeringInput, float ThrottleInput, float BrakeInput)
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

float UATS_VehicleNavigation::CalculateTurnSharpness(const FVector& CurrentDirection, const FVector& DirectionToNextPoint, float DistanceToNextPoint)
{
	// Normalize the vectors
	FVector NormCurrentDirection = CurrentDirection.GetSafeNormal();		//Normalize the vehicle direction vector
	FVector NormDirectionToNextPoint = DirectionToNextPoint.GetSafeNormal(); //Normilize the direction to the next point

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

bool UATS_VehicleNavigation::ShouldBrake(float TurnSharpness, float currentSpeed, float maxCornerSpeed)
{
	return currentSpeed > maxCornerSpeed;
}

float UATS_VehicleNavigation::CalculateCornerAngle(const FVector& firstPoint, const FVector& cornerPoint, const FVector& secondPoint)
{
	if (cornerPoint == FVector::ZeroVector)
	{
		return 0.f;
	}

	// Lengths of sides of the triangle formed by the points
	float a = (cornerPoint - secondPoint).Size();
	float b = (firstPoint - secondPoint).Size();
	float c = (firstPoint - cornerPoint).Size();

	// Using the Law of Cosines to find the angle at B
	float angleB = acosf(FMath::Clamp((a * a + c * c - b * b) / (2 * a * c), -1.0f, 1.0f));
	float angleBdegrees = FMath::RadiansToDegrees(angleB);

	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::CalculateCornerAngle() -- Angle %f"), angleBdegrees);
	}

	return angleBdegrees;
}

float UATS_VehicleNavigation::CalculateMaxCorneringSpeed(float TurnRadius, float FrictionCoefficient)
{
	const float Gravity = 981.0f; // cm/s^2
	float MaxSpeed = FMath::Sqrt(TurnRadius * Gravity * FrictionCoefficient); // in cm/s
	return MaxSpeed;
}

float UATS_VehicleNavigation::CalculateMaxCorneringSpeed(float frictionCoefficient, FVector agentPoint, FVector nextPoint, FVector secondNextPoint)
{
	// Check if the vehicle has a valid vehicleComponent
	if (m_pVehicleComponent == nullptr)
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::VehicleComponent is null, defaulting curvature."));
		}
		return m_MaxSpeedUnrealunits;
	}

	// Get the mass of the vehicle
	float agentMass = m_pVehicleComponent->Mass;

	// Calculate the side lengths of the triangle formed by the points
	float a = (nextPoint - agentPoint).Size();
	float b = (secondNextPoint - nextPoint).Size();
	float c = (secondNextPoint - agentPoint).Size();
	float s = (a + b + c) / 2.0f; // Semi-perimeter

	// Check for nearly collinear points to avoid division by zero
	if (FMath::Abs(s * (s - a) * (s - b) * (s - c)) < SMALL_NUMBER)
	{
		if (bDebug)
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
		if (bDebug)
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
	float lateralAcceleration = sqrt(_Gravity * frictionCoefficient * agentMass);

	// Calculate the maximum cornering speed
	float maxSpeed = sqrt(lateralAcceleration / curvature);

	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::curvature: %f"), curvature);
		UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::agentMass: %f"), agentMass);
		UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::maxSpeed: %f"), maxSpeed * 0.036f);
		UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::lateralAcceleration: %f"), lateralAcceleration);
	}

	// Check speed boundaries
	if (maxSpeed > m_MaxSpeedUnrealunits || maxSpeed < 0)
	{
		return m_MaxSpeedUnrealunits;
	}

	return maxSpeed;
}

float UATS_VehicleNavigation::CalculateTurnRadius(float TurnSharpness, float CurrentSpeed)
{
	// Determine the speed factor based on current speed
	float speedFactor = FMath::Clamp(1.0f - (CurrentSpeed / m_MaxSpeedUnrealunits), 0.0f, 1.0f);

	float tireWidth = 50.f;

	// Calculate the turn radius
	float tunrSharpnessRadians = FMath::DegreesToRadians(TurnSharpness);
	float turnRadius = (m_pVehicleComponent->ChassisWidth / FMath::Sin(tunrSharpnessRadians)) + (tireWidth / 2) * (1.0f - speedFactor);


	return turnRadius;
}

float UATS_VehicleNavigation::CalculateBrakingForCorner(float currentSpeed, float maxLaneSpeed, FVector cornerPoint, FVector afterCornerPoint, float& canTurn)
{
	/*
		r = TurnRadius			-- CALCULATED HERE
		v = CurrentSpeed		-- RECEIVE FROM VEHICLE
		g = Gravity				-- CONSTANT
		u = FrictionCoefficient -- CONSTANT
		d = distance to corner  -- CALCULATED HERE

		We will calculate the current TurnRadius(r) based on the velocity

		r = v^2 / g * u

		You can now calculate the width it will use based on the turn radius
		For this you will need to center of the turn circle
		The point for this can be extracted as follows:
		- Get the current position of the vehicle and go turnRadius to the right this is the center point
		now we need to get the distance from the cornerpoint to the centerpoint
		subtract distance with the current turnradius and you will get the width

		if this is bigger than the width of the lane, you will need to slow down to take the corner well

		Depending on the difference between this length and the desired length you can calculate how hard to brake

		The after corner point should be at the same distance from the corner as the current distance from the vehicle to the corner
		To check if the vehicle can take this corner, the distance from that point to the centerpoint should be the same(within certain range) as the turnradius
	*/
	if (m_pVehicleComponent == nullptr)
	{
		return 0.f;
	}

	if (FVector::DistSquared(cornerPoint, afterCornerPoint) < 0.1f)
	{
		return 0.f;
	}


	//Initialize the variables
	const float DISTANCE_ERROR{ 10.f };

	float turnRadius{ 0.f };
	float turnWidth{ 0.f };
	float maxTurnRadiusForCorner{ 0.f };
	float distanceToCorner{ 0.f };
	canTurn = 0.f;

	//Calculate the turn radius
	turnRadius				= currentSpeed * currentSpeed / (_Gravity * _DryAsphaltFriction);
	maxTurnRadiusForCorner	= maxLaneSpeed * maxLaneSpeed / (_Gravity * _DryAsphaltFriction);
	//turnRadius = maxTurnRadiusForCorner;

	//Make the turn circle
	FVector turnRadiusCircleCenter{};

	// Calculate the circle centere point
	{
		FVector agentLocation = GetOwner()->GetActorLocation();

		// Get the Player's right vector
		FVector agentRightVector = GetOwner()->GetActorRightVector();

		// Scale the right vector by the desired distance
		FVector ScaledRightVector = agentRightVector * turnRadius;

		turnRadiusCircleCenter = agentLocation + ScaledRightVector;
	}

	if (bDrawDebugObjects)
	{
		DrawDebugSphere(GetWorld(), turnRadiusCircleCenter, turnRadius, 12, _DrawDebugTurnCircle, false, 0.f, 0, _DrawDebugThickness);
	}

	//Distance from center to after corner point
	float distanceToAfterCornerPoint = FVector::Dist(cornerPoint, afterCornerPoint);
	distanceToCorner = FVector::Dist(cornerPoint, GetOwner()->GetActorLocation());
	float brakeDistance = CalculateBrakingDistance(currentSpeed);
	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::CalculateCorneringSpeed() -- DistanceToAfterCornerPoint %f"), distanceToAfterCornerPoint);
		UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::CalculateCorneringSpeed() -- TurnRadius %f"), turnRadius);
		UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::CalculateCorneringSpeed() -- DesiredTurnRadius %f"), maxTurnRadiusForCorner);
	}

	//This should be similar to the turn radius 
	// -- if the distance is smaller you will need to slow down
	// -- if the distance is bigger or the same you don't need to change your speed
	if (distanceToAfterCornerPoint >= turnRadius - DISTANCE_ERROR)
	{
		//The car doesn't need to slow down yet and can keep driving its speed
		return 0.f;
	}	

	//Calculate the distance from the corner to the center of the turn circle
	float cornerToCenterDistance = FVector::Dist(cornerPoint, turnRadiusCircleCenter);
	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::CalculateCorneringSpeed() -- CornerToCenterDistance %f"), cornerToCenterDistance);
	}
	
	//Calculate the width of the turn
	turnWidth = cornerToCenterDistance - turnRadius;
	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::CalculateCorneringSpeed() -- TurnWidth %f"),	turnWidth);
		UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::CalculateCorneringSpeed() -- MaxTurnWidth %f"), _MaxTurnWidth);
	}

	if (turnWidth <= _MaxTurnWidth)
	{
		//Can make the corner
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::CalculateCorneringSpeed() -- Turning..."));
		}
		canTurn = 1.f;
		return 0.f;
	}

	//Turn width is too big and thus the vehicle has to slow down

	//The car will need to slow down
	// -- Calculate the difference between the turn width and the max turn width
	// -- float widthDifference = turnWidth - _MaxTurnWidth;

	//Depending on the difference and the distance to the corner you can calculate how hard to brake [0 - 1]

	//Calculate the braking distance
	//float deceleration = _DryAsphaltFriction * _Gravity;
	//float brakingDistance = (currentSpeed * currentSpeed) / (2 * deceleration);


	//Calculate the braking input based on difference between turnRadius and distanceAfterCornerPoint
	float brakingInput = FMath::Clamp((turnRadius - distanceToAfterCornerPoint) / turnRadius, 0.f, 1.f);
	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::CalculateCorneringSpeed() -- BrakingInput %f"), brakingInput);
	}
	brakingInput = 1.f;
	return brakingInput;
}

float UATS_VehicleNavigation::CalculateBrakingDistance(float currentSpeed) const
{
	//Calculate the braking distance
	float deceleration		= _DryAsphaltFriction * _Gravity;
	float brakingDistance	= (currentSpeed * currentSpeed) / (2 * deceleration);

	return brakingDistance;
}

bool UATS_VehicleNavigation::RetrieveSpline()
{
	if (m_pSplineComponent)
	{
		return true;
	}

	//Find the spline component
	m_pSplineComponent = Cast<USplineComponent>(GetOwner()->GetComponentByClass(USplineComponent::StaticClass()));
	if (m_pSplineComponent)
	{
		return true;
	}

	return false;
}

bool UATS_VehicleNavigation::GenerateSplineFromLanePoints()
{
	if (m_pSplineComponent == nullptr)
	{
		return false;
	}

	//Retrieve the lane points
	if (RetrieveLanePoints() == false)
	{
		return false;
	}

	//Check if the points have changed since the last time
	//TODO


	/*
		We will now generate the spline points
		 -- Lane points that are a certain distance from each other can be combined to only use the last point
		 -- Each lane point is a corner and will need an extra 2 points the make the curve smooth, HOW?
		 -- -- The distance for the extra points needs to be calculated depending on the speed the lane can have in that corner
	*/

	TArray<FVector> splinePoints{};
	for (int i = 0; i < m_LanePoints.Num(); i++)
	{
		FVector point = m_LanePoints[i].position;
		splinePoints.Add(point);
	}




	return true;
}