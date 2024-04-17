// Fill out your copyright notice in the Description page of Project Settings.


#include "../Public/ATS_AgentMain.h"
#include "Components/BoxComponent.h"

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

bool UATS_AgentMain::RetrieveBoxComponent()
{
	m_pBoxComponent = GetOwner()->FindComponentByClass<UBoxComponent>();
	if (m_pBoxComponent == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("AgentMain::RetrieveBoxComponent() -- Box component not found"));
		return false;
	}

	//Print box information
	if (bDebug)
	{
		FVector origin = m_pBoxComponent->GetComponentLocation();
		FVector extent = m_pBoxComponent->GetScaledBoxExtent();
		UE_LOG(LogTemp, Warning, TEXT("AgentMain::RetrieveBoxComponent() -- Origin: %f, %f, %f"), origin.X, origin.Y, origin.Z);
		UE_LOG(LogTemp, Warning, TEXT("AgentMain::RetrieveBoxComponent() -- Extent: %f, %f, %f"), extent.X, extent.Y, extent.Z);
	}

	return true;
}

FVector UATS_AgentMain::GetOrigin() const
{
	if (m_pBoxComponent)
	{
		return m_pBoxComponent->GetComponentLocation();
	}
	return m_Origin;
}

// Called when the game starts
void UATS_AgentMain::BeginPlay()
{
	Super::BeginPlay();

	if (bUseBoxXForDistance || bUseBoxYForDistance || bUseBoxZForDistance)
	{
		FVector extents{ FVector::ZeroVector };

		if (RetrieveBoxComponent() && m_pBoxComponent)
		{
			extents		= m_pBoxComponent->GetScaledBoxExtent();
			m_Origin	= m_pBoxComponent->GetComponentLocation();
		}
		else
		{
			GetOwner()->GetActorBounds(false, m_Origin, extents, true);
		}


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

