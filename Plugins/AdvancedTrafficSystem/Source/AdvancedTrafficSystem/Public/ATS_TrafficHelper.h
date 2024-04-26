// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ZoneGraphTypes.h"
#include "ATS_TrafficHelper.generated.h"

/*
	This class is used to help with traffic simulation.
 */

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

//Enum with type of goal
UENUM(BlueprintType)
enum class ENavGoalType : uint8
{
	Parking,
	Work,
	Home,
	Other
};

class ADVANCEDTRAFFICSYSTEM_API ATS_TrafficHelper
{
public:
	ATS_TrafficHelper();
	~ATS_TrafficHelper();
};
