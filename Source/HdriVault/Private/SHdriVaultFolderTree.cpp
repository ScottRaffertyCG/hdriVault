// Copyright Pyre Labs. All Rights Reserved.

#include "SHdriVaultFolderTree.h"
#include "HdriVaultManager.h"
#include "Engine/Engine.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SMenuAnchor.h"
#include "Widgets/Layout/SScrollBox.h"

void SHdriVaultFolderTreeItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	FolderNode = InArgs._FolderNode;

	STableRow<TSharedPtr<FHdriVaultFolderNode>>::Construct(
		STableRow::FArguments()
		.Style(FAppStyle::Get(), "ContentBrowser.AssetListView.ColumnListTableRow")
		.Padding(FMargin(0, 2, 0, 0))
		.Content()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 4, 0)
			[
				SNew(SImage)
				.Image(this, &SHdriVaultFolderTreeItem::GetFolderIcon)
				.ColorAndOpacity(FSlateColor::UseForeground())
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SHdriVaultFolderTreeItem::GetFolderText)
				.ColorAndOpacity(this, &SHdriVaultFolderTreeItem::GetFolderTextColor)
				.ToolTipText(this, &SHdriVaultFolderTreeItem::GetFolderTooltip)
				.HighlightText_Lambda([this]() { return FText::GetEmpty(); }) // TODO: Add search highlighting
			]
		],
		InOwnerTableView
	);
}

const FSlateBrush* SHdriVaultFolderTreeItem::GetFolderIcon() const
{
	if (FolderNode.IsValid())
	{
		// Use the same folder icon for all folders to match Engine and Content
		return FAppStyle::GetBrush("ContentBrowser.AssetTreeFolderClosed");
	}
	return FAppStyle::GetBrush("ContentBrowser.AssetTreeFolderClosed");
}

FText SHdriVaultFolderTreeItem::GetFolderText() const
{
	if (FolderNode.IsValid())
	{
		return FText::FromString(FolderNode->FolderName);
	}
	return FText::GetEmpty();
}

FText SHdriVaultFolderTreeItem::GetFolderTooltip() const
{
	if (FolderNode.IsValid())
	{
		int32 MaterialCount = FolderNode->Materials.Num();
		int32 SubfolderCount = FolderNode->Children.Num();
		
		FString TooltipText = FString::Printf(TEXT("Path: %s\nMaterials: %d\nSubfolders: %d"), 
			*FolderNode->FolderPath, MaterialCount, SubfolderCount);
		
		return FText::FromString(TooltipText);
	}
	return FText::GetEmpty();
}

FSlateColor SHdriVaultFolderTreeItem::GetFolderTextColor() const
{
	if (FolderNode.IsValid() && FolderNode->Materials.Num() > 0)
	{
		return FSlateColor::UseForeground();
	}
	return FSlateColor::UseSubduedForeground();
}

void SHdriVaultFolderTree::Construct(const FArguments& InArgs)
{
	HdriVaultManager = GEditor->GetEditorSubsystem<UHdriVaultManager>();
	SelectedFolder = nullptr;
	CurrentFilterText = TEXT("");

	// Create the tree view
	TreeView = SNew(STreeView<TSharedPtr<FHdriVaultFolderNode>>)
		.TreeItemsSource(&RootNodes)
		.OnGenerateRow(this, &SHdriVaultFolderTree::OnGenerateRow)
		.OnGetChildren(this, &SHdriVaultFolderTree::OnGetChildren)
		.OnSelectionChanged(this, &SHdriVaultFolderTree::OnSelectionChanged)
		.OnExpansionChanged(this, &SHdriVaultFolderTree::OnExpansionChanged)
		.OnMouseButtonDoubleClick(this, &SHdriVaultFolderTree::OnFolderDoubleClick)
		.OnContextMenuOpening(this, &SHdriVaultFolderTree::OnContextMenuOpening)
		.SelectionMode(ESelectionMode::Single)
		.ClearSelectionOnClick(false);

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(0)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4, 4, 4, 2)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("HdriVault", "FoldersTitle", "Folders"))
				.Font(FAppStyle::GetFontStyle("ContentBrowser.SourceTitleFont"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2)
			[
				TreeView.ToSharedRef()
			]
		]
	];

	// Bind to manager events
	if (HdriVaultManager)
	{
		HdriVaultManager->OnRefreshRequested.AddSP(this, &SHdriVaultFolderTree::OnManagerRefreshRequested);
	}

	// Initial setup
	RefreshTree();
}

void SHdriVaultFolderTree::RefreshTree()
{
	// Store expanded folders before refresh
	TSet<FString> ExpandedFolders;
	StoreExpandedFolders(RootNodes, ExpandedFolders);
	
	BuildTreeFromManager();
	TreeView->RequestTreeRefresh();
	
	// Restore expanded folders or expand defaults
	if (ExpandedFolders.Num() > 0)
	{
		RestoreExpandedFolders(RootNodes, ExpandedFolders);
	}
	else
	{
		ExpandDefaultFolders();
	}
}

void SHdriVaultFolderTree::SetSelectedFolder(TSharedPtr<FHdriVaultFolderNode> Folder)
{
	if (Folder != SelectedFolder)
	{
		SelectedFolder = Folder;
		TreeView->SetSelection(Folder);
		ScrollToFolder(Folder);
	}
}

TSharedPtr<FHdriVaultFolderNode> SHdriVaultFolderTree::GetSelectedFolder() const
{
	return SelectedFolder;
}

void SHdriVaultFolderTree::ExpandFolder(TSharedPtr<FHdriVaultFolderNode> Folder)
{
	if (Folder.IsValid())
	{
		TreeView->SetItemExpansion(Folder, true);
		Folder->bIsExpanded = true;
	}
}

void SHdriVaultFolderTree::CollapseFolder(TSharedPtr<FHdriVaultFolderNode> Folder)
{
	if (Folder.IsValid())
	{
		TreeView->SetItemExpansion(Folder, false);
		Folder->bIsExpanded = false;
	}
}

TSharedRef<ITableRow> SHdriVaultFolderTree::OnGenerateRow(TSharedPtr<FHdriVaultFolderNode> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SHdriVaultFolderTreeItem, OwnerTable)
		.FolderNode(Item);
}

void SHdriVaultFolderTree::OnGetChildren(TSharedPtr<FHdriVaultFolderNode> Item, TArray<TSharedPtr<FHdriVaultFolderNode>>& OutChildren)
{
	if (Item.IsValid())
	{
		OutChildren = Item->Children;
	}
}

void SHdriVaultFolderTree::OnSelectionChanged(TSharedPtr<FHdriVaultFolderNode> SelectedItem, ESelectInfo::Type SelectInfo)
{
	if (SelectedItem != SelectedFolder)
	{
		SelectedFolder = SelectedItem;
		OnFolderSelected.ExecuteIfBound(SelectedFolder);
	}
}

void SHdriVaultFolderTree::OnExpansionChanged(TSharedPtr<FHdriVaultFolderNode> Item, bool bExpanded)
{
	if (Item.IsValid())
	{
		Item->bIsExpanded = bExpanded;
	}
}

void SHdriVaultFolderTree::OnFolderDoubleClick(TSharedPtr<FHdriVaultFolderNode> Item)
{
	if (Item.IsValid())
	{
		// Toggle expansion on double click
		bool bIsExpanded = TreeView->IsItemExpanded(Item);
		TreeView->SetItemExpansion(Item, !bIsExpanded);
	}
}

TSharedPtr<SWidget> SHdriVaultFolderTree::OnContextMenuOpening()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.BeginSection(NAME_None, NSLOCTEXT("HdriVault", "FolderActions", "Folder Actions"));
	{
		MenuBuilder.AddMenuEntry(
			FUIAction(FExecuteAction::CreateSP(this, &SHdriVaultFolderTree::OnRefreshFolder)),
			SNew(STextBlock).Text(NSLOCTEXT("HdriVault", "RefreshFolder", "Refresh")),
			NAME_None,
			NSLOCTEXT("HdriVault", "RefreshFolderTooltip", "Refresh this folder and its contents")
		);

		if (SelectedFolder.IsValid())
		{
			MenuBuilder.AddMenuEntry(
				FUIAction(FExecuteAction::CreateSP(this, &SHdriVaultFolderTree::OnCreateFolder)),
				SNew(STextBlock).Text(NSLOCTEXT("HdriVault", "CreateFolder", "Create Subfolder")),
				NAME_None,
				NSLOCTEXT("HdriVault", "CreateFolderTooltip", "Create a new subfolder")
			);

			MenuBuilder.AddMenuEntry(
				FUIAction(FExecuteAction::CreateSP(this, &SHdriVaultFolderTree::OnRenameFolder)),
				SNew(STextBlock).Text(NSLOCTEXT("HdriVault", "RenameFolder", "Rename")),
				NAME_None,
				NSLOCTEXT("HdriVault", "RenameFolderTooltip", "Rename this folder")
			);

			// Only allow delete if folder is empty
			if (SelectedFolder->Materials.Num() == 0 && SelectedFolder->Children.Num() == 0)
			{
				MenuBuilder.AddMenuEntry(
					FUIAction(FExecuteAction::CreateSP(this, &SHdriVaultFolderTree::OnDeleteFolder)),
					SNew(STextBlock).Text(NSLOCTEXT("HdriVault", "DeleteFolder", "Delete")),
					NAME_None,
					NSLOCTEXT("HdriVault", "DeleteFolderTooltip", "Delete this empty folder")
				);
			}
		}
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SHdriVaultFolderTree::OnCreateFolder()
{
	// TODO: Implement folder creation
	UE_LOG(LogTemp, Log, TEXT("Create folder functionality not yet implemented"));
}

void SHdriVaultFolderTree::OnRenameFolder()
{
	// TODO: Implement folder renaming
	UE_LOG(LogTemp, Log, TEXT("Rename folder functionality not yet implemented"));
}

void SHdriVaultFolderTree::OnDeleteFolder()
{
	// TODO: Implement folder deletion
	UE_LOG(LogTemp, Log, TEXT("Delete folder functionality not yet implemented"));
}

void SHdriVaultFolderTree::OnRefreshFolder()
{
	// Store current selection before refreshing
	FString CurrentFolderPath;
	if (SelectedFolder.IsValid())
	{
		CurrentFolderPath = SelectedFolder->FolderPath;
	}
	
	RefreshTree();
	
	// Restore selection after refresh
	if (!CurrentFolderPath.IsEmpty() && HdriVaultManager)
	{
		TSharedPtr<FHdriVaultFolderNode> RestoredFolder = HdriVaultManager->FindFolder(CurrentFolderPath);
		if (RestoredFolder.IsValid())
		{
			SetSelectedFolder(RestoredFolder);
		}
	}
}

void SHdriVaultFolderTree::BuildTreeFromManager()
{
	RootNodes.Empty();
	
	if (!HdriVaultManager)
	{
		return;
	}

	TSharedPtr<FHdriVaultFolderNode> RootFolder = HdriVaultManager->GetRootFolder();
	if (RootFolder.IsValid())
	{
		RootNodes = RootFolder->Children;
	}
}

void SHdriVaultFolderTree::ExpandDefaultFolders()
{
	// Expand the first level of folders by default
	for (const auto& RootNode : RootNodes)
	{
		if (RootNode.IsValid())
		{
			TreeView->SetItemExpansion(RootNode, true);
			RootNode->bIsExpanded = true;
		}
	}
}

void SHdriVaultFolderTree::ScrollToFolder(TSharedPtr<FHdriVaultFolderNode> Folder)
{
	if (Folder.IsValid())
	{
		TreeView->RequestScrollIntoView(Folder);
	}
}

bool SHdriVaultFolderTree::DoesNodeMatchFilter(TSharedPtr<FHdriVaultFolderNode> Node, const FString& FilterText) const
{
	if (FilterText.IsEmpty())
	{
		return true;
	}

	if (Node.IsValid())
	{
		// Check if folder name contains filter text
		if (Node->FolderName.Contains(FilterText))
		{
			return true;
		}

		// Check if any child folders match
		for (const auto& Child : Node->Children)
		{
			if (DoesNodeMatchFilter(Child, FilterText))
			{
				return true;
			}
		}

		// Check if any materials in this folder match
		for (const auto& Material : Node->Materials)
		{
			if (Material.IsValid() && Material->DisplayName.Contains(FilterText))
			{
				return true;
			}
		}
	}

	return false;
}

void SHdriVaultFolderTree::OnManagerRefreshRequested()
{
	// Store current selection before refreshing
	FString CurrentFolderPath;
	if (SelectedFolder.IsValid())
	{
		CurrentFolderPath = SelectedFolder->FolderPath;
	}
	
	RefreshTree();
	
	// Restore selection after refresh
	if (!CurrentFolderPath.IsEmpty() && HdriVaultManager)
	{
		TSharedPtr<FHdriVaultFolderNode> RestoredFolder = HdriVaultManager->FindFolder(CurrentFolderPath);
		if (RestoredFolder.IsValid())
		{
			SetSelectedFolder(RestoredFolder);
		}
	}
}

void SHdriVaultFolderTree::SetFilterText(const FString& FilterText)
{
	CurrentFilterText = FilterText;
	ApplyFilter();
}

void SHdriVaultFolderTree::ApplyFilter()
{
	// TODO: Implement filtering logic
	// For now, just refresh the tree
	TreeView->RequestTreeRefresh();
}

void SHdriVaultFolderTree::StoreExpandedFolders(const TArray<TSharedPtr<FHdriVaultFolderNode>>& Folders, TSet<FString>& OutExpandedFolders)
{
	for (const auto& Folder : Folders)
	{
		if (Folder.IsValid())
		{
			if (TreeView->IsItemExpanded(Folder))
			{
				OutExpandedFolders.Add(Folder->FolderPath);
			}
			
			// Recursively store child folder expansion states
			StoreExpandedFolders(Folder->Children, OutExpandedFolders);
		}
	}
}

void SHdriVaultFolderTree::RestoreExpandedFolders(const TArray<TSharedPtr<FHdriVaultFolderNode>>& Folders, const TSet<FString>& ExpandedFolders)
{
	for (const auto& Folder : Folders)
	{
		if (Folder.IsValid())
		{
			if (ExpandedFolders.Contains(Folder->FolderPath))
			{
				TreeView->SetItemExpansion(Folder, true);
				Folder->bIsExpanded = true;
			}
			
			// Recursively restore child folder expansion states
			RestoreExpandedFolders(Folder->Children, ExpandedFolders);
		}
	}
} 