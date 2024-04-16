// Fill out your copyright notice in the Description page of Project Settings.


#include "../Public/ATS_TrafficLight.h"

// Sets default values
AATS_TrafficLight::AATS_TrafficLight()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AATS_TrafficLight::BeginPlay()
{
	Super::BeginPlay();
	ChangeColor();

	//Filling TimeStateMap
	m_TimeForState.Add(ETrafficLightState::Red,		m_RedTime);
	m_TimeForState.Add(ETrafficLightState::Orange,	m_OrangeTime);
	m_TimeForState.Add(ETrafficLightState::Green,	m_GreenTime);
}

// Called every frame
void AATS_TrafficLight::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Statehandler(DeltaTime);
}

void AATS_TrafficLight::Statehandler(float DeltaTime)
{
	//Will switch states
	m_CurrentTime += DeltaTime;
	if(m_CurrentTime >= m_TimeForState[m_CurrentState])
	{
		m_CurrentTime = 0;
		m_CurrentState = (ETrafficLightState)(((int)m_CurrentState + 1) % LIGHT_COUNT);
		ChangeColor();
	}
}

bool AATS_TrafficLight::IsOpen() const
{
	return m_CurrentState == ETrafficLightState::Green;
}