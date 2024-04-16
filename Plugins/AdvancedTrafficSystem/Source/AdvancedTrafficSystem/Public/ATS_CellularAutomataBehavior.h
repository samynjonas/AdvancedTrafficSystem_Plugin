// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ATS_CellularAutomataBehavior.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ADVANCEDTRAFFICSYSTEM_API UATS_CellularAutomataBehavior : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UATS_CellularAutomataBehavior();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

    // The current speed of the vehicle
    int32 CurrentSpeed{ 0 };
    
    // The maximum speed of the vehicle
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic")
    int32 MaxSpeed{ 5 };

    // Randomization factor
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic")
    float RandomizationProbability;


public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Update the vehicle state
    void UpdateState(const TArray<UATS_CellularAutomataBehavior*>& Vehicles, int32 MyIndex);
    
		
};
