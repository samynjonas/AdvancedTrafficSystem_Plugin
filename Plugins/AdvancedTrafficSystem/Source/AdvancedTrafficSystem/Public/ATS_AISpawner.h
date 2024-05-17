// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATS_TrafficHelper.h"
#include "ATS_AISpawner.generated.h"

class USphereComponent;
class AATS_NavigationManager;

USTRUCT()
struct FSpawnableAI
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> AIActorClass{ nullptr };

	UPROPERTY(EditAnywhere)
	TArray<ELaneType> lanes{  };
};

UCLASS()
class ADVANCEDTRAFFICSYSTEM_API AATS_AISpawner : public AActor
{
	GENERATED_BODY()
	
/*
	--------------------------------------
		UNREAL ENGINE FUNCTIONS
	--------------------------------------
*/
public:	
	AATS_AISpawner();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	
/*
	--------------------------------------
		CUSTOM FUNCTIONS
	--------------------------------------
*/
public:	


protected:
	bool Initialize();
	bool UpdateAI();

	bool ShouldLive(AActor* AIagent);
	bool SpawnAI();
	bool CleanupInvalid();

	FVector RandomPointInSphere() const;
	FVector RandomPointInSphere(float innerRadius) const;

/*
	--------------------------------------
		CODE VARIABLES
	--------------------------------------
*/

// CODE
protected:
	TArray<AActor*> _SpawnedAI;

	AATS_NavigationManager* _pNavigationManager{ nullptr };

	float _TimeSinceLastUpdate{ 0.0f };
	bool _bIsInitialized{ false };

// SETTINGS
protected:
	UPROPERTY(EditAnywhere, Category = "Components")
	USphereComponent* _pSphereComponent{ nullptr };
	
	UPROPERTY(EditAnywhere, Category = "Settings")
	uint32 _MaxAICount{ 10 };

	UPROPERTY(EditAnywhere, Category = "Settings")
	float _UpdateRate{ 1.0f };

	UPROPERTY(EditAnywhere, Category = "Settings")
	TArray<FSpawnableAI> _AIActorClasses{};

	UPROPERTY(EditAnywhere, Category = "Settings")
	bool _bAttachToAnActor{ false };

	UPROPERTY(EditAnywhere, Category = "Settings")
	bool _bDebug{ false };

	UPROPERTY(EditAnywhere, Category = "Settings")
	AActor* _pActorToAttachTo{ nullptr };
};
