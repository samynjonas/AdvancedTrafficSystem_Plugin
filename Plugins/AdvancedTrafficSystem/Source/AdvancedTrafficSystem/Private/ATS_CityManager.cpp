// Fill out your copyright notice in the Description page of Project Settings.


#include "../Public/ATS_CityManager.h"
#include "../Public/ATS_NavigationGoal.h"
#include "../Public/ATS_AgentSpawner.h"

#include "Kismet/GameplayStatics.h"

// Sets default values
AATS_CityManager::AATS_CityManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AATS_CityManager::BeginPlay()
{
	Super::BeginPlay();
	
	//Search for all NavigationGoals in the level
	//Sort houses - workplaces and parkings

	m_pAgentSpawner = Cast<AATS_AgentSpawner>(UGameplayStatics::GetActorOfClass(GetWorld(), AATS_AgentSpawner::StaticClass()));
	if (m_pAgentSpawner == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("ACityManager::BeginPlay() - m_pAgentSpawner is nullptr"));
		return;
	}

	TArray<AActor*> NavigationGoals;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AATS_NavigationGoal::StaticClass(), NavigationGoals);

	for (auto* NavigationGoal : NavigationGoals)
	{
		AATS_NavigationGoal* NavigationGoalCasted = Cast<AATS_NavigationGoal>(NavigationGoal);
		if (NavigationGoalCasted->GetNavGoalType() == ENavGoalType::Home)
		{
			//Calculate the amount of agent that will live in this house
			m_vecHouses.Add(NavigationGoalCasted);
		}
		else if (NavigationGoalCasted->GetNavGoalType() == ENavGoalType::Work)
		{
			NavigationGoalCasted->SetWorkHours(m_WorkTimeRange);
			m_vecWorkplaces.Add(NavigationGoalCasted);
		}
		else if (NavigationGoalCasted->GetNavGoalType() == ENavGoalType::Parking)
		{
			m_vecParkings.Add(NavigationGoalCasted);
		}
	}

	if (bIsDebugMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("ACityManager::CityAgentCount: %d"),	m_CityAgentCount);
		UE_LOG(LogTemp, Warning, TEXT("ACityManager::Houses: %d"),			m_vecHouses.Num());
		UE_LOG(LogTemp, Warning, TEXT("ACityManager::Workplaces: %d"),		m_vecWorkplaces.Num());
		UE_LOG(LogTemp, Warning, TEXT("ACityManager::Parkings: %d"),		m_vecParkings.Num());
	}

	for (auto* pHouse : m_vecHouses)
	{
		if (m_CityAgentCount >= m_MaxAgentCount)
		{
			break;
		}

		int pHouseAgentCount = GetRandomAmountOfAgents();

		m_CityAgentCount += pHouseAgentCount;
		pHouse->SetAgentCount(pHouseAgentCount);

		for (size_t i = 0; i < pHouseAgentCount; i++)
		{
			if (m_vecWorkplaces.IsEmpty())
			{
				break;
			}

			if (bUseNavigationGoals)
			{
				int32 randomWorkplaceIndex = FMath::RandRange(0, m_vecWorkplaces.Num() - 1);
				if (m_vecWorkplaces[randomWorkplaceIndex])
				{
					m_pAgentSpawner->SpawnAgentWithNavigationGoals(pHouse, m_vecWorkplaces[randomWorkplaceIndex]);
				}
			}
			else
			{
				m_pAgentSpawner->SpawnAgentAtHome(pHouse);
			}
			
		}
	}
}

// Called every frame
void AATS_CityManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

int AATS_CityManager::GetRandomAmountOfAgents() const
{
	return static_cast<int>(FMath::RandRange(m_MinMaxAgentPerHouse.X, m_MinMaxAgentPerHouse.Y));
}

