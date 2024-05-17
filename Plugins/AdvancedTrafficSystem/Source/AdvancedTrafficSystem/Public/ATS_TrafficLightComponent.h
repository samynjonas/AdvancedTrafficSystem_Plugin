// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ATS_TrafficAwarenessComponent.h"
#include "ATS_TrafficLightComponent.generated.h"

UENUM(BlueprintType)
enum class ETrafficLightState : uint8
{
	ATS_TL_STOP,
	ATS_TL_GO,
	ATS_TL_SLOWDOWN
};

USTRUCT(BlueprintType)
struct FTrafficLightStateStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic Light")
	ETrafficLightState State{ ETrafficLightState::ATS_TL_GO };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic Light")
	float Duration{ 1.f };
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTrafficLightStateChanged, ETrafficLightState, NewState);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ADVANCEDTRAFFICSYSTEM_API UATS_TrafficLightComponent : public UATS_TrafficAwarenessComponent
{
	GENERATED_BODY()
	
	UATS_TrafficLightComponent();

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Traffic Light")
	ETrafficLightState GetTrafficLightState() const { return _TrafficLightState; }

	UFUNCTION(BlueprintCallable, Category = "Traffic Light")
	void SetTrafficLightState(ETrafficLightState NewState);

	UFUNCTION(BlueprintCallable, Category = "Traffic Light")
	void SetControlled(bool bIsControlled) { _bIsControlled = bIsControlled; }

	UPROPERTY(BlueprintAssignable)
	FOnTrafficLightStateChanged TrafficLightChangedEvent;

protected:
	ETrafficLightState _TrafficLightState{ ETrafficLightState::ATS_TL_STOP };

	UPROPERTY(EditAnywhere, Category = "Traffic Light")
	TArray<FTrafficLightStateStruct> _TrafficLightStatesSequence{};

	float _ElapsedTime{ 0.f };
	int _CurrentStateIndex{ 0 };

	bool _bIsControlled{ false };
};
