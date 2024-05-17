// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ZoneGraphTypes.h"
#include "ATS_TrafficHelper.generated.h"

/*
	This class is used to help with traffic simulation.
*/

class USplineComponent;
class UATS_TrafficAwarenessComponent;

UENUM(BlueprintType)
enum class ELaneType : uint8
{
	ATS_All,
	ATS_Car,
	ATS_Pedestrian,
	ATS_Bicycle,
	ATS_Truck,
	ATS_Crosswalk
};

USTRUCT()
struct ADVANCEDTRAFFICSYSTEM_API FLaneNavigationPath
{
	GENERATED_BODY()

	FLaneNavigationPath() :
		pSpline(nullptr),
		speedLimit(30.f)
	{}

	//Override = operator
	bool operator==(const FLaneNavigationPath& other) const
	{
		return laneIndex == other.laneIndex;
	}

	bool IsValid() const
	{
		return pSpline != nullptr;
	}

	bool ContainsTag(ELaneType tag) const
	{
		return laneTags.Contains(tag);
	}

	bool HasOverlappingTags(const TArray<ELaneType>& tags) const
	{
		for (const auto& tag : tags)
		{
			if (laneTags.Contains(tag))
			{
				return true;
			}
		}
		return false;
	}

	USplineComponent* pSpline{ nullptr };
	TArray<UATS_TrafficAwarenessComponent*> arrTrafficObjects{};

	TArray<UINT32> nextPaths{};
	TArray<UINT32> previousPaths{};

	TArray<ELaneType> laneTags{};

	float speedLimit{ 30.f };
	UINT32 laneIndex{ 0 };
};

USTRUCT()
struct ADVANCEDTRAFFICSYSTEM_API FAgentNavigationData
{
	GENERATED_BODY()

	FZoneGraphLaneLocation currentLaneLocation;
	FZoneGraphLaneLocation nextLaneLocation;

	float distanceSqrt{};
	float AdvanceDistanceOffset{};
};

USTRUCT()
struct ADVANCEDTRAFFICSYSTEM_API FAgentData
{
	GENERATED_BODY()

	float agentDistanceOnSpline{ 0.f };
	float agentSpeed{};
	FTransform agentTransform{};

	//width, length, height
	FVector3d agentBox{};

	bool bFollowsSpline{ false }; //This can be used for more advanced agents -- for example a player won't follow the spline and checking for it will be more advanced
	bool bIsEnabled{ true };
};

USTRUCT()
struct ADVANCEDTRAFFICSYSTEM_API FAgentPoint
{
	GENERATED_BODY()

	FVector position{};
	FVector direction{};
};

USTRUCT()
struct ADVANCEDTRAFFICSYSTEM_API FLanePoint
{
	GENERATED_BODY()

	FVector position{};
	float distanceAlongLane{};
};

USTRUCT()
struct ADVANCEDTRAFFICSYSTEM_API FTrafficNavigationPath
{
	GENERATED_BODY()

	TArray<FZoneGraphLaneHandle> path{};

	FZoneGraphLaneLocation startLaneLocation{};
	FZoneGraphLaneLocation endLaneLocation{};

	bool bHasReachedEnd{ false };
	bool bIsFollowingPath{ false };
	UINT32 pathIndex{ 0 };
};

UENUM(BlueprintType)
enum class ENavGoalType : uint8
{
	Parking,
	Work,
	Home,
	Other
};

UENUM(BlueprintType)
enum class ESteeringBehaviors : uint8
{
	ATS_Seek,
	ATS_Flee,
	ATS_Wander,
	ATS_FollowPath,
	ATS_Seperation,
	ATS_Cohesion,
	ATS_Alignment
};

class ADVANCEDTRAFFICSYSTEM_API ATS_TrafficHelper
{
public:
	ATS_TrafficHelper();
	~ATS_TrafficHelper();
};

USTRUCT()
struct ADVANCEDTRAFFICSYSTEM_API FSteeringStruct
{
	GENERATED_BODY()

	FSteeringStruct() :
		weight(0.f),
		behavior(ESteeringBehaviors::ATS_Seek)
	{}

	FSteeringStruct(ESteeringBehaviors _behavior, float _weight = 1) :
		weight(_weight),
		behavior(_behavior)
	{}

	float weight{};
	ESteeringBehaviors behavior{};
};


