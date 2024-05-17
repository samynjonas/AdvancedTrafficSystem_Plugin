// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ATS_AgentNavigation.h"
#include "ATS_PedestrianNavigation.generated.h"

/*
	This will be used for navigation of pedestrian - the idea is not to control the agent directly
	but to be a connection between the agent and the navigation system
*/

class UATS_TrafficAwarenessComponent;
class UATS_PedestrianNavigation;
class AATS_LaneSpline;

class ADVANCEDTRAFFICSYSTEM_API steeringManager final
{
public:
	steeringManager(UATS_PedestrianNavigation* host);
	~steeringManager() = default;

	void Update(float deltaTime);

public:
	// Simple steering behaviors
	void Seek(const FVector& target, float slowingRadius, float impactScale = 1.f);
	void Flee(const FVector& target, float impactScale = 1.f);
	void Wander(float& agentWanderAngle, float maxAngleChange, float circleRadius, float impactScale = 1.f);
	void FollowPath(const TArray<FVector>& pathPoints, int& currentPoint, float switchPointDistance, float impactScale = 1.f);

	// Advanced steering behaviors
	void Seperation(TArray<AActor*> agents, float minSeperationDistance, float impactScale = 1.f);
	void Cohesion(TArray<AActor*> agents, float maxCohesion, float impactScale = 1.f);
	void Alignment(TArray<AActor*> agents, float maxCohesion, float impactScale = 1.f);

private:
	FVector DoSeek(const FVector& target, float slowingRadius, float impactScale = 1.f);
	FVector DoFlee(const FVector& target, float impactScale = 1.f);
	FVector DoWander(float& agentWanderAngle, float maxAngleChange, float circleRadius, float impactScale = 1.f);
	FVector DoFollowPath(const TArray<FVector>& pathPoints, int& currentPoint, float switchPointDistance, float impactScale = 1.f);

	FVector DoSeperation(TArray<AActor*> agents, float minSeperationDistance, float impactScale = 1.f);
	FVector DoCohesion(TArray<AActor*> agents, float maxCohesion, float impactScale = 1.f);
	FVector DoAlignment(TArray<AActor*> agents, float maxCohesion, float impactScale = 1.f);


public:
	UATS_PedestrianNavigation* _pHost{ nullptr };
	FVector _Steering;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ADVANCEDTRAFFICSYSTEM_API UATS_PedestrianNavigation : public UATS_AgentNavigation
{
	GENERATED_BODY()

protected:
	void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable)
	TArray<FVector> GetPathPoints() const;

	UFUNCTION(BlueprintCallable)
	FVector GetCurrentVelocity() const;


	UFUNCTION(BlueprintCallable)
	FRotator GetCurrentRotation() const;

	UFUNCTION(BlueprintCallable)
	FVector GetPosition() const;

	float GetMaxSpeed() const;
	float GetMaxForce() const;
	float GetMass() const;

	UFUNCTION(BlueprintCallable)
	void SetPosition(const FVector& newPosition);

	UFUNCTION(BlueprintCallable)
	void SetCurrentRotation(const FRotator& newPosition);

	UFUNCTION(BlueprintCallable)
	void SetCurrentVelocity(const FVector& newVelocity);

protected:
	virtual bool MoveToNextPointSimple(float deltaTime) override;

protected:
	FVector RetrieveTarget();
	bool GetPathPointsFromNavigationSystem(AATS_LaneSpline* pLane);
	bool RetrieveNextLanePoints();
	UATS_TrafficAwarenessComponent* WillPassLaneModifier();

protected:
	// AGENT DATA
	TUniquePtr<steeringManager> _pSteeringManager{ nullptr };

	FRotator _CurrentRotation{};
	FVector _CurrentVelocity{};
	FVector _Position{};
	
	float _WanderAngle{ 0.f };

	TArray<FVector> _PathPoints;
	int _CurrentPathPointIndex{ 0 };

	AATS_LaneSpline* _pCurrentLane{ nullptr };

protected:
	// AGENT SETTINGS

	UPROPERTY(EditAnywhere, Category = "Steering|wander")
	float _WanderRadius{ 100.f };
	
	UPROPERTY(EditAnywhere, Category = "Steering|wander")
	float _WanderAngleChange{ 0.5f };

	UPROPERTY(EditAnywhere, Category = "Steering|PathFollowing")
	float _PathPointCheckDistance{ 500.f };

	UPROPERTY(EditAnywhere, Category = "Steering|Seperation")
	float _SeperationDistance{ 150.f };

	UPROPERTY(EditAnywhere, Category = "Steering|Seperation")
	float _CohesionDistance{ 150.f };

	UPROPERTY(EditAnywhere, Category = "Steering|Seperation")
	float _AlignmentDistance{ 150.f };

	UPROPERTY(EditAnywhere, Category = "Steering")
	float _MaxForce{ 7500.f };

	UPROPERTY(EditAnywhere, Category = "Steering")
	float _MaxSpeed{ 750.0f };

	UPROPERTY(EditAnywhere, Category = "Steering")
	float _Mass{ 75.f };

	UPROPERTY(EditAnywhere, Category = "Steering")
	AActor* _GoalActor{ nullptr };

protected:
	UPROPERTY(EditAnywhere, Category = "Steering|Behaviors")
	bool _bUseSeek{ false };

	UPROPERTY(EditAnywhere, Category = "Steering|Behaviors", meta = (EditCondition = "_bUseSeek", UIMin = "0.0", UIMax = "100.0", SliderExponent = "1.0"))
	float _SeekImpact{ 100.f };

	UPROPERTY(EditAnywhere, Category = "Steering|Behaviors")
	bool _bUseFlee{ false };

	UPROPERTY(EditAnywhere, Category = "Steering|Behaviors", meta = (EditCondition = "_bUseFlee", UIMin = "0.0", UIMax = "100.0", SliderExponent = "1.0"))
	float _FleeImpact{ 100.f };

	UPROPERTY(EditAnywhere, Category = "Steering|Behaviors")
	bool _bUseWander{ false };

	UPROPERTY(EditAnywhere, Category = "Steering|Behaviors", meta = (EditCondition = "_bUseWander", UIMin = "0.0", UIMax = "100.0", SliderExponent = "1.0"))
	float _WanderImpact{ 100.f };

	UPROPERTY(EditAnywhere, Category = "Steering|Behaviors")
	bool _bUseFollowPath{ false };

	UPROPERTY(EditAnywhere, Category = "Steering|Behaviors", meta = (EditCondition = "_bUseFollowPath", UIMin = "0.0", UIMax = "100.0", SliderExponent = "1.0"))
	float _FollowPathImpact{ 100.f };

	UPROPERTY(EditAnywhere, Category = "Steering|Behaviors")
	bool _bUseSeperation{ false };

	UPROPERTY(EditAnywhere, Category = "Steering|Behaviors", meta = (EditCondition = "_bUseSeperation", UIMin = "0.0", UIMax = "100.0", SliderExponent = "1.0"))
	float _SeperationImpact{ 100.f };

	UPROPERTY(EditAnywhere, Category = "Steering|Behaviors")
	bool _bUseCohesion{ false };

	UPROPERTY(EditAnywhere, Category = "Steering|Behaviors", meta = (EditCondition = "_bUseCohesion", UIMin = "0.0", UIMax = "100.0", SliderExponent = "1.0"))
	float _CohesionImpact{ 100.f };

	UPROPERTY(EditAnywhere, Category = "Steering|Behaviors")
	bool _bUseAlignment{ false };

	UPROPERTY(EditAnywhere, Category = "Steering|Behaviors", meta = (EditCondition = "_bUseAlignment", UIMin = "0.0", UIMax = "100.0", SliderExponent = "1.0"))
	float _AlignmentImpact{ 100.f };

};