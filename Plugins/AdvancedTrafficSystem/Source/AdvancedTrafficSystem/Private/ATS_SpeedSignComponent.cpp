// Fill out your copyright notice in the Description page of Project Settings.


#include "../Public/ATS_SpeedSignComponent.h"
#include "../Public/ATS_AgentNavigation.h"

bool UATS_SpeedSignComponent::AdjustAgent(AActor* pAgent)
{
	if (!pAgent)
	{
		return false;
	}

	//Get the agent navigation component
	UATS_AgentNavigation* pAgentNavigation = pAgent->FindComponentByClass<UATS_AgentNavigation>();
	if (pAgentNavigation == nullptr)
	{
		return false;
	}

	if (pAgentNavigation->GetMaxSpeed() == _SpeedLimit)
	{
		return false;
	}

	//Set the speed limit
	pAgentNavigation->SetMaxSpeed(_SpeedLimit);
	if (_bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpeedSignComponent::AdjustAgent() -- Speed limit set to: %f"), _SpeedLimit);
	}

	return true;
}