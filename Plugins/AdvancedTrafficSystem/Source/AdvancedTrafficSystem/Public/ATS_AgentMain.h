// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ATS_AgentMain.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ADVANCEDTRAFFICSYSTEM_API UATS_AgentMain : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UATS_AgentMain();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	float GetHighDetailDistance() const { return m_HighDetailDistance; }
	float GetHighDetailSquaredDistance() const { return m_HighDetailDistanceSquared; }

	float GetLowDetailDistance()  const { return m_LowDetailDistance;  }
	float GetLowDetailSquaredDistance()  const { return m_LowDetailDistanceSquared;  }
	
	void RegisterAgentLoss();

	int GetAgentLoss() const { return m_AgentLoss; }
	void ResetAgentLoss() { m_AgentLoss = 0; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	float m_HighDetailDistance{ 7500.f };
	float m_HighDetailDistanceSquared{ 0.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	float m_LowDetailDistance{ 15000.f };
	float m_LowDetailDistanceSquared{ 0.f };

	int m_AgentLoss{ 0 };

};
