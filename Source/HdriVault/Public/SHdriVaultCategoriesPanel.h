// Copyright Pyre Labs 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SSplitter.h"
#include "HdriVaultTypes.h"

class UHdriVaultManager;

/**
 * Category item structure for the category tree
 */
struct FHdriVaultCategoryItem
{
	FString CategoryName;
	TArray<TSharedPtr<FHdriVaultMaterialItem>> Materials;
	TArray<TSharedPtr<FHdriVaultCategoryItem>> Children;
	TSharedPtr<FHdriVaultCategoryItem> Parent;
	bool bIsExpanded = false;

	FHdriVaultCategoryItem(const FString& InCategoryName)
		: CategoryName(InCategoryName)
		, Parent(nullptr)
		, bIsExpanded(false)
	{
	}
};

/**
 * Table row widget for category items
 */
class SHdriVaultCategoryTreeItem : public STableRow<TSharedPtr<FHdriVaultCategoryItem>>
{
public:
	SLATE_BEGIN_ARGS(SHdriVaultCategoryTreeItem) {}
		SLATE_ARGUMENT(TSharedPtr<FHdriVaultCategoryItem>, CategoryItem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);

private:
	TSharedPtr<FHdriVaultCategoryItem> CategoryItem;

	FText GetCategoryName() const;
	FText GetMaterialCount() const;
	const FSlateBrush* GetCategoryIcon() const;
};

/**
 * Categories panel widget for HdriVault
 */
class HDRIVAULT_API SHdriVaultCategoriesPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SHdriVaultCategoriesPanel) {}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	// Category management
	void RefreshCategories();
	void SetFilterText(const FString& FilterText);
	
	// Tag management
	void RefreshTags();
	
	// Selection
	TSharedPtr<FHdriVaultCategoryItem> GetSelectedCategory() const;
	void SetSelectedCategory(TSharedPtr<FHdriVaultCategoryItem> Category);
	void SetSelectedCategoryByName(const FString& CategoryName);
	void SetSelectedTag(const FString& TagName);

	// Delegates
	DECLARE_DELEGATE_OneParam(FOnCategorySelected, TSharedPtr<FHdriVaultCategoryItem>);
	DECLARE_DELEGATE_OneParam(FOnTagSelected, FString);
	
	FOnCategorySelected OnCategorySelected;
	FOnTagSelected OnTagSelected;

private:
	// Tree view
	TSharedPtr<STreeView<TSharedPtr<FHdriVaultCategoryItem>>> CategoryTreeView;
	
	// Tags view
	TSharedPtr<SListView<TSharedPtr<FString>>> TagsListView;
	
	// Data
	TArray<TSharedPtr<FHdriVaultCategoryItem>> RootCategories;
	TArray<TSharedPtr<FHdriVaultCategoryItem>> FilteredCategories;
	TSharedPtr<FHdriVaultCategoryItem> SelectedCategory;
	
	// Tags data
	TArray<TSharedPtr<FString>> AllTags;
	TArray<TSharedPtr<FString>> FilteredTags;
	
	// Filtering
	FString CurrentFilterText;
	
	// Manager reference
	UHdriVaultManager* HdriVaultManager;

	// Tree view callbacks
	TSharedRef<ITableRow> OnGenerateCategoryWidget(TSharedPtr<FHdriVaultCategoryItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void OnCategorySelectionChanged(TSharedPtr<FHdriVaultCategoryItem> SelectedItem, ESelectInfo::Type SelectInfo);
	void OnCategoryExpansionChanged(TSharedPtr<FHdriVaultCategoryItem> Item, bool bIsExpanded);
	void OnGetCategoryChildren(TSharedPtr<FHdriVaultCategoryItem> Item, TArray<TSharedPtr<FHdriVaultCategoryItem>>& OutChildren);
	TSharedPtr<SWidget> OnCategoryContextMenuOpening();

	// Filter callbacks
	void OnFilterTextChanged(const FText& FilterText);

	// Category building
	void BuildCategoryStructure();
	TSharedPtr<FHdriVaultCategoryItem> GetOrCreateCategory(const FString& CategoryName);
	void AddMaterialToCategory(TSharedPtr<FHdriVaultMaterialItem> Material, const FString& CategoryName);
	
	// Category operations
	void OnDeleteCategory(TSharedPtr<FHdriVaultCategoryItem> CategoryToDelete);
	void DeleteCategoryRecursive(TSharedPtr<FHdriVaultCategoryItem> Category);

	// UI creation
	TSharedRef<SWidget> CreateTagsPanel();
	
	// Tags functionality
	TSharedRef<ITableRow> OnGenerateTagWidget(TSharedPtr<FString> TagItem, const TSharedRef<STableViewBase>& OwnerTable);
	void OnTagSelectionChanged(TSharedPtr<FString> SelectedTag, ESelectInfo::Type SelectInfo);
	TSharedPtr<SWidget> OnTagContextMenuOpening();
	void OnDeleteTag(TSharedPtr<FString> TagToDelete);
	
	// Filtering
	void ApplyFilter();
	bool DoesCategoryPassFilter(TSharedPtr<FHdriVaultCategoryItem> Category) const;
	bool HasFilteredChildren(TSharedPtr<FHdriVaultCategoryItem> Category) const;
}; 