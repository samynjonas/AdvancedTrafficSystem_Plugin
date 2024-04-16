// Fill out your copyright notice in the Description page of Project Settings.


#include "../Public/ATS_AgentMain.h"

UATS_AgentMain::UATS_AgentMain()
{
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UATS_AgentMain::BeginPlay()
{
	Super::BeginPlay();

	m_HighDetailDistanceSquared = m_HighDetailDistance * m_HighDetailDistance;
	m_LowDetailDistanceSquared	= m_LowDetailDistance  * m_LowDetailDistance;	
}


// Called every frame
void UATS_AgentMain::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UATS_AgentMain::RegisterAgentLoss()
{
	++m_AgentLoss;
}

