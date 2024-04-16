// Fill out your copyright notice in the Description page of Project Settings.
#include "../Public/ATS_CellularAutomataBehavior.h"

// Sets default values for this component's properties
UATS_CellularAutomataBehavior::UATS_CellularAutomataBehavior()
{
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UATS_CellularAutomataBehavior::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void UATS_CellularAutomataBehavior::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UATS_CellularAutomataBehavior::UpdateState(const TArray<UATS_CellularAutomataBehavior*>& Vehicles, int32 MyIndex)
{
    // Step 1: Acceleration
    if (CurrentSpeed < MaxSpeed)
    {
        CurrentSpeed++;
    }

    // Step 2: Slowing down due to other cars
    int32 DistanceToNextCar = 0;
    for (int32 i = MyIndex + 1; i < Vehicles.Num(); ++i)
    {
        if (Vehicles[i] != nullptr) // Assuming a nullptr indicates an empty cell
        {
            DistanceToNextCar = i - MyIndex - 1;
            break;
        }
    }

    if (DistanceToNextCar > 0 && DistanceToNextCar < CurrentSpeed)
    {
        CurrentSpeed = DistanceToNextCar;
    }

    // Step 3: Randomization
    if (FMath::FRand() < RandomizationProbability)
    {
        CurrentSpeed = FMath::Max(CurrentSpeed - 1, 0);
    }

    // Step 4: Car motion - this would actually be moving the car in the world
    // For a grid-based simulation, you'd just update the vehicle's position in the array.
    // For Unreal, you'd translate this to an actual movement in the game world.
    FVector NewLocation = GetOwner()->GetActorLocation() + FVector(CurrentSpeed * 100, 0, 0); // Assuming 100 units per cell
    GetOwner()->SetActorLocation(NewLocation);
}

