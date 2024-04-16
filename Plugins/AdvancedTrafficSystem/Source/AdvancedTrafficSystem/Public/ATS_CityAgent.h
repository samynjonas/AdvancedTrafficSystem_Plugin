// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Public/ATS_TrafficHelper.h"
#include "ATS_CityAgent.generated.h"

class AATS_NavigationGoal;
class UATS_AgentNavigation;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ADVANCEDTRAFFICSYSTEM_API UATS_CityAgent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UATS_CityAgent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void ManageAgent();


public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Agent")
	bool AssignHome(AATS_NavigationGoal* pHome);

	UFUNCTION(BlueprintCallable, Category = "Agent")
	bool AssignWork(AATS_NavigationGoal* pWork);

	UFUNCTION(BlueprintCallable, Category = "Agent")
	void SendToHome();

	UFUNCTION(BlueprintCallable, Category = "Agent")
	void SendToWork();

	UFUNCTION(BlueprintCallable, Category = "Agent")
	void VisualizeAgentGoals(bool isVisible);

protected:
	AATS_NavigationGoal* m_pHome{ nullptr };
	AATS_NavigationGoal* m_pWork{ nullptr };

	UATS_AgentNavigation* m_pNavigation{ nullptr };
	
	ENavGoalType m_CurrentAgentState{ ENavGoalType::Home };
	
	float m_ElapsedTime{}; //Temp for time management
	bool bIsDebugging{ false };


};
