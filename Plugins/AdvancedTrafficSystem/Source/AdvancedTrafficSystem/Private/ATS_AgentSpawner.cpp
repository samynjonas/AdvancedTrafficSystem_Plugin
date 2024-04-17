// Fill out your copyright notice in the Description page of Project Settings.


#include "../Public/ATS_AgentSpawner.h"
#include "../Public/ATS_TrafficManager.h"
#include "../Public/ATS_AgentMain.h"

#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "ZoneShapeComponent.h"

#include "DrawDebugHelpers.h"

AATS_AgentSpawner::AATS_AgentSpawner()
{
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
	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::Initialize() -- Initializing AgentSpawner..."));
	}

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

	SpawnAgents(true);

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
			SpawnAgents(false);
		}
	}	
}

void AATS_AgentSpawner::SpawnAgents(bool fillBox)
{
	// Check if TrafficManager is available
	if (m_pTrafficManager == nullptr)
	{
		return;
	}

	for (int i = 0; i < m_AgentCount; ++i)
	{
		// Place a random point within the zone shape
		FVector randomPoint = GetActorLocation();
		if(fillBox == true)
		{
			//Random points will be inside the bounding box of the actor
			randomPoint = GenerateRandomPointWithinBoundingBox();
		}
		else
		{
			//Random points on the outer edge of the bounding box of the actor
			randomPoint = GenerateRandomPointOnEdgeOfBoundingBox();
		}

		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::SpawnAgents() -- Random point: %s"), *randomPoint.ToString());
		}

		// Find the closest point on a lane
		FAgentPoint closestPointOnLane = m_pTrafficManager->GetClosestPointOnLane(FBox(randomPoint - FVector(1000), randomPoint + FVector(1000)), FZoneGraphTagFilter(), randomPoint);
		
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::SpawnAgents() -- Closest point on lane: %s"), *closestPointOnLane.position.ToString());
		}

		if (!IsLocationOccupied(closestPointOnLane.position))
		{
			if (bDebug)
			{
				UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::SpawnAgents() -- Spawning agent at: %s"), *closestPointOnLane.position.ToString());
			}

			bool bSuccess{ false };
			SpawnAgentAtLocation(closestPointOnLane.position, closestPointOnLane.direction, bSuccess);
			if (bSuccess)
			{
				m_SpawnedAgents++;
				UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::SpawnAgents() -- succesfully spawned agents: %d"), m_SpawnedAgents);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::SpawnAgents() -- failed to spawn agent"));
			}
		}
	}


	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::SpawnAgents() -- Spawned agents: %d"), m_SpawnedAgents);
	}
}

// Helper function to generate a random point within a 2D bounding box
FVector AATS_AgentSpawner::GenerateRandomPointWithinBoundingBox() const
{
	FSpawnBox spawnBox{ GetSpawnBox() };

	return spawnBox.GetRandomPoint();
}

FVector AATS_AgentSpawner::GenerateRandomPointOnEdgeOfBoundingBox() const
{
	FSpawnBox spawnBox{ GetSpawnBox() };

	return spawnBox.GetRandomPointOnEdge();
}

void AATS_AgentSpawner::RegisterAgentLoss(int count)
{
	m_SpawnedAgents -= count;
	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::RegisterAgentLoss() -- Lost agents: %d -> New agent count: %d"), count, m_SpawnedAgents);
	}
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
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::GetClosestLanePoint() -- TrafficManager is nullptr"));
		}
		Initialize();
	}

	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::GetClosestLanePoint() -- Getting closest lane point"));
	}

	safeLocation = m_pTrafficManager->GetClosestLanePoint(Location, searchDistance);
	return safeLocation;
}

bool AATS_AgentSpawner::IsLocationOccupied(const FVector& Location) const
{
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	FCollisionShape CollisionShape = FCollisionShape::MakeBox(FVector(50.f, 50.f, 50.f));

	if (bDebug)
	{
		DrawDebugBox(GetWorld(), Location, CollisionShape.GetExtent(), FColor::Red, false, 10.0f, 0, 1);
	}

	return false;
}

