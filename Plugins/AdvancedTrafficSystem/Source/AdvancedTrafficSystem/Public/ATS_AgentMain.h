// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ATS_AgentMain.generated.h"

class UBoxComponent;

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

	bool RetrieveBoxComponent();

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FVector GetOrigin() const;

	float GetHighDetailDistance() const { return m_HighDetailDistance; }
	float GetHighDetailSquaredDistance() const { return m_HighDetailDistanceSquared; }

	float GetLowDetailDistance()  const { return m_LowDetailDistance;  }
	float GetLowDetailSquaredDistance()  const { return m_LowDetailDistanceSquared;  }
	
	void RegisterAgentLoss();

	int GetAgentLoss() const { return m_AgentLoss; }
	void ResetAgentLoss() { m_AgentLoss = 0; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	bool bUseBoxXForDistance{ true };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	bool bUseBoxYForDistance{ false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	bool bUseBoxZForDistance{ false };

	FVector m_BoxExtentUsage{ 0.f, 0.f, 0.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	float m_HighDetailDistance{ 7500.f };
	float m_HighDetailDistanceSquared{ 0.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	float m_LowDetailDistance{ 15000.f };
	float m_LowDetailDistanceSquared{ 0.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	bool bDebug{ true };

	int m_AgentLoss{ 0 };

	FVector m_Origin{ FVector::ZeroVector };
	UBoxComponent* m_pBoxComponent{ nullptr };
};
