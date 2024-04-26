// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ATS_AgentNavigation.h"
#include "ATS_PedestrianNavigation.generated.h"

/*
	This will be used for navigation of pedestrian - the idea is not to control the agent directly
	but to be a connection between the agent and the navigation system
*/

class UATS_PedestrianNavigation;

class ADVANCEDTRAFFICSYSTEM_API steeringManager final
{
public:
	steeringManager(UATS_PedestrianNavigation* host);
	~steeringManager() = default;

	void Update(float deltaTime);

public:
	void Seek(const FVector& target, float slowingRadius, float impactScale = 1.f);
	void Flee(const FVector& target, float impactScale = 1.f);
	void Wander(float& agentWanderAngle, float maxAngleChange, float circleRadius, float impactScale = 1.f);
	void FollowPath(const TArray<FVector>& pathPoints, int& currentPoint, float switchPointDistance, float impactScale = 1.f);

private:
	FVector DoSeek(const FVector& target, float slowingRadius, float impactScale = 1.f);
	FVector DoFlee(const FVector& target, float impactScale = 1.f);
	FVector DoWander(float& agentWanderAngle, float maxAngleChange, float circleRadius, float impactScale = 1.f);
	FVector DoFollowPath(const TArray<FVector>& pathPoints, int& currentPoint, float switchPointDistance, float impactScale = 1.f);

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
	TArray<FVector> GetPathPoints() const;

	FVector GetCurrentVelocity() const;
	FVector GetPosition() const;
	float GetMaxSpeed() const;
	float GetMaxForce() const;
	float GetMass() const;

	void SetPosition(const FVector& newPosition);
	void SetCurrentVelocity(const FVector& newVelocity);

protected:
	virtual bool MoveToNextPointSimple(float deltaTime) override;

protected:
	FVector RetrieveTarget();
	bool GetPathPointsFromNavigationSystem();


protected:
	// AGENT DATA
	TUniquePtr<steeringManager> _pSteeringManager{ nullptr };

	FVector _CurrentVelocity{};
	FVector _Position{};
	
	float _WanderAngle{ 0.f };

	TArray<FVector> _PathPoints;
	int _CurrentPathPointIndex{ 0 };

protected:
	// AGENT SETTINGS

	UPROPERTY(EditAnywhere, Category = "Steering|wander")
	float _WanderRadius{ 100.f };
	
	UPROPERTY(EditAnywhere, Category = "Steering|wander")
	float _WanderAngleChange{ 0.5f };

	UPROPERTY(EditAnywhere, Category = "Steering|PathFollowing")
	float _PathPointCheckDistance{ 200.f };

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

};