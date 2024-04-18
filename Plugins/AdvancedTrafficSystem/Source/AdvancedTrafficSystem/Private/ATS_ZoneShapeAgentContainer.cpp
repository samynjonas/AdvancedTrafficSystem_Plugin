// Fill out your copyright notice in the Description page of Project Settings.


#include "../Public/ATS_ZoneShapeAgentContainer.h"

ATS_ZoneShapeAgentContainer::ATS_ZoneShapeAgentContainer()
{
	m_LaneColor = FColor::MakeRandomColor();
}

ATS_ZoneShapeAgentContainer::~ATS_ZoneShapeAgentContainer()
{
	m_Agents.Empty();
	m_TrafficObjects.Empty();
}

bool ATS_ZoneShapeAgentContainer::RegisterAgent(UATS_AgentNavigation* pAgent)
{
	if (pAgent == nullptr || m_Agents.Contains(pAgent))
	{
		return false;
	}
	m_Agents.Add(pAgent);
	return true;
}

bool ATS_ZoneShapeAgentContainer::UnregisterAgent(UATS_AgentNavigation* pAgent)
{
	if (pAgent == nullptr || !m_Agents.Contains(pAgent))
	{
		return false;
	}
	m_Agents.Remove(pAgent);
	return true;
}

bool ATS_ZoneShapeAgentContainer::RegisterTrafficObject(UATS_TrafficAwarenessComponent* pTrafficObject)
{
	if (pTrafficObject == nullptr || m_TrafficObjects.Contains(pTrafficObject))
	{
		return false;
	}
	m_TrafficObjects.Add(pTrafficObject);
	return true;
}

bool ATS_ZoneShapeAgentContainer::UnregisterTrafficObject(UATS_TrafficAwarenessComponent* pTrafficObject)
{
	if (pTrafficObject == nullptr || !m_TrafficObjects.Contains(pTrafficObject))
	{
		return false;
	}
	m_TrafficObjects.Remove(pTrafficObject);
	return true;
}