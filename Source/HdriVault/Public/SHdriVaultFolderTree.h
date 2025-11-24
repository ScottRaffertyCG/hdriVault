// Copyright Pyre Labs. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "HdriVaultTypes.h"

class UHdriVaultManager;

/**
 * Tree row widget for folder items
 */
class SHdriVaultFolderTreeItem : public STableRow<TSharedPtr<FHdriVaultFolderNode>>
{
public:
	SLATE_BEGIN_ARGS(SHdriVaultFolderTreeItem) {}
		SLATE_ARGUMENT(TSharedPtr<FHdriVaultFolderNode>, FolderNode)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);

private:
	TSharedPtr<FHdriVaultFolderNode> FolderNode;
	
	const FSlateBrush* GetFolderIcon() const;
	FText GetFolderText() const;
	FText GetFolderTooltip() const;
	FSlateColor GetFolderTextColor() const;
};

/**
 * Folder tree widget for HdriVault
 */
class HDRIVAULT_API SHdriVaultFolderTree : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SHdriVaultFolderTree) {}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	// Public interface
	void RefreshTree();
	void SetSelectedFolder(TSharedPtr<FHdriVaultFolderNode> Folder);
	TSharedPtr<FHdriVaultFolderNode> GetSelectedFolder() const;
	void ExpandFolder(TSharedPtr<FHdriVaultFolderNode> Folder);
	void CollapseFolder(TSharedPtr<FHdriVaultFolderNode> Folder);

	// Delegates
	DECLARE_DELEGATE_OneParam(FOnFolderSelected, TSharedPtr<FHdriVaultFolderNode>);
	FOnFolderSelected OnFolderSelected;

private:
	// Tree view widget
	TSharedPtr<STreeView<TSharedPtr<FHdriVaultFolderNode>>> TreeView;
	
	// Data
	TArray<TSharedPtr<FHdriVaultFolderNode>> RootNodes;
	TSharedPtr<FHdriVaultFolderNode> SelectedFolder;
	
	// Manager reference
	UHdriVaultManager* HdriVaultManager;
	
	// Tree view callbacks
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FHdriVaultFolderNode> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void OnGetChildren(TSharedPtr<FHdriVaultFolderNode> Item, TArray<TSharedPtr<FHdriVaultFolderNode>>& OutChildren);
	void OnSelectionChanged(TSharedPtr<FHdriVaultFolderNode> SelectedItem, ESelectInfo::Type SelectInfo);
	void OnExpansionChanged(TSharedPtr<FHdriVaultFolderNode> Item, bool bExpanded);
	void OnFolderDoubleClick(TSharedPtr<FHdriVaultFolderNode> Item);
	
	// Context menu
	TSharedPtr<SWidget> OnContextMenuOpening();
	void OnCreateFolder();
	void OnRenameFolder();
	void OnDeleteFolder();
	void OnRefreshFolder();
	
	// Helper functions
	void BuildTreeFromManager();
	void ExpandDefaultFolders();
	void ScrollToFolder(TSharedPtr<FHdriVaultFolderNode> Folder);
	bool DoesNodeMatchFilter(TSharedPtr<FHdriVaultFolderNode> Node, const FString& FilterText) const;
	void StoreExpandedFolders(const TArray<TSharedPtr<FHdriVaultFolderNode>>& Folders, TSet<FString>& OutExpandedFolders);
	void RestoreExpandedFolders(const TArray<TSharedPtr<FHdriVaultFolderNode>>& Folders, const TSet<FString>& ExpandedFolders);
	
	// Manager event handlers
	void OnManagerRefreshRequested();
	
	// Filter support
	FString CurrentFilterText;
	void SetFilterText(const FString& FilterText);
	void ApplyFilter();
}; 