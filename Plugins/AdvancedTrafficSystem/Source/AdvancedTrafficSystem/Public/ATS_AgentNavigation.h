// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Components/ActorComponent.h"
#include "../Public/ATS_TrafficHelper.h"

#include "ATS_AgentNavigation.generated.h"


class AATS_TrafficManager;
class UChaosVehicleMovementComponent;
class UATS_SpatialUnit;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ADVANCEDTRAFFICSYSTEM_API UATS_AgentNavigation : public UActorComponent
{
	GENERATED_BODY()
public:	
	UATS_AgentNavigation();

//--------------------------------------------------------------------------------------------
// Default Unreal Engine Functions
//--------------------------------------------------------------------------------------------
public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

//--------------------------------------------------------------------------------------------
// Navigation functions
//--------------------------------------------------------------------------------------------
public:
	void SetFollowPath(bool isEnabled);
	void SetNavGoal(const FVector& destination);

protected:
	bool MoveToNextPoint(float deltaTime);

	virtual bool MoveToNextPointPhysics(float deltaTime);
	virtual bool MoveToNextPointSimple(float deltaTime);

	bool IsGoalPointReached(const FVector& goalPoint) const;
	bool FollowPath(float deltaTime);
	void RetrievePath();

	bool RetrieveLanePoints();

//--------------------------------------------------------------------------------------------
// Agent calculation functions
//--------------------------------------------------------------------------------------------
public:
	UFUNCTION(BlueprintCallable, Category = "AgentNavigation")
	void SetMaxSpeed(float speed);

	UFUNCTION(BlueprintCallable, Category = "AgentNavigation")
	float GetMaxSpeed() const { return m_MaxSpeedkmph; }

protected:

//--------------------------------------------------------------------------------------------
// Debugging functions
//--------------------------------------------------------------------------------------------
public:
	UFUNCTION(BlueprintCallable, Category = "AgentNavigation")
	void VisualizePath(bool isVisible);

protected:
	void Debugging(float currentSpeed, float steeringInput, float throttleInput, float brakingInput, float cornerAngle, bool isEnabled = true);

//--------------------------------------------------------------------------------------------
// Spatial partitioning functions
//--------------------------------------------------------------------------------------------
public:


protected:
	bool RetrieveSpatialUnit();

	template <typename T>
	TArray<AActor*> GetNearbyActors() 
	{
		TArray<AActor*> actors;

		if (m_pSpatialUnit)
		{
			actors = m_pSpatialUnit->GetNearbyActors();

			//Filter out the actors that have the actor component
			for (int i = actors.Num() - 1; i >= 0; --i)
			{
				if (actors[i]->FindComponentByClass<T>() == nullptr)
				{
					actors.RemoveAt(i);
				}
			}
		}
		else
		{
			//Search all the actors in the world
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), actors);
			for (int i = actors.Num() - 1; i >= 0; --i)
			{
				if (actors[i]->FindComponentByClass<T>() == nullptr)
				{
					actors.RemoveAt(i);
				}
			}
		}

		return actors;
	}

//--------------------------------------------------------------------------------------------
// Other functions
//--------------------------------------------------------------------------------------------
public:
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

protected:
	void UpdateAgentData();

//--------------------------------------------------------------------------------------------
// Code Variables
//--------------------------------------------------------------------------------------------
protected:
	// Agent data
	FAgentData m_AgentData{};

	// Components and references
	AATS_TrafficManager* m_pTrafficManager;
	UChaosVehicleMovementComponent* m_pVehicleComponent;	
	FTrafficNavigationPath m_NavigationPath{};

	// Spatial unit for spatial partitioning
	UATS_SpatialUnit* m_pSpatialUnit{ nullptr };

	// Speed variables
	const float BASE_SPEED{ 30.f };
	float m_MaxSpeedkmph{ 30.f };
	float m_MaxSpeedUnrealunits{};
	float m_DesiredSpeed{};

	// Vehicle variables
	bool bIsCornering{ false };
	FVector m_CornerPoint{};
	float m_CornerAngle{};

	// Lane variables
	TArray<FLanePoint> m_LanePoints{};
	int m_CurrentPointIndex{ 1 };

//--------------------------------------------------------------------------------------------
// Settings
//--------------------------------------------------------------------------------------------
protected:
	UPROPERTY(EditAnywhere, Category = "Settings", DisplayName = "Is the agent disabled")
	bool bIsDisabled{ false };

	UPROPERTY(EditAnywhere, Category = "Settings", DisplayName = "Is the agent parked")
	bool bIsParked{ false };
	
	UPROPERTY(EditAnywhere, Category = "Navigation", DisplayName = "Should follow path")
	bool bFollowPath{ false };

	UPROPERTY(EditAnywhere, Category = "Navigation", meta = (EditCondition = "bFollowPath"), DisplayName = "Path destination")
	FVector m_Destination{};
	
	UPROPERTY(EditAnywhere, Category = "Settings|Physics", DisplayName = "Is the agent physics based")
	bool bIsPhysicsBased{ true };

	// The minimum distance a point can be from the vehicle
	UPROPERTY(EditAnywhere, Category = "Settings|Physics", meta = (EditCondition = "bIsPhysicsBased"), DisplayName = "Min point distance")
	float MinPointDistance{ 750.f }; // BASE VALUE: 750.f

	// The maximum distance a point can be from the vehicle
	UPROPERTY(EditAnywhere, Category = "Settings|Physics", meta = (EditCondition = "bIsPhysicsBased"), DisplayName = "Max point distance")
	float MaxPointDistance{ 1000.f }; // BASE VALUE: 1000.f

	// The gravity in cm
	UPROPERTY(EditAnywhere, Category = "Physics", DisplayName = "Gravity in cm");
	float _Gravity{ 981.f }; // BASE VALUE: 981.f

//--------------------------------------------------------------------------------------------
// Debugging variables
//--------------------------------------------------------------------------------------------
protected:
	UPROPERTY(EditAnywhere, Category = "Debug", DisplayName = "Debug to console")
	bool bDebug{ false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Drawing", DisplayName = "Debug draw objects")
	bool bDrawDebugObjects{ false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Drawing", meta = (EditCondition = "bDrawDebugObjects"), DisplayName = "Thickness")
	float _DrawDebugThickness{ 5.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Drawing", meta = (EditCondition = "bDrawDebugObjects"), DisplayName = "Circle Radius")
	float _DrawDebugCircleRadius{ 100.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Drawing|colors", meta = (EditCondition = "bDrawDebugObjects"), DisplayName = "First point color")
	FColor _DrawDebugNextPointColor{ FColor::Red };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Drawing|colors", meta = (EditCondition = "bDrawDebugObjects"), DisplayName = "Second point Color")
	FColor _DrawDebugSecondNextPointColor{ FColor::Red };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Drawing|colors", meta = (EditCondition = "bDrawDebugObjects"), DisplayName = "Desired speed point color")
	FColor _DrawDebugDesiredPointColor{ FColor::Green };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Drawing|colors", meta = (EditCondition = "bDrawDebugObjects"), DisplayName = "Turn circle color")
	FColor _DrawDebugTurnCircle{ FColor::Blue };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Drawing|colors", meta = (EditCondition = "bDrawDebugObjects"), DisplayName = "Corner point color")
	FColor _DrawDebugCornerPointColor{ FColor::Blue };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Drawing|colors", meta = (EditCondition = "bDrawDebugObjects"), DisplayName = "Corner exit point color")
	FColor _DrawDebugCornerExitColor{ FColor::Blue };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Drawing", DisplayName = "Debug draw a path")
	bool bDrawDebugPath{ false };
};
