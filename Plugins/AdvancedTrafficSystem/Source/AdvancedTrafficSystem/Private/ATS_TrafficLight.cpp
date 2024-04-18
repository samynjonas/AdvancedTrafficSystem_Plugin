// Fill out your copyright notice in the Description page of Project Settings.
#include "../Public/ATS_TrafficLight.h"

AATS_TrafficLight::AATS_TrafficLight()
{
 	PrimaryActorTick.bCanEverTick = true;
}

void AATS_TrafficLight::BeginPlay()
{
	Super::BeginPlay();

	Initialize();
}

void AATS_TrafficLight::Initialize()
{
	if (_bIsInitialized)
	{
		return;
	}

	ChangeColor();

	//Filling TimeStateMap
	_MapTimeForState.Add(ETrafficLightState::Red,		_RedTime);
	_MapTimeForState.Add(ETrafficLightState::Orange,	_OrangeTime);
	_MapTimeForState.Add(ETrafficLightState::Green,		_GreenTime);

	//Filling the traffic lights sequence
	InitializeTrafficLights();
	_bIsInitialized = true;
}

void AATS_TrafficLight::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Statehandler(DeltaTime);
}

void AATS_TrafficLight::MarkController(bool isController)
{
	_bIsController = isController;
}

void AATS_TrafficLight::Statehandler(float DeltaTime)
{
	//Will switch states
	_CurrentTime += DeltaTime;
	if(_CurrentTime >= _MapTimeForState[_CurrentState])
	{
		_CurrentTime = 0;
		_CurrentState = (ETrafficLightState)(((int)_CurrentState + 1) % LIGHT_COUNT);
		ChangeColor();
	}
}

bool AATS_TrafficLight::IsOpen() const
{
	return _CurrentState == ETrafficLightState::Green;
}

void AATS_TrafficLight::SetCurrentState(ETrafficLightState state, bool resetTime)
{
	_CurrentState = state;
	if (resetTime)
	{
		_CurrentTime = 0;
	}
	ChangeColor();
}

void AATS_TrafficLight::SetTimes(float redTime, float orangeTime, float greenTime, float startTime)
{
	_CurrentTime = startTime;

	_RedTime	= redTime;
	_OrangeTime = orangeTime;
	_GreenTime	= greenTime;

	_MapTimeForState[ETrafficLightState::Red]		= redTime;
	_MapTimeForState[ETrafficLightState::Orange]	= orangeTime;
	_MapTimeForState[ETrafficLightState::Green]		= greenTime;
}

void AATS_TrafficLight::InitializeTrafficLights()
{
	if (_bIsController == false || _ArrTrafficLightsSequence.IsEmpty())
	{
		return;
	}

	bool bhasGreenBeenSet{ false };
	ETrafficLightState controllerState = _CurrentState;
	if(_CurrentState == ETrafficLightState::Green)
	{
		bhasGreenBeenSet = true;
	}
	
	float redBaseTime		= _RedTime;
	float redSequenceTime	= redBaseTime;
	float orangeBaseTime	= _OrangeTime;
	float greenBaseTime		= _GreenTime;

	//Add time to red time
	redSequenceTime = redBaseTime * (_ArrTrafficLightsSequence.Num() - 1) + orangeBaseTime;

	int sequenceCounter{ -1 };
	//Go over the traffic lights list and set them as none controller
	for (FTrafficLightContainer& trafficLightCopies : _ArrTrafficLightsSequence)
	{
		ETrafficLightState sequenceState = ETrafficLightState::Green;
		if (bhasGreenBeenSet == true)
		{
			sequenceCounter += 1;
			sequenceState = ETrafficLightState::Red;
		}
		else if (trafficLightCopies.bContainsThis && _CurrentState == ETrafficLightState::Red)
		{
			sequenceCounter += 1;
			sequenceState = ETrafficLightState::Red;
		}

		for (AATS_TrafficLight* trafficLight : trafficLightCopies._ArrCopyTrafficLights)
		{
			trafficLight->MarkController(false);

			if (trafficLight->IsInitialized() == false)
			{
				trafficLight->Initialize();
			}
			
			float passedTime{ redBaseTime * sequenceCounter };
			if (sequenceState == ETrafficLightState::Green)
			{
				passedTime = 0;
				bhasGreenBeenSet = true;
			}
			trafficLight->SetCurrentState(sequenceState);
			trafficLight->SetTimes(redSequenceTime, orangeBaseTime, greenBaseTime, passedTime);
		}
	}
}