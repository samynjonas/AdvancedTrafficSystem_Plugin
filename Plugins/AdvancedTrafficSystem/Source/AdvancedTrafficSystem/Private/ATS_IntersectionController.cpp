// Fill out your copyright notice in the Description page of Project Settings.
#include "../Public/ATS_IntersectionController.h"
#include "../Public/ATS_TrafficLightComponent.h"
#include "Components/SphereComponent.h"

AATS_IntersectionController::AATS_IntersectionController()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	_SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	RootComponent = _SphereComponent;
}

void AATS_IntersectionController::BeginPlay()
{
	Super::BeginPlay();
	
	_SphereComponent->bHiddenInGame = true;

	for (auto& sequence : _ArrSequences)
	{
		sequence.RetrieveComponentsFromActors();
	}

}

void AATS_IntersectionController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (_ArrSequences.IsEmpty())
	{
		return;
	}

	_ElapsedTime += DeltaTime;
	if (_ElapsedTime >= _TimeGoal)
	{
		ControlLights();
	}
}

void AATS_IntersectionController::ControlLights()
{
	//Loop over the sequences 
	// -- make the current sequence active
	// -- make the other sequences inactive

	if(_ArrSequences.IsEmpty())
	{
		return;
	}

	if (_ArrSequences.IsValidIndex(_CurrentSequenceIndex) == false)
	{
		_CurrentSequenceIndex = 0;
	}

	int32 nextSequenceIndex = _CurrentSequenceIndex + 1;
	if (nextSequenceIndex >= _ArrSequences.Num())
	{
		nextSequenceIndex = 0;
	}	

	FSequence& currentSequence	= _ArrSequences[_CurrentSequenceIndex];
	FSequence& nextSequence		= _ArrSequences[nextSequenceIndex];

	if (_ElapsedTime < currentSequence.duration)
	{
		for (UATS_TrafficLightComponent* trafficLight : currentSequence.trafficLights)
		{
			trafficLight->SetTrafficLightState(ETrafficLightState::ATS_TL_GO);
		}
		return;
	}

	for(UATS_TrafficLightComponent* trafficLight : currentSequence.trafficLights)
	{
		trafficLight->SetTrafficLightState(ETrafficLightState::ATS_TL_STOP);
	}

	for (UATS_TrafficLightComponent* trafficLight : nextSequence.trafficLights)
	{
		trafficLight->SetTrafficLightState(ETrafficLightState::ATS_TL_GO);
	}

	_TimeGoal = nextSequence.duration;
	_ElapsedTime = 0.f;
	_CurrentSequenceIndex++;
}


void FSequence::RetrieveComponentsFromActors()
{
	for (AActor* pActor : trafficLightsActors)
	{
		if (pActor == nullptr)
		{
			continue;
		}

		UATS_TrafficLightComponent* trafficLight = pActor->GetComponentByClass<UATS_TrafficLightComponent>();
		if (trafficLight)
		{
			trafficLight->SetControlled(true);
			trafficLight->SetTrafficLightState(ETrafficLightState::ATS_TL_STOP);

			trafficLights.Add(trafficLight);
		}
	}
}
