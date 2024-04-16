#include "../Public/ATS_LaneModifierComponent.h"

UATS_LaneModifierComponent::UATS_LaneModifierComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UATS_LaneModifierComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UATS_LaneModifierComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UATS_LaneModifierComponent::ModifyLane()
{
	UE_LOG(LogTemp, Warning, TEXT("Lane modified"));
}

