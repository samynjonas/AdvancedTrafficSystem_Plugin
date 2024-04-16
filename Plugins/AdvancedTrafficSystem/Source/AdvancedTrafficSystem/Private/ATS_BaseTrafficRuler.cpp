// Fill out your copyright notice in the Description page of Project Settings.


#include "../Public/ATS_BaseTrafficRuler.h"

// Sets default values
AATS_BaseTrafficRuler::AATS_BaseTrafficRuler()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AATS_BaseTrafficRuler::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AATS_BaseTrafficRuler::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

bool AATS_BaseTrafficRuler::IsOpen() const
{
	return true;
}