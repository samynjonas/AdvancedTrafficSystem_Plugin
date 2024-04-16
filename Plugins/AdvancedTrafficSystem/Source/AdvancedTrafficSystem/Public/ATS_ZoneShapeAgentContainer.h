// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class UATS_AgentNavigation;

class ADVANCEDTRAFFICSYSTEM_API ATS_ZoneShapeAgentContainer
{
public:
	ATS_ZoneShapeAgentContainer();
	~ATS_ZoneShapeAgentContainer();

	void RegisterAgent(UATS_AgentNavigation* agent);
	void UnregisterAgent(UATS_AgentNavigation* agent);

	int GetAgentsCount() const { return m_Agents.Num(); }
	TArray<UATS_AgentNavigation*> GetAgents() const { return m_Agents; }

	FColor GetLaneColor() const { return m_LaneColor; }

private:
	TArray<UATS_AgentNavigation*> m_Agents;


	//Debugging
	FColor m_LaneColor{};
};