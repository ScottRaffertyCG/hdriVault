// Copyright Pyre Labs. All Rights Reserved.

#include "SHdriVaultMaterialGrid.h"
#include "HdriVaultManager.h"
#include "Engine/Engine.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Input/SMenuAnchor.h"
#include "Widgets/Images/SThrobber.h"
#include "AssetThumbnail.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "HAL/PlatformApplicationMisc.h"
#include "DragAndDrop/AssetDragDropOp.h"
// #include "DragAndDrop/ExternalDragOperation.h" // Removed: causing include errors, using FDragDropOperation with IsOfType instead or finding correct header
#include "Styling/AppStyle.h"
#include "Input/DragAndDrop.h"

#define LOCTEXT_NAMESPACE "HdriVaultMaterialGrid"

void SHdriVaultMaterialTile::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	MaterialItem = InArgs._MaterialItem;
	ThumbnailSize = InArgs._ThumbnailSize;

	// Create asset thumbnail
	if (MaterialItem.IsValid())
	{
		FAssetThumbnailConfig ThumbnailConfig;
		ThumbnailConfig.bAllowFadeIn = true;
		ThumbnailConfig.bAllowHintText = true;
		ThumbnailConfig.ThumbnailLabel = EThumbnailLabel::ClassName;
		ThumbnailConfig.HighlightedText = FText::GetEmpty();

		AssetThumbnail = MakeShareable(new FAssetThumbnail(MaterialItem->AssetData, ThumbnailSize * 2.0f, ThumbnailSize, UThumbnailManager::Get().GetSharedThumbnailPool()));
	}

	STableRow<TSharedPtr<FHdriVaultMaterialItem>>::Construct(
		STableRow::FArguments()
		.Style(FAppStyle::Get(), "TableView.Row")
		.Padding(FMargin(2))
		.Content()
		[
			SNew(SBox)
			.Padding(4)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0, 0, 0, 4)
				[
					SNew(SBox)
					.WidthOverride(ThumbnailSize * 2.0f)
					.HeightOverride(ThumbnailSize)
					[
						AssetThumbnail.IsValid() ? AssetThumbnail->MakeThumbnailWidget(FAssetThumbnailConfig()) : SNullWidget::NullWidget
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(this, &SHdriVaultMaterialTile::GetMaterialName)
					.Font(FAppStyle::GetFontStyle("ContentBrowser.AssetTileViewNameFont"))
					.ColorAndOpacity(FSlateColor::UseForeground())
					.ShadowColorAndOpacity(FLinearColor::Black)
					.ShadowOffset(FVector2D(1, 1))
					.Justification(ETextJustify::Center)
					.WrapTextAt(ThumbnailSize * 2.0f)
					.ToolTipText(this, &SHdriVaultMaterialTile::GetMaterialTooltip)
				]
			]
		],
		InOwnerTableView
	);
}

FReply SHdriVaultMaterialTile::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnMaterialLeftClicked.ExecuteIfBound(MaterialItem);
		return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
	}
	else if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		OnMaterialRightClicked.ExecuteIfBound(MaterialItem);
		return FReply::Handled();
	}
	else if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		OnMaterialMiddleClicked.ExecuteIfBound(MaterialItem);
		return FReply::Handled();
	}

	return STableRow<TSharedPtr<FHdriVaultMaterialItem>>::OnMouseButtonDown(MyGeometry, MouseEvent);
}

FReply SHdriVaultMaterialTile::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return STableRow<TSharedPtr<FHdriVaultMaterialItem>>::OnMouseButtonUp(MyGeometry, MouseEvent);
}

FReply SHdriVaultMaterialTile::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnMaterialDoubleClicked.ExecuteIfBound(MaterialItem);
		return FReply::Handled();
	}

	return STableRow<TSharedPtr<FHdriVaultMaterialItem>>::OnMouseButtonDoubleClick(InMyGeometry, InMouseEvent);
}

void SHdriVaultMaterialTile::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	// Visual feedback for drag and drop
}

void SHdriVaultMaterialTile::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	// Remove visual feedback
}

FReply SHdriVaultMaterialTile::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MaterialItem.IsValid() && MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		if (OnMaterialDragDetected.IsBound())
		{
			return OnMaterialDragDetected.Execute(MaterialItem, MyGeometry, MouseEvent);
		}
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FText SHdriVaultMaterialTile::GetMaterialName() const
{
	if (MaterialItem.IsValid())
	{
		return FText::FromString(MaterialItem->DisplayName);
	}
	return FText::GetEmpty();
}

FText SHdriVaultMaterialTile::GetMaterialTooltip() const
{
	if (MaterialItem.IsValid())
	{
		FString Dimensions = TEXT("Unknown");
		MaterialItem->AssetData.GetTagValue(TEXT("Dimensions"), Dimensions);
		
		int64 ResourceSizeBytes = 0;
		MaterialItem->AssetData.GetTagValue(TEXT("ResourceSize"), ResourceSizeBytes);
		FString SizeText = (ResourceSizeBytes > 0) ? FText::AsMemory(ResourceSizeBytes).ToString() : TEXT("Unknown");

		FString TooltipText = FString::Printf(TEXT("HDRI: %s\nPath: %s\nDimensions: %s\nSize: %s"),
			*MaterialItem->DisplayName,
			*MaterialItem->AssetData.PackageName.ToString(),
			*Dimensions,
			*SizeText
		);
		return FText::FromString(TooltipText);
	}
	return FText::GetEmpty();
}

EVisibility SHdriVaultMaterialTile::GetLoadingVisibility() const
{
	// Show loading indicator if thumbnail is not ready
	return (AssetThumbnail.IsValid() && AssetThumbnail->GetViewportRenderTargetTexture() == nullptr) ? EVisibility::Visible : EVisibility::Collapsed;
}

void SHdriVaultMaterialTile::RefreshThumbnail()
{
	if (AssetThumbnail.IsValid())
	{
		AssetThumbnail->RefreshThumbnail();
	}
}

void SHdriVaultMaterialListItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	MaterialItem = InArgs._MaterialItem;

	// Create asset thumbnail for list view (smaller)
	if (MaterialItem.IsValid())
	{
		AssetThumbnail = MakeShareable(new FAssetThumbnail(MaterialItem->AssetData, 32, 32, UThumbnailManager::Get().GetSharedThumbnailPool()));
	}

	STableRow<TSharedPtr<FHdriVaultMaterialItem>>::Construct(
		STableRow::FArguments()
		.Style(FAppStyle::Get(), "ContentBrowser.AssetListView.ColumnListTableRow")
		.Padding(FMargin(0, 2, 0, 0))
		.Content()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(4, 0, 8, 0)
			[
				SNew(SBox)
				.WidthOverride(32)
				.HeightOverride(32)
				[
					AssetThumbnail.IsValid() ? AssetThumbnail->MakeThumbnailWidget() : SNullWidget::NullWidget
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.4f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SHdriVaultMaterialListItem::GetMaterialName)
				.ToolTipText(this, &SHdriVaultMaterialListItem::GetMaterialTooltip)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.3f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SHdriVaultMaterialListItem::GetMaterialType)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.3f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SHdriVaultMaterialListItem::GetMaterialPath)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		],
		InOwnerTableView
	);
}

FReply SHdriVaultMaterialListItem::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnMaterialLeftClicked.ExecuteIfBound(MaterialItem);
		return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
	}
	else if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		OnMaterialRightClicked.ExecuteIfBound(MaterialItem);
		return FReply::Handled();
	}

	return STableRow<TSharedPtr<FHdriVaultMaterialItem>>::OnMouseButtonDown(MyGeometry, MouseEvent);
}

FReply SHdriVaultMaterialListItem::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnMaterialDoubleClicked.ExecuteIfBound(MaterialItem);
		return FReply::Handled();
	}

	return STableRow<TSharedPtr<FHdriVaultMaterialItem>>::OnMouseButtonDoubleClick(InMyGeometry, InMouseEvent);
}

FReply SHdriVaultMaterialListItem::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MaterialItem.IsValid() && MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		if (OnMaterialDragDetected.IsBound())
		{
			return OnMaterialDragDetected.Execute(MaterialItem, MyGeometry, MouseEvent);
		}
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FText SHdriVaultMaterialListItem::GetMaterialName() const
{
	if (MaterialItem.IsValid())
	{
		return FText::FromString(MaterialItem->DisplayName);
	}
	return FText::GetEmpty();
}

FText SHdriVaultMaterialListItem::GetMaterialType() const
{
	if (MaterialItem.IsValid())
	{
		FString ClassName = MaterialItem->AssetData.AssetClassPath.GetAssetName().ToString();
		return FText::FromString(ClassName);
	}
	return FText::GetEmpty();
}

FText SHdriVaultMaterialListItem::GetMaterialPath() const
{
	if (MaterialItem.IsValid())
	{
		FString Path = MaterialItem->AssetData.PackagePath.ToString();
		Path.RemoveFromStart(TEXT("/Game/"));
		return FText::FromString(Path);
	}
	return FText::GetEmpty();
}

FText SHdriVaultMaterialListItem::GetMaterialTooltip() const
{
	if (MaterialItem.IsValid())
	{
		FString Dimensions = TEXT("Unknown");
		MaterialItem->AssetData.GetTagValue(TEXT("Dimensions"), Dimensions);
		
		int64 ResourceSizeBytes = 0;
		MaterialItem->AssetData.GetTagValue(TEXT("ResourceSize"), ResourceSizeBytes);
		FString SizeText = (ResourceSizeBytes > 0) ? FText::AsMemory(ResourceSizeBytes).ToString() : TEXT("Unknown");

		FString TooltipText = FString::Printf(TEXT("HDRI: %s\nPath: %s\nDimensions: %s\nSize: %s"),
			*MaterialItem->DisplayName,
			*MaterialItem->AssetData.PackageName.ToString(),
			*Dimensions,
			*SizeText
		);
		return FText::FromString(TooltipText);
	}
	return FText::GetEmpty();
}

void SHdriVaultMaterialGrid::Construct(const FArguments& InArgs)
{
	HdriVaultManager = GEditor->GetEditorSubsystem<UHdriVaultManager>();
	ViewMode = EHdriVaultViewMode::Grid;
	ThumbnailSize = 128.0f;
	CurrentFilterText = TEXT("");

	// Create thumbnail pool
	ThumbnailPool = MakeShareable(new FAssetThumbnailPool(1000, true));

	// Create view container
	ViewContainer = SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(0);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4, 4, 4, 2)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("MaterialsTitle", "Materials"))
				.Font(FAppStyle::GetFontStyle("ContentBrowser.SourceTitleFont"))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SHdriVaultMaterialGrid::GetStatusText)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			ViewContainer.ToSharedRef()
		]
	];

	// Initialize with tile view
	SwitchToViewMode(ViewMode);
}

void SHdriVaultMaterialGrid::RefreshGrid()
{
	UpdateFilteredMaterials();

	if (TileView.IsValid())
	{
		TileView->RequestListRefresh();
	}
	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

void SHdriVaultMaterialGrid::SetMaterials(const TArray<TSharedPtr<FHdriVaultMaterialItem>>& InMaterials)
{
	AllMaterials = InMaterials;
	UpdateFilteredMaterials();
	RefreshGrid();
}

void SHdriVaultMaterialGrid::SetSelectedMaterial(TSharedPtr<FHdriVaultMaterialItem> Material)
{
	UpdateSelection(Material);
}

TSharedPtr<FHdriVaultMaterialItem> SHdriVaultMaterialGrid::GetSelectedMaterial() const
{
	return SelectedMaterial;
}

void SHdriVaultMaterialGrid::SetViewMode(EHdriVaultViewMode InViewMode)
{
	if (ViewMode != InViewMode)
	{
		ViewMode = InViewMode;
		SwitchToViewMode(ViewMode);
	}
}

void SHdriVaultMaterialGrid::SetThumbnailSize(float InThumbnailSize)
{
	ThumbnailSize = FMath::Clamp(InThumbnailSize, 32.0f, 512.0f);
	
	// Recreate the view with new thumbnail size
	if (ViewMode == EHdriVaultViewMode::Grid)
	{
		SwitchToViewMode(EHdriVaultViewMode::Grid);
	}
}

void SHdriVaultMaterialGrid::ClearSelection()
{
	UpdateSelection(nullptr);
}

void SHdriVaultMaterialGrid::SetFolder(const FString& FolderPath)
{
	if (HdriVaultManager)
	{
		TArray<TSharedPtr<FHdriVaultMaterialItem>> FolderMaterials = HdriVaultManager->GetMaterialsInFolder(FolderPath);
		SetMaterials(FolderMaterials);
	}
}

void SHdriVaultMaterialGrid::SetFilterText(const FString& FilterText)
{
	CurrentFilterText = FilterText;
	ApplyFilters();
}

void SHdriVaultMaterialGrid::ApplyFilters()
{
	UpdateFilteredMaterials();
	RefreshGrid();
}

TSharedRef<SWidget> SHdriVaultMaterialGrid::CreateTileView()
{
	TileView = SNew(STileView<TSharedPtr<FHdriVaultMaterialItem>>)
		.ListItemsSource(&FilteredMaterials)
		.OnGenerateTile(this, &SHdriVaultMaterialGrid::OnGenerateTileWidget)
		.OnSelectionChanged(this, &SHdriVaultMaterialGrid::OnTileSelectionChanged)
		.OnContextMenuOpening(this, &SHdriVaultMaterialGrid::OnContextMenuOpening)
		.ItemWidth(ThumbnailSize * 2.0f + 32)
		.ItemHeight(ThumbnailSize + 48)
		.SelectionMode(ESelectionMode::Single)
		.ClearSelectionOnClick(false);

	return TileView.ToSharedRef();
}

TSharedRef<SWidget> SHdriVaultMaterialGrid::CreateListView()
{
	ListView = SNew(SListView<TSharedPtr<FHdriVaultMaterialItem>>)
		.ListItemsSource(&FilteredMaterials)
		.OnGenerateRow(this, &SHdriVaultMaterialGrid::OnGenerateListWidget)
		.OnSelectionChanged(this, &SHdriVaultMaterialGrid::OnListSelectionChanged)
		.OnContextMenuOpening(this, &SHdriVaultMaterialGrid::OnContextMenuOpening)
		.SelectionMode(ESelectionMode::Single)
		.ClearSelectionOnClick(false);

	return ListView.ToSharedRef();
}

void SHdriVaultMaterialGrid::SwitchToViewMode(EHdriVaultViewMode NewViewMode)
{
	ViewMode = NewViewMode;

	TSharedRef<SWidget> NewView = (ViewMode == EHdriVaultViewMode::Grid) ? CreateTileView() : CreateListView();
	
	ViewContainer->SetContent(NewView);
	RefreshGrid();
}

TSharedRef<ITableRow> SHdriVaultMaterialGrid::OnGenerateTileWidget(TSharedPtr<FHdriVaultMaterialItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	TSharedRef<SHdriVaultMaterialTile> TileWidget = SNew(SHdriVaultMaterialTile, OwnerTable)
		.MaterialItem(Item)
		.ThumbnailSize(ThumbnailSize);

	TileWidget->OnMaterialLeftClicked.BindSP(this, &SHdriVaultMaterialGrid::OnMaterialLeftClicked);
	TileWidget->OnMaterialRightClicked.BindSP(this, &SHdriVaultMaterialGrid::OnMaterialRightClicked);
	TileWidget->OnMaterialMiddleClicked.BindSP(this, &SHdriVaultMaterialGrid::OnMaterialMiddleClicked);
	TileWidget->OnMaterialDoubleClicked.BindSP(this, &SHdriVaultMaterialGrid::OnMaterialDoubleClickedInternal);
	TileWidget->OnMaterialDragDetected.BindSP(this, &SHdriVaultMaterialGrid::HandleMaterialDragDetected);

	return TileWidget;
}

void SHdriVaultMaterialGrid::OnTileSelectionChanged(TSharedPtr<FHdriVaultMaterialItem> SelectedItem, ESelectInfo::Type SelectInfo)
{
	UpdateSelection(SelectedItem);
}

TSharedRef<ITableRow> SHdriVaultMaterialGrid::OnGenerateListWidget(TSharedPtr<FHdriVaultMaterialItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	TSharedRef<SHdriVaultMaterialListItem> ListWidget = SNew(SHdriVaultMaterialListItem, OwnerTable)
		.MaterialItem(Item);

	ListWidget->OnMaterialLeftClicked.BindSP(this, &SHdriVaultMaterialGrid::OnMaterialLeftClicked);
	ListWidget->OnMaterialRightClicked.BindSP(this, &SHdriVaultMaterialGrid::OnMaterialRightClicked);
	ListWidget->OnMaterialDoubleClicked.BindSP(this, &SHdriVaultMaterialGrid::OnMaterialDoubleClickedInternal);
	ListWidget->OnMaterialDragDetected.BindSP(this, &SHdriVaultMaterialGrid::HandleMaterialDragDetected);

	return ListWidget;
}

void SHdriVaultMaterialGrid::OnListSelectionChanged(TSharedPtr<FHdriVaultMaterialItem> SelectedItem, ESelectInfo::Type SelectInfo)
{
	UpdateSelection(SelectedItem);
}

void SHdriVaultMaterialGrid::OnMaterialLeftClicked(TSharedPtr<FHdriVaultMaterialItem> Material)
{
	// Left click should select the material
	UpdateSelection(Material);
}

void SHdriVaultMaterialGrid::OnMaterialRightClicked(TSharedPtr<FHdriVaultMaterialItem> Material)
{
	// Right click should select the material and show context menu
	UpdateSelection(Material);
	// Context menu will be shown automatically by the STileView/SListView OnContextMenuOpening delegate
}

void SHdriVaultMaterialGrid::OnMaterialMiddleClicked(TSharedPtr<FHdriVaultMaterialItem> Material)
{
	// Middle click should show large thumbnail preview (from original HdriVault behavior)
	UpdateSelection(Material);
	// TODO: Implement large thumbnail preview window
}

void SHdriVaultMaterialGrid::OnMaterialDoubleClickedInternal(TSharedPtr<FHdriVaultMaterialItem> Material)
{
	OnMaterialDoubleClicked.ExecuteIfBound(Material);
}

FReply SHdriVaultMaterialGrid::HandleMaterialDragDetected(TSharedPtr<FHdriVaultMaterialItem> Material, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!Material.IsValid() || !MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		return FReply::Unhandled();
	}

	TArray<TSharedPtr<FHdriVaultMaterialItem>> MaterialsForDrag;
	GatherDragMaterials(MaterialsForDrag, Material);

	TArray<FAssetData> DraggedAssets;
	for (const TSharedPtr<FHdriVaultMaterialItem>& DragMaterial : MaterialsForDrag)
	{
		if (DragMaterial.IsValid() && DragMaterial->AssetData.IsValid())
		{
			DraggedAssets.Add(DragMaterial->AssetData);
		}
	}

	if (DraggedAssets.Num() == 0)
	{
		return FReply::Unhandled();
	}

	TSharedRef<FAssetDragDropOp> DragDropOp = FAssetDragDropOp::New(DraggedAssets);

	return FReply::Handled().BeginDragDrop(DragDropOp);
}

void SHdriVaultMaterialGrid::GatherDragMaterials(TArray<TSharedPtr<FHdriVaultMaterialItem>>& OutMaterials, TSharedPtr<FHdriVaultMaterialItem> PrimaryItem) const
{
	OutMaterials.Reset();

	auto AddUniqueMaterial = [&OutMaterials](TSharedPtr<FHdriVaultMaterialItem> InMaterial)
	{
		if (InMaterial.IsValid())
		{
			OutMaterials.AddUnique(InMaterial);
		}
	};

	AddUniqueMaterial(PrimaryItem);

	if (TileView.IsValid())
	{
		const TArray<TSharedPtr<FHdriVaultMaterialItem>> SelectedTiles = TileView->GetSelectedItems();
		for (const TSharedPtr<FHdriVaultMaterialItem>& SelectedTileMaterial : SelectedTiles)
		{
			AddUniqueMaterial(SelectedTileMaterial);
		}
	}

	if (ListView.IsValid())
	{
		const TArray<TSharedPtr<FHdriVaultMaterialItem>> SelectedListItems = ListView->GetSelectedItems();
		for (const TSharedPtr<FHdriVaultMaterialItem>& SelectedListMaterial : SelectedListItems)
		{
			AddUniqueMaterial(SelectedListMaterial);
		}
	}
}

TSharedPtr<SWidget> SHdriVaultMaterialGrid::OnContextMenuOpening()
{
	if (!SelectedMaterial.IsValid())
	{
		return nullptr;
	}

	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("HdriActions", "HDRI Actions"));
	{
		MenuBuilder.AddMenuEntry(
			FUIAction(FExecuteAction::CreateSP(this, &SHdriVaultMaterialGrid::OnApplyMaterial)),
			SNew(STextBlock).Text(LOCTEXT("ApplyHdri", "Apply HDRI")),
			NAME_None,
			LOCTEXT("ApplyHdriTooltip", "Apply this HDRI to Skylight")
		);

		MenuBuilder.AddMenuEntry(
			FUIAction(FExecuteAction::CreateSP(this, &SHdriVaultMaterialGrid::OnBrowseToMaterial)),
			SNew(STextBlock).Text(LOCTEXT("BrowseToMaterial", "Browse to Asset")),
			NAME_None,
			LOCTEXT("BrowseToMaterialTooltip", "Browse to this material in the Content Browser")
		);

		MenuBuilder.AddMenuEntry(
			FUIAction(FExecuteAction::CreateSP(this, &SHdriVaultMaterialGrid::OnCopyMaterialPath)),
			SNew(STextBlock).Text(LOCTEXT("CopyMaterialPath", "Copy Asset Path")),
			NAME_None,
			LOCTEXT("CopyMaterialPathTooltip", "Copy the asset path to clipboard")
		);

		MenuBuilder.AddMenuEntry(
			FUIAction(FExecuteAction::CreateSP(this, &SHdriVaultMaterialGrid::OnEditMaterialMetadata)),
			SNew(STextBlock).Text(LOCTEXT("EditMetadata", "Edit Metadata")),
			NAME_None,
			LOCTEXT("EditMetadataTooltip", "Edit HDRI metadata")
		);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SHdriVaultMaterialGrid::OnApplyMaterial()
{
	if (SelectedMaterial.IsValid())
	{
		OnMaterialApplied.ExecuteIfBound(SelectedMaterial);
	}
}

void SHdriVaultMaterialGrid::OnBrowseToMaterial()
{
	if (SelectedMaterial.IsValid())
	{
		TArray<FAssetData> AssetDataArray;
		AssetDataArray.Add(SelectedMaterial->AssetData);

		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets(AssetDataArray);
	}
}

void SHdriVaultMaterialGrid::OnCopyMaterialPath()
{
	if (SelectedMaterial.IsValid())
	{
		FString AssetPath = SelectedMaterial->AssetData.GetObjectPathString();
		FPlatformApplicationMisc::ClipboardCopy(*AssetPath);
	}
}

void SHdriVaultMaterialGrid::OnEditMaterialMetadata()
{
	// TODO: Open metadata editing dialog
	UE_LOG(LogTemp, Log, TEXT("Edit metadata functionality not yet implemented"));
}

void SHdriVaultMaterialGrid::UpdateFilteredMaterials()
{
	FilteredMaterials.Empty();

	for (const auto& Material : AllMaterials)
	{
		if (DoesItemPassFilter(Material))
		{
			FilteredMaterials.Add(Material);
		}
	}
}

bool SHdriVaultMaterialGrid::DoesItemPassFilter(TSharedPtr<FHdriVaultMaterialItem> Item) const
{
	if (!Item.IsValid())
	{
		return false;
	}

	if (CurrentFilterText.IsEmpty())
	{
		return true;
	}

	// Check name
	if (Item->DisplayName.Contains(CurrentFilterText))
	{
		return true;
	}

	// Check path
	if (Item->AssetData.PackagePath.ToString().Contains(CurrentFilterText))
	{
		return true;
	}

	// Check metadata tags
	for (const FString& ItemTag : Item->Metadata.Tags)
	{
		if (ItemTag.Contains(CurrentFilterText))
		{
			return true;
		}
	}

	return false;
}

FReply SHdriVaultMaterialGrid::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FDragDropOperation> Operation = DragDropEvent.GetOperation();
	if (Operation.IsValid() && Operation->IsOfType<FExternalDragOperation>())
	{
		TSharedPtr<FExternalDragOperation> DragDropOp = StaticCastSharedPtr<FExternalDragOperation>(Operation);
		if (DragDropOp->HasFiles())
		{
			return FReply::Handled();
		}
	}
	return FReply::Unhandled();
}

void SHdriVaultMaterialGrid::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	// Optional: clear any drag highlights
}

FReply SHdriVaultMaterialGrid::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FDragDropOperation> Operation = DragDropEvent.GetOperation();
	if (Operation.IsValid() && Operation->IsOfType<FExternalDragOperation>())
	{
		TSharedPtr<FExternalDragOperation> DragDropOp = StaticCastSharedPtr<FExternalDragOperation>(Operation);
		if (DragDropOp->HasFiles())
		{
			const TArray<FString>& Files = DragDropOp->GetFiles();
			TArray<FString> HdriFiles;
			
			for (const FString& File : Files)
			{
				FString Extension = FPaths::GetExtension(File).ToLower();
				if (Extension == TEXT("hdr") || Extension == TEXT("exr"))
				{
					HdriFiles.Add(File);
				}
			}

			if (HdriFiles.Num() > 0 && HdriVaultManager)
			{
				HdriVaultManager->ImportHdriFiles(HdriFiles);
				return FReply::Handled();
			}
		}
	}
	return FReply::Unhandled();
}

void SHdriVaultMaterialGrid::UpdateSelection(TSharedPtr<FHdriVaultMaterialItem> NewSelection)
{
	if (SelectedMaterial != NewSelection)
	{
		SelectedMaterial = NewSelection;

		// Update view selection
		if (ViewMode == EHdriVaultViewMode::Grid && TileView.IsValid())
		{
			TileView->SetSelection(SelectedMaterial);
		}
		else if (ViewMode == EHdriVaultViewMode::List && ListView.IsValid())
		{
			ListView->SetSelection(SelectedMaterial);
		}

		OnMaterialSelected.ExecuteIfBound(SelectedMaterial);
	}
}

void SHdriVaultMaterialGrid::ScrollToMaterial(TSharedPtr<FHdriVaultMaterialItem> Material)
{
	if (Material.IsValid())
	{
		if (ViewMode == EHdriVaultViewMode::Grid && TileView.IsValid())
		{
			TileView->RequestScrollIntoView(Material);
		}
		else if (ViewMode == EHdriVaultViewMode::List && ListView.IsValid())
		{
			ListView->RequestScrollIntoView(Material);
		}
	}
}

FText SHdriVaultMaterialGrid::GetStatusText() const
{
	int32 TotalMaterials = AllMaterials.Num();
	int32 FilteredCount = FilteredMaterials.Num();

	if (CurrentFilterText.IsEmpty())
	{
		return FText::Format(LOCTEXT("MaterialCountFormat", "{0} materials"), FText::AsNumber(TotalMaterials));
	}
	else
	{
		return FText::Format(LOCTEXT("FilteredMaterialCountFormat", "{0} of {1} materials"), 
			FText::AsNumber(FilteredCount), FText::AsNumber(TotalMaterials));
	}
}

#undef LOCTEXT_NAMESPACE 