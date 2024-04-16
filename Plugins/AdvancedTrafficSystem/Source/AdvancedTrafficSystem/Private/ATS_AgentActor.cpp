// Fill out your copyright notice in the Description page of Project Settings.


#include "../Public/ATS_AgentActor.h"
#include "../Public/ATS_AgentNavigation.h"
#include "../Public/ATS_AgentMain.h"

// Sets default values
AATS_AgentActor::AATS_AgentActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AATS_AgentActor::BeginPlay()
{
	Super::BeginPlay();
	SpawnActor();
	SwitchToActor(false);
}

// Called every frame
void AATS_AgentActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	m_ElapsedTime += DeltaTime;
	if (m_ElapsedTime > m_DistanceCheckInterval)
	{
		UE_LOG(LogTemp, Warning, TEXT("AgentActor::Distance check"));
		m_ElapsedTime = 0.f;
		DistanceToMainAgentCheck();
	}
}

void AATS_AgentActor::SwitchToActor(bool bPhysics)
{
	if(bPhysics == bIsPhysicsActor)
	{
		return;
	}


	if (m_PhysicsActor == nullptr || m_SimpleActor == nullptr)
	{
		return;
	}

	EnableActor(m_PhysicsActor, bPhysics);
	EnableActor(m_SimpleActor, !bPhysics);

	if (bPhysics)
	{
		CompareAgentData(m_PhysicsActor, m_SimpleActor);
	}
	else
	{
		CompareAgentData(m_SimpleActor, m_PhysicsActor);
	}

	bIsPhysicsActor = bPhysics;
}

void AATS_AgentActor::EnableActor(AActor* actor, bool bEnable)
{
	actor->SetActorHiddenInGame(!bEnable);
	actor->SetActorTickEnabled(bEnable);
	actor->SetActorEnableCollision(bEnable);
}

void AATS_AgentActor::CompareAgentData(AActor* receiver, AActor* sender)
{
	UATS_AgentNavigation* receiverNavigation	= receiver->FindComponentByClass<UATS_AgentNavigation>();
	UATS_AgentNavigation* senderNavigation		= sender->FindComponentByClass<UATS_AgentNavigation>();

	if (receiverNavigation == nullptr || senderNavigation == nullptr)
	{
		return;
	}

	receiverNavigation->SetAgentData(senderNavigation->GetAgentData());

	receiverNavigation->EnableAgent();
	senderNavigation->DissableAgent();

	// Assuming `sender` is the actor you want to move
	FTransform SenderOriginalTransform = sender->GetActorTransform();

	FVector SenderLocation = SenderOriginalTransform.GetLocation();
	SenderLocation.Z = -1000.0f; // Move the actor 1000 units down on the Z-axis

	// Use SetActorLocation with the Teleport flag set to true
	sender->SetActorLocation(SenderLocation, false, nullptr, ETeleportType::ResetPhysics);

	// If you want to set the entire transform, including rotation and scale, use SetActorTransform
	receiver->SetActorTransform(SenderOriginalTransform, true, nullptr, ETeleportType::ResetPhysics);

}

void AATS_AgentActor::DistanceToMainAgentCheck()
{
	if (m_AgentMain == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("AgentActor::Main agent is null"));
		return;
	}

	if (m_PhysicsActor == nullptr || m_SimpleActor == nullptr)
	{
		return;
	}

	float distance{ 0.f };
	if (bIsPhysicsActor)
	{
		distance = m_PhysicsActor->GetSquaredDistanceTo(m_AgentMain->GetOwner());
	}
	else
	{
		distance = m_SimpleActor->GetSquaredDistanceTo(m_AgentMain->GetOwner());
	}
	UE_LOG(LogTemp, Warning, TEXT("AgentActor::Distance to main agent: %f"), distance);
	UE_LOG(LogTemp, Warning, TEXT("AgentActor::Distance to LowDetail Distance: %f"), m_AgentMain->GetLowDetailSquaredDistance());
	UE_LOG(LogTemp, Warning, TEXT("AgentActor::Distance to HighDetail Distance: %f"), m_AgentMain->GetHighDetailSquaredDistance());

	if (distance > m_AgentMain->GetLowDetailSquaredDistance())
	{
		//To far away -- nothing to show
		UE_LOG(LogTemp, Warning, TEXT("AgentActor::Too far away -- remove"));
		
		m_PhysicsActor->Destroy();
		m_SimpleActor->Destroy();
		Destroy();
	}
	else
	{
		//Show high detail
		SwitchToActor(true);
		UE_LOG(LogTemp, Warning, TEXT("AgentActor::Switch to high detail"));
	}
}

void AATS_AgentActor::SetMainAgent(UATS_AgentMain* pAgentMain)
{
	if (pAgentMain == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("AgentActor::Main agent is null"));
		return;
	}

	m_AgentMain = pAgentMain;
}
