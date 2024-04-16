// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATS_TrafficHelper.h"
#include "ATS_AgentSpawner.generated.h"


class UZoneShapeComponent;
class AATS_TrafficManager;
class UATS_AgentMain;
class AATS_NavigationGoal;

UCLASS()
class ADVANCEDTRAFFICSYSTEM_API AATS_AgentSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AATS_AgentSpawner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void Initialize();

	void SpawnAgents();
	FVector GenerateRandomPointWithinBoundingBox() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Spawner")
	void SpawnAgentAtLocation(FVector position, FVector direction);

	bool IsLocationOccupied(const FVector& Location) const;

	void CheckForAgentLoss();	

	UFUNCTION(BlueprintCallable, Category = "Spawner")
	FTransform GetClosestLanePoint(const FVector& Location, float searchDistance);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void RegisterAgentLoss(int count = 1);

	UFUNCTION(BlueprintImplementableEvent, Category = "Spawner")
	void SpawnAgentWithNavigationGoals(AATS_NavigationGoal* pHome, AATS_NavigationGoal* pWork);

	UFUNCTION(BlueprintImplementableEvent, Category = "Spawner")
	void SpawnAgentAtHome(AATS_NavigationGoal* pHome);

protected:
	UPROPERTY(EditAnywhere, Category = "Settings")
	int m_AgentCount{ 0 };
	
	int m_SpawnedAgents{ 0 };

	AATS_TrafficManager* m_pTrafficManager;

	UPROPERTY(EditAnywhere, Category = "Settings")
	FVector2D m_BoundingBox{};

	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bUpdateOnTick{ false };

	bool bIsInitialized{ false };

	UATS_AgentMain* m_pAgentMain;
};
