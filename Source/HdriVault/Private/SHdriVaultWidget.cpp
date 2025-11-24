// Copyright Pyre Labs. All Rights Reserved.

#include "SHdriVaultWidget.h"
#include "HdriVaultManager.h"
#include "SHdriVaultFolderTree.h"
#include "SHdriVaultMaterialGrid.h"
#include "SHdriVaultMetadataPanel.h"
#include "SHdriVaultCategoriesPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "Engine/Engine.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "Framework/Application/SlateApplication.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"

#define LOCTEXT_NAMESPACE "HdriVaultWidget"

void SHdriVaultWidget::Construct(const FArguments& InArgs)
{
	// Get the HdriVault manager
	HdriVaultManager = GEditor->GetEditorSubsystem<UHdriVaultManager>();
	
	// Initialize settings
	CurrentSettings = FHdriVaultSettings();
	
	// Create the main layout
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		[
			CreateToolbar()
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			CreateMainContent()
		]
	];
	
	// Bind to manager events
	if (HdriVaultManager)
	{
		HdriVaultManager->OnFolderSelected.AddSP(this, &SHdriVaultWidget::OnFolderSelected);
		HdriVaultManager->OnMaterialSelected.AddSP(this, &SHdriVaultWidget::OnMaterialSelected);
		HdriVaultManager->OnMaterialDoubleClicked.AddSP(this, &SHdriVaultWidget::OnMaterialDoubleClicked);
		HdriVaultManager->OnSettingsChanged.AddSP(this, &SHdriVaultWidget::OnSettingsChanged);
		HdriVaultManager->OnRefreshRequested.AddSP(this, &SHdriVaultWidget::OnRefreshRequested);
	}
	
	// Bind widget events
	if (FolderTreeWidget.IsValid())
	{
		FolderTreeWidget->OnFolderSelected.BindSP(this, &SHdriVaultWidget::OnFolderSelected);
	}
	
	if (CategoriesWidget.IsValid())
	{
		CategoriesWidget->OnCategorySelected.BindSP(this, &SHdriVaultWidget::OnCategorySelected);
		CategoriesWidget->OnTagSelected.BindSP(this, &SHdriVaultWidget::OnTagSelected);
	}
	
	if (MaterialGridWidget.IsValid())
	{
		MaterialGridWidget->OnMaterialSelected.BindSP(this, &SHdriVaultWidget::OnMaterialSelected);
		MaterialGridWidget->OnMaterialDoubleClicked.BindSP(this, &SHdriVaultWidget::OnMaterialDoubleClicked);
		MaterialGridWidget->OnMaterialApplied.BindSP(this, &SHdriVaultWidget::OnMaterialApplied);
	}
	
	if (MetadataWidget.IsValid())
	{
		MetadataWidget->OnMetadataChanged.BindSP(this, &SHdriVaultWidget::OnMetadataChanged);
	}
	
	// Initial refresh
	RefreshInterface();

	// Ensure default tab content is initialized
	if (!bShowFolders)
	{
		OnCategoriesTabClicked();
	}
	else
	{
		OnFoldersTabClicked();
	}
}

void SHdriVaultWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	
	// Update thumbnails or other time-based operations if needed
}

void SHdriVaultWidget::RefreshInterface()
{
	if (HdriVaultManager)
	{
		// Store current selections before refreshing
		FString CurrentFolderPath;
		FString CurrentCategoryName;
		FString CurrentTag = CurrentSelectedTag;
		
		if (CurrentSelectedFolder.IsValid())
		{
			CurrentFolderPath = CurrentSelectedFolder->FolderPath;
		}
		
		if (CurrentSelectedCategory.IsValid())
		{
			CurrentCategoryName = CurrentSelectedCategory->CategoryName;
		}
		
		// Refresh the database
		HdriVaultManager->RefreshMaterialDatabase();
		
		// Refresh categories and tags explicitly
		if (CategoriesWidget.IsValid())
		{
			CategoriesWidget->RefreshCategories();
			CategoriesWidget->RefreshTags();
		}
		
		// Restore selections after refresh
		if (!CurrentFolderPath.IsEmpty() && bShowFolders)
		{
			// Find and restore folder selection
			if (FolderTreeWidget.IsValid())
			{
				TSharedPtr<FHdriVaultFolderNode> RestoredFolder = HdriVaultManager->FindFolder(CurrentFolderPath);
				if (RestoredFolder.IsValid())
				{
					CurrentSelectedFolder = RestoredFolder;
					FolderTreeWidget->SetSelectedFolder(RestoredFolder);
				}
			}
		}
		else if (!CurrentCategoryName.IsEmpty() && !bShowFolders)
		{
			// Restore category selection
			if (CategoriesWidget.IsValid())
			{
				CategoriesWidget->RefreshCategories();
				CategoriesWidget->SetSelectedCategoryByName(CurrentCategoryName);
				// Update our local pointer to the new one
				CurrentSelectedCategory = CategoriesWidget->GetSelectedCategory();
			}
		}
		else if (!CurrentTag.IsEmpty())
		{
			// Restore tag selection
			CurrentSelectedTag = CurrentTag;
			if (CategoriesWidget.IsValid())
			{
				CategoriesWidget->RefreshTags();
				CategoriesWidget->SetSelectedTag(CurrentTag);
			}
		}
		
		// Update the material grid with restored selection
		UpdateMaterialGrid();
	}
}

void SHdriVaultWidget::SetSettings(const FHdriVaultSettings& NewSettings)
{
	CurrentSettings = NewSettings;
	ApplySettings();
}

TSharedRef<SWidget> SHdriVaultWidget::CreateToolbar()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(4.0f)
		[
			SNew(SHorizontalBox)
			
			// Refresh button
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "FlatButton")
				.OnClicked(this, &SHdriVaultWidget::OnRefreshClicked)
				.ToolTipText(NSLOCTEXT("HdriVault", "RefreshTooltip", "Refresh material database"))
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("Icons.Refresh"))
				]
			]
			
			// Browse to folder button
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "FlatButton")
				.OnClicked(this, &SHdriVaultWidget::OnBrowseToFolderClicked)
				.ToolTipText(NSLOCTEXT("HdriVault", "BrowseToFolderTooltip", "Browse to selected material location"))
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("Icons.FolderOpen"))
				]
			]
			
			// Search box
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(10.0f, 2.0f)
			[
				SNew(SSearchBox)
				.OnTextChanged(this, &SHdriVaultWidget::OnSearchTextChanged)
				.HintText(NSLOCTEXT("HdriVault", "SearchHint", "Search materials..."))
			]
			
			// Thumbnail size slider
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.0f)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("HdriVault", "ThumbnailSize", "Size:"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SBox)
					.WidthOverride(100.0f)
					[
						SNew(SSlider)
						.Value(CurrentSettings.ThumbnailSize / 256.0f)
						.OnValueChanged(this, &SHdriVaultWidget::OnThumbnailSizeChanged)
						.ToolTipText(NSLOCTEXT("HdriVault", "ThumbnailSizeTooltip", "Adjust thumbnail size"))
					]
				]
			]
		];
}

TSharedRef<SWidget> SHdriVaultWidget::CreateMainContent()
{
	MainSplitter = SNew(SSplitter)
		.Orientation(Orient_Horizontal)
		.PhysicalSplitterHandleSize(2.0f)
		.ResizeMode(ESplitterResizeMode::FixedPosition);
	
	// Left panel - Folder tree
	MainSplitter->AddSlot()
		.Value(0.25f)
		.MinSize(200.0f)
		[
			CreateFolderTreePanel()
		];
	
	// Right side splitter for materials and metadata
	ContentSplitter = SNew(SSplitter)
		.Orientation(Orient_Horizontal)
		.PhysicalSplitterHandleSize(2.0f)
		.ResizeMode(ESplitterResizeMode::FixedPosition);
	
	// Center panel - Material grid
	ContentSplitter->AddSlot()
		.Value(0.7f)
		.MinSize(400.0f)
		[
			CreateMaterialGridPanel()
		];
	
	// Right panel - Metadata
	ContentSplitter->AddSlot()
		.Value(0.3f)
		.MinSize(250.0f)
		[
			CreateMetadataPanel()
		];
	
	MainSplitter->AddSlot()
		.Value(0.75f)
		.MinSize(650.0f)
		[
			ContentSplitter.ToSharedRef()
		];
	
	return MainSplitter.ToSharedRef();
}

TSharedRef<SWidget> SHdriVaultWidget::CreateFolderTreePanel()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(0)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				// Tab selector
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.5f)
				[
													SAssignNew(FoldersTabButton, SButton)
				.ButtonStyle(FAppStyle::Get(), bShowFolders ? "PrimaryButton" : "FlatButton")
				.OnClicked(this, &SHdriVaultWidget::OnFoldersTabClicked)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("FoldersTab", "Folders"))
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.5f)
			[
				SAssignNew(CategoriesTabButton, SButton)
				.ButtonStyle(FAppStyle::Get(), !bShowFolders ? "PrimaryButton" : "FlatButton")
				.OnClicked(this, &SHdriVaultWidget::OnCategoriesTabClicked)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("CategoriesTab", "Categories"))
				]
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				// Content switcher
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SAssignNew(FolderTreeWidget, SHdriVaultFolderTree)
					.Visibility(this, &SHdriVaultWidget::GetFoldersVisibility)
				]
				+ SOverlay::Slot()
				[
					SAssignNew(CategoriesWidget, SHdriVaultCategoriesPanel)
					.Visibility(this, &SHdriVaultWidget::GetCategoriesVisibility)
				]
			]
		];
}

TSharedRef<SWidget> SHdriVaultWidget::CreateMaterialGridPanel()
{
	return SAssignNew(MaterialGridWidget, SHdriVaultMaterialGrid);
}

TSharedRef<SWidget> SHdriVaultWidget::CreateMetadataPanel()
{
	return SAssignNew(MetadataWidget, SHdriVaultMetadataPanel);
}

FReply SHdriVaultWidget::OnRefreshClicked()
{
	RefreshInterface();
	return FReply::Handled();
}

FReply SHdriVaultWidget::OnBrowseToFolderClicked()
{
	// Browse to the currently selected material in the Content Browser
	if (CurrentSelectedMaterial.IsValid())
	{
		TArray<FAssetData> AssetDataArray;
		AssetDataArray.Add(CurrentSelectedMaterial->AssetData);

		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets(AssetDataArray);
	}
	return FReply::Handled();
}

void SHdriVaultWidget::OnViewModeChanged(EHdriVaultViewMode NewViewMode)
{
	CurrentSettings.ViewMode = NewViewMode;
	ApplySettings();
}

void SHdriVaultWidget::OnThumbnailSizeChanged(float NewSize)
{
	CurrentSettings.ThumbnailSize = NewSize * 256.0f;
	ApplySettings();
}

void SHdriVaultWidget::OnSearchTextChanged(const FText& SearchText)
{
	CurrentSearchText = SearchText.ToString();
	UpdateMaterialGrid();
}

void SHdriVaultWidget::OnSortModeChanged(EHdriVaultSortMode NewSortMode)
{
	CurrentSettings.SortMode = NewSortMode;
	ApplySettings();
}

void SHdriVaultWidget::OnFolderSelected(TSharedPtr<FHdriVaultFolderNode> SelectedFolder)
{
	CurrentSelectedFolder = SelectedFolder;
	// Do not clear CurrentSelectedCategory here to preserve it when switching back
	UpdateMaterialGrid();
}

void SHdriVaultWidget::OnCategorySelected(TSharedPtr<FHdriVaultCategoryItem> SelectedCategory)
{
	CurrentSelectedCategory = SelectedCategory;
	// Do not clear CurrentSelectedFolder here to preserve it when switching back
	CurrentSelectedTag.Reset(); // Clear tag selection when category is selected (optional, but usually standard behavior)
	UpdateMaterialGridFromCategory();
}

void SHdriVaultWidget::OnTagSelected(FString SelectedTag)
{
	// Handle tag selection and clear other selections
	CurrentSelectedTag = SelectedTag;
	CurrentSelectedFolder.Reset(); // Clear folder selection when tag is selected
	CurrentSelectedCategory.Reset(); // Clear category selection when tag is selected
	UpdateMaterialGridFromTag();
}

void SHdriVaultWidget::OnMaterialSelected(TSharedPtr<FHdriVaultMaterialItem> SelectedMaterial)
{
	// If selected material is null, we simply ignore it to preserve the last selection in the Metadata Panel.
	// This solves both "view switching clears selection" and "clicking empty space clears selection".
	// If explicit clearing is needed later, we can add a "Clear Selection" button or similar.
	if (!SelectedMaterial.IsValid())
	{
		return;
	}

	CurrentSelectedMaterial = SelectedMaterial;
	UpdateMetadataPanel();
}

void SHdriVaultWidget::OnMaterialDoubleClicked(TSharedPtr<FHdriVaultMaterialItem> SelectedMaterial)
{
	// Apply material to selected objects or open material editor
	if (HdriVaultManager)
	{
		HdriVaultManager->ApplyMaterialToSelection(SelectedMaterial);
	}
}

void SHdriVaultWidget::OnMaterialApplied(TSharedPtr<FHdriVaultMaterialItem> MaterialToApply)
{
	// Apply material to selected objects
	if (HdriVaultManager && MaterialToApply.IsValid())
	{
		HdriVaultManager->ApplyMaterialToSelection(MaterialToApply);
	}
}

void SHdriVaultWidget::OnMetadataChanged(TSharedPtr<FHdriVaultMaterialItem> ChangedMaterial)
{
	// Refresh the material grid to show updated metadata
	if (MaterialGridWidget.IsValid())
	{
		MaterialGridWidget->RefreshGrid();
	}
}

void SHdriVaultWidget::OnSettingsChanged(const FHdriVaultSettings& NewSettings)
{
	CurrentSettings = NewSettings;
}

void SHdriVaultWidget::OnRefreshRequested()
{
	// Update categories and tags
	if (CategoriesWidget.IsValid())
	{
		CategoriesWidget->RefreshCategories();
		CategoriesWidget->RefreshTags();
	}

	UpdateMaterialGrid();
}

void SHdriVaultWidget::UpdateMaterialGrid()
{
	if (MaterialGridWidget.IsValid())
	{
		bIsUpdatingView = true;

		if (bShowFolders && CurrentSelectedFolder.IsValid())
		{
			FString FolderPath = CurrentSelectedFolder->FolderPath;
			MaterialGridWidget->SetFolder(FolderPath);
			MaterialGridWidget->SetFilterText(CurrentSearchText);
		}
		else if (!bShowFolders)
		{
			if (CurrentSelectedCategory.IsValid())
			{
				// For categories, set materials directly
				MaterialGridWidget->SetMaterials(CurrentSelectedCategory->Materials);
				MaterialGridWidget->SetFilterText(CurrentSearchText);
			}
			else if (!CurrentSelectedTag.IsEmpty() && HdriVaultManager)
			{
				// Restore tag view if no category is selected but a tag is
				TArray<TSharedPtr<FHdriVaultMaterialItem>> TaggedMaterials = HdriVaultManager->FilterMaterialsByTag(CurrentSelectedTag);
				MaterialGridWidget->SetMaterials(TaggedMaterials);
				MaterialGridWidget->SetFilterText(CurrentSearchText);
			}
			else
			{
				// Clear selection if nothing is selected in this view
				MaterialGridWidget->SetFolder(FString());
				MaterialGridWidget->SetFilterText(CurrentSearchText);
			}
		}
		else
		{
			// Clear selection (folder view but no folder selected)
			MaterialGridWidget->SetFolder(FString());
			MaterialGridWidget->SetFilterText(CurrentSearchText);
		}

		// Try to restore selection if the item exists in the new view
		if (CurrentSelectedMaterial.IsValid())
		{
			MaterialGridWidget->SetSelectedMaterial(CurrentSelectedMaterial);
		}

		bIsUpdatingView = false;
	}
}

void SHdriVaultWidget::UpdateMaterialGridFromCategory()
{
	if (MaterialGridWidget.IsValid() && CurrentSelectedCategory.IsValid())
	{
		bIsUpdatingView = true;
		
		MaterialGridWidget->SetMaterials(CurrentSelectedCategory->Materials);
		MaterialGridWidget->SetFilterText(CurrentSearchText);
		
		if (CurrentSelectedMaterial.IsValid())
		{
			MaterialGridWidget->SetSelectedMaterial(CurrentSelectedMaterial);
		}

		bIsUpdatingView = false;
	}
}

void SHdriVaultWidget::UpdateMaterialGridFromTag()
{
	if (MaterialGridWidget.IsValid() && !CurrentSelectedTag.IsEmpty() && HdriVaultManager)
	{
		bIsUpdatingView = true;

		// Get materials with the selected tag
		TArray<TSharedPtr<FHdriVaultMaterialItem>> TaggedMaterials = HdriVaultManager->FilterMaterialsByTag(CurrentSelectedTag);
		MaterialGridWidget->SetMaterials(TaggedMaterials);
		MaterialGridWidget->SetFilterText(CurrentSearchText);

		if (CurrentSelectedMaterial.IsValid())
		{
			MaterialGridWidget->SetSelectedMaterial(CurrentSelectedMaterial);
		}

		bIsUpdatingView = false;
	}
}

void SHdriVaultWidget::UpdateMetadataPanel()
{
	if (MetadataWidget.IsValid())
	{
		MetadataWidget->SetMaterialItem(CurrentSelectedMaterial);
	}
}

void SHdriVaultWidget::ApplySettings()
{
	if (HdriVaultManager)
	{
		HdriVaultManager->SetSettings(CurrentSettings);
	}
	
	// Apply settings directly to widgets
	if (MaterialGridWidget.IsValid())
	{
		MaterialGridWidget->SetViewMode(CurrentSettings.ViewMode);
		MaterialGridWidget->SetThumbnailSize(CurrentSettings.ThumbnailSize);
	}
}

void SHdriVaultWidget::SaveSettings()
{
	// TODO: Save settings to config file
}

void SHdriVaultWidget::LoadSettings()
{
	// TODO: Load settings from config file
}

FReply SHdriVaultWidget::OnFoldersTabClicked()
{
	bShowFolders = true;
	// CurrentSelectedCategory.Reset(); // Removed reset to persist category selection
	
	// Update button styles
	if (FoldersTabButton.IsValid())
	{
		FoldersTabButton->SetButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"));
	}
	if (CategoriesTabButton.IsValid())
	{
		CategoriesTabButton->SetButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("FlatButton"));
	}
	
	UpdateMaterialGrid();
	return FReply::Handled();
}

FReply SHdriVaultWidget::OnCategoriesTabClicked()
{
	bShowFolders = false;
	// CurrentSelectedFolder.Reset(); // Removed reset to persist folder selection
	
	// Update button styles
	if (FoldersTabButton.IsValid())
	{
		FoldersTabButton->SetButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("FlatButton"));
	}
	if (CategoriesTabButton.IsValid())
	{
		CategoriesTabButton->SetButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"));
	}
	
	if (CategoriesWidget.IsValid())
	{
		CategoriesWidget->RefreshCategories();
	}
	UpdateMaterialGrid();
	return FReply::Handled();
}

EVisibility SHdriVaultWidget::GetFoldersVisibility() const
{
	return bShowFolders ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SHdriVaultWidget::GetCategoriesVisibility() const
{
	return !bShowFolders ? EVisibility::Visible : EVisibility::Collapsed;
}

#undef LOCTEXT_NAMESPACE