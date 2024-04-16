// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Public/ATS_BaseTrafficRuler.h"
#include "ATS_TrafficLight.generated.h"

//Unreal Engine Enum
UENUM(BlueprintType)
enum class ETrafficLightState : uint8
{
	Red,
	Green,
	Orange
};


UCLASS()
class ADVANCEDTRAFFICSYSTEM_API AATS_TrafficLight : public AATS_BaseTrafficRuler
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AATS_TrafficLight();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void Statehandler(float DeltaTime);


	//Define function in blueprint
	UFUNCTION(BlueprintImplementableEvent, Category = "TrafficLight")
	void ChangeColor();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	ETrafficLightState GetCurrentState() const { return m_CurrentState; }

	bool IsOpen() const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrafficLight")
	ETrafficLightState m_CurrentState{ ETrafficLightState::Red };
	

	const int LIGHT_COUNT{ 3 };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrafficLight|Lights")
	float m_RedTime{ 5.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrafficLight|Lights")
	float m_OrangeTime{ 1.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrafficLight|Lights")
	float m_GreenTime{ 5.f };

	//Map of all the lights
	TMap<ETrafficLightState, float> m_TimeForState;

	float m_CurrentTime{ 0.f };

};
