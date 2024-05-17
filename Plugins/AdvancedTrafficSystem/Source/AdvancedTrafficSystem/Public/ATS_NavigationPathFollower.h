// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "ATS_TrafficHelper.h"
#include "Components/ActorComponent.h"
#include "ATS_NavigationPathFollower.generated.h"

class AATS_NavigationManager;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ADVANCEDTRAFFICSYSTEM_API UATS_NavigationPathFollower : public UActorComponent
{
	GENERATED_BODY()

public:	
	UATS_NavigationPathFollower();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable, Category = "Navigation")
	FTransform GetNewPosition(float deltaTime, FVector position, float speed, bool loopOnEnd);

	UFUNCTION(BlueprintCallable, Category = "Navigation")
	FTransform GetNewPostionBasedOn2Points(float deltaTime, FVector frontPoint, FVector backPoint, float speed, bool loopOnEnd);

	UFUNCTION(BlueprintCallable, Category = "Navigation")
	bool GetObjectOnPath(FVector location, float distanceOffset, bool& canAgentPass, FVector& outLocation, float& distanceToObject, AActor* askingActor);

protected:
	UINT32 _CurrentPathIndex{ 0 };

	float _DistanceAlongPath{ 0 };
	float _SpeedLimit{ 0 };

	AATS_NavigationManager* _pNavigationManager{ nullptr };

	int _Direction{ 1 };

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
	TArray<ELaneType> _LaneTags{};

};
