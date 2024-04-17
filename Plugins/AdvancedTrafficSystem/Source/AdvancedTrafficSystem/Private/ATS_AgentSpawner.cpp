// Fill out your copyright notice in the Description page of Project Settings.


#include "../Public/ATS_AgentSpawner.h"
#include "../Public/ATS_TrafficManager.h"
#include "../Public/ATS_AgentMain.h"
#include "../Public/ATS_AgentLifeTime.h"
#include "../Public/ATS_CityAgent.h"
#include "../Public/ATS_AgentNavigation.h"
#include "../Public/ATS_NavigationGoal.h"

#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "ZoneShapeComponent.h"

#include "Components/BoxComponent.h"
#include "Components/BillboardComponent.h"

#include "DrawDebugHelpers.h"

AATS_AgentSpawner::AATS_AgentSpawner()
{
	PrimaryActorTick.bCanEverTick = true;

	_BillBoardComponent = CreateDefaultSubobject<UBillboardComponent>( TEXT("Billboard") );
	_BillBoardComponent->SetupAttachment(RootComponent);

	_BoxComponent = CreateDefaultSubobject<UBoxComponent>(  TEXT("SpawnRangeBox") );
	_BoxComponent->SetupAttachment(_BillBoardComponent);
	_BoxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	_AgentMain		= CreateDefaultSubobject<UATS_AgentMain>( TEXT("ATS_AgentMain") );
}

// Called when the game starts or when spawned
void AATS_AgentSpawner::BeginPlay()
{
	Super::BeginPlay();

	AttachSpawnerToActor();
	Initialize();
}

void AATS_AgentSpawner::AttachSpawnerToActor()
{
	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::AttachSpawnerToActor() -- Attaching spawner to actor..."));
	}

	if (bAttachToActor == false)
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::AttachSpawnerToActor() -- Not attaching to an actor"));
		}
		return;
	}

	if (_pAttachToActor == nullptr)
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::AttachSpawnerToActor() -- Can't attach to NULL actor"));
		}
		return;
	}

	FAttachmentTransformRules attachRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false);
	
	if (bAttachToActorComponent && _pComponentNameToAttach.IsEmpty() == false)
	{		
		USceneComponent* pComponentToAttachTo = FindObject<USceneComponent>(_pAttachToActor, *_pComponentNameToAttach);
		if (pComponentToAttachTo)
		{
			if (AttachToComponent(pComponentToAttachTo, attachRules))
			{
				if (bDebug)
				{
					UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::AttachSpawnerToActor() -- Attached to component: %s"), *_pComponentNameToAttach);
				}
				return;
			}
		}
	}

	if (AttachToActor(_pAttachToActor, attachRules))
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::AttachSpawnerToActor() -- Attached to actor: %s"), *_pAttachToActor->GetName());
		}
		return;
	}

	//Everything failed
	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::AttachSpawnerToActor() -- Failed to attach to actor: %s"), *_pAttachToActor->GetName());
	}
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

	if (_AgentMain == nullptr)
	{
		_AgentMain = GetComponentByClass<UATS_AgentMain>();
	}

	bIsInitialized = true;
}

FSpawnBox AATS_AgentSpawner::GetSpawnBox() const
{
	FSpawnBox spawnBox{};

	if (_BoxComponent)
	{
		spawnBox.origin = _BoxComponent->GetComponentLocation();
		spawnBox.extent = _BoxComponent->GetScaledBoxExtent();
		return spawnBox;
	}

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

			if (SpawnAgentAtLocation(closestPointOnLane.position, closestPointOnLane.direction))
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
	if (_AgentMain)
	{
		RegisterAgentLoss(_AgentMain->GetAgentLoss());
		_AgentMain->ResetAgentLoss();
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

bool AATS_AgentSpawner::SpawnAgentAtLocation(FVector position, FVector direction)
{
	//Pick a random actor from the array of agent classes
	if (_ArrAgentClasses.Num() == 0)
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::SpawnAgentAtLocation() -- No agent classes available"));
		}
		return false;
	}

	int randomIndex = FMath::RandRange(0, _ArrAgentClasses.Num() - 1);
	TSubclassOf<AActor> agentClass = _ArrAgentClasses[randomIndex];

	if (agentClass == nullptr)
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::SpawnAgentAtLocation() -- Agent class is nullptr"));
		}
		return false;
	}

	//Spawn the agent
	FActorSpawnParameters spawnParams;
	spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;

	AActor* pAgent = GetWorld()->SpawnActor<AActor>(agentClass, position, direction.Rotation(), spawnParams);
	if (pAgent == nullptr)
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::SpawnAgentAtLocation() -- Failed to spawn agent"));
		}
		return false;
	}
	return ValidateSpawnedAgent(pAgent);
}

bool AATS_AgentSpawner::ValidateSpawnedAgent(AActor* pAgent)
{
	//Check if the agent has AgentLife Component
	UATS_AgentLifeTime* pAgentLife = pAgent->FindComponentByClass<UATS_AgentLifeTime>();
	if (pAgentLife == nullptr)
	{
		//Add it to the agent
		pAgentLife = NewObject<UATS_AgentLifeTime>(pAgent);
		pAgentLife->RegisterComponent();
	}

	if (pAgentLife)
	{
		pAgentLife->SetMainAgent(_AgentMain);
	}
	else
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::SpawnAgentAtLocation() -- Failed to add AgentLifeTime component"))
		}
		pAgent->Destroy();
		return false;
	}


	//Check if it contains CityAgent Component
	UATS_CityAgent* pCityAgent = pAgent->FindComponentByClass<UATS_CityAgent>();
	if (pCityAgent == nullptr)
	{
		//Add it to the agent
		pCityAgent = NewObject<UATS_CityAgent>(pAgent);
		pCityAgent->RegisterComponent();
	}

	if (pCityAgent == nullptr)
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::SpawnAgentAtLocation() -- Failed to add CityAgent component"));
		}

		pAgent->Destroy();
		return false;
	}


	//Check if it contains AgentNavigation Component
	UATS_AgentNavigation* pAgentNavigation = pAgent->FindComponentByClass<UATS_AgentNavigation>();
	if (pAgentNavigation == nullptr)
	{
		//Add it to the agent
		pAgentNavigation = NewObject<UATS_AgentNavigation>(pAgent);
		pAgentNavigation->RegisterComponent();
	}

	if (pAgentNavigation == nullptr)
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::SpawnAgentAtLocation() -- Failed to add AgentNavigation component"));
		}

		pAgent->Destroy();
		return false;
	}

	return true;
}

bool AATS_AgentSpawner::SpawnAgentWithNavigationGoals(AATS_NavigationGoal* pHome, AATS_NavigationGoal* pWork)
{
	if (pHome == nullptr || pWork == nullptr)
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::SpawnAgentWithNavigationGoals_impl() -- Navigation goals are nullptr"));
		}
		return false;
	}

	FVector homeLocation = pHome->GetActorLocation();
	FTransform homeTransform = GetClosestLanePoint(homeLocation, 1000.f);

	//Pick a random actor from the array of agent classes
	if (_ArrAgentClasses.Num() == 0)
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::SpawnAgentAtLocation() -- No agent classes available"));
		}
		return false;
	}

	int randomIndex = FMath::RandRange(0, _ArrAgentClasses.Num() - 1);
	TSubclassOf<AActor> agentClass = _ArrAgentClasses[randomIndex];

	if (agentClass == nullptr)
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::SpawnAgentAtLocation() -- Agent class is nullptr"));
		}
		return false;
	}

	//Spawn the agent
	FActorSpawnParameters spawnParams;
	spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;

	AActor* pAgent = GetWorld()->SpawnActor<AActor>(agentClass, homeTransform, spawnParams);
	if (pAgent == nullptr)
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::SpawnAgentAtLocation() -- Failed to spawn agent"));
		}
		return false;
	}
	
	if (ValidateSpawnedAgent(pAgent) == false)
	{
		return false;
	}

	//Set the navigation goals
	UATS_CityAgent* pCityAgent = pAgent->FindComponentByClass<UATS_CityAgent>();
	if (pCityAgent == nullptr )
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::SpawnAgentWithNavigationGoals_impl() -- Failed to set navigation goals"));
		}
		return false;
	}

	pCityAgent->AssignHome(pHome);
	pCityAgent->AssignWork(pWork);
	
	return true;
}

bool AATS_AgentSpawner::SpawnAgentAtHome(AATS_NavigationGoal* pHome)
{
	if (pHome == nullptr)
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::SpawnAgentWithNavigationGoals_impl() -- Navigation goals are nullptr"));
		}
		return false;
	}

	FVector homeLocation = pHome->GetActorLocation();
	FTransform homeTransform = GetClosestLanePoint(homeLocation, 1000.f);

	//Pick a random actor from the array of agent classes
	if (_ArrAgentClasses.Num() == 0)
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::SpawnAgentAtLocation() -- No agent classes available"));
		}
		return false;
	}

	int randomIndex = FMath::RandRange(0, _ArrAgentClasses.Num() - 1);
	TSubclassOf<AActor> agentClass = _ArrAgentClasses[randomIndex];

	if (agentClass == nullptr)
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::SpawnAgentAtLocation() -- Agent class is nullptr"));
		}
		return false;
	}

	//Spawn the agent
	FActorSpawnParameters spawnParams;
	spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;

	AActor* pAgent = GetWorld()->SpawnActor<AActor>(agentClass, homeTransform, spawnParams);
	if (pAgent == nullptr)
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentSpawner::SpawnAgentAtLocation() -- Failed to spawn agent"));
		}
		return false;
	}

	return ValidateSpawnedAgent(pAgent);
}