// Fill out your copyright notice in the Description page of Project Settings.


#include "../Public/ATS_AgentLifeTime.h"
#include "../Public/ATS_AgentMain.h"


UATS_AgentLifeTime::UATS_AgentLifeTime()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UATS_AgentLifeTime::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void UATS_AgentLifeTime::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsDisabled)
	{
		return;
	}

	m_ElapsedTime += DeltaTime;
	if (m_ElapsedTime > m_DistanceCheckInterval)
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentLifeTime::TickComponent() -- Distance check"));
		}

		m_ElapsedTime = 0.f;
		DistanceToMainAgentCheck();
	}
}

void UATS_AgentLifeTime::DistanceToMainAgentCheck()
{
	if (m_AgentMain == nullptr)
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentLifeTime::DistanceToMainAgentCheck() -- Main agent is null"));
		}
		return;
	}

	float distance = GetOwner()->GetSquaredDistanceTo(m_AgentMain->GetOwner());
	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("AgentLifeTime::DistanceToMainAgentCheck() -- Distance to main agent: %f"), distance);
		UE_LOG(LogTemp, Warning, TEXT("AgentLifeTime::DistanceToMainAgentCheck() -- Distance to LowDetail Distance: %f"), m_AgentMain->GetLowDetailSquaredDistance());
	}

	if (distance > m_AgentMain->GetHighDetailSquaredDistance())
	{
		//To far away -- nothing to show
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentLifeTime::DistanceToMainAgentCheck() -- Too far away -- remove"));
		}

		GetOwner()->Destroy();
		m_AgentMain->RegisterAgentLoss();
	}
}

void UATS_AgentLifeTime::SetMainAgent(UATS_AgentMain* pAgentMain)
{
	if (pAgentMain == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("AgentLifeTime::SetMainAgent() -- Main agent is null"));
		return;
	}

	m_AgentMain = pAgentMain;
}
