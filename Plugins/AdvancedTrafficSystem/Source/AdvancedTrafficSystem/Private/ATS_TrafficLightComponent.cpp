// Fill out your copyright notice in the Description page of Project Settings.
#include "../Public/ATS_TrafficLightComponent.h"

UATS_TrafficLightComponent::UATS_TrafficLightComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UATS_TrafficLightComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (_TrafficLightStatesSequence.IsEmpty())
	{
		return;
	}

	if (_bIsControlled)
	{
		return;
	}

	_ElapsedTime += DeltaTime;
	if (_ElapsedTime >= _TrafficLightStatesSequence[_CurrentStateIndex].Duration)
	{
		_ElapsedTime = 0.f;
		_CurrentStateIndex = (_CurrentStateIndex + 1) % _TrafficLightStatesSequence.Num();
		_TrafficLightState = _TrafficLightStatesSequence[_CurrentStateIndex].State;

		if (_TrafficLightState == ETrafficLightState::ATS_TL_GO)
		{
			_bCanAgentPass = true;
		}
		else
		{
			_bCanAgentPass = false;
		}
		
		TrafficLightChangedEvent.Broadcast(_TrafficLightState);
	}
}

void UATS_TrafficLightComponent::SetTrafficLightState(ETrafficLightState NewState)
{
	_TrafficLightState = NewState;
	if (_TrafficLightState == ETrafficLightState::ATS_TL_GO)
	{
		_bCanAgentPass = true;
	}
	else
	{
		_bCanAgentPass = false;
	}

	TrafficLightChangedEvent.Broadcast(_TrafficLightState);
}