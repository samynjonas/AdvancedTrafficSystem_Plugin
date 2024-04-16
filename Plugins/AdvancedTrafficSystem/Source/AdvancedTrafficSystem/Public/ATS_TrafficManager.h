// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATS_TrafficHelper.h"

#include "ATS_ZoneShapeAgentContainer.h"
#include "ATS_TrafficManager.generated.h"

class UZoneShapeComponent;
class UZoneGraphSubsystem;
class UATS_AgentNavigation;
class AATS_BaseTrafficRuler;

UCLASS()
class ADVANCEDTRAFFICSYSTEM_API AATS_TrafficManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AATS_TrafficManager();

protected:
	~AATS_TrafficManager();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void SortZoneShapes();
	void AttachTrafficLights();

	void AgentAwareness();
	void Debugging();

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//Returns VehicleZoneShapes
	const TArray<UZoneShapeComponent*> GetVehicleZoneShapes() const
	{
		return pArrVehicleZoneShapes;
	}

	//Return PedestrianZoneShapes
	const TArray<UZoneShapeComponent*> GetPedestrianZoneShapes() const
	{
		return pArrPedestrianZoneShapes;
	}

	bool ContainsZoneshapes() const
	{
		return bContainsZoneshapes;
	}
	void SearchZoneShapes();

	void Initialize();
	bool IsInitialized() const
	{
		return bIsInitialized;
	}

	void UnregisterAgent(AActor* pAgent, UATS_AgentNavigation* pAgentNavigation);

	FVector GetNextNavigationPoint(AActor* Agent, float AdvanceDistance, bool& stopDueTrafficRule, FAgentData& agentData);
	FVector GetNextNavigationPathPoint(AActor* Agent, float AdvanceDistance, bool& stopDueTrafficRule, FAgentData& agentData, FTrafficNavigationPath& navPath);

	FVector GetTrafficAwareNavigationPoint(UATS_AgentNavigation* pAgent, FAgentData& agentData, float frontCarDistance);
	FVector GetPathEndNavigationPoint(UATS_AgentNavigation* pAgent, FAgentData& agentData, FTrafficNavigationPath& navPath, float lookdistance);

	TArray<FZoneGraphLaneLocation> GetNavigationPointLinkedLane(FZoneGraphLaneLocation currentLaneLocation, float AdvanceDistance);
	TArray<FZoneGraphLaneLocation> GetNavigationPointLinkedLane(FZoneGraphLaneLocation currentLaneLocation);

	FTrafficNavigationPath FindPath(FVector currentPosition, FVector goalPosition);

	bool IsLaneOpenToEnter(const FZoneGraphLaneLocation& lane) const;
	FAgentPoint GetClosestPointOnLane(const FBox& box, const FZoneGraphTagFilter& zoneGraphTagFilter, const FVector& point) const;
	FTransform GetClosestLanePoint(const FVector& Location, float searchDistance) const;

	void DrawPath(const TArray<FZoneGraphLaneHandle>& lanes);

protected:

	//----------------------------------------------------------\\
	//----------------ZoneShape Arrays--------------------------\\
	//----------------TODO SMART POINTERS?----------------------\\
	//----------------------------------------------------------\\

	UZoneGraphSubsystem* m_pZoneGraphSubSystem{ nullptr };

	TArray<UZoneShapeComponent*> pArrZoneShapes;

	TArray<UZoneShapeComponent*> pArrVehicleZoneShapes;
	TArray<UZoneShapeComponent*> pArrPedestrianZoneShapes;

	TMap<FZoneGraphLaneHandle, TUniquePtr<ATS_ZoneShapeAgentContainer>> m_pMapZoneShapeAgentContainer;

	//----------------------------------------------------------\\
	//----------------Settings----------------------------------\\
	//----------------------------------------------------------\\

	bool bContainsZoneshapes{ false };
	bool bIsInitialized{ false };

	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bIsDebugging{ false };

	//Make editable in editor
	UPROPERTY(EditAnywhere, Category = "Settings|Tags")
	FZoneGraphTag m_VehicleTagMask{ 1 };

	UPROPERTY(EditAnywhere, Category = "Settings|Tags")
	FZoneGraphTag m_PedestrianTag{ 3 };

	UPROPERTY(EditAnywhere, Category = "Settings|Tags")
	FZoneGraphTag m_IntersectionTag{ 4 };

	//Map of all agents and their navigation data
	TMap<AActor*, FAgentNavigationData> m_MapAgentNavigationData;

	TMap<FZoneGraphLaneHandle, AATS_BaseTrafficRuler*> m_MapLaneRuler;
	float m_SearchDistance{ 1000.f };
};
