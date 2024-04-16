// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATS_CityManager.generated.h"

class AATS_NavigationGoal;
class AATS_AgentSpawner;

UCLASS()
class ADVANCEDTRAFFICSYSTEM_API AATS_CityManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AATS_CityManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	int GetRandomAmountOfAgents() const;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	int m_CityAgentCount{ 0 }; //Amount of agents that will "live" in the city

	UPROPERTY(EditAnywhere, Category = "NavigationGoal")
	int m_MaxAgentCount{ 1000 }; //Max amount of agents that can be spawned


	UPROPERTY(EditAnywhere, Category = "NavigationGoal")
	bool bIsDebugMode{ true };

	UPROPERTY(EditAnywhere, Category = "NavigationGoal")
	bool bUseNavigationGoals{ true };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavigationGoal")
	FVector2D m_MinMaxAgentPerHouse{ 0, 2 }; //Range of agents per house


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavigationGoal")
	FVector2D m_WorkTimeRange{ 8, 17 }; //Start and end of work time in hours


	//ARRAYS OF NAVIGATION GOALS
	TArray<AATS_NavigationGoal*> m_vecHouses;
	TArray<AATS_NavigationGoal*> m_vecWorkplaces;
	TArray<AATS_NavigationGoal*> m_vecParkings;

	AATS_AgentSpawner* m_pAgentSpawner;
};
