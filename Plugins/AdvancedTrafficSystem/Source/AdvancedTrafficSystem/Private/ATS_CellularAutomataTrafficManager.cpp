// Fill out your copyright notice in the Description page of Project Settings.


#include "../Public/ATS_CellularAutomataTrafficManager.h"

#include "../Public/ATS_CellularAutomataBehavior.h"

// Sets default values
AATS_CellularAutomataTrafficManager::AATS_CellularAutomataTrafficManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AATS_CellularAutomataTrafficManager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AATS_CellularAutomataTrafficManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    UpdateTraffic(DeltaTime);
}

void AATS_CellularAutomataTrafficManager::UpdateTraffic(float DeltaTime)
{
    // Update all vehicles
    for (int32 i = 0; i < m_VehicleGrid.Num(); ++i)
    {
        if (m_VehicleGrid[i] != nullptr)
        {
            m_VehicleGrid[i]->UpdateState(m_VehicleGrid, i);
        }
    }

    // Here you would handle wrapping or removal of vehicles at the end of the road
    // This depends on whether you're using open or periodic boundary conditions
}
