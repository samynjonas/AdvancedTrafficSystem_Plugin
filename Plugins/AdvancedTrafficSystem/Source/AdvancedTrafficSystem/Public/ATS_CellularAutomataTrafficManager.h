// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATS_CellularAutomataTrafficManager.generated.h"

class UATS_CellularAutomataBehavior;

UCLASS()
class ADVANCEDTRAFFICSYSTEM_API AATS_CellularAutomataTrafficManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AATS_CellularAutomataTrafficManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// An array representing the grid of vehicles
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic")
	TArray<UATS_CellularAutomataBehavior*> m_VehicleGrid;

	// Update the state of all vehicles
	void UpdateTraffic(float DeltaTime);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
