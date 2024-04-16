// Fill out your copyright notice in the Description page of Project Settings.


#include "../Public/ATS_NavigationGoal.h"

// Sets default values
AATS_NavigationGoal::AATS_NavigationGoal()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AATS_NavigationGoal::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AATS_NavigationGoal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

