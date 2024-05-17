// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ATS_SpatialPartitioning.generated.h"

/*
	This class is responsible for spatial partitioning of the world. It will be used to divide the world into smaller cells
	Each cell will contain a list of actors that are present in that cell. This will be used to optimize the search for nearby actors
*/

USTRUCT(BlueprintType)
struct FCell
{
	GENERATED_BODY()

	FCell() : _x(0), _y(0) {}
	FCell(int x, int y)
		: _x(x)
		, _y(y)
	{

	}

	bool operator==(const FCell& Other) const
	{
		return _x == Other._x && _y == Other._y;
	}

	int _x{ 0 };
	int _y{ 0 };
};

FORCEINLINE uint32 GetTypeHash(const FCell& Cell)
{
	return HashCombine(GetTypeHash(Cell._x), GetTypeHash(Cell._y));
}

class UATS_SpatialUnit;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ADVANCEDTRAFFICSYSTEM_API UATS_SpatialGrid : public UActorComponent
{
	GENERATED_BODY()

public:	
	UATS_SpatialGrid();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;


public:
	void Add(UATS_SpatialUnit* pSpatialUnit);
	void Remove(UATS_SpatialUnit* pSpatialUnit);

	void AddToCell(const FCell& cell, UATS_SpatialUnit* pSpatialUnit);
	void RemoveFromCell(const FCell& cell, UATS_SpatialUnit* pSpatialUnit);

	TArray<UATS_SpatialUnit*> GetCellActors(const FVector2D& location);

	void Move(UATS_SpatialUnit* pSpatialUnit, const FVector2D& oldLocation, const FVector2D& newLocation);

protected:
	UPROPERTY(EditAnywhere, Category = "Grid Settings")
	int _MaxCellCount{ -1 }; // -1 means no limit

	UPROPERTY(EditAnywhere, Category = "Grid Settings")
	float _CellSize{};

	TMap<FCell, TArray<UATS_SpatialUnit*>> _cells{};
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ADVANCEDTRAFFICSYSTEM_API UATS_SpatialUnit : public UActorComponent
{
	GENERATED_BODY()

public:
	UATS_SpatialUnit();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

public:
	FVector2D Get2DLocation() const;
	TArray<AActor*> GetNearbyActors();

	void SetIsMoveable(bool isMoveable);

protected:
	UATS_SpatialGrid* _pSpatialGrid{ nullptr };

	bool _bIsMoveable{ true };
	FVector2D _prev2DLocation{ FVector2D::ZeroVector };

	UPROPERTY(EditAnywhere, Category = "Grid Settings")
	AActor* _pSpatialGridActor{};
};
