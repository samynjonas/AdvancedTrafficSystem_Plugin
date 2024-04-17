// Fill out your copyright notice in the Description page of Project Settings.


#include "../Public/ATS_AgentMain.h"

UATS_AgentMain::UATS_AgentMain()
{
	PrimaryComponentTick.bCanEverTick = true;

	if (bUseBoxXForDistance)
	{
		bUseBoxYForDistance = false;
		bUseBoxZForDistance = false;
	}
	else if (bUseBoxYForDistance)
	{
		bUseBoxXForDistance = false;
		bUseBoxZForDistance = false;
	}
	else if (bUseBoxZForDistance)
	{
		bUseBoxXForDistance = false;
		bUseBoxYForDistance = false;
	}
}


// Called when the game starts
void UATS_AgentMain::BeginPlay()
{
	Super::BeginPlay();

	if (bUseBoxXForDistance || bUseBoxYForDistance || bUseBoxZForDistance)
	{
		FVector origin{ FVector::ZeroVector };
		FVector extents{ FVector::ZeroVector };

		GetOwner()->GetActorBounds(false, origin, extents, true);

		if (bUseBoxXForDistance)
		{
			m_BoxExtentUsage = FVector(1.0f, 0.0f, 0.0f);
		}
		else if (bUseBoxYForDistance)
		{
			m_BoxExtentUsage = FVector(0.0f, 1.0f, 0.0f);
		}
		else if (bUseBoxZForDistance)
		{
			m_BoxExtentUsage = FVector(0.0f, 0.0f, 1.0f);
		}

		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentMain::BeginPlay() -- Extents: %f, %f, %f"), extents.X, extents.Y, extents.Z);
		}

		m_HighDetailDistance	= (extents.X * m_BoxExtentUsage.X) + (extents.Y * m_BoxExtentUsage.Y) + (extents.Z * m_BoxExtentUsage.Z);
		if (bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentMain::BeginPlay() -- High Detail Distance: %f"), m_HighDetailDistance);
		}
		
		m_LowDetailDistance		= m_HighDetailDistance * 2.0f;
		if(bDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("AgentMain::BeginPlay() -- Low Detail Distance: %f"), m_LowDetailDistance);
		}
	}

	m_HighDetailDistanceSquared = m_HighDetailDistance * m_HighDetailDistance;
	m_LowDetailDistanceSquared	= m_LowDetailDistance  * m_LowDetailDistance;	
}


// Called every frame
void UATS_AgentMain::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UATS_AgentMain::RegisterAgentLoss()
{
	++m_AgentLoss;
}

