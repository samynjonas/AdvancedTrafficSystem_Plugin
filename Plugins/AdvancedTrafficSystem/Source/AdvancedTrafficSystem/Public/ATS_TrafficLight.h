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
	Orange,
	Red,
	Green
};

USTRUCT(BlueprintType)
struct FTrafficLightContainer
{
	GENERATED_BODY()

	FTrafficLightContainer() = default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrafficLight", DisplayName = "Contains this Traffic Light")
	bool bContainsThis{ false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrafficLight", DisplayName = "Traffic lights that will have the same state")
	TArray<AATS_TrafficLight*> _ArrCopyTrafficLights{};
};

UCLASS()
class ADVANCEDTRAFFICSYSTEM_API AATS_TrafficLight : public AATS_BaseTrafficRuler
{
	GENERATED_BODY()
	
public:	
	AATS_TrafficLight();

protected:
	virtual void BeginPlay() override;

	void Statehandler(float DeltaTime);

	//Define function in blueprint
	UFUNCTION(BlueprintImplementableEvent, Category = "TrafficLight")
	void ChangeColor();

	void InitializeTrafficLights();

public:	
	virtual void Tick(float DeltaTime) override;
	ETrafficLightState GetCurrentState() const { return _CurrentState; }

	void SetCurrentState(ETrafficLightState state, bool resetTime = true);
	bool IsOpen() const override;
	void MarkController(bool isController);
	
	void Initialize();
	bool IsInitialized() const { return _bIsInitialized; }

	void SetTimes(float redTime, float orangeTime, float greenTime, float startTime);

//---------------------------------------------------
// Code Variables
//---------------------------------------------------
protected:
	TMap<ETrafficLightState, float> _MapTimeForState{};
	float _CurrentTime{ 0.f };

	bool _bIsController{ true };
	bool _bIsInitialized{ false };

//---------------------------------------------------
// Settings
//---------------------------------------------------
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrafficLight")
	ETrafficLightState _CurrentState{ ETrafficLightState::Red };

	const int LIGHT_COUNT{ 3 };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrafficLight|Lights")
	float _RedTime{ 5.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrafficLight|Lights")
	float _OrangeTime{ 1.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrafficLight|Lights")
	float _GreenTime{ 5.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrafficLight", DisplayName = "Traffic Lights sequence")
	TArray<FTrafficLightContainer> _ArrTrafficLightsSequence{};

//---------------------------------------------------
// Debugging
//---------------------------------------------------
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrafficLight|Debug")
	bool _Debug{ false };

};
