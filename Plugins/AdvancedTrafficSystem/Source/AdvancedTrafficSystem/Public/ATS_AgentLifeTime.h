// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ATS_AgentLifeTime.generated.h"

class UATS_AgentMain;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ADVANCEDTRAFFICSYSTEM_API UATS_AgentLifeTime : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UATS_AgentLifeTime();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void DistanceToMainAgentCheck();

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Agent")
	void SetMainAgent(UATS_AgentMain* pAgentMain);

protected:
	UATS_AgentMain* m_AgentMain{ nullptr };

	UPROPERTY(EditAnywhere, Category = "Agent")
	float m_DistanceCheckInterval{ 2.f };

	float m_ElapsedTime{};

	UPROPERTY(EditAnywhere, Category = "Agent")
	bool bIsDisabled{ true };

	UPROPERTY(EditAnywhere, Category = "Agent")
	bool bDebug{ false };
};
