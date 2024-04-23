#include "../Public/ATS_VehicleNavigation.h"
#include "../Public/ATS_TrafficManager.h"

#include "ChaosVehicleMovementComponent.h"
#include "ZoneShapeComponent.h"

#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

UATS_VehicleNavigation::UATS_VehicleNavigation()
{
	PrimaryComponentTick.bCanEverTick = true;
}

bool UATS_VehicleNavigation::MoveToNextPointPhysics(float deltaTime)
{	// Retrieve information about the lane
	float CurrentSpeed			= m_pVehicleComponent->GetForwardSpeed();								// Get the current speed of the vehicle
	float brakingDistance		= CalculateBrakingDistance(CurrentSpeed) * m_BrakingMultiplier + m_BrakingDistanceOffset;								// Calculate the braking distance based on the current speed

	float desiredSpeedFactor	= FMath::Clamp(m_DesiredSpeed / m_MaxSpeedUnrealunits, 0.0f, 1.0f);		// Get the desired speed factor based on the desired speed and speedLimit
	float desiredPointDistance	= FMath::Lerp(MinPointDistance, MaxPointDistance, desiredSpeedFactor);	// Calculate the desired point distance based on the desired speed factor

	bool	brakeBool{ false };
	bool	bStopDueTrafficRule{ false };

	FAgentData brakeData{ m_AgentData };
	FAgentData secondBrakeData{ m_AgentData };
	
	FVector desiredSpeedPoint	= m_pTrafficManager->GetNextNavigationPoint(GetOwner(), desiredPointDistance,	bStopDueTrafficRule,	m_AgentData);
	
	FVector brakePoint			= m_pTrafficManager->GetNextNavigationPoint(GetOwner(), brakingDistance,		bStopDueTrafficRule,	brakeData,			false);
	FVector secondBrakePoint	= m_pTrafficManager->GetNextNavigationPoint(GetOwner(), brakingDistance * 2.f,	brakeBool,				secondBrakeData,	false);
	
	FVector stopDueTraffic = m_pTrafficManager->GetTrafficAwareNavigationPoint(this, m_AgentData, brakingDistance);
	if (stopDueTraffic.IsZero() == false)
	{
		desiredSpeedPoint	= stopDueTraffic;	// This point will be set as the desired speed point
		bStopDueTrafficRule = true;				// Set the stop due to traffic rule to true
	}

	//--------------------------------------------------------
	//----------------VECTOR CALCULATIONS---------------------
	//--------------------------------------------------------
	FVector CurrentPosition = GetOwner()->GetActorLocation();		//Vehicle actor position
	FVector CurrentForward	= GetOwner()->GetActorForwardVector();	//Vehicle actor forward vector
	
	if (_NextCornerPoint == FVector::ZeroVector)
	{
		m_CornerAngle = CalculateCornerAngle(desiredSpeedPoint, brakePoint, secondBrakePoint);
		if (m_CornerAngle >= 10.f && m_CornerAngle <= 150.f)
		{
			// The point is on a corner, we should remember this point untill we passed it
			_NextCornerPoint		= brakePoint;
			_CornerExit 			= secondBrakePoint;

			nextCornerLaneData = nextCornerLaneData;
			nextCornerLaneData.agentDistanceOnSpline += brakingDistance / 2.f;

			desiredSpeedPoint = secondBrakePoint;
		}
	}
	else
	{
		if (m_AgentData.agentDistanceOnSpline > nextCornerLaneData.agentDistanceOnSpline)
		{
			_NextCornerPoint = FVector::ZeroVector;
		}
	}
	
	
	if (bDrawDebugObjects)
	{
		DrawDebugSphere(GetWorld(), brakePoint,			_DrawDebugCircleRadius, 12, FColor::Black, false, 0.f, 0, _DrawDebugThickness);
		DrawDebugSphere(GetWorld(), secondBrakePoint,	_DrawDebugCircleRadius, 12, FColor::Black, false, 0.f, 0, _DrawDebugThickness);

		DrawDebugSphere(GetWorld(), desiredSpeedPoint,	_DrawDebugCircleRadius, 12, _DrawDebugDesiredPointColor,	false, 0.f, 0, _DrawDebugThickness);
	}

	//--------------------------------------------------------
	//----------------Corner CALCULATIONS---------------------
	//--------------------------------------------------------	
	float brakeFromCornerInput{ 0.0f };
	if(_NextCornerPoint != FVector::ZeroVector)
	{
		brakeFromCornerInput = CalculateBrakingForCorner(CurrentSpeed, brakingDistance, brakePoint, secondBrakePoint);
	}

	//--------------------------------------------------------
	//----------------STEERING CALCULATIONS-------------------
	//--------------------------------------------------------
	float SteeringInput{ 0.0f };
	FVector ToNextPoint		= (desiredSpeedPoint - CurrentPosition).GetSafeNormal();	//Direction vector from actor towards the next point
	FVector ToCornerExit	= (_CornerExit - CurrentPosition).GetSafeNormal();	//Direction vector from actor towards the next point

	if(_NextCornerPoint != FVector::ZeroVector && brakeFromCornerInput < 0.2f)
	{
		SteeringInput = CalculateSteeringInput(CurrentForward, ToCornerExit);
	}
	else
	{
		SteeringInput = CalculateSteeringInput(CurrentForward, ToNextPoint);
	}
	
	

	//--------------------------------------------------------
	//----------------BRAKING AND THROTTLE--------------------
	//--------------------------------------------------------
	float ThrottleInput{ 0.0f };
	float BrakeInput{ brakeFromCornerInput };
	
	if (BrakeInput > 0.2f || bStopDueTrafficRule || _bBrake)
	{
		ThrottleInput	= 0.0f; // No throttle when braking
		SteeringInput	= 0.0f;	// No steering when braking
		BrakeInput		= 1.0f;
	}
	else
	{
		ThrottleInput	= CalculateThrottleInput(CurrentPosition, desiredSpeedPoint);
		BrakeInput		= 0.0f; // Make sure we are not braking
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

float UATS_VehicleNavigation::CalculateCornerAngle(const FVector& currentPoint, const FVector& nextPoint, const FVector& secondNextPoint)
{
	// Lengths of sides of the triangle formed by the points
	float a = (nextPoint - secondNextPoint).Size();
	float b = (currentPoint - secondNextPoint).Size();
	float c = (currentPoint - nextPoint).Size();

	// Using the Law of Cosines to find the angle at B
	float angleB = acosf(FMath::Clamp((a * a + c * c - b * b) / (2 * a * c), -1.0f, 1.0f));

	return FMath::RadiansToDegrees(angleB);
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

float UATS_VehicleNavigation::CalculateBrakingForCorner(float currentSpeed, float distanceToCorner, FVector cornerPoint, FVector afterCornerPoint)
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
		To check wether the vehicle can take this corner, the distance from that point to the centerpoint should be the same(within certain range) as the turnradius
	*/
	if (m_pVehicleComponent == nullptr)
	{
		return 0.f;
	}

	//Initialize the variables
	const float DISTANCE_ERROR{ 10.f };

	float turnRadius{ 0.f };
	float turnWidth{ 0.f };

	//Calculate the turn radius
	turnRadius = currentSpeed * currentSpeed / (_Gravity * _DryAsphaltFriction);
	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::CalculateCorneringSpeed() -- TurnRadius %f"), turnRadius);
	}

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

	//Calculate the width of the turn
	turnWidth = cornerToCenterDistance - turnRadius;

	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("AgentNavigation::CalculateCorneringSpeed() -- TurnWidth %f"), turnWidth);
	}

	if (turnWidth <= _MaxTurnWidth)
	{
		//The car can take the corner at this speed
		return 0.f;
	}

	//The car will need to slow down
	// -- Calculate the difference between the turn width and the max turn width
	float widthDifference = turnWidth - _MaxTurnWidth;

	//Depending on the difference and the distance to the corner you can calculate how hard to brake [0 - 1]

	//Calculate the braking distance
	//float deceleration = _DryAsphaltFriction * _Gravity;
	//float brakingDistance = (currentSpeed * currentSpeed) / (2 * deceleration);


	//Calculate the braking input based on difference between turnRadius and distanceAfterCornerPoint
	float brakingInput = FMath::Clamp((turnRadius - distanceToAfterCornerPoint) / turnRadius, 0.f, 1.f);


	return brakingInput;
}

float UATS_VehicleNavigation::CalculateBrakingDistance(float currentSpeed) const
{
	//Calculate the braking distance
	float deceleration		= _DryAsphaltFriction * _Gravity;
	float brakingDistance	= (currentSpeed * currentSpeed) / (2 * deceleration);

	return brakingDistance;
}