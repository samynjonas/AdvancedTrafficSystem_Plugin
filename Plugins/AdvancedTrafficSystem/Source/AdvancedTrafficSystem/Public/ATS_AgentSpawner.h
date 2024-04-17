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
class UBoxComponent;

USTRUCT()
struct FSpawnBox
{
	GENERATED_BODY()

	FVector origin{};
	FVector extent{};

	FSpawnBox() = default;

	FVector GetRandomPoint(bool zeroZ = true) const
	{
		FVector randomPoint{ FVector(FMath::RandRange(origin.X - extent.X, origin.X + extent.X), FMath::RandRange(origin.Y - extent.Y, origin.Y + extent.Y), FMath::RandRange(origin.Z - extent.Z, origin.Z + extent.Z)) };
		
		if (zeroZ)
		{
			randomPoint.Z = 0.0f;
		
		}
		return randomPoint;
	}

	FVector GetRandomPointOnEdge() const
	{
		FVector randomPoint{ FVector::ZeroVector };

		//Pick a point on the edge of the box
		int FrontSideEdge = FMath::RandRange(0, 1);
		if (FrontSideEdge == 0)
		{
			//Pick front or back edge
			int FrontBackEdge = FMath::RandRange(0, 1);
			if (FrontBackEdge == 0)
			{
				FrontBackEdge = -1;
			}

			randomPoint = FVector(FMath::RandRange(origin.X - extent.X, origin.X + extent.X), origin.Y + (extent.Y * FrontBackEdge), 0.f);
		}
		else
		{
			//Pick left or right edge
			int LeftRightEdge = FMath::RandRange(0, 1);
			if (LeftRightEdge == 0)
			{
				LeftRightEdge = -1;
			}

			randomPoint = FVector(origin.X + (extent.X * LeftRightEdge), FMath::RandRange(origin.Y - extent.Y, origin.Y + extent.Y), 0.f);
		}		
		
		return randomPoint;
	}
};

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
	void AttachSpawnerToActor();
	FSpawnBox GetSpawnBox() const;

	void SpawnAgents(bool fillBox);
	FVector GenerateRandomPointWithinBoundingBox() const;
	FVector GenerateRandomPointOnEdgeOfBoundingBox() const;

	bool SpawnAgentAtLocation(FVector position, FVector direction);

	bool ValidateSpawnedAgent(AActor* pAgent);

	bool IsLocationOccupied(const FVector& Location) const;

	void CheckForAgentLoss();	

	UFUNCTION(BlueprintCallable, Category = "Spawner")
	FTransform GetClosestLanePoint(const FVector& Location, float searchDistance);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void RegisterAgentLoss(int count = 1);

	UFUNCTION(BlueprintCallable, Category = "Spawner")
	bool SpawnAgentWithNavigationGoals(AATS_NavigationGoal* pHome, AATS_NavigationGoal* pWork);

	UFUNCTION(BlueprintCallable, Category = "Spawner")
	bool SpawnAgentAtHome(AATS_NavigationGoal* pHome);

protected:
	int m_SpawnedAgents{ 0 };
	bool bIsInitialized{ false };

	UPROPERTY(EditAnywhere, Category = "SpawnerSettings", DisplayName = "Amount of agent to spawn")
	int m_AgentCount{ 0 };

	UPROPERTY(EditAnywhere, Category = "SpawnerSettings", DisplayName = "Keep spawning during runtime")
	bool bUpdateOnTick{ false };

	UPROPERTY(EditAnywhere, Category = "SpawnerSettings|Debugging", DisplayName = "Enable debugging")
	bool bDebug{ false };

	UPROPERTY(EditAnywhere, Category = "SpawnerSettings", DisplayName = "Actors to pick from to spawn")
	TArray<TSubclassOf<AActor>> _ArrAgentClasses{};

	UPROPERTY(EditAnywhere, Category = "Attachment Settings", DisplayName = "Attach to an actor")
	bool bAttachToActor{ false };

	UPROPERTY(EditAnywhere, Category = "Attachment Settings", meta = (EditCondition = "bAttachToActor", DisplayName = "Actor to attach to"))
	AActor* _pAttachToActor{ nullptr };
	
	UPROPERTY(EditAnywhere, Category = "Attachment Settings", meta = (EditCondition = "bAttachToActor", DisplayName = "Attach to a component"))
	bool bAttachToActorComponent{ false };

	UPROPERTY(EditAnywhere, Category = "Attachment Settings", meta = (EditCondition = "bAttachToActor && bAttachToActorComponent", DisplayName = "Component to attach to"))
	FString _pComponentNameToAttach{ "" };	

	AATS_TrafficManager* m_pTrafficManager{ nullptr };

	UPROPERTY(EditAnywhere, Category = "SpawnerComponents")
	UATS_AgentMain* _AgentMain{ nullptr };

	UPROPERTY(EditAnywhere, Category = "SpawnerComponents")
	UBoxComponent* _BoxComponent{ nullptr };
};
