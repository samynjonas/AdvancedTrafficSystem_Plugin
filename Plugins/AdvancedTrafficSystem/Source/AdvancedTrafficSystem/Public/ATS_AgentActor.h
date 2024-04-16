// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATS_AgentActor.generated.h"

class UATS_AgentMain;

UCLASS()
class ADVANCEDTRAFFICSYSTEM_API AATS_AgentActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AATS_AgentActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void EnableActor(AActor* actor, bool bEnable);
	void CompareAgentData(AActor* receiver, AActor* sender);

public:
	UFUNCTION(BlueprintCallable, Category = "Agent")
	void SwitchToActor(bool bPhysics);

	//Blueprint function to spawn actor
	UFUNCTION(BlueprintImplementableEvent, Category = "Agent")
	void SpawnActor();

	UFUNCTION(BlueprintCallable, Category = "Agent")
	void SetMainAgent(UATS_AgentMain* pAgentMain);

	void DistanceToMainAgentCheck();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;


protected:
	UPROPERTY(BlueprintReadWrite, Category = "Agent")
	AActor* m_PhysicsActor{ nullptr };

	UPROPERTY(BlueprintReadWrite, Category = "Agent")
	AActor* m_SimpleActor{ nullptr };

	bool bIsPhysicsActor{ false };

	UATS_AgentMain* m_AgentMain{ nullptr };
	
	UPROPERTY(EditAnywhere, Category = "Agent")
	float m_DistanceCheckInterval{ 2.f };

	float m_ElapsedTime{};
};
