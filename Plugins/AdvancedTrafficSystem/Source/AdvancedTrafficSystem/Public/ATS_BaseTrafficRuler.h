// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATS_BaseTrafficRuler.generated.h"

UCLASS()
class ADVANCEDTRAFFICSYSTEM_API AATS_BaseTrafficRuler : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AATS_BaseTrafficRuler();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override; 


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual bool IsOpen() const;
};
