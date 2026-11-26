#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Inventory/Components/InventoryComponent.h"
#include "Inventory/Data/InventoryTypes.h"
#include "Inventory/Data/ItemDefinition.h"
#include "Inventory/Data/ItemInstanceFragment.h"

namespace InventoryTestHelpers
{
	UItemDefinition* CreateTestItemDef(const FName Id, int32 MaxStackSize, EItemCategory Category = EItemCategory::Misc)
	{
		UItemDefinition* Def = NewObject<UItemDefinition>(GetTransientPackage());
		Def->ItemId = Id;
		Def->DisplayName = FText::FromName(Id);
		Def->MaxStackSize = MaxStackSize;
		Def->Category = Category;
		return Def;
	}

	UInventoryComponent* CreateTestInventory(int32 Capacity)
	{
		UInventoryComponent* Inv = NewObject<UInventoryComponent>(GetTransientPackage());
		Inv->InitializeCapacity(Capacity);
		return Inv;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInventoryAddAndStackTest,
	"Empty.Inventory.AddAndStack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FInventoryAddAndStackTest::RunTest(const FString& Parameters)
{
	UItemDefinition* WoodDef = InventoryTestHelpers::CreateTestItemDef(TEXT("Wood"), 10, EItemCategory::Resource);
	UInventoryComponent* Inv = InventoryTestHelpers::CreateTestInventory(2);

	// Add 7 -> fills slot 0 with 7.
	FInventoryAddResult Res1 = Inv->TryAddItem(FInventoryItem::MakeFromDefinition(WoodDef, 7));
	TestEqual(TEXT("First add added 7"), Res1.AddedQuantity, 7);
	TestEqual(TEXT("First add remainder 0"), Res1.RemainderQuantity, 0);
	TestEqual(TEXT("Slot 0 has 7"), Inv->GetSlot(0).Item.Quantity, 7);
	TestTrue(TEXT("Slot 1 still empty"), Inv->IsSlotEmpty(1));

	// Add 8 -> fills slot 0 to 10 (+3) and puts 5 in slot 1.
	FInventoryAddResult Res2 = Inv->TryAddItem(FInventoryItem::MakeFromDefinition(WoodDef, 8));
	TestEqual(TEXT("Second add added 8"), Res2.AddedQuantity, 8);
	TestEqual(TEXT("Second add remainder 0"), Res2.RemainderQuantity, 0);
	TestEqual(TEXT("Slot 0 topped up to 10"), Inv->GetSlot(0).Item.Quantity, 10);
	TestEqual(TEXT("Slot 1 received 5"), Inv->GetSlot(1).Item.Quantity, 5);

	// Add 10 -> only 5 fit in slot 1, remainder 5.
	FInventoryAddResult Res3 = Inv->TryAddItem(FInventoryItem::MakeFromDefinition(WoodDef, 10));
	TestEqual(TEXT("Third add added 5"), Res3.AddedQuantity, 5);
	TestEqual(TEXT("Third add remainder 5"), Res3.RemainderQuantity, 5);
	TestEqual(TEXT("Total Wood is 20"), Inv->GetTotalQuantityOfItem(WoodDef), 20);
	TestFalse(TEXT("Inventory full for Wood"), Inv->CanAcceptItem(FInventoryItem::MakeFromDefinition(WoodDef, 1)));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInventoryNonStackableAndFragmentsTest,
	"Empty.Inventory.NonStackableAndFragments",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FInventoryNonStackableAndFragmentsTest::RunTest(const FString& Parameters)
{
	UItemDefinition* SwordDef = InventoryTestHelpers::CreateTestItemDef(TEXT("Sword"), 1, EItemCategory::Equipment);
	UItemDefinition* PotionDef = InventoryTestHelpers::CreateTestItemDef(TEXT("Potion"), 5, EItemCategory::Consumable);
	UInventoryComponent* Inv = InventoryTestHelpers::CreateTestInventory(4);

	// Non-stackable items occupy separate slots.
	FInventoryAddResult SwordRes = Inv->TryAddItem(FInventoryItem::MakeFromDefinition(SwordDef, 2));
	TestEqual(TEXT("2 swords added"), SwordRes.AddedQuantity, 2);
	TestEqual(TEXT("Slot 0 has 1 sword"), Inv->GetSlot(0).Item.Quantity, 1);
	TestEqual(TEXT("Slot 1 has 1 sword"), Inv->GetSlot(1).Item.Quantity, 1);

	// Stackable item without fragment vs with fragment should NOT merge together.
	FInventoryItem PlainPotion = FInventoryItem::MakeFromDefinition(PotionDef, 2);
	FInventoryItem SpecialPotion = FInventoryItem::MakeFromDefinition(PotionDef, 2);
	SpecialPotion.FindOrAddFragment<FItemInstanceFragment>();

	Inv->TryAddItem(PlainPotion);
	Inv->TryAddItem(SpecialPotion);

	TestEqual(TEXT("Plain potion in slot 2"), Inv->GetSlot(2).Item.Quantity, 2);
	TestEqual(TEXT("Special potion separated in slot 3"), Inv->GetSlot(3).Item.Quantity, 2);
	TestFalse(TEXT("Plain and Special cannot stack"), PlainPotion.CanStackWith(SpecialPotion));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInventoryTransferAndQuickTransferTest,
	"Empty.Inventory.TransferAndQuickTransfer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FInventoryTransferAndQuickTransferTest::RunTest(const FString& Parameters)
{
	UItemDefinition* StoneDef = InventoryTestHelpers::CreateTestItemDef(TEXT("Stone"), 10, EItemCategory::Resource);
	UItemDefinition* GemDef = InventoryTestHelpers::CreateTestItemDef(TEXT("Gem"), 5, EItemCategory::Misc);

	UInventoryComponent* PlayerInv = InventoryTestHelpers::CreateTestInventory(4);
	UInventoryComponent* ChestInv = InventoryTestHelpers::CreateTestInventory(2);

	PlayerInv->TryAddItem(FInventoryItem::MakeFromDefinition(StoneDef, 8)); // Slot 0: 8 Stone
	PlayerInv->TryAddItem(FInventoryItem::MakeFromDefinition(GemDef, 3));   // Slot 1: 3 Gem

	// Split 3 Stone from slot 0 into empty slot 2.
	TestTrue(TEXT("Split into empty slot"), UInventoryComponent::TransferSlot(PlayerInv, 0, PlayerInv, 2, 3));
	TestEqual(TEXT("Slot 0 has 5 Stone"), PlayerInv->GetSlot(0).Item.Quantity, 5);
	TestEqual(TEXT("Slot 2 has 3 Stone"), PlayerInv->GetSlot(2).Item.Quantity, 3);

	// Partial split onto different item is rejected.
	TestFalse(TEXT("Reject partial split onto different item"), UInventoryComponent::TransferSlot(PlayerInv, 0, PlayerInv, 1, 2));

	// Full move onto different item swaps them.
	TestTrue(TEXT("Swap Stone and Gem"), UInventoryComponent::TransferSlot(PlayerInv, 0, PlayerInv, 1));
	TestTrue(TEXT("Slot 0 now has Gem"), PlayerInv->GetSlot(0).Item.Definition == GemDef);
	TestTrue(TEXT("Slot 1 now has Stone"), PlayerInv->GetSlot(1).Item.Definition == StoneDef);

	// Merge slot 2 (3 Stone) into slot 1 (5 Stone) -> slot 1 has 8, slot 2 empty.
	TestTrue(TEXT("Merge compatible stacks"), UInventoryComponent::TransferSlot(PlayerInv, 2, PlayerInv, 1));
	TestEqual(TEXT("Slot 1 merged to 8 Stone"), PlayerInv->GetSlot(1).Item.Quantity, 8);
	TestTrue(TEXT("Slot 2 now empty"), PlayerInv->IsSlotEmpty(2));

	// Quick transfer 8 Stone from Player to Chest.
	FInventoryAddResult QuickRes = UInventoryComponent::QuickTransferSlot(PlayerInv, 1, ChestInv);
	TestEqual(TEXT("Quick transferred 8 Stone"), QuickRes.AddedQuantity, 8);
	TestTrue(TEXT("Player slot 1 empty after quick transfer"), PlayerInv->IsSlotEmpty(1));
	TestEqual(TEXT("Chest slot 0 has 8 Stone"), ChestInv->GetSlot(0).Item.Quantity, 8);

	// Cross-inventory partial merge back from Chest into Player slot 0? Put 6 Stone in Player slot 2, then merge Chest's 8 -> fills to 10, leaves 4 in Chest.
	PlayerInv->TryAddItem(FInventoryItem::MakeFromDefinition(StoneDef, 6)); // goes into first empty slot (slot 1)
	TestTrue(TEXT("Cross-inventory partial merge"), UInventoryComponent::TransferSlot(ChestInv, 0, PlayerInv, 1));
	TestEqual(TEXT("Player slot 1 topped to 10"), PlayerInv->GetSlot(1).Item.Quantity, 10);
	TestEqual(TEXT("Chest slot 0 has 4 remainder"), ChestInv->GetSlot(0).Item.Quantity, 4);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
