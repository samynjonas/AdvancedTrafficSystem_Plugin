// Fill out your copyright notice in the Description page of Project Settings.


#include "../Public/ATS_ZoneShapeAgentContainer.h"

ATS_ZoneShapeAgentContainer::ATS_ZoneShapeAgentContainer()
{
	m_LaneColor = FColor::MakeRandomColor();
}

ATS_ZoneShapeAgentContainer::~ATS_ZoneShapeAgentContainer()
{
}

void ATS_ZoneShapeAgentContainer::RegisterAgent(UATS_AgentNavigation* agent)
{
	m_Agents.Add(agent);
}

void ATS_ZoneShapeAgentContainer::UnregisterAgent(UATS_AgentNavigation* agent)
{
	m_Agents.Remove(agent);
}