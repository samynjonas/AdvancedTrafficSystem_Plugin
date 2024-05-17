// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATS_TrafficHelper.h"

#include "ATS_ZoneShapeAgentContainer.h"
#include "ATS_TrafficManager.generated.h"

class UZoneShapeComponent;
class UZoneGraphSubsystem;

class AATS_LaneSpline;
class UATS_AgentNavigation;
class AATS_BaseTrafficRuler;
class UATS_TrafficAwarenessComponent;

UCLASS()
class ADVANCEDTRAFFICSYSTEM_API AATS_TrafficManager : public AActor
{
	GENERATED_BODY()
	
public:	
	AATS_TrafficManager();
	virtual void Tick(float DeltaTime) override;

protected:
	~AATS_TrafficManager();
	virtual void BeginPlay() override;

protected:
	void SortZoneShapes();
	void AttachTrafficLights();

	void AgentAwareness();
	void Debugging();

	void RetrieveLanes();

public:
	TArray<AATS_LaneSpline*> GetLaneSplines() const
	{
		return _pArrLaneSplines;
	}
	AATS_LaneSpline* GetClosestLane(const FVector& Location, ELaneType laneType);

public:

	const TArray<UZoneShapeComponent*> GetVehicleZoneShapes() const
	{
		return pArrVehicleZoneShapes;
	}
	const TArray<UZoneShapeComponent*> GetPedestrianZoneShapes() const
	{
		return pArrPedestrianZoneShapes;
	}

	bool ContainsZoneshapes() const
	{
		return bContainsZoneshapes;
	}
	void SearchZoneShapes();

	bool Initialize();
	bool IsInitialized() const
	{
		return bIsInitialized;
	}

	void UnregisterAgent(AActor* pAgent, UATS_AgentNavigation* pAgentNavigation);

	bool RegisterTrafficObject(UATS_TrafficAwarenessComponent* pTrafficObject, FVector& connectionPoint, float maxAttachDistance = -1);

	TArray<FLanePoint> GetLanePoints(AActor* pAgent);
	TArray<FLanePoint> GetAllLanePoints(AActor* pAgent, FZoneGraphTag tagFilter);

	FVector GetNextNavigationPoint(AActor* Agent, float AdvanceDistance, bool& stopDueTrafficRule, FAgentData& agentData, bool canRegisterAgent = true);
	FVector GetNextNavigationPathPoint(AActor* Agent, float AdvanceDistance, bool& stopDueTrafficRule, FAgentData& agentData, FTrafficNavigationPath& navPath);

	FVector GetTrafficAwareNavigationPoint(UATS_AgentNavigation* pAgent, FAgentData& agentData, float frontCarDistance);
	FVector GetPathEndNavigationPoint(UATS_AgentNavigation* pAgent, FAgentData& agentData, FTrafficNavigationPath& navPath, float lookdistance);

	TArray<FZoneGraphLaneLocation> GetNavigationPointLinkedLane(FZoneGraphLaneLocation currentLaneLocation, float AdvanceDistance);
	TArray<FZoneGraphLaneLocation> GetNavigationPointLinkedLane(FZoneGraphLaneLocation currentLaneLocation);

	FTrafficNavigationPath FindPath(FVector currentPosition, FVector goalPosition);

	bool IsLaneOpenToEnter(const FZoneGraphLaneLocation& lane) const;
	FAgentPoint GetClosestPointOnLane(const FBox& box, const FZoneGraphTagFilter& zoneGraphTagFilter, const FVector& point) const;
	FTransform GetClosestLanePoint(const FVector& Location, float searchDistance) const;

	FTransform GetClosestLanePoint(const FVector& location, float distanceOffset, FZoneGraphTag tagFilter);

	void DrawPath(const TArray<FZoneGraphLaneHandle>& lanes);

	FZoneGraphTag GetPedestrianTag() const
	{
		return m_PedestrianTag;
	}

	FZoneGraphTag GetVehicleTagMask() const
	{
		return m_VehicleTagMask;
	}

	FZoneGraphTag GetIntersectionTag() const
	{
		return m_IntersectionTag;
	}

protected:

	//----------------------------------------------------------\\
	//----------------ZoneShape Arrays--------------------------\\
	//----------------------------------------------------------\\

	UZoneGraphSubsystem* m_pZoneGraphSubSystem{ nullptr };

	TArray<UZoneShapeComponent*> pArrZoneShapes;
	TArray<UZoneShapeComponent*> pArrVehicleZoneShapes;
	TArray<UZoneShapeComponent*> pArrPedestrianZoneShapes;

	TMap<FZoneGraphLaneHandle, TUniquePtr<ATS_ZoneShapeAgentContainer>> m_pMapZoneShapeAgentContainer;

	TArray<AATS_LaneSpline*> _pArrLaneSplines;

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
