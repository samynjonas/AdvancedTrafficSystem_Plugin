#include "../Public/ATS_TrafficManager.h"
#include "../Public/ATS_TrafficLight.h"
#include "../Public/ATS_AgentNavigation.h"
#include "../Public/ATS_BaseTrafficRuler.h"
#include "../Public/ATS_TrafficAwarenessComponent.h"
#include "../Public/ATS_LaneSpline.h"

#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

#include "Components/SplineComponent.h"

#include "ZoneGraphAStar.h"
#include "ZoneShapeComponent.h"
#include <ZoneGraphRenderingUtilities.h>

AATS_TrafficManager::AATS_TrafficManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

AATS_TrafficManager::~AATS_TrafficManager()
{
    //Cleanup of all containers
    pArrZoneShapes.Empty();
    pArrVehicleZoneShapes.Empty();
    pArrPedestrianZoneShapes.Empty();
}

void AATS_TrafficManager::BeginPlay()
{    
	Super::BeginPlay();
    Initialize();
}

bool AATS_TrafficManager::Initialize()
{
    if (bIsInitialized == true)
    {
        return true;
    }

    //Retrieve ZoneGraphSubsystem
    UWorld* World = GetWorld();
    m_pZoneGraphSubSystem = UWorld::GetSubsystem<UZoneGraphSubsystem>(World);
    if (m_pZoneGraphSubSystem == nullptr)
    {
        return false;
    }

    RetrieveLanes();
    SearchZoneShapes();
    AttachTrafficLights();

    bIsInitialized = true;
    return true;
}

void AATS_TrafficManager::SearchZoneShapes()
{
    //Check if it already contains ZoneShapes
    if (bContainsZoneshapes)
    {
        return;
    }

    //GETTING ALL ZONESHAPES IN CURRENT LEVEL AND SAVING THEM
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* pActor = *It;
        UZoneShapeComponent* pZoneShape = pActor->FindComponentByClass<UZoneShapeComponent>();
        if (pZoneShape)
        {
            if (bIsDebugging)
            {
                UE_LOG(LogTemp, Warning, TEXT("TrafficManager::Checking for lanes"));
            }

            TArray<FZoneGraphLaneHandle> OutLanes{};
            m_pZoneGraphSubSystem->FindOverlappingLanes(pZoneShape->CalcBounds(FTransform{}).GetBox(), FZoneGraphTagFilter(), OutLanes);
            if (bIsDebugging)
            {
                for (const auto& lane : OutLanes)
                {
			    	UE_LOG(LogTemp, Warning, TEXT("TrafficManager::LANE FOUND"));
			    	UE_LOG(LogTemp, Warning, TEXT("TrafficManager::LANE HANDLE: %d"), lane);
			    }
            }
            pArrZoneShapes.Add(pZoneShape);
        }
    }

    //LOGGING ZONESHAPES FINDINGS
    if (pArrZoneShapes.IsEmpty())
    {
        if (bIsDebugging)
        {
            UE_LOG(LogTemp, Error, TEXT("TrafficManager::NO ZONESHAPES FOUND IN CURRENT LEVEL"));
        }
        return;
    }

    if (bIsDebugging)
    {
        UE_LOG(LogTemp, Warning, TEXT("TrafficManager::FOUND %d ZONESHAPES IN CURRENT LEVEL"), pArrZoneShapes.Num());
    }
    SortZoneShapes();

    bContainsZoneshapes = true;
}

void AATS_TrafficManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    if (bIsDebugging)
    {
        Debugging();
    }
}

void AATS_TrafficManager::SortZoneShapes()
{
    //Sorts zoneshapes into seperated containers depending on their tags
    for (const auto& pZoneShape : pArrZoneShapes)
    {
        if (pZoneShape == nullptr)
        {
            continue;
        }

        if(pZoneShape->GetTags().Contains(m_VehicleTagMask))
        {   
            if (bIsDebugging)
            {
                UE_LOG(LogTemp, Warning, TEXT("TrafficManager::ZONESHAPE IS OF TYPE: VEHICLE"));
            }
            pArrVehicleZoneShapes.Add(pZoneShape);
        }
        else if (pZoneShape->GetTags().Contains(m_PedestrianTag))
        {
            if (bIsDebugging)
            {
                UE_LOG(LogTemp, Warning, TEXT("TrafficManager::ZONESHAPE IS OF TYPE: PEDESTRIAN"));
            }
            pArrPedestrianZoneShapes.Add(pZoneShape);
        }
        else
        {
            if (bIsDebugging)
            {
                UE_LOG(LogTemp, Error, TEXT("TrafficManager::ZONESHAPE DOESN'T CONTAINS TAG"));
            }
        }
    }
}

void AATS_TrafficManager::RetrieveLanes()
{
    //Retrieve all LaneSplines in the world
    for (TActorIterator<AATS_LaneSpline> It(GetWorld()); It; ++It)
    {
		AATS_LaneSpline* pLaneSpline = *It;
        if (pLaneSpline)
        {
            if (bIsDebugging)
            {
				UE_LOG(LogTemp, Warning, TEXT("TrafficManager::LANE FOUND"));
			}
            _pArrLaneSplines.Add(pLaneSpline);
		}	
    }
}

FTransform AATS_TrafficManager::GetClosestLanePoint(const FVector& location, float distanceOffset, FZoneGraphTag tagFilter)
{
    //Find the closest lane to the location
    FZoneGraphTagFilter tagFilterStruct{};

    FZoneGraphTagMask tagMask{};
    tagMask.Add(tagFilter);
    tagFilterStruct.Pass(tagMask);

    FZoneGraphLaneLocation closestLaneLocation{};
    float distanceSqr{};
    m_pZoneGraphSubSystem->FindNearestLane(FBox(location - FVector(10000.f), location + FVector(10000.f)), tagFilterStruct, closestLaneLocation, distanceSqr);

    //Get the distance along the lane
    float distanceAlongLane{ closestLaneLocation.DistanceAlongLane };
    distanceAlongLane += distanceOffset;

    //Check if the distance is smaller than the current lane
    float laneLength{};    

    return FTransform{};
}

AATS_LaneSpline* AATS_TrafficManager::GetClosestLane(const FVector& Location, ELaneType laneType)
{
    if (_pArrLaneSplines.IsEmpty())
    {
        RetrieveLanes();
	}

    //Filter out the lanes with the laneType
    TArray<AATS_LaneSpline*> filteredLanes{};
    for (auto pLane : _pArrLaneSplines)
    {
        if (pLane->GetLaneType() == laneType)
        {
			filteredLanes.Add(pLane);
		}
	}

    //Find the closest lane
    AATS_LaneSpline* pClosestLane{ nullptr };
    float closestDistance{ TNumericLimits<float>::Max() };
    for (auto pLane : filteredLanes)
    {
        USplineComponent* pSpline = pLane->GetSpline();
        if (pSpline == nullptr)
        {
			continue;
        }

        FVector closestPoint    = pSpline->FindLocationClosestToWorldLocation(Location, ESplineCoordinateSpace::World);
        float distance          = FVector::Dist(closestPoint, Location);
        if (distance < closestDistance)
        {
			closestDistance = distance;
			pClosestLane    = pLane;
		}
    }

    return pClosestLane;
}

TArray<FLanePoint> AATS_TrafficManager::GetLanePoints(AActor* pAgent)
{
    /*
        TODO extend -- add the function to add all lanes and retrieve the points but increase their distance along lane per lane to have a better overview of the lane
    */

    TArray<FLanePoint> currentLanePoints{};
    TArray<FLanePoint> nextLanePoints{};

    if (m_MapAgentNavigationData.Contains(pAgent) == false)
    {
        return currentLanePoints;
    }

    FZoneGraphLaneLocation currentLaneLocation  = m_MapAgentNavigationData[pAgent].currentLaneLocation;
    FZoneGraphLaneLocation nextLaneLocation     = m_MapAgentNavigationData[pAgent].nextLaneLocation;

    //Get the lane points
    const FZoneGraphStorage* zoneGraphData = m_pZoneGraphSubSystem->GetZoneGraphStorage(currentLaneLocation.LaneHandle.DataHandle);
 
    if(zoneGraphData == nullptr)
    {
		return currentLanePoints;
	}

    //Loop over all lane points
    for (auto lanePoint : zoneGraphData->LanePoints)
    {
	    //Check if they are on the same or next lane of the agent
        FZoneGraphLaneLocation closestLaneLocation{};
        float distanceSqr{};
        FVector Location = lanePoint;
        float searchDistance = 10.f;

        m_pZoneGraphSubSystem->FindNearestLane(FBox(Location - FVector(searchDistance), Location + FVector(searchDistance)), FZoneGraphTagFilter(), closestLaneLocation, distanceSqr);
        
        FLanePoint fLanePoint{};
        fLanePoint.position          = lanePoint;
        fLanePoint.distanceAlongLane = closestLaneLocation.DistanceAlongLane;

        if (closestLaneLocation.LaneHandle == currentLaneLocation.LaneHandle)
        {
            if (currentLaneLocation.DistanceAlongLane > fLanePoint.distanceAlongLane)
            {
                continue;
            }
            currentLanePoints.Add(fLanePoint);
		}
        else if (closestLaneLocation.LaneHandle == nextLaneLocation.LaneHandle)
        {
            nextLanePoints.Add(fLanePoint);
        }
    }

    //Sort both the arrays based on their distance along the lane which is stored in Z
    currentLanePoints.Sort([](const FLanePoint& a, const FLanePoint& b) { return a.distanceAlongLane < b.distanceAlongLane; });
    nextLanePoints.Sort([](const FLanePoint& a, const FLanePoint& b) { return a.distanceAlongLane < b.distanceAlongLane; });

    //Remove the first from the nextLanePoints
    if (nextLanePoints.Num() > 0)
    {
		nextLanePoints.RemoveAt(0);
	}

    //Debug draw the points
    for (auto& point : currentLanePoints)
    {
        DrawDebugSphere(GetWorld(), point.position, 75.f, 12, FColor::Blue, false, 0.1f);
    }

    for (auto& point : nextLanePoints)
    {
        DrawDebugSphere(GetWorld(), point.position, 150.f, 12, FColor::Red, false, 0.1f);
    }

    //Combine the arrays by placing the next after the current
    currentLanePoints.Append(nextLanePoints);

    return currentLanePoints;
}

TArray<FLanePoint> AATS_TrafficManager::GetAllLanePoints(AActor* pAgent, FZoneGraphTag tagFilter)
{
    /*
        TODO extend -- add the function to add all lanes and retrieve the points 
                       but increase their distance along lane per lane to have a better overview of the lane

        TODO extend -- Currently all lanes are added so that is good
                       but the problem is that the lanes aren't correctly sorted
    */

    TArray<FLanePoint> allLanePoints{};
    TArray<TArray<FLanePoint>> lanePoints{};

    if (m_MapAgentNavigationData.Contains(pAgent) == false)
    {
        return allLanePoints;
    }

    FZoneGraphLaneLocation currentLaneLocation = m_MapAgentNavigationData[pAgent].currentLaneLocation;

    //Get the lane points
    const FZoneGraphStorage* zoneGraphData = m_pZoneGraphSubSystem->GetZoneGraphStorage(currentLaneLocation.LaneHandle.DataHandle);

    if (zoneGraphData == nullptr)
    {
        return allLanePoints;
    }

    FZoneGraphLaneHandle lastLaneHandle{};

    //Loop over all lane points
    for (auto lanePoint : zoneGraphData->LanePoints)
    {
        //Check if they are on the same or next lane of the agent
        FZoneGraphLaneLocation closestLaneLocation{};
        float distanceSqr{};
        FVector Location = lanePoint;
        float searchDistance = 10.f;

        m_pZoneGraphSubSystem->FindNearestLane(FBox(Location - FVector(searchDistance), Location + FVector(searchDistance)), FZoneGraphTagFilter(), closestLaneLocation, distanceSqr);

        //Adding tag checking to only receive the zoneGraph from the tags we want
        FZoneGraphTagMask laneTags{};
        m_pZoneGraphSubSystem->GetLaneTags(closestLaneLocation.LaneHandle, laneTags);

        if (laneTags.Contains(tagFilter) == false)
        {
            continue;
        }

        FLanePoint fLanePoint{};
        fLanePoint.position = lanePoint;
        fLanePoint.distanceAlongLane = closestLaneLocation.DistanceAlongLane;

        if (closestLaneLocation.LaneHandle == lastLaneHandle)
        {
            lanePoints.Last().Add(fLanePoint);
		}
		else
		{
			TArray<FLanePoint> newLanePoints{};
			newLanePoints.Add(fLanePoint);

			lanePoints.Add(newLanePoints);
			lastLaneHandle = closestLaneLocation.LaneHandle;
        }
    }

    //Sort all the arrays based on their distance along
    for (auto& lane : lanePoints)
	{
		lane.Sort([](const FLanePoint& a, const FLanePoint& b) { return a.distanceAlongLane < b.distanceAlongLane; });
	}

    //Loop over each laneArray and add the length of the previous lane to it
    float distanceAlongLane{ 0.f };
    for (auto& lane : lanePoints)
	{
		for (auto& point : lane)
		{
			point.distanceAlongLane += distanceAlongLane;
			allLanePoints.Add(point);
		}

        //Take the distanceAlongLane from the last point
        distanceAlongLane += lane.Last().distanceAlongLane;
	}

    //Combine all the arrays
    for (auto& lane : lanePoints)
    {
        for (auto& point : lane)
        {
        	allLanePoints.Add(point);
        }
    }

    //Debugging
    if (bIsDebugging)
    {
        for (auto& point : allLanePoints)
	    {
	    	DrawDebugSphere(GetWorld(), point.position, 75.f, 12, FColor::Orange, false, 10.f);
	    }
    }
    return allLanePoints;
}

FVector AATS_TrafficManager::GetNextNavigationPoint(AActor* pAgent, float AdvanceDistance, bool& stopDueTrafficRule, FAgentData& agentData, bool canRegisterAgent)
{
    if (pAgent == nullptr)
    {
        if (bIsDebugging)
        {
            UE_LOG(LogTemp, Error, TEXT("TrafficManager::GetNextNavigationPoint() -- Agent is null"));
        }
        return FVector::ZeroVector;
    }

    if (m_pZoneGraphSubSystem == nullptr)
    {
        if (bIsDebugging)
        {
            UE_LOG(LogTemp, Error, TEXT("TrafficManager::GetNextNavigationPoint() -- ZoneGraphSubsystem is not available"));
        }
        return FVector::ZeroVector;
    }

    FZoneGraphLaneLocation CurrentLaneLocation;
    FVector CurrentPosition = pAgent->GetActorLocation();

    //Check if agent is in map already
    if (m_MapAgentNavigationData.Contains(pAgent))
    {    
        if (!m_pZoneGraphSubSystem->FindNearestLocationOnLane(m_MapAgentNavigationData[pAgent].currentLaneLocation.LaneHandle, FBox(CurrentPosition - FVector(AdvanceDistance), CurrentPosition + FVector(AdvanceDistance)), m_MapAgentNavigationData[pAgent].currentLaneLocation, m_MapAgentNavigationData[pAgent].distanceSqrt))
        {
            if (bIsDebugging)
            {
			    UE_LOG(LogTemp, Error, TEXT("TrafficManager::GetNextNavigationPoint() -- No location found nearby lane found"));
            }
		}
        CurrentLaneLocation = m_MapAgentNavigationData[pAgent].currentLaneLocation;

        //Get the lane length
        float laneLength{};
        m_pZoneGraphSubSystem->GetLaneLength(CurrentLaneLocation.LaneHandle, laneLength);
        
        //Get the agent distance along the lane
        float agentDistanceAlongLane{ CurrentLaneLocation.DistanceAlongLane };
        agentData.agentDistanceOnSpline = agentDistanceAlongLane;

        //Calculate the forward point distance along the lane
        float pointDistanceAlongLane{ agentDistanceAlongLane + AdvanceDistance };

        //Calculate the difference between the point distance and the lane length
        float difference{ pointDistanceAlongLane - laneLength };

        //Calculate the distance to the next lane
        float distanceToNextLane{ laneLength - agentDistanceAlongLane };

        //Check if the agent is at the end of the lane
        if (agentDistanceAlongLane >= laneLength)
        {
            //Agent is at the end of the lane, so we need to move it to the next lane
            //Get the agent navigation component
            UATS_AgentNavigation* pAgentNavigation = pAgent->FindComponentByClass<UATS_AgentNavigation>();
            if (pAgentNavigation == nullptr)
            {
                return FVector::ZeroVector;
            }

            if (canRegisterAgent)
            {

                //Register the agent to the next lane
                bool succesfullRegister{ false };
                if (m_pMapZoneShapeAgentContainer.Contains(m_MapAgentNavigationData[pAgent].nextLaneLocation.LaneHandle) == false)
                {
                    m_pMapZoneShapeAgentContainer.Add(m_MapAgentNavigationData[pAgent].nextLaneLocation.LaneHandle, MakeUnique<ATS_ZoneShapeAgentContainer>());
                }

                succesfullRegister = m_pMapZoneShapeAgentContainer[m_MapAgentNavigationData[pAgent].nextLaneLocation.LaneHandle]->RegisterAgent(pAgentNavigation);
                if (succesfullRegister)
                {
                    //Unregister the agent from the current lane
                    if (m_pMapZoneShapeAgentContainer.Contains(m_MapAgentNavigationData[pAgent].currentLaneLocation.LaneHandle) == false)
                    {
                        m_pMapZoneShapeAgentContainer.Add(m_MapAgentNavigationData[pAgent].currentLaneLocation.LaneHandle, MakeUnique<ATS_ZoneShapeAgentContainer>());
                    }
                    m_pMapZoneShapeAgentContainer[m_MapAgentNavigationData[pAgent].currentLaneLocation.LaneHandle]->UnregisterAgent(pAgentNavigation);
                    
                    //Set the current lane location to the next lane location
                    m_MapAgentNavigationData[pAgent].currentLaneLocation = m_MapAgentNavigationData[pAgent].nextLaneLocation;

                    //Reset the next lane location
                    m_MapAgentNavigationData[pAgent].nextLaneLocation.Reset();
                }
            }
        }
        else if (difference > 0)
        {
            //Agent is not at the end of the lane, so we need to move it to the next lane
            if (!m_MapAgentNavigationData[pAgent].nextLaneLocation.IsValid())
            {
                //Pick the next lane
                TArray<FZoneGraphLaneLocation> possibleLanes = GetNavigationPointLinkedLane(CurrentLaneLocation, difference);
                if (possibleLanes.IsEmpty())
                {
                    //No possible lanes found -- Handle
                    if(bIsDebugging)
                    {
                        UE_LOG(LogTemp, Error, TEXT("TrafficManager::GetNextNavigationPoint() -- No lanes found to continue on!"));
                    }
                }
                else
                {
                    //Pick a random lane to continue on
                    int randomLane = FMath::RandRange(0, possibleLanes.Num() - 1);

                    if (possibleLanes[randomLane].IsValid() == false)
                    {
                        if (bIsDebugging)
                        {
                            UE_LOG(LogTemp, Warning, TEXT("TrafficManager::GetNextNavigationPoint() -- Agent's next lane is not valid"));
                        }
                        return FVector::ZeroVector;
                    }

                    m_MapAgentNavigationData[pAgent].nextLaneLocation = possibleLanes[randomLane];
                }
            }

            //Calculate the advance distance
            AdvanceDistance = difference;

            //Check if the lane is open
            if (m_MapLaneRuler.Contains(CurrentLaneLocation.LaneHandle) && m_MapLaneRuler[CurrentLaneLocation.LaneHandle] != nullptr && m_MapLaneRuler[CurrentLaneLocation.LaneHandle]->IsOpen() == false)
            {
                AdvanceDistance = 0;
                stopDueTrafficRule = true;
            }
            CurrentLaneLocation = m_MapAgentNavigationData[pAgent].nextLaneLocation;

            //Point is at the end of the lane, TODO: handle it
            if (!m_pZoneGraphSubSystem->FindNearestLocationOnLane(CurrentLaneLocation.LaneHandle, FBox(CurrentPosition - FVector(distanceToNextLane + difference), CurrentPosition + FVector(distanceToNextLane + difference)), CurrentLaneLocation, m_MapAgentNavigationData[pAgent].distanceSqrt))
            {
                if (bIsDebugging)
                {
                    UE_LOG(LogTemp, Error, TEXT("TrafficManager::No location found nearby lane found"));
                }
            }
        }
    }
    else
    {
        if (bIsDebugging)
        {
            UE_LOG(LogTemp, Warning, TEXT("TrafficManager::New agent -- add to map"));
        }

        //Agent is not in map, so we need to add it
        FAgentNavigationData agentNavigationData{};        
        float DistanceSqr{};

        // Find the nearest lane to the agent's current position
        if (!m_pZoneGraphSubSystem->FindNearestLane(FBox(CurrentPosition - FVector(AdvanceDistance), CurrentPosition + FVector(AdvanceDistance)), FZoneGraphTagFilter(), CurrentLaneLocation, DistanceSqr))
        {
            if (bIsDebugging)
            {
                UE_LOG(LogTemp, Error, TEXT("TrafficManager::No nearby lane found for new vehicle"));
            }
        }
        agentNavigationData.currentLaneLocation = CurrentLaneLocation;
        agentNavigationData.distanceSqrt = DistanceSqr;

        float agentDistanceAlongLane{ CurrentLaneLocation.DistanceAlongLane };
        agentData.agentDistanceOnSpline = agentDistanceAlongLane;

        UATS_AgentNavigation* pAgentNavigation = pAgent->FindComponentByClass<UATS_AgentNavigation>();
        if (pAgentNavigation == nullptr)
        {
            return FVector::ZeroVector;
        }

        if (canRegisterAgent)
        {
            m_MapAgentNavigationData.Add(pAgent, agentNavigationData);

            if (m_pMapZoneShapeAgentContainer.Contains(agentNavigationData.currentLaneLocation.LaneHandle) == false)
            {
                m_pMapZoneShapeAgentContainer.Add(agentNavigationData.currentLaneLocation.LaneHandle, MakeUnique<ATS_ZoneShapeAgentContainer>());
		    }
            m_pMapZoneShapeAgentContainer[agentNavigationData.currentLaneLocation.LaneHandle]->RegisterAgent(pAgentNavigation);
        }

        if (bIsDebugging)
        {
            UE_LOG(LogTemp, Warning, TEXT("TrafficManager::Agent added to map"));
        }
    }

    // Calculate the next position along the lane
    FZoneGraphLaneLocation NextLaneLocation; // Could be usefull for more stuff
    if (!m_pZoneGraphSubSystem->AdvanceLaneLocation(CurrentLaneLocation, AdvanceDistance, NextLaneLocation))
    {
        if (bIsDebugging)
        {
            UE_LOG(LogTemp, Error, TEXT("TrafficManager::Could not advance along the lane"));
        }
        return FVector::ZeroVector;       
    }
    return NextLaneLocation.Position;
}

FVector AATS_TrafficManager::GetNextNavigationPathPoint(AActor* pAgent, float AdvanceDistance, bool& stopDueTrafficRule, FAgentData& agentData, FTrafficNavigationPath& navPath)
{
    if (pAgent == nullptr)
    {
        if (bIsDebugging)
        {
            UE_LOG(LogTemp, Error, TEXT("TrafficManager::Agent is null"));
        }
        return FVector::ZeroVector;
    }

    if (m_pZoneGraphSubSystem == nullptr)
    {
        if (bIsDebugging)
        {
            UE_LOG(LogTemp, Error, TEXT("TrafficManager::ZoneGraphSubsystem is not available"));
        }
        return FVector::ZeroVector;
    }

    //Check if current lane on path is valid
    int currentIndex = static_cast<int>(navPath.pathIndex);
    if (currentIndex >= navPath.path.Num())
    {
        if (bIsDebugging)
        {
            UE_LOG(LogTemp, Error, TEXT("TrafficManager::Current path is not valid"));
        }
        return FVector::ZeroVector;
    }

    FZoneGraphLaneLocation CurrentLaneLocation;
    FVector CurrentPosition = pAgent->GetActorLocation();

    //Check if agent is in map already
    if (m_MapAgentNavigationData.Contains(pAgent))
    {
        if (!m_pZoneGraphSubSystem->FindNearestLocationOnLane(m_MapAgentNavigationData[pAgent].currentLaneLocation.LaneHandle, FBox(CurrentPosition - FVector(AdvanceDistance), CurrentPosition + FVector(AdvanceDistance)), m_MapAgentNavigationData[pAgent].currentLaneLocation, m_MapAgentNavigationData[pAgent].distanceSqrt))
        {
            if (bIsDebugging)
            {
                UE_LOG(LogTemp, Error, TEXT("TrafficManager::No location found nearby lane found"));
            }
        }
        CurrentLaneLocation = m_MapAgentNavigationData[pAgent].currentLaneLocation;

        float laneLength{};
        m_pZoneGraphSubSystem->GetLaneLength(CurrentLaneLocation.LaneHandle, laneLength);

        float agentDistanceAlongLane{ CurrentLaneLocation.DistanceAlongLane };
        agentData.agentDistanceOnSpline = agentDistanceAlongLane;

        float pointDistanceAlongLane{ agentDistanceAlongLane + AdvanceDistance };
        float difference{ pointDistanceAlongLane - laneLength };
        float distanceToNextLane{ laneLength - agentDistanceAlongLane };

        if (agentDistanceAlongLane >= laneLength)
        {
            UATS_AgentNavigation* pAgentNavigation = pAgent->FindComponentByClass<UATS_AgentNavigation>();
            if (pAgentNavigation == nullptr)
            {
                return FVector::ZeroVector;
            }

            if (m_pMapZoneShapeAgentContainer.Contains(m_MapAgentNavigationData[pAgent].currentLaneLocation.LaneHandle) == false)
            {
                m_pMapZoneShapeAgentContainer.Add(m_MapAgentNavigationData[pAgent].currentLaneLocation.LaneHandle, MakeUnique<ATS_ZoneShapeAgentContainer>());
            }
            m_pMapZoneShapeAgentContainer[m_MapAgentNavigationData[pAgent].currentLaneLocation.LaneHandle]->UnregisterAgent(pAgentNavigation);

            if (m_pMapZoneShapeAgentContainer.Contains(m_MapAgentNavigationData[pAgent].nextLaneLocation.LaneHandle) == false)
            {
                m_pMapZoneShapeAgentContainer.Add(m_MapAgentNavigationData[pAgent].nextLaneLocation.LaneHandle, MakeUnique<ATS_ZoneShapeAgentContainer>());
            }
            m_pMapZoneShapeAgentContainer[m_MapAgentNavigationData[pAgent].nextLaneLocation.LaneHandle]->RegisterAgent(pAgentNavigation);

            m_MapAgentNavigationData[pAgent].currentLaneLocation = m_MapAgentNavigationData[pAgent].nextLaneLocation;
            m_MapAgentNavigationData[pAgent].nextLaneLocation.Reset();
            navPath.pathIndex++;
        }
        else if (difference > 0)
        {
            if (!m_MapAgentNavigationData[pAgent].nextLaneLocation.IsValid())
            {
                //Pick the next lane
                int32 nextLaneIndex = static_cast<int>(navPath.pathIndex) + 1;
                if (nextLaneIndex >= navPath.path.Num())
                {
                    if (bIsDebugging)
                    {
						UE_LOG(LogTemp, Error, TEXT("TrafficManager::No more lanes to continue on!"));
					}
					return FVector::ZeroVector;
				}

                //Get the next lane
                FZoneGraphLaneLocation nextLaneLocation{};
                if (!m_pZoneGraphSubSystem->FindNearestLocationOnLane(navPath.path[nextLaneIndex], FBox(CurrentPosition - FVector(AdvanceDistance), CurrentPosition + FVector(AdvanceDistance)), nextLaneLocation, m_MapAgentNavigationData[pAgent].distanceSqrt))
                {
                    if (bIsDebugging)
                    {
						UE_LOG(LogTemp, Error, TEXT("TrafficManager::No location found nearby lane found"));
                        return FVector::ZeroVector;
					}
				}
                m_MapAgentNavigationData[pAgent].nextLaneLocation = nextLaneLocation;
            }

            AdvanceDistance = difference;
            if (m_MapLaneRuler.Contains(CurrentLaneLocation.LaneHandle) && m_MapLaneRuler[CurrentLaneLocation.LaneHandle] != nullptr && m_MapLaneRuler[CurrentLaneLocation.LaneHandle]->IsOpen() == false)
            {
                AdvanceDistance = 0;
                stopDueTrafficRule = true;
            }
            CurrentLaneLocation = m_MapAgentNavigationData[pAgent].nextLaneLocation;

            //Point is at the end of the lane, TODO: handle it
            if (!m_pZoneGraphSubSystem->FindNearestLocationOnLane(CurrentLaneLocation.LaneHandle, FBox(CurrentPosition - FVector(distanceToNextLane + difference), CurrentPosition + FVector(distanceToNextLane + difference)), CurrentLaneLocation, m_MapAgentNavigationData[pAgent].distanceSqrt))
            {
                if (bIsDebugging)
                {
                    UE_LOG(LogTemp, Error, TEXT("TrafficManager::No location found nearby lane found"));
                }
            }
        }
    }
    else
    {
        if (bIsDebugging)
        {
            UE_LOG(LogTemp, Warning, TEXT("TrafficManager::New agent -- add to map"));
        }

        //Agent is not in map, so we need to add it
        FAgentNavigationData agentNavigationData{};
        float DistanceSqr{};

        if (!m_pZoneGraphSubSystem->FindNearestLocationOnLane(navPath.path[navPath.pathIndex], FBox(CurrentPosition - FVector(AdvanceDistance), CurrentPosition + FVector(AdvanceDistance)), CurrentLaneLocation, DistanceSqr))
        {
            if (bIsDebugging)
            {
                UE_LOG(LogTemp, Error, TEXT("TrafficManager::No valid lane found"));
            }
        }
        agentNavigationData.currentLaneLocation = CurrentLaneLocation;
        agentNavigationData.distanceSqrt = DistanceSqr;

        float agentDistanceAlongLane{ CurrentLaneLocation.DistanceAlongLane };
        agentData.agentDistanceOnSpline = agentDistanceAlongLane;

        UATS_AgentNavigation* pAgentNavigation = pAgent->FindComponentByClass<UATS_AgentNavigation>();
        if (pAgentNavigation == nullptr)
        {
            return FVector::ZeroVector;
        }

        m_MapAgentNavigationData.Add(pAgent, agentNavigationData);

        if (m_pMapZoneShapeAgentContainer.Contains(agentNavigationData.currentLaneLocation.LaneHandle) == false)
        {
            m_pMapZoneShapeAgentContainer.Add(agentNavigationData.currentLaneLocation.LaneHandle, MakeUnique<ATS_ZoneShapeAgentContainer>());
        }
        m_pMapZoneShapeAgentContainer[agentNavigationData.currentLaneLocation.LaneHandle]->RegisterAgent(pAgentNavigation);

        if (bIsDebugging)
        {
            UE_LOG(LogTemp, Warning, TEXT("TrafficManager::Agent added to map"));
        }
    }

    // Calculate the next position along the lane
    FZoneGraphLaneLocation NextLaneLocation; // Could be usefull for more stuff
    if (!m_pZoneGraphSubSystem->AdvanceLaneLocation(CurrentLaneLocation, AdvanceDistance, NextLaneLocation))
    {
        if (bIsDebugging)
        {
            UE_LOG(LogTemp, Error, TEXT("TrafficManager::Could not advance along the lane"));
        }
        return FVector::ZeroVector;
    }
    return NextLaneLocation.Position;
}

TArray<FZoneGraphLaneLocation> AATS_TrafficManager::GetNavigationPointLinkedLane(FZoneGraphLaneLocation currentLaneLocation, float AdvanceDistance)
{
    //Check end of current road if it is an intersection, handle it
    TArray<FZoneGraphLaneLocation> possibleLaneLocation{};
    TArray<FZoneGraphLinkedLane> outgoingLanes;
    if (m_pZoneGraphSubSystem->GetLinkedLanes(currentLaneLocation.LaneHandle, EZoneLaneLinkType::Outgoing, EZoneLaneLinkFlags::All, EZoneLaneLinkFlags::Merging & EZoneLaneLinkFlags::OppositeDirection, outgoingLanes))
    {
        //Adjecent Lanes are possible lanes the car could go to, depending on what the controller want there these are both possible lanes
        FZoneGraphTagMask outgoingLaneTags;
        for (const auto& outgoingLane : outgoingLanes)
        {
            FZoneGraphLaneLocation startPointOfLane{};
            m_pZoneGraphSubSystem->CalculateLocationAlongLane(outgoingLane.DestLane, AdvanceDistance, startPointOfLane);
            if (startPointOfLane.LaneHandle == currentLaneLocation.LaneHandle)
            {
                continue;
            }
            possibleLaneLocation.Add(startPointOfLane);
            continue;
        }
    }
    return possibleLaneLocation;
}

TArray<FZoneGraphLaneLocation> AATS_TrafficManager::GetNavigationPointLinkedLane(FZoneGraphLaneLocation currentLaneLocation)
{
    //Check end of current road if it is an intersection, handle it
    TArray<FZoneGraphLaneLocation> possibleLaneLocation{};
    TArray<FZoneGraphLinkedLane> outgoingLanes;
    float advanceDistace{ 0.f };

    if (m_pZoneGraphSubSystem->GetLinkedLanes(currentLaneLocation.LaneHandle, EZoneLaneLinkType::Outgoing, EZoneLaneLinkFlags::All, EZoneLaneLinkFlags::Merging & EZoneLaneLinkFlags::OppositeDirection, outgoingLanes))
    {
        //Adjecent Lanes are possible lanes the car could go to, depending on what the controller want there these are both possible lanes
        FZoneGraphTagMask outgoingLaneTags;
        for (const auto& outgoingLane : outgoingLanes)
        {
            FZoneGraphLaneLocation startPointOfLane{};
            m_pZoneGraphSubSystem->CalculateLocationAlongLane(outgoingLane.DestLane, advanceDistace, startPointOfLane);
            if (startPointOfLane.LaneHandle == currentLaneLocation.LaneHandle)
            {
                continue;
            }
            possibleLaneLocation.Add(startPointOfLane);
            continue;
        }
    }
    return possibleLaneLocation;
}

void AATS_TrafficManager::AttachTrafficLights()
{
    //Get all traffic lights in level and attach them to the their lights
    if (m_pZoneGraphSubSystem == nullptr)
    {
        if (bIsDebugging)
        {
            UE_LOG(LogTemp, Error, TEXT("TrafficManager::ZoneGraphSubsystem is not available"));
        }
        return;
    }

    //Get all actors of type traffic light
    TArray<AActor*> pArrTrafficLights;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AATS_TrafficLight::StaticClass(), pArrTrafficLights);
    for(auto* pTrafficLightActor : pArrTrafficLights)
	{
		if (pTrafficLightActor == nullptr)
		{
			continue;
		}

        //Cast actor to traffic light
        AATS_TrafficLight* pTrafficLight = Cast<AATS_TrafficLight>(pTrafficLightActor);
        if (pTrafficLight == nullptr)
        {
            continue;
        }

        //Attach traffic light to lane
        FVector trafficLightLocation = pTrafficLightActor->GetActorLocation();
        FZoneGraphLaneLocation closestLane;
        float distanceSqr{};

        if (!m_pZoneGraphSubSystem->FindNearestLane(FBox(trafficLightLocation - FVector(m_SearchDistance), trafficLightLocation + FVector(m_SearchDistance)), FZoneGraphTagFilter(), closestLane, distanceSqr))
        {
            if (bIsDebugging)
            {
                UE_LOG(LogTemp, Error, TEXT("TrafficManager::No nearby lane found for traffic light"));
            }
        }

        m_MapLaneRuler.Add(closestLane.LaneHandle, pTrafficLight);
	}
}

bool AATS_TrafficManager::IsLaneOpenToEnter(const FZoneGraphLaneLocation& lane) const
{
    return true;
}

void AATS_TrafficManager::AgentAwareness()
{
    //This function will make every agent aware of the other agents on the same lane
    //Exact expression of the function is not yet known

    //Loop over all zoneshapes - check if they contain agents > 1
    //If so make them aware of each other    

    //All vehicle zoneshapes
}

FVector AATS_TrafficManager::GetTrafficAwareNavigationPoint(UATS_AgentNavigation* pAgent, FAgentData& agentData, float frontCarDistance)
{
	//This function will make the agent aware of the traffic in front of it
	//It will return a new navigation point based on the traffic in front of it
    if (pAgent == nullptr)
    {
        return FVector::ZeroVector;
    }

    //Based on the distance on lane + front car distance we can calculate if a car is too close to the agent and it should stop
    if (m_MapAgentNavigationData.Contains(pAgent->GetOwner()) == false)
    {
        if (bIsDebugging)
        {
		    UE_LOG(LogTemp, Error, TEXT("TrafficManager::Agent is not in map"));
        }
		return FVector::ZeroVector;
	}

    //Get the current lane and the next lane
    FZoneGraphLaneHandle currentLaneHandle  = m_MapAgentNavigationData[pAgent->GetOwner()].currentLaneLocation.LaneHandle;
    FZoneGraphLaneHandle nextLaneHandle     = m_MapAgentNavigationData[pAgent->GetOwner()].nextLaneLocation.LaneHandle;

    FVector actorLocation = pAgent->GetOwner()->GetActorLocation();
    FVector closestAgent{ FVector::ZeroVector };
    FVector closestObject{ FVector::ZeroVector };

    //-------------------------------------------------------------------------------------------------
    // Current lane check
    //-------------------------------------------------------------------------------------------------

    //Check if the lane is in the map
    if (m_pMapZoneShapeAgentContainer.Contains(currentLaneHandle) == false)
    {
        return FVector::ZeroVector;
    }

    ATS_ZoneShapeAgentContainer* pZoneShapeAgentContainer = m_pMapZoneShapeAgentContainer[currentLaneHandle].Get();
    for (auto* agent : pZoneShapeAgentContainer->GetAgents())
    {
        //Check if agent is the current agent
        if (agent == pAgent)
        {
            continue;
        }

        //Check if agent is enabled
        if (agent->GetAgentData().bIsEnabled == false)
        {
            continue;
        }

        //Check if agent is in front of the current agent
        if (agentData.agentDistanceOnSpline < agent->GetAgentData().agentDistanceOnSpline)
        {
            //Check if agent is too close to the current agent
            if (agentData.agentDistanceOnSpline + frontCarDistance > agent->GetAgentData().agentDistanceOnSpline - agent->GetAgentData().agentBox.Y)
            {
                //Agent is too close, stop
                FZoneGraphLaneLocation agentLaneLocation{};
                m_pZoneGraphSubSystem->CalculateLocationAlongLane(currentLaneHandle, agent->GetAgentData().agentDistanceOnSpline - agent->GetAgentData().agentBox.Y * 4, agentLaneLocation);

                closestAgent = agentLaneLocation.Position;
                break;
                //return agentLaneLocation.Position;
            }
        }
    }

    UATS_TrafficAwarenessComponent* pClosestAgentChangingObject{ nullptr };
    float closestAgentChangingObjectDistance{ MAX_FLT };

    for (auto* object : pZoneShapeAgentContainer->GetTrafficObjects())
    {
        float objectDistanceOnSpline = object->GetDistanceAlongLane();
        float objectlength{ 10 };
        
        //Check if agent is in front of the current agent
        if (object->CanAgentPass())
        {
            //Agent hasn't passed the object yet
            if (objectDistanceOnSpline > agentData.agentDistanceOnSpline)
            {
                continue;
            }

            float distanceToAgent = FMath::Abs(objectDistanceOnSpline - agentData.agentDistanceOnSpline);
            if (distanceToAgent < closestAgentChangingObjectDistance)
            {
				closestAgentChangingObjectDistance = distanceToAgent;
				pClosestAgentChangingObject = object;
			}
            continue;
        }

        if (agentData.agentDistanceOnSpline < objectDistanceOnSpline)
        {
            //Check if agent is too close to the current agent
            if (agentData.agentDistanceOnSpline + frontCarDistance > objectDistanceOnSpline - objectlength)
            {
                //Agent is too close, stop
                FZoneGraphLaneLocation objectLaneLocation{};
                m_pZoneGraphSubSystem->CalculateLocationAlongLane(currentLaneHandle, objectDistanceOnSpline - objectlength * 2, objectLaneLocation);

                closestObject = objectLaneLocation.Position;
                break;
            }
        }
    }

    if (pClosestAgentChangingObject != nullptr)
    {
        pClosestAgentChangingObject->AdjustAgent(pAgent->GetOwner());
    }

    if(closestAgent != FVector::ZeroVector && closestObject == FVector::ZeroVector)
	{
		return closestAgent;
	}
    else if(closestObject != FVector::ZeroVector && closestAgent == FVector::ZeroVector)
    {
        return closestObject;
	}
    else if(closestObject != FVector::ZeroVector && closestAgent != FVector::ZeroVector)
	{
		if(FVector::DistSquared(actorLocation, closestAgent) < FVector::DistSquared(actorLocation, closestObject))
		{
			return closestAgent;
		}
		else
		{
			return closestObject;
		}
	}

    //-------------------------------------------------------------------------------------------------
    // next lane check
    //-------------------------------------------------------------------------------------------------

    if (m_pMapZoneShapeAgentContainer.Contains(nextLaneHandle) == false)
    {
        return FVector::ZeroVector;
    }

    closestAgent    = FVector::ZeroVector;
    closestObject   = FVector::ZeroVector;

    //Get the agent container and loop over all agents
    pZoneShapeAgentContainer = m_pMapZoneShapeAgentContainer[nextLaneHandle].Get();    
    for (auto* agent : pZoneShapeAgentContainer->GetAgents())
    {
        //Check if agent is the current agent
        if (agent == pAgent)
        {
            continue;
        }

        //Check if agent is enabled
        if (agent->GetAgentData().bIsEnabled == false)
        {
            continue;
        }

        //Check if agent is in front of the current agent
        if (agentData.agentDistanceOnSpline < agent->GetAgentData().agentDistanceOnSpline)
        {
            //Check if agent is too close to the current agent
            if (agentData.agentDistanceOnSpline + frontCarDistance > agent->GetAgentData().agentDistanceOnSpline - agent->GetAgentData().agentBox.Y)
            {
                //Agent is too close, stop
                if (bIsDebugging)
                {
                    UE_LOG(LogTemp, Warning, TEXT("TrafficManager::Agent is too close to the agent in front of it"));
                }
                FZoneGraphLaneLocation agentLaneLocation{};
                m_pZoneGraphSubSystem->CalculateLocationAlongLane(nextLaneHandle, agent->GetAgentData().agentDistanceOnSpline - agent->GetAgentData().agentBox.Y * 2, agentLaneLocation);

                closestAgent = agentLaneLocation.Position;
                break;
                //return agentLaneLocation.Position;
            }
        }
    }

    for (auto* object : pZoneShapeAgentContainer->GetTrafficObjects())
    {
        //Check if agent is in front of the current agent
        float objectDistanceOnSpline = object->GetDistanceAlongLane();
        float objectlength{ 10 };

        if (object->CanAgentPass())
        {
            continue;
        }

        if (agentData.agentDistanceOnSpline < objectDistanceOnSpline)
        {
            //Check if agent is too close to the current agent
            if (agentData.agentDistanceOnSpline + frontCarDistance > objectDistanceOnSpline - objectlength)
            {
                //Agent is too close, stop
                FZoneGraphLaneLocation objectLaneLocation{};
                m_pZoneGraphSubSystem->CalculateLocationAlongLane(nextLaneHandle, objectDistanceOnSpline - objectlength * 4, objectLaneLocation);

                closestObject = objectLaneLocation.Position;
                break;
            }
        }
    }

    if (closestAgent != FVector::ZeroVector && closestObject == FVector::ZeroVector)
    {
        return closestAgent;
    }
    else if (closestObject != FVector::ZeroVector && closestAgent == FVector::ZeroVector)
    {
        return closestObject;
    }
    else if (closestObject != FVector::ZeroVector && closestAgent != FVector::ZeroVector)
    {
        if (FVector::DistSquared(actorLocation, closestAgent) < FVector::DistSquared(actorLocation, closestObject))
        {
            return closestAgent;
        }
        else
        {
            return closestObject;
        }
    }

    return FVector::ZeroVector;
}

FVector AATS_TrafficManager::GetPathEndNavigationPoint(UATS_AgentNavigation* pAgent, FAgentData& agentData, FTrafficNavigationPath& navPath, float lookdistance)
{
    if (pAgent == nullptr)
    {
        return FVector::ZeroVector;
    }

    if (navPath.bHasReachedEnd == true || navPath.bIsFollowingPath == false)
    {
        if (bIsDebugging)
        {
            UE_LOG(LogTemp, Error, TEXT("TrafficManager::Agent doesn't need to look to the end"));
        }
        return FVector::ZeroVector;
    }

    if (m_pZoneGraphSubSystem == nullptr)
    {
        if (bIsDebugging)
        {
			UE_LOG(LogTemp, Error, TEXT("TrafficManager::ZoneGraphSubsystem is not available"));
		}
		return FVector::ZeroVector;
	}

    FZoneGraphLaneLocation currentLaneLocation   = m_MapAgentNavigationData[pAgent->GetOwner()].currentLaneLocation;
    FZoneGraphLaneLocation goalLaneLocation      = navPath.endLaneLocation;
    
    float currentLaneLength{};
    m_pZoneGraphSubSystem->GetLaneLength(currentLaneLocation.LaneHandle, currentLaneLength);

    
    if (currentLaneLocation.LaneHandle == goalLaneLocation.LaneHandle)
    {
		//Agent is on the same lane as the goal
        if (agentData.agentDistanceOnSpline + lookdistance >= goalLaneLocation.DistanceAlongLane)
        {
			//Agent is close enough to the goal
			return goalLaneLocation.Position;
		}
	}

    lookdistance = (agentData.agentDistanceOnSpline + lookdistance) - currentLaneLength;
    if (lookdistance > 0)
    {
        if (lookdistance >= goalLaneLocation.DistanceAlongLane)
        {
            return goalLaneLocation.Position;
        }
    }

    //Not on the same lane, check if still in vision    
    return FVector::ZeroVector;
}

FTrafficNavigationPath AATS_TrafficManager::FindPath(FVector currentPosition, FVector goalPosition)
{
    FTrafficNavigationPath navigationPath{};

    if (!m_pZoneGraphSubSystem)
    {
        return navigationPath;
    }

    // Find nearest lanes to StartPoint and EndPoint
    FZoneGraphLaneLocation StartLaneLocation, EndLaneLocation;
    float StartDistanceSqr, EndDistanceSqr;
    float searchDistance = 2500.0f;

    m_pZoneGraphSubSystem->FindNearestLane(FBox(currentPosition - FVector(searchDistance),  currentPosition + FVector(searchDistance)), FZoneGraphTagFilter(), StartLaneLocation,   StartDistanceSqr);
    m_pZoneGraphSubSystem->FindNearestLane(FBox(goalPosition - FVector(searchDistance),     goalPosition + FVector(searchDistance)),    FZoneGraphTagFilter(), EndLaneLocation,     EndDistanceSqr);

    // Ensure valid start and end lane locations
    if (!StartLaneLocation.IsValid())
    {
        if (bIsDebugging)
        {
            UE_LOG(LogTemp, Error, TEXT("TrafficManager::Could not find valid start location"));
        }
        return navigationPath;
    }
    TArray<FZoneGraphLaneLocation> outgoingLanes = GetNavigationPointLinkedLane(StartLaneLocation, 0);
    if (outgoingLanes.IsEmpty())
    {
        if (bIsDebugging)
        {
            UE_LOG(LogTemp, Error, TEXT("TrafficManager::No lanes found to continue on!"));
        }
        navigationPath.startLaneLocation = StartLaneLocation;
    }
    else
    {
        //Log amount of lanes
        if (bIsDebugging)
        {
            UE_LOG(LogTemp, Warning, TEXT("TrafficManager::Amount of lanes: %d"), outgoingLanes.Num());
        }

        //Pick a random lane to continue on
		int randomLane = outgoingLanes.Num() - 1;

        if (outgoingLanes[randomLane].IsValid() == false)
        {
            if (bIsDebugging)
            {
				UE_LOG(LogTemp, Warning, TEXT("TrafficManager::Agent's next lane is not valid"));
			}
			return navigationPath;
		}
		
        FZoneGraphLaneLocation startPointOfLane{};
		m_pZoneGraphSubSystem->CalculateLocationAlongLane(outgoingLanes[randomLane].LaneHandle, 0, startPointOfLane);

        //navigationPath.path.Add(StartLaneLocation.LaneHandle);

		navigationPath.startLaneLocation = StartLaneLocation;
        navigationPath.path.Add(StartLaneLocation.LaneHandle);
        StartLaneLocation = startPointOfLane;
    }

    if (!EndLaneLocation.IsValid())
    {
        if (bIsDebugging)
        {
            UE_LOG(LogTemp, Error, TEXT("TrafficManager::Could not find valid end location"));
        }
        return navigationPath;
    }
    navigationPath.endLaneLocation = EndLaneLocation;

    // Perform pathfinding
    const AZoneGraphData* ZoneGraphData = m_pZoneGraphSubSystem->GetZoneGraphData(StartLaneLocation.LaneHandle.DataHandle);
    if (!ZoneGraphData)
    {
        return navigationPath;
    }

    const FZoneGraphStorage& ZoneGraphStorage = ZoneGraphData->GetStorage();
    FZoneGraphAStarWrapper Graph(ZoneGraphStorage);
    FZoneGraphAStar Pathfinder(Graph);
    
    FZoneGraphAStarNode StartNode(StartLaneLocation.LaneHandle.Index, StartLaneLocation.Position);
    
    FZoneGraphAStarNode EndNode(EndLaneLocation.LaneHandle.Index, EndLaneLocation.Position);

    FZoneGraphPathFilter PathFilter(ZoneGraphStorage, StartLaneLocation, EndLaneLocation);
    
    TArray<FZoneGraphAStarWrapper::FNodeRef> ResultPath;

    Pathfinder.ShouldIncludeStartNodeInPath(false);

    EGraphAStarResult Result = Pathfinder.FindPath(StartNode, EndNode, PathFilter, ResultPath);
    Result = Pathfinder.FindPath(StartNode, EndNode, PathFilter, ResultPath);

    if (Result != EGraphAStarResult::SearchSuccess)
    {
        return navigationPath; // Pathfinding failed
    }

    // Convert the result to a usable path for the agent
    TArray<FVector> PathPoints;
    for (const FZoneGraphAStarWrapper::FNodeRef& NodeRef : ResultPath)
    {
        // Retrieve lane position from NodeRef and add to PathPoints
        FZoneGraphLaneHandle LaneHandle(NodeRef, StartLaneLocation.LaneHandle.DataHandle);
        navigationPath.path.Add(LaneHandle);
    }
    return navigationPath;
}

FAgentPoint AATS_TrafficManager::GetClosestPointOnLane(const FBox& box, const FZoneGraphTagFilter& zoneGraphTagFilter, const FVector& point) const
{
    FZoneGraphLaneLocation closestLaneLocation{};
    float distanceSqr{};
    FAgentPoint agentPoint{};

    if (!m_pZoneGraphSubSystem)
    {
        UE_LOG(LogTemp, Error, TEXT("TrafficManager::GetClosestPointOnLane -- ZoneGraphSubsystem is not available"));
        return agentPoint;
    }

    if (!m_pZoneGraphSubSystem->FindNearestLane(box, zoneGraphTagFilter, closestLaneLocation, distanceSqr))
    {
        UE_LOG(LogTemp, Error, TEXT("TrafficManager::GetClosestPointOnLane -- No nearby lane found"));
        return agentPoint;
    }
	
    agentPoint.position     = closestLaneLocation.Position;
    agentPoint.direction    = closestLaneLocation.Direction;
    
    return agentPoint;
}

void AATS_TrafficManager::UnregisterAgent(AActor* pAgent, UATS_AgentNavigation* pAgentNavigation)
{
    FAgentNavigationData agentNavigationData = m_MapAgentNavigationData[pAgent];
    m_pMapZoneShapeAgentContainer[agentNavigationData.currentLaneLocation.LaneHandle]->UnregisterAgent(pAgentNavigation);

    m_MapAgentNavigationData.Remove(pAgent);
}

FTransform AATS_TrafficManager::GetClosestLanePoint(const FVector& Location, float searchDistance) const
{
    FTransform closestLaneTransform{};
    FZoneGraphLaneLocation closestLaneLocation{};
	float distanceSqr{};

	m_pZoneGraphSubSystem->FindNearestLane(FBox(Location - FVector(searchDistance), Location + FVector(searchDistance)), FZoneGraphTagFilter(), closestLaneLocation, distanceSqr);

    closestLaneTransform.SetLocation(closestLaneLocation.Position);
    closestLaneTransform.SetRotation(closestLaneLocation.Direction.ToOrientationQuat());

	return closestLaneTransform;
}

void AATS_TrafficManager::DrawPath(const TArray<FZoneGraphLaneHandle>& lanes)
{
    TArray<FVector3d> lanePoints{};

    for (const auto& lane : lanes)
    {
		FZoneGraphLaneLocation laneLocation{};
        float laneLenght{};
        m_pZoneGraphSubSystem->GetLaneLength(lane, laneLenght);

		m_pZoneGraphSubSystem->CalculateLocationAlongLane(lane, 0, laneLocation);
        lanePoints.Add(laneLocation.Position);
        
        m_pZoneGraphSubSystem->CalculateLocationAlongLane(lane, laneLenght / 2.f, laneLocation);
		lanePoints.Add(laneLocation.Position);
	}

    //Draw the path
    FColor color = FColor::Red;
    for (int i = 0; i < lanePoints.Num() - 1; i++)
    {
        DrawDebugSphere(GetWorld(), lanePoints[i], 50.f, 10, color, false, 0.f, 0, 5.f);
    }

}

bool AATS_TrafficManager::RegisterTrafficObject(UATS_TrafficAwarenessComponent* pTrafficObject, FVector& connectionPoint, float maxAttachDistance)
{
    if (pTrafficObject == nullptr)
    {
        if (bIsDebugging)
        {
            UE_LOG(LogTemp, Error, TEXT("TrafficManager::RegisterTrafficObject() -- TrafficObject is null"));
        }
		return false;
	}

    if (maxAttachDistance == -1)
    {
		maxAttachDistance = m_SearchDistance;
	}

    float distanceSqr{};
    FZoneGraphLaneLocation closestLane;
    FVector trafficObjectLocation = pTrafficObject->GetOwner()->GetActorLocation();

    if (!m_pZoneGraphSubSystem->FindNearestLane(FBox(trafficObjectLocation - FVector(maxAttachDistance), trafficObjectLocation + FVector(maxAttachDistance)), FZoneGraphTagFilter(), closestLane, distanceSqr))
    {
        if (bIsDebugging)
        {
            UE_LOG(LogTemp, Error, TEXT("TrafficManager::No nearby lane found for traffic light"));
            return false;
        }
    }

    connectionPoint = closestLane.Position;
    pTrafficObject->SetDistanceAlongLane(closestLane.DistanceAlongLane);
    
    if (m_pMapZoneShapeAgentContainer.Contains(closestLane.LaneHandle) == false)
	{
		m_pMapZoneShapeAgentContainer.Add(closestLane.LaneHandle, MakeUnique<ATS_ZoneShapeAgentContainer>());
	}
    m_pMapZoneShapeAgentContainer[closestLane.LaneHandle]->RegisterTrafficObject(pTrafficObject);

    return true;
}

void AATS_TrafficManager::Debugging()
{
    //Debugging traffic on lane
    for (const auto& lane : m_pMapZoneShapeAgentContainer)
    {
        if (lane.Value->GetAgentsCount() == 0)
        {
            continue;
        }

        FColor laneColor = lane.Value->GetLaneColor();

        //Give everylane a different color
        for (const auto& agent : lane.Value->GetAgents())
        {
            DrawDebugBox(GetWorld(), agent->GetAgentData().agentTransform.GetLocation(), agent->GetAgentData().agentBox, laneColor, false, 0.f, 0, 5.f);
        }
    }
}