// Copyright Pyre Labs. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SComboBox.h"
#include "HdriVaultTypes.h"
#include "HdriVaultManager.h"

class SHdriVaultFolderTree;
class SHdriVaultMaterialGrid;
class SHdriVaultMetadataPanel;
class SHdriVaultCategoriesPanel;
class SHdriVaultToolbar;

/**
 * Main HdriVault widget with three-panel layout
 */
class HDRIVAULT_API SHdriVaultWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SHdriVaultWidget)
		{}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	// SWidget interface
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	// Refresh the entire interface
	void RefreshInterface();

	// Settings
	void SetSettings(const FHdriVaultSettings& NewSettings);
	const FHdriVaultSettings& GetSettings() const { return CurrentSettings; }

private:
	// UI Components
	TSharedPtr<SHdriVaultToolbar> ToolbarWidget;
	TSharedPtr<SHdriVaultFolderTree> FolderTreeWidget;
	TSharedPtr<SHdriVaultCategoriesPanel> CategoriesWidget;
	TSharedPtr<SHdriVaultMaterialGrid> MaterialGridWidget;
	TSharedPtr<SHdriVaultMetadataPanel> MetadataWidget;
	TSharedPtr<SSplitter> MainSplitter;
	TSharedPtr<SSplitter> ContentSplitter;
	TSharedPtr<SButton> FoldersTabButton;
	TSharedPtr<SButton> CategoriesTabButton;

	// Event handlers
	void OnFolderSelected(TSharedPtr<FHdriVaultFolderNode> SelectedFolder);
	void OnCategorySelected(TSharedPtr<struct FHdriVaultCategoryItem> SelectedCategory);
	void OnTagSelected(FString SelectedTag); // Handle tag selection
	void OnMaterialSelected(TSharedPtr<FHdriVaultMaterialItem> SelectedMaterial);
	void OnMaterialDoubleClicked(TSharedPtr<FHdriVaultMaterialItem> SelectedMaterial);
	void OnMaterialApplied(TSharedPtr<FHdriVaultMaterialItem> MaterialToApply);
	void OnMetadataChanged(TSharedPtr<FHdriVaultMaterialItem> ChangedMaterial);
	void OnSettingsChanged(const FHdriVaultSettings& NewSettings);
	void OnRefreshRequested();

	// Toolbar event handlers
	FReply OnRefreshClicked();
	FReply OnBrowseToFolderClicked();
	void OnViewModeChanged(EHdriVaultViewMode NewViewMode);
	void OnThumbnailSizeChanged(float NewSize);
	void OnSearchTextChanged(const FText& SearchText);
	void OnSortModeChanged(EHdriVaultSortMode NewSortMode);

	// Tab event handlers
	FReply OnFoldersTabClicked();
	FReply OnCategoriesTabClicked();
	EVisibility GetFoldersVisibility() const;
	EVisibility GetCategoriesVisibility() const;

	// UI state
	FHdriVaultSettings CurrentSettings;
	TSharedPtr<FHdriVaultFolderNode> CurrentSelectedFolder;
	TSharedPtr<struct FHdriVaultCategoryItem> CurrentSelectedCategory;
	TSharedPtr<FHdriVaultMaterialItem> CurrentSelectedMaterial;
	FString CurrentSelectedTag; // Currently selected tag for filtering
	FString CurrentSearchText;
	bool bShowFolders = false;
	bool bIsUpdatingView = false; // Flag to prevent selection clearing during view updates

	// Manager reference
	UHdriVaultManager* HdriVaultManager;

	// Layout helpers
	TSharedRef<SWidget> CreateToolbar();
	TSharedRef<SWidget> CreateMainContent();
	TSharedRef<SWidget> CreateFolderTreePanel();
	TSharedRef<SWidget> CreateMaterialGridPanel();
	TSharedRef<SWidget> CreateMetadataPanel();

	// Utility functions
	void UpdateMaterialGrid();
	void UpdateMaterialGridFromCategory();
	void UpdateMaterialGridFromTag(); // Update grid for tag filtering
	void UpdateMetadataPanel();
	void ApplySettings();
	void SaveSettings();
	void LoadSettings();
}; 