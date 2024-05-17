// Fill out your copyright notice in the Description page of Project Settings.
#include "../Public/ATS_AISpawner.h"
#include "../Public/ATS_NavigationManager.h"

#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"


AATS_AISpawner::AATS_AISpawner()
{
	PrimaryActorTick.bCanEverTick = true;

	_pSphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SpawnBounds"));

	_pSphereComponent->SetCollisionObjectType(ECC_WorldDynamic);
	_pSphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	_pSphereComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AATS_AISpawner::BeginPlay()
{
	Super::BeginPlay();
	Initialize();
}

void AATS_AISpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	_TimeSinceLastUpdate += DeltaTime;
	if (_TimeSinceLastUpdate >= _UpdateRate)
	{
		_TimeSinceLastUpdate = 0.0f;
		UpdateAI();
	}
}

bool AATS_AISpawner::Initialize()
{
	if (_bIsInitialized)
	{
		return true;
	}

	if (!_pSphereComponent)
	{
		if (_bDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("Sphere component is not set!"));
		}
		return false;
	}

	// Find the navigation manager
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AATS_NavigationManager::StaticClass(), FoundActors);
	for (AActor* pActor : FoundActors)
	{
		_pNavigationManager = Cast<AATS_NavigationManager>(pActor);
		if (_pNavigationManager)
		{
			break;
		}
	}

	if (_pNavigationManager == nullptr)
	{
		_pNavigationManager = GetWorld()->SpawnActor<AATS_NavigationManager>();
		if (_pNavigationManager == nullptr)
		{
			if (_bDebug)
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to receive/spawn Navigation Manager"));
			}
			return false;
		}
	}
	_pNavigationManager->Initialize();

	
	if (_bAttachToAnActor && _pActorToAttachTo)
	{
		AttachToActor(_pActorToAttachTo, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}

	_bIsInitialized = true;
	return true;
}

bool AATS_AISpawner::UpdateAI()
{
	if (Initialize() == false)
	{
		return false;
	}

	// Check if we have reached the max AI count
	if (static_cast<uint32>(_SpawnedAI.Num()) < _MaxAICount)
	{
		// We can spawn new AI
		SpawnAI();
	}

	// Loop through all the spawned AI and check if they should live
	for (auto AI : _SpawnedAI)
	{
		if (ShouldLive(AI) == false)
		{
			AI->Destroy();
		}
	}

	CleanupInvalid();

	return true;
}

bool AATS_AISpawner::ShouldLive(AActor* AIagent)
{
	// Check if the AI is still inside the sphere bounds
	if (_pSphereComponent == nullptr)
	{
		return false;
	}

	return _pSphereComponent->IsOverlappingActor(AIagent);
}

bool AATS_AISpawner::SpawnAI()
{
	if (_pNavigationManager == nullptr)
	{
		if (_bDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("Navigation Manager is not set!"));
		}
		return false;
	}

	if (_pNavigationManager->Initialize() == false)
	{
		if (_bDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to initialize Navigation Manager!"));
		}
		return false;
	}

	float sphereRadius = _pSphereComponent->GetScaledSphereRadius();

	FVector location{ RandomPointInSphere( sphereRadius * 0.85f ) };
	FSpawnableAI AIRandomAI = _AIActorClasses[FMath::RandRange(0, _AIActorClasses.Num() - 1)];
	
	UINT32 closestPath{ _pNavigationManager->GetClosestPath(location, AIRandomAI.lanes) };
	FTransform transformOnPath{ _pNavigationManager->GetTransformOnPath(closestPath, location, 0) };

	// Get a random AI class
	if (_AIActorClasses.IsEmpty())
	{
		if (_bDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("No AI classes set!"));
		}
		return false;
	}

	FActorSpawnParameters spawnParams;
	spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;

	FVector spawnLocation = transformOnPath.GetLocation();
	spawnLocation.Z += 100.0f;
	transformOnPath.SetLocation(spawnLocation);

	AActor* pAI = GetWorld()->SpawnActor<AActor>(AIRandomAI.AIActorClass, transformOnPath, spawnParams);

	if (pAI == nullptr)
	{
		if (_bDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to spawn AI!"));
		}
		return false;
	}

	_SpawnedAI.Add(pAI);

	return true;
}

FVector AATS_AISpawner::RandomPointInSphere() const
{
	if (_pSphereComponent == nullptr)
	{
		return FVector::ZeroVector;
	}

	FVector origin	= _pSphereComponent->GetComponentLocation();
	float radius	= _pSphereComponent->GetScaledSphereRadius();

	float theta = FMath::FRand() * 2 * PI;
	float phi	= FMath::FRand() * PI;

	float x = origin.X + radius * FMath::Sin(phi) * FMath::Cos(theta);
	float y = origin.Y + radius * FMath::Sin(phi) * FMath::Sin(theta);
	float z = origin.Z + radius * FMath::Cos(phi);

	return FVector(x, y, z);
}

FVector AATS_AISpawner::RandomPointInSphere(float innerRadius) const
{
	if (_pSphereComponent == nullptr)
	{
		return FVector::ZeroVector;
	}

	
	// Get the radius of the sphere
	float sphereRadius = _pSphereComponent->GetScaledSphereRadius();

	float randomRadius = FMath::FRandRange(innerRadius, sphereRadius);

	// Generate a random unit vector
	FVector randomDirection = UKismetMathLibrary::RandomUnitVector();

	// Calculate the random point on the outer bounds of the sphere
	FVector sphereCenter = _pSphereComponent->GetComponentLocation();
	FVector randomPointOnSphere = sphereCenter + randomDirection * randomRadius;

	return randomPointOnSphere;
}


bool AATS_AISpawner::CleanupInvalid()
{
	for (int i = _SpawnedAI.Num() - 1; i >= 0; --i)
	{
		AActor* pAI = _SpawnedAI[i];

		if (pAI == nullptr || pAI->IsActorBeingDestroyed())
		{
			_SpawnedAI.RemoveAt(i);
		}
	}

	return true;
}
