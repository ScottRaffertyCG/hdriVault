// Copyright Pyre Labs 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STileView.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "AssetThumbnail.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "HdriVaultTypes.h"

class UHdriVaultManager;

/**
 * Tile widget for material items in grid view
 */
class SHdriVaultMaterialTile : public STableRow<TSharedPtr<FHdriVaultMaterialItem>>
{
public:
	SLATE_BEGIN_ARGS(SHdriVaultMaterialTile) {}
		SLATE_ARGUMENT(TSharedPtr<FHdriVaultMaterialItem>, MaterialItem)
		SLATE_ARGUMENT(float, ThumbnailSize)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);

	// SWidget interface
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	// Delegates
	DECLARE_DELEGATE_OneParam(FOnMaterialClicked, TSharedPtr<FHdriVaultMaterialItem>);
	DECLARE_DELEGATE_OneParam(FOnMaterialDoubleClicked, TSharedPtr<FHdriVaultMaterialItem>);
	DECLARE_DELEGATE_RetVal_ThreeParams(FReply, FOnMaterialDragDetected, TSharedPtr<FHdriVaultMaterialItem>, const FGeometry&, const FPointerEvent&);
	
	FOnMaterialClicked OnMaterialLeftClicked;
	FOnMaterialClicked OnMaterialRightClicked;
	FOnMaterialClicked OnMaterialMiddleClicked;
	FOnMaterialDoubleClicked OnMaterialDoubleClicked;
	FOnMaterialDragDetected OnMaterialDragDetected;

private:
	TSharedPtr<FHdriVaultMaterialItem> MaterialItem;
	TSharedPtr<FAssetThumbnail> AssetThumbnail;
	float ThumbnailSize;

	// UI helpers
	FText GetMaterialName() const;
	FText GetMaterialTooltip() const;
	EVisibility GetLoadingVisibility() const;
	
	// Thumbnail helpers
	void RefreshThumbnail();
};

/**
 * List row widget for material items in list view
 */
class SHdriVaultMaterialListItem : public STableRow<TSharedPtr<FHdriVaultMaterialItem>>
{
public:
	SLATE_BEGIN_ARGS(SHdriVaultMaterialListItem) {}
		SLATE_ARGUMENT(TSharedPtr<FHdriVaultMaterialItem>, MaterialItem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);

	// SWidget interface
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	// Delegates
	DECLARE_DELEGATE_OneParam(FOnMaterialClicked, TSharedPtr<FHdriVaultMaterialItem>);
	DECLARE_DELEGATE_OneParam(FOnMaterialDoubleClicked, TSharedPtr<FHdriVaultMaterialItem>);
	
	FOnMaterialClicked OnMaterialLeftClicked;
	FOnMaterialClicked OnMaterialRightClicked;
	FOnMaterialDoubleClicked OnMaterialDoubleClicked;
	SHdriVaultMaterialTile::FOnMaterialDragDetected OnMaterialDragDetected;

private:
	TSharedPtr<FHdriVaultMaterialItem> MaterialItem;
	TSharedPtr<FAssetThumbnail> AssetThumbnail;

	// UI helpers
	FText GetMaterialName() const;
	FText GetMaterialType() const;
	FText GetMaterialPath() const;
	FText GetMaterialTooltip() const;
};

/**
 * Material grid widget for HdriVault
 */
class HDRIVAULT_API SHdriVaultMaterialGrid : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SHdriVaultMaterialGrid) {}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	// SWidget interface
	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

	// Public interface
	void RefreshGrid();
	void SetMaterials(const TArray<TSharedPtr<FHdriVaultMaterialItem>>& InMaterials);
	void SetSelectedMaterial(TSharedPtr<FHdriVaultMaterialItem> Material);
	TSharedPtr<FHdriVaultMaterialItem> GetSelectedMaterial() const;
	void SetViewMode(EHdriVaultViewMode InViewMode);
	void SetThumbnailSize(float InThumbnailSize);
	void ClearSelection();
	void SetFolder(const FString& FolderPath);

	// Search and filtering
	void SetFilterText(const FString& FilterText);
	void ApplyFilters();

	// Delegates
	DECLARE_DELEGATE_OneParam(FOnMaterialSelected, TSharedPtr<FHdriVaultMaterialItem>);
	DECLARE_DELEGATE_OneParam(FOnMaterialDoubleClicked, TSharedPtr<FHdriVaultMaterialItem>);
	DECLARE_DELEGATE_OneParam(FOnMaterialApplied, TSharedPtr<FHdriVaultMaterialItem>);
	
	FOnMaterialSelected OnMaterialSelected;
	FOnMaterialDoubleClicked OnMaterialDoubleClicked;
	FOnMaterialApplied OnMaterialApplied;

private:
	// View widgets
	TSharedPtr<STileView<TSharedPtr<FHdriVaultMaterialItem>>> TileView;
	TSharedPtr<SListView<TSharedPtr<FHdriVaultMaterialItem>>> ListView;
	TSharedPtr<SBorder> ViewContainer;

	// Data
	TArray<TSharedPtr<FHdriVaultMaterialItem>> AllMaterials;
	TArray<TSharedPtr<FHdriVaultMaterialItem>> FilteredMaterials;
	TSharedPtr<FHdriVaultMaterialItem> SelectedMaterial;

	// Settings
	EHdriVaultViewMode ViewMode;
	float ThumbnailSize;
	FString CurrentFilterText;

	// Manager reference
	UHdriVaultManager* HdriVaultManager;

	// Thumbnail management
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool;

	// View creation
	TSharedRef<SWidget> CreateTileView();
	TSharedRef<SWidget> CreateListView();
	void SwitchToViewMode(EHdriVaultViewMode NewViewMode);

	// Tile view callbacks
	TSharedRef<ITableRow> OnGenerateTileWidget(TSharedPtr<FHdriVaultMaterialItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void OnTileSelectionChanged(TSharedPtr<FHdriVaultMaterialItem> SelectedItem, ESelectInfo::Type SelectInfo);
	
	// List view callbacks
	TSharedRef<ITableRow> OnGenerateListWidget(TSharedPtr<FHdriVaultMaterialItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void OnListSelectionChanged(TSharedPtr<FHdriVaultMaterialItem> SelectedItem, ESelectInfo::Type SelectInfo);

	// Material interaction callbacks
	void OnMaterialLeftClicked(TSharedPtr<FHdriVaultMaterialItem> Material);
	void OnMaterialRightClicked(TSharedPtr<FHdriVaultMaterialItem> Material);
	void OnMaterialMiddleClicked(TSharedPtr<FHdriVaultMaterialItem> Material);
	void OnMaterialDoubleClickedInternal(TSharedPtr<FHdriVaultMaterialItem> Material);
	FReply HandleMaterialDragDetected(TSharedPtr<FHdriVaultMaterialItem> Material, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	void GatherDragMaterials(TArray<TSharedPtr<FHdriVaultMaterialItem>>& OutMaterials, TSharedPtr<FHdriVaultMaterialItem> PrimaryItem) const;

	// Context menu
	TSharedPtr<SWidget> OnContextMenuOpening();
	void OnApplyMaterial();
	void OnBrowseToMaterial();
	void OnCopyMaterialPath();
	void OnEditMaterialMetadata();

	// Filtering
	void UpdateFilteredMaterials();
	bool DoesItemPassFilter(TSharedPtr<FHdriVaultMaterialItem> Item) const;

	// Helper functions
	void UpdateSelection(TSharedPtr<FHdriVaultMaterialItem> NewSelection);
	void ScrollToMaterial(TSharedPtr<FHdriVaultMaterialItem> Material);
	FText GetStatusText() const;
}; 