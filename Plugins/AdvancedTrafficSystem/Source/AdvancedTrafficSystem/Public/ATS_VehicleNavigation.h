// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ATS_AgentNavigation.h"
#include "ATS_VehicleNavigation.generated.h"

/*
	ATS_VehicleNavigation is a subclass of UAgentNavigation
	It is used to provide a custom navigation system for vehicles
*/

class USplineComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ADVANCEDTRAFFICSYSTEM_API UATS_VehicleNavigation : public UATS_AgentNavigation
{
	GENERATED_BODY()

public:
	UATS_VehicleNavigation();

protected:
	bool MoveToNextPointPhysics(float deltaTime) override;
	bool MoveToNextPointSimple(float deltaTime) override;

	bool RetrieveSpline();
	bool GenerateSplineFromLanePoints();

protected:
	float CalculateSteeringInput(const FVector& CurrentForward, const FVector& ToNextPoint);
	float CalculateThrottleInput(const FVector& CurrentPosition, const FVector& NextPoint);
	float CalculateTurnSharpness(const FVector& CurrentDirection, const FVector& DirectionToNextPoint, float DistanceToNextPoint);
	float CalculateCornerAngle(const FVector& currentPoint, const FVector& nextPoint, const FVector& secondNextPoint);

	float CalculateMaxCorneringSpeed(float TurnRadius, float FrictionCoefficient);
	float CalculateMaxCorneringSpeed(float frictionCoefficient, FVector agentPoint, FVector nextPoint, FVector secondNextPoint);

	float CalculateTurnRadius(float TurnSharpness, float CurrentSpeed);

	bool ShouldBrake(float TurnSharpness, float currentSpeed, float maxCornerSpeed);

	UFUNCTION(BlueprintCallable, Category = "VehicleNavigation")
	float CalculateBrakingDistance(float currentSpeed) const;

	float CalculateBrakingForCorner(float currentSpeed, float distanceToCorner, FVector cornerPoint, FVector afterCornerPoint, float& canTurn);

	bool ApplyVehicleControl(float SteeringInput, float ThrottleInput, float BrakeInput);

protected:
	UPROPERTY(EditAnywhere, Category = "VehicleSettings|Braking", DisplayName = "Braking multiplier")
	float m_BrakingMultiplier{ 1.0f }; // BASE VALUE: 1.f

	UPROPERTY(EditAnywhere, Category = "VehicleSettings|Braking", DisplayName = "Braking distance offset")
	float m_BrakingDistanceOffset{ 400.0f }; // BASE VALUE: 400.f
	
	UPROPERTY(EditAnywhere, Category = "VehicleSettings|Steering", DisplayName = "Max steering value")
	float m_MaxSteeringValue{ 1.0f }; // MAX VALUE: 1.f

	UPROPERTY(EditAnywhere, Category = "VehicleSettings|Steering", DisplayName = "Min steering value")
	float m_MinSteeringValue{ -1.0f }; // MIN VALUE: -1.f


	// The maximum steering angle the vehicle can have
	UPROPERTY(EditAnywhere, Category = "VehicleSettings|Physics", meta = (EditCondition = "bIsPhysicsBased"), DisplayName = "Max steering angle")
	float m_MaxSteeringAngle{ 35.f }; // BASE VALUE: 35.f

	// Max distance to the next points to which we will scale the throttle -> longer equals slower acceleration
	UPROPERTY(EditAnywhere, Category = "VehicleSettings|Physics", meta = (EditCondition = "bIsPhysicsBased"), DisplayName = "Max throttle distance")
	float m_MaxThrottleDistance{ 250.f }; // BASE VALUE: 250.f

	UPROPERTY(EditAnywhere, Category = "VehicleSettings|Physics", meta = (EditCondition = "bIsPhysicsBased"), DisplayName = "Max turn width")
	float _MaxTurnWidth{ 400.f }; // BASE VALUE: 400.f

	// The friction coefficient of dry asphalt
	UPROPERTY(EditAnywhere, Category = "Physics|Friction", DisplayName = "Friction coefficient dry asphalt");
	float _DryAsphaltFriction{ 0.7f }; // BASE VALUE: 0.7f

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Drawing|colors", meta = (EditCondition = "bDrawDebugObjects"), DisplayName = "First point color")
	FColor _DrawDebugBrakeColor{ FColor::Black };

protected:
	USplineComponent* m_pSplineComponent{ nullptr };

	/*
	UPROPERTY(EditAnywhere, Category = "VehicleSettings|Steering")
	float m_WheelBase{ 2.5f }; // BASE VALUE: 2.5f

	UPROPERTY(EditAnywhere, Category = "VehicleSettings|Steering")
	float m_TrackWidth{ 4.f }; // BASE VALUE: 2.5f

	UPROPERTY(EditAnywhere, Category = "VehicleSettings|Steering")
	float m_MaxSteeringAngle{ 30.f }; // BASE VALUE: 2.5f
	*/




};
