// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Public/ATS_TrafficHelper.h"
#include "ATS_AgentNavigation.generated.h"

class AATS_TrafficManager;
class UChaosVehicleMovementComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ADVANCEDTRAFFICSYSTEM_API UATS_AgentNavigation : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UATS_AgentNavigation();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	bool MoveToNextPoint(float deltaTime);

	bool MoveToNextPointPhysics(float deltaTime);
	bool MoveToNextPointSimple(float deltaTime);
	
	float CalculateSteeringInput(const FVector& CurrentForward, const FVector& ToNextPoint);
	float CalculateThrottleInput(const FVector& CurrentPosition, const FVector& NextPoint);
	float CalculateTurnSharpness(const FVector& CurrentDirection, const FVector& DirectionToNextPoint, float DistanceToNextPoint);
	float CalculateCornerAngle(const FVector& currentPoint, const FVector& nextPoint, const FVector& secondNextPoint);
	
	float CalculateMaxCorneringSpeed(float TurnRadius, float FrictionCoefficient);
	float CalculateMaxCorneringSpeed(float frictionCoefficient, FVector agentPoint, FVector nextPoint, FVector secondNextPoint);
	
	float CalculateTurnRadius(float TurnSharpness, float CurrentSpeed);

	bool ShouldBrake(float TurnSharpness, float currentSpeed, float maxCornerSpeed);

	bool ApplyVehicleControl(float SteeringInput, float ThrottleInput, float BrakeInput);

	void UpdateAgentData();
	void Debugging(float currentSpeed, float steeringInput, float throttleInput, float brakingInput, float cornerAngle, bool isEnabled = true);

	bool IsGoalPointReached(const FVector& goalPoint) const;
	bool FollowPath(float deltaTime);
	void RetrievePath();	

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FAgentData GetAgentData() const
	{
		return m_AgentData;
	}
	void SetAgentData(const FAgentData& agentData)
	{
		m_AgentData = agentData;
	}


	UFUNCTION(BlueprintCallable, Category = "AgentNavigation")
	void DissableAgent();

	UFUNCTION(BlueprintCallable, Category = "AgentNavigation")
	void EnableAgent();

	void SetFollowPath(bool isEnabled);

	void SetNavGoal(const FVector& destination);

	UFUNCTION(BlueprintCallable, Category = "AgentNavigation")
	void VisualizePath(bool isVisible);

	UFUNCTION(BlueprintCallable, Category = "AgentNavigation")
	void SetMaxSpeed(float speed);
	
	UFUNCTION(BlueprintCallable, Category = "AgentNavigation")
	float GetMaxSpeed() const { return m_MaxSpeedkmph; }

protected:
	AATS_TrafficManager* m_pTrafficManager;
	UChaosVehicleMovementComponent* m_pVehicleComponent;	

	float m_MaxSteeringAngle{ 35.f };
	float m_MaxThrottleDistance{ 250.f };

	//Max speed the agent can go in kmph - not traffic dependent
	UPROPERTY(EditAnywhere, Category = "AgentNavigation|vehicle")
	float m_MaxSpeedkmph{ 30.f };

	float m_MaxSpeedUnrealunits{};

	//This will change depending on the traffic
	float m_DesiredSpeed{};

	float MinPointDistance{ 750.f };
	float MaxPointDistance{ 1000.f };

	bool bIsCornering{ false };
	FVector m_CornerPoint{};
	FVector m_ExitPoint{};

	FAgentData m_AgentData{};
	FTrafficNavigationPath m_NavigationPath{};

	UPROPERTY(EditAnywhere, Category = "AgentNavigation|vehicle")
	bool bIsDisabled{ false };

	UPROPERTY(EditAnywhere, Category = "AgentNavigation|vehicle")
	bool bIsPhysicsBased{ true };

	UPROPERTY(EditAnywhere, Category = "AgentNavigation|vehicle")
	bool bIsParked{ false };

	UPROPERTY(EditAnywhere, Category = "AgentNavigation|vehicle")
	bool bDebug{ false };

	UPROPERTY(BlueprintReadWrite, Category = "AgentNavigation|vehicle")
	bool bDrawDebugPath{ false };	

	UPROPERTY(EditAnywhere, Category = "AgentNavigation|Path")
	bool bFollowPath{ false };

	UPROPERTY(EditAnywhere, Category = "AgentNavigation|Path")
	FVector m_Destination{};

	
};
