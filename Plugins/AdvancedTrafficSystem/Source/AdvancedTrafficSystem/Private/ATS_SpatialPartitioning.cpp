// Fill out your copyright notice in the Description page of Project Settings.
#include "../Public/ATS_SpatialPartitioning.h"

#include "Kismet/GameplayStatics.h"

//-----------------------------------------------------------------------------------------------
// UATS_SpatialGrid
//-----------------------------------------------------------------------------------------------
UATS_SpatialGrid::UATS_SpatialGrid()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UATS_SpatialGrid::BeginPlay()
{
	Super::BeginPlay();
}


void UATS_SpatialGrid::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UATS_SpatialGrid::Add(UATS_SpatialUnit* pSpatialUnit)
{
	// Determine the cell in which the spatial unit is present
	FVector2D location = pSpatialUnit->Get2DLocation();

	// Calculate the cell index
	int x = int(location.X / _CellSize);
	int y = int(location.Y / _CellSize);

	if (_MaxCellCount != -1)
	{
		if (x >= _MaxCellCount)
	{
		x = _MaxCellCount - 1;
	}
		if (y >= _MaxCellCount)
	{
		y = _MaxCellCount - 1;
	}
	}

	// Add the spatial unit to the cell
	FCell cell(x, y);

	AddToCell(cell, pSpatialUnit);
}

void UATS_SpatialGrid::AddToCell(const FCell& cell, UATS_SpatialUnit* pSpatialUnit)
{
	//Check if the cells is already present in the map
	if (_cells.Contains(cell) == false)
	{
		_cells.Add(cell, TArray<UATS_SpatialUnit*>());
	}

	//Add the unit to the cell
	_cells[cell].Add(pSpatialUnit);
}

void UATS_SpatialGrid::Remove(UATS_SpatialUnit* pSpatialUnit)
{
	// Determine the cell in which the spatial unit is present
	FVector2D location = pSpatialUnit->Get2DLocation();

	// Calculate the cell index
	int x = int(location.X / _CellSize);
	int y = int(location.Y / _CellSize);

	if (_MaxCellCount != -1)
	{
		if (x >= _MaxCellCount)
		{
			x = _MaxCellCount - 1;
		}
		if (y >= _MaxCellCount)
		{
			y = _MaxCellCount - 1;
		}
	}

	FCell cell(x, y);

	RemoveFromCell(cell, pSpatialUnit);
}

void UATS_SpatialGrid::RemoveFromCell(const FCell& cell, UATS_SpatialUnit* pSpatialUnit)
{
	if (_cells.Contains(cell))
	{
		_cells[cell].Remove(pSpatialUnit);
	}
}


TArray<UATS_SpatialUnit*> UATS_SpatialGrid::GetCellActors(const FVector2D& location)
{
	// Calculate the cell index
	int x = int(location.X / _CellSize);
	int y = int(location.Y / _CellSize);

	if (_MaxCellCount != -1)
	{
		if (x >= _MaxCellCount)
		{
			x = _MaxCellCount - 1;
		}
		if (y >= _MaxCellCount)
		{
			y = _MaxCellCount - 1;
		}
	}

	FCell cell(x, y);

	//Check if the cells is already present in the map
	if (_cells.Contains(cell) == false)
	{
		return TArray<UATS_SpatialUnit*>();
	}

	return _cells[cell];
}

void UATS_SpatialGrid::Move(UATS_SpatialUnit* pSpatialUnit, const FVector2D& oldLocation, const FVector2D& newLocation)
{
	// Get the old cell index	
	int oldX = int(oldLocation.X / _CellSize);
	int oldY = int(oldLocation.Y / _CellSize);

	if (_MaxCellCount != -1)
	{
		if (oldX >= _MaxCellCount)
		{
			oldX = _MaxCellCount - 1;
		}
		if (oldY >= _MaxCellCount)
		{
			oldY = _MaxCellCount - 1;
		}
	}

	// Get the new cell index
	int newX = int(newLocation.X / _CellSize);
	int newY = int(newLocation.Y / _CellSize);

	if (_MaxCellCount != -1)
	{
		if (newX >= _MaxCellCount)
		{
			newX = _MaxCellCount - 1;
		}
		if (newY >= _MaxCellCount)
		{
			newY = _MaxCellCount - 1;
		}
	}
	
	// If the cell has changed, update the cell
	if (oldX != newX || oldY != newY)
	{
		FCell oldCell(oldX, oldY);
		FCell newCell(newX, newY);

		// Remove the spatial unit from the old cell
		RemoveFromCell(oldCell, pSpatialUnit);
		AddToCell(newCell, pSpatialUnit);
	}
}



//-----------------------------------------------------------------------------------------------
// UATS_SpatialUnit
//-----------------------------------------------------------------------------------------------

UATS_SpatialUnit::UATS_SpatialUnit()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UATS_SpatialUnit::BeginPlay()
{
	Super::BeginPlay();
	
	// Get the initial location of the actor
	_prev2DLocation = FVector2D(GetOwner()->GetActorLocation().X, GetOwner()->GetActorLocation().Y);

	// Find the actor that has the spatial grid component
	if (_pSpatialGridActor == nullptr)
	{
		//Search for the actor that has the spatial grid component
		TArray<AActor*> actors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), actors);
		for (AActor* actor : actors)
		{
			if (actor->FindComponentByClass<UATS_SpatialGrid>())
			{
				_pSpatialGridActor = actor;
				break;
			}
		}

		if (_pSpatialGridActor == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("No actor with spatial grid component found"));
			return;
		}
	}

	//Register this spatial unit with the spatial grid
	_pSpatialGrid = _pSpatialGridActor->FindComponentByClass<UATS_SpatialGrid>();
	if (_pSpatialGrid == nullptr )
	{
		UE_LOG(LogTemp, Error, TEXT("No spatial grid component found"));
		return;
	}
	
	_pSpatialGrid->Add(this);
}


void UATS_SpatialUnit::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (_bIsMoveable == false)
	{
		return;
	}

	// Check if the position has changed -- if so update the spatial grid -- we also only care if the actor has moved in the XY plane, Z-plane(up_down) doesn't matter
	FVector currentLocation = GetOwner()->GetActorLocation();
	if (_prev2DLocation != FVector2D(currentLocation.X, currentLocation.Y))
	{
		//TODO UPDATE GRID
		_prev2DLocation = FVector2D(currentLocation.X, currentLocation.Y);
	}
}

FVector2D UATS_SpatialUnit::Get2DLocation() const
{
	return FVector2D(GetOwner()->GetActorLocation().X, GetOwner()->GetActorLocation().Y);
}

TArray<AActor*> UATS_SpatialUnit::GetNearbyActors()
{
	TArray<AActor*> nearbyActors;

	// Get the nearby cells
	TArray<UATS_SpatialUnit*> nearbySpatialUnits = _pSpatialGrid->GetCellActors(Get2DLocation());

	// Get the actors from the spatial units
	for (UATS_SpatialUnit* pSpatialUnit : nearbySpatialUnits)
	{
		nearbyActors.Add(pSpatialUnit->GetOwner());
	}

	return nearbyActors;
}

void UATS_SpatialUnit::SetIsMoveable(bool isMoveable)
{
	_bIsMoveable = isMoveable;
}