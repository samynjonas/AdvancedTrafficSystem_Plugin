// Fill out your copyright notice in the Description page of Project Settings.


#include "../Public/ATS_CityAgent.h"
#include "../Public/ATS_AgentNavigation.h"
#include "../Public/ATS_NavigationGoal.h"

// Sets default values for this component's properties
UATS_CityAgent::UATS_CityAgent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UATS_CityAgent::BeginPlay()
{
	Super::BeginPlay();

	m_pNavigation = GetOwner()->FindComponentByClass<UATS_AgentNavigation>();
	if (m_pNavigation == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("UCityAgent::BeginPlay() - m_pNavigation is nullptr"));
	}

	m_ElapsedTime = 7.9f * 60.f;
}


// Called every frame
void UATS_CityAgent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	m_ElapsedTime += DeltaTime;
	if (m_ElapsedTime / 60.f > 24.f)
	{
		m_ElapsedTime = 0.f;
	}

	ManageAgent();
}

bool UATS_CityAgent::AssignHome(AATS_NavigationGoal* pHome)
{
	m_pHome = pHome;
	return true;
}

bool UATS_CityAgent::AssignWork(AATS_NavigationGoal* pWork)
{
	if (pWork == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("UCityAgent::AssignWork() - pWork is nullptr"));
		return false;
	}

	m_pWork = pWork;
	return true;
}

void UATS_CityAgent::SendToHome()
{
	if (m_pNavigation == nullptr)
	{
		return;
	}

	if (m_pHome)
	{
		if (bIsDebugging)
		{
			UE_LOG(LogTemp, Warning, TEXT("SendToHome"));
		}
		m_pNavigation->SetNavGoal(m_pHome->GetActorLocation());
	}
	else
	{
		m_pNavigation->EnableAgent();
		m_pNavigation->SetFollowPath(false);
	}
}
void UATS_CityAgent::SendToWork()
{
	if (m_pNavigation == nullptr)
	{
		return;
	}

	if (m_pWork)
	{
		if (bIsDebugging)
		{
			UE_LOG(LogTemp, Warning, TEXT("SendToWork"));
		}
		m_pNavigation->SetNavGoal(m_pWork->GetActorLocation());
	}
	else
	{
		m_pNavigation->EnableAgent();
		m_pNavigation->SetFollowPath(false);
	}
}

void UATS_CityAgent::ManageAgent()
{
	if (m_pNavigation == nullptr)
	{
		if (bIsDebugging)
		{
			UE_LOG(LogTemp, Error, TEXT("UCityAgent::ManageAgent() - m_pNavigation is nullptr"));
		}
		return;
	}

	if (m_pWork == nullptr)
	{
		m_pNavigation->EnableAgent();
		m_pNavigation->SetFollowPath(false);

		if (bIsDebugging)
		{
			UE_LOG(LogTemp, Error, TEXT("UCityAgent::ManageAgent() - m_pWork is nullptr"));
		}
		return;
	}

	float timeConversion = m_ElapsedTime / 60.f;
	if (m_CurrentAgentState == ENavGoalType::Home)
	{
		//Check if you need to go to work
		if (timeConversion > m_pWork->GetWorkHours().X && timeConversion < m_pWork->GetWorkHours().Y)
		{
			SendToWork();
			m_CurrentAgentState = ENavGoalType::Work;
		}
	}
	else if (m_CurrentAgentState == ENavGoalType::Work)
	{
		//Check if you need to go home
		if (timeConversion > m_pWork->GetWorkHours().Y)
		{
			SendToHome();
			m_CurrentAgentState = ENavGoalType::Home;
		}
	}
}

void UATS_CityAgent::VisualizeAgentGoals(bool isVisible)
{
	if (m_pHome)
	{
		m_pHome->SetActorHiddenInGame(isVisible);
	}

	if (m_pWork)
	{
		m_pWork->SetActorHiddenInGame(isVisible);
	}
}

