#include "Inventory/Data/InventoryTypes.h"

FInventoryItem FInventoryItem::MakeFromDefinition(const UItemDefinition* InDefinition, int32 InQuantity)
{
	FInventoryItem Result;
	if (!InDefinition || InQuantity <= 0)
	{
		return Result;
	}

	Result.Definition = InDefinition;
	Result.Quantity = InQuantity;
	Result.InstanceFragments = InDefinition->DefaultInstanceFragments;
	return Result;
}

bool FInventoryItem::CanStackWith(const FInventoryItem& Other) const
{
	if (!IsValid() || !Other.IsValid())
	{
		return false;
	}

	if (Definition != Other.Definition || !Definition->IsStackable())
	{
		return false;
	}

	return InstanceFragments == Other.InstanceFragments;
}

FInventoryItem FInventoryItem::Split(int32 SplitQuantity)
{
	FInventoryItem SplitStack;
	if (!IsValid() || SplitQuantity <= 0)
	{
		return SplitStack;
	}

	const int32 ClampedAmount = FMath::Min(SplitQuantity, Quantity);
	SplitStack.Definition = Definition;
	SplitStack.Quantity = ClampedAmount;
	SplitStack.InstanceFragments = InstanceFragments;

	Quantity -= ClampedAmount;
	if (Quantity <= 0)
	{
		Reset();
	}

	return SplitStack;
}

void FInventoryItem::Reset()
{
	Definition = nullptr;
	Quantity = 0;
	InstanceFragments.Reset();
}
