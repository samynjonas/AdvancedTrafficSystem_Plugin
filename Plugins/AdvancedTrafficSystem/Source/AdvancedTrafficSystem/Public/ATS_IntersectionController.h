// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATS_IntersectionController.generated.h"

class UATS_TrafficLightComponent;
class USphereComponent;

USTRUCT()
struct FSequence
{
	GENERATED_BODY()

	FSequence() {};

	void RetrieveComponentsFromActors();

	UPROPERTY(EditAnywhere)
	TArray<AActor*> trafficLightsActors{};

	UPROPERTY(EditAnywhere)
	float duration{ 5.f };

	TArray<UATS_TrafficLightComponent*> trafficLights{};
};


UCLASS()
class ADVANCEDTRAFFICSYSTEM_API AATS_IntersectionController : public AActor
{
	GENERATED_BODY()
	
/*
	--------------------------------------
		UNREAL ENGINE FUNCTIONS
	--------------------------------------
*/
public:	
	AATS_IntersectionController();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

/*
	--------------------------------------
		UNREAL ENGINE FUNCTIONS
	--------------------------------------
*/
public:


protected:
	void ControlLights();

/*
	--------------------------------------
		CODE VARIABLES
	--------------------------------------
*/

// CODE
protected:
	USphereComponent* _SphereComponent{ nullptr };
	
	int32 _CurrentSequenceIndex{ 0 };
	float _ElapsedTime{ 0.f };
	float _TimeGoal{};

// SETTINGS
protected:
	UPROPERTY(EditAnywhere, Category = "Settings")
	TArray<FSequence> _ArrSequences{};

	UPROPERTY(EditAnywhere, Category = "Settings")
	bool _bDebug{ false };
	

};
