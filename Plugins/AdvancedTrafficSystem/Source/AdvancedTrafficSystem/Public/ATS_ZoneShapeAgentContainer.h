// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class UATS_AgentNavigation;
class UATS_TrafficAwarenessComponent;

class ADVANCEDTRAFFICSYSTEM_API ATS_ZoneShapeAgentContainer
{
public:
	ATS_ZoneShapeAgentContainer();
	~ATS_ZoneShapeAgentContainer();

	bool RegisterAgent(UATS_AgentNavigation* agent);
	bool UnregisterAgent(UATS_AgentNavigation* agent);

	bool RegisterTrafficObject(UATS_TrafficAwarenessComponent* trafficObject);
	bool UnregisterTrafficObject(UATS_TrafficAwarenessComponent* trafficObject);

	int GetTrafficObjectsCount() const { return m_TrafficObjects.Num(); }
	TArray<UATS_TrafficAwarenessComponent*> GetTrafficObjects() const { return m_TrafficObjects; }

	int GetAgentsCount() const { return m_Agents.Num(); }
	TArray<UATS_AgentNavigation*> GetAgents() const { return m_Agents; }

	FColor GetLaneColor() const { return m_LaneColor; }

private:
	TArray<UATS_AgentNavigation*> m_Agents;
	TArray<UATS_TrafficAwarenessComponent*> m_TrafficObjects;

	//Debugging
	FColor m_LaneColor{};
};