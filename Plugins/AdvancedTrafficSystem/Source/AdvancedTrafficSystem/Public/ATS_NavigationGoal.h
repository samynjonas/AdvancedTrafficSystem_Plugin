// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATS_TrafficHelper.h"
#include "ATS_NavigationGoal.generated.h"

UCLASS()
class ADVANCEDTRAFFICSYSTEM_API AATS_NavigationGoal : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AATS_NavigationGoal();

	ENavGoalType GetNavGoalType() const
	{
		return m_NavGoalType;
	}

	FVector GetNavGoalLocation() const
	{
		return GetActorLocation();
	}

	int GetAgentCount() const
	{
		return m_AgentCount;
	}

	void SetAgentCount(int agentCount)
	{
		m_AgentCount = agentCount;
	}

	void SetWorkHours(FVector2D workHours)
	{
		m_WorkTimes = workHours;
	}

	FVector2D GetWorkHours() const
	{
		return m_WorkTimes;
	}


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "NavigationGoal")
	void IsVisible(bool isVisible);


protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavigationGoal")
	ENavGoalType m_NavGoalType{ ENavGoalType::Other };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavigationGoal")
	int m_AgentCount{ 1 }; //Amount of agents that can be at this point at the same time will usually be higher for work places compared to houses

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavigationGoal")
	FVector2D m_WorkTimes{ 8.f, 17.f };
};
