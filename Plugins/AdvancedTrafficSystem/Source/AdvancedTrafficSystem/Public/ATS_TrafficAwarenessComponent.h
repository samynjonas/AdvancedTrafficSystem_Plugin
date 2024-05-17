// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ATS_TrafficHelper.h"
#include "ATS_TrafficAwarenessComponent.generated.h"

/*
     When this component is attached to an actor, traffic will be aware of it.
*/

//  If it should interact with the lane or the agent
//	- Lane:  example. Open or close the lane - making agents stop
//	- Agent: example. Set speed limit for the agent

UENUM(BlueprintType)
enum class EATS_AwarenessType
{
	Lane,
	Agent
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ADVANCEDTRAFFICSYSTEM_API UATS_TrafficAwarenessComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UATS_TrafficAwarenessComponent();

protected:
	virtual void BeginPlay() override;
	bool Initialize();
	bool UpdateLocation();

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	void	SetDistanceAlongLane(float distanceAlongLane);
	void	SetLanePoint(FVector position);
	void	SetIsDirty(bool bIsDirty);

	UFUNCTION(BlueprintCallable, Category = "Traffic Awareness")
	void	SetCanAgentPass(bool bCanAgentPass);

	float	GetDistanceAlongLane() const { return _DistanceAlongLane; }
	FVector GetLanePoint() const { return _ConnectionPoint; }
	bool	GetIsDirty() const { return _bIsDirty; }

	UFUNCTION(BlueprintCallable, Category = "Traffic Awareness")
	bool CanAgentPass(bool respectOverride = true) const;

	virtual bool AdjustAgent(AActor* pAgent);

	FVector GetOffsetPosition() const;
	FVector GetOffset() const;

	bool HasDebuggingEnabled() const { return _bDebug; }

//---------------------------------------------------
// Code Variables
//---------------------------------------------------
protected:
	bool _bIsConnectedToLane{ false };
	bool _bCanAgentPass{ false };
	bool _bIsInitialized{ false };
	
	float   _DistanceAlongLane{ 0.0f };
	FVector _ConnectionPoint{ FVector::ZeroVector };
	FVector _LastLocation{ FVector::ZeroVector };

//---------------------------------------------------
// Settings
//---------------------------------------------------
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool _bIsMoveable{ false };
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", DisplayName = "Search to lane distance")
	float _SearchRadius{ -1.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	EATS_AwarenessType _AwarenessType{ EATS_AwarenessType::Lane };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool _bIsDirty{ false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool _bOverrideCanAgentPassGetter{ false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FTransform _TransformOffset{ FTransform::Identity };

//---------------------------------------------------
// Debugging
//---------------------------------------------------
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", DisplayName = "Debug to console")
	bool _bDebug{ false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", DisplayName = "Draw debug lines")
	bool _bDrawDebug{ false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (EditCondition = "_bDrawDebug", DisplayName = "Debug drawings color"))
	FColor _DebugColor{ FColor::Green };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (EditCondition = "_bDrawDebug", DisplayName = "Debug drawings time"))
	float _DebugDrawTime{ 0.0f };

};
