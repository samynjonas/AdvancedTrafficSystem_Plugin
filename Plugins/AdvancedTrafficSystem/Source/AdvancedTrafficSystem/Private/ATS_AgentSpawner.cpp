// Fill out your copyright notice in the Description page of Project Settings.


#include "../Public/ATS_AgentSpawner.h"
#include "../Public/ATS_TrafficManager.h"
#include "../Public/ATS_AgentMain.h"

#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "ZoneShapeComponent.h"

#include "DrawDebugHelpers.h"

// Sets default values
AATS_AgentSpawner::AATS_AgentSpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AATS_AgentSpawner::BeginPlay()
{
	Super::BeginPlay();

	Initialize();
}

void AATS_AgentSpawner::Initialize()
{
	//Get TrafficManager and make sure it's initialzed already
	TArray<AActor*> pTrafficManagers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AATS_TrafficManager::StaticClass(), pTrafficManagers);

	for (AActor* pActor : pTrafficManagers)
	{
		// Do something with each actor
		AATS_TrafficManager* pTrafficManager = Cast<AATS_TrafficManager>(pActor);
		if (pTrafficManager)
		{
			m_pTrafficManager = pTrafficManager;
			//Break out of loop - found trafficManager
			break;
		}
	}

	if (m_pTrafficManager)
	{
		if (m_pTrafficManager->IsInitialized() == false)
		{
			m_pTrafficManager->Initialize();
		}
	}

	//Get Box from spawner actor
	//GetSpawnBox();

	SpawnAgents();

	m_pAgentMain = GetComponentByClass<UATS_AgentMain>();

	bIsInitialized = true;
}

FSpawnBox AATS_AgentSpawner::GetSpawnBox() const
{
	FSpawnBox spawnBox{};

	GetActorBounds(false, spawnBox.origin, spawnBox.extent, true);
	
	return spawnBox;
}

// Called every frame
void AATS_AgentSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bUpdateOnTick)
	{
		CheckForAgentLoss();
		if (m_SpawnedAgents < m_AgentCount)
		{
			SpawnAgents();
		}
	}	
}

void AATS_AgentSpawner::SpawnAgents()
{
	// Check if TrafficManager is available
	if (m_pTrafficManager == nullptr)
	{
		return;
	}

	for (int i = 0; i < m_AgentCount; ++i)
	{
		// Place a random point within the zone shape
		FVector randomPoint = GenerateRandomPointWithinBoundingBox();
		UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::Random point: %s"), *randomPoint.ToString());

		// Find the closest point on a lane
		FAgentPoint closestPointOnLane = m_pTrafficManager->GetClosestPointOnLane(FBox(randomPoint - FVector(1000), randomPoint + FVector(1000)), FZoneGraphTagFilter(), randomPoint);
		
		UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::Closest point on lane: %s"), *closestPointOnLane.position.ToString());
		if (!IsLocationOccupied(closestPointOnLane.position))
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::Spawning agent at: %s"), *closestPointOnLane.position.ToString());
			SpawnAgentAtLocation(closestPointOnLane.position, closestPointOnLane.direction);
			m_SpawnedAgents++;
		}
	}
}

// Helper function to generate a random point within a 2D bounding box
FVector AATS_AgentSpawner::GenerateRandomPointWithinBoundingBox() const
{
	FSpawnBox spawnBox{ GetSpawnBox() };

	return spawnBox.GetRandomPoint();
}

void AATS_AgentSpawner::RegisterAgentLoss(int count)
{
	m_SpawnedAgents -= count;
}

void AATS_AgentSpawner::CheckForAgentLoss()
{
	if (m_pAgentMain)
	{
		RegisterAgentLoss(m_pAgentMain->GetAgentLoss());
		m_pAgentMain->ResetAgentLoss();
	}
}

FTransform AATS_AgentSpawner::GetClosestLanePoint(const FVector& Location, float searchDistance)
{
	FTransform safeLocation{};
	if (m_pTrafficManager == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::TrafficManager is nullptr"));
		Initialize();
	}

	UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::Getting closest lane point"));

	safeLocation = m_pTrafficManager->GetClosestLanePoint(Location, searchDistance);
	return safeLocation;
}

bool AATS_AgentSpawner::IsLocationOccupied(const FVector& Location) const
{
	//TODO FIX this
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	FCollisionShape CollisionShape = FCollisionShape::MakeBox(FVector(50.f, 50.f, 50.f));

	// Add a debug draw to visualize the overlap check
	if (GEngine)
	{
		DrawDebugBox(GetWorld(), Location, CollisionShape.GetExtent(), FColor::Red, false, 10.0f, 0, 1);
	}

	return false;
}

