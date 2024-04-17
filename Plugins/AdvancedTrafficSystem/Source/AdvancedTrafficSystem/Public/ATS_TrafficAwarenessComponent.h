// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ATS_TrafficAwarenessComponent.generated.h"

/*
     When this component is attached to an actor, traffic will be aware of it.
*/

class AATS_TrafficManager;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ADVANCEDTRAFFICSYSTEM_API UATS_TrafficAwarenessComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UATS_TrafficAwarenessComponent();

protected:
	virtual void BeginPlay() override;
	bool Initialize();

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	

//---------------------------------------------------
// Code Variables
//---------------------------------------------------
protected:
	AATS_TrafficManager* _pTrafficManager{ nullptr };

	bool _bIsConnectedToLane{ false };
	FVector _ConnectionPoint{ FVector::ZeroVector };

//---------------------------------------------------
// Settings
//---------------------------------------------------
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool _bIsMoveable{ false };

//---------------------------------------------------
// Debugging
//---------------------------------------------------
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool _bDebug{ false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool _bDrawDebug{ false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (EditCondition = "_bDrawDebug", DisplayName = "Debug drawings color"))
	FColor _DebugColor{ FColor::Green };

};
