// Copyright Pyre Labs 2025. All Rights Reserved.

#include "SHdriVaultCategoriesPanel.h"
#include "HdriVaultManager.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SButton.h"
#include "Styling/AppStyle.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "SHdriVaultCategoriesPanel"

void SHdriVaultCategoryTreeItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	CategoryItem = InArgs._CategoryItem;

	STableRow<TSharedPtr<FHdriVaultCategoryItem>>::Construct(
		STableRow<TSharedPtr<FHdriVaultCategoryItem>>::FArguments()
		.Padding(2.0f),
		InOwnerTableView
	);

	this->SetContent(
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 0.0f, 4.0f, 0.0f)
		[
			SNew(SImage)
			.Image(this, &SHdriVaultCategoryTreeItem::GetCategoryIcon)
			.ColorAndOpacity(FSlateColor::UseForeground())
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(STextBlock)
			.Text(this, &SHdriVaultCategoryTreeItem::GetCategoryName)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f, 0.0f)
		[
			SNew(STextBlock)
			.Text(this, &SHdriVaultCategoryTreeItem::GetMaterialCount)
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		]
	);
}

FText SHdriVaultCategoryTreeItem::GetCategoryName() const
{
	return CategoryItem.IsValid() ? FText::FromString(CategoryItem->CategoryName) : FText::GetEmpty();
}

FText SHdriVaultCategoryTreeItem::GetMaterialCount() const
{
	if (CategoryItem.IsValid())
	{
		return FText::Format(LOCTEXT("CountFormat", "({0})"), FText::AsNumber(CategoryItem->Materials.Num()));
	}
	return FText::GetEmpty();
}

const FSlateBrush* SHdriVaultCategoryTreeItem::GetCategoryIcon() const
{
	if (CategoryItem.IsValid() && CategoryItem->bIsExpanded)
	{
		return FAppStyle::GetBrush("Icons.FolderOpen");
	}
	return FAppStyle::GetBrush("Icons.FolderClosed");
}

void SHdriVaultCategoriesPanel::Construct(const FArguments& InArgs)
{
	HdriVaultManager = GEditor->GetEditorSubsystem<UHdriVaultManager>();

	ChildSlot
	[
		SNew(SVerticalBox)
		
		// Search/Filter
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		[
			SNew(SSearchBox)
			.OnTextChanged(this, &SHdriVaultCategoriesPanel::OnFilterTextChanged)
			.HintText(LOCTEXT("FilterCategoriesHint", "Filter categories..."))
		]

		// Splitter for Categories and Tags
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SSplitter)
			.Orientation(Orient_Vertical)
			
			// Categories Tree
			+ SSplitter::Slot()
			.Value(0.6f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				.Padding(0)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("CategoriesLabel", "Categories"))
						.Font(FAppStyle::GetFontStyle("HeadingExtraSmall"))
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SAssignNew(CategoryTreeView, STreeView<TSharedPtr<FHdriVaultCategoryItem>>)
						.TreeItemsSource(&FilteredCategories)
						.OnGenerateRow(this, &SHdriVaultCategoriesPanel::OnGenerateCategoryWidget)
						.OnSelectionChanged(this, &SHdriVaultCategoriesPanel::OnCategorySelectionChanged)
						.OnExpansionChanged(this, &SHdriVaultCategoriesPanel::OnCategoryExpansionChanged)
						.OnGetChildren(this, &SHdriVaultCategoriesPanel::OnGetCategoryChildren)
						.OnContextMenuOpening(this, &SHdriVaultCategoriesPanel::OnCategoryContextMenuOpening)
					]
				]
			]

			// Tags List
			+ SSplitter::Slot()
			.Value(0.4f)
			[
				CreateTagsPanel()
			]
		]
	];

	RefreshCategories();
	RefreshTags();
}

void SHdriVaultCategoriesPanel::RefreshCategories()
{
	BuildCategoryStructure();
	ApplyFilter();
	
	if (CategoryTreeView.IsValid())
	{
		CategoryTreeView->RequestTreeRefresh();
	}
}

void SHdriVaultCategoriesPanel::SetFilterText(const FString& FilterText)
{
	CurrentFilterText = FilterText;
	ApplyFilter();
	
	if (CategoryTreeView.IsValid())
	{
		CategoryTreeView->RequestTreeRefresh();
	}
}

void SHdriVaultCategoriesPanel::RefreshTags()
{
	AllTags.Empty();
	FilteredTags.Empty();

	if (!HdriVaultManager) return;

	// Collect all unique tags
	TSet<FString> UniqueTags;
	for (const auto& MaterialPair : HdriVaultManager->MaterialMap)
	{
		const TSharedPtr<FHdriVaultMaterialItem>& Material = MaterialPair.Value;
		if (Material.IsValid())
		{
			for (const FString& MatTag : Material->Metadata.Tags)
			{
				if (!MatTag.IsEmpty())
				{
					UniqueTags.Add(MatTag);
				}
			}
		}
	}

	// Create tag items
	for (const FString& TagStr : UniqueTags)
	{
		AllTags.Add(MakeShared<FString>(TagStr));
	}

	// Sort tags
	AllTags.Sort([](const TSharedPtr<FString>& A, const TSharedPtr<FString>& B) {
		return *A < *B;
	});

	// Filter tags (if needed) - currently showing all
	FilteredTags = AllTags;

	if (TagsListView.IsValid())
	{
		TagsListView->RequestListRefresh();
	}
}

TSharedPtr<FHdriVaultCategoryItem> SHdriVaultCategoriesPanel::GetSelectedCategory() const
{
	return SelectedCategory;
}

void SHdriVaultCategoriesPanel::SetSelectedCategory(TSharedPtr<FHdriVaultCategoryItem> Category)
{
	SelectedCategory = Category;
	if (CategoryTreeView.IsValid())
	{
		CategoryTreeView->SetSelection(Category);
	}
}

void SHdriVaultCategoriesPanel::SetSelectedCategoryByName(const FString& CategoryName)
{
	if (CategoryName.IsEmpty())
	{
		return;
	}

	// Helper lambda to find category recursively
	TFunction<TSharedPtr<FHdriVaultCategoryItem>(const TArray<TSharedPtr<FHdriVaultCategoryItem>>&, const FString&)> FindCategory;
	FindCategory = [&FindCategory](const TArray<TSharedPtr<FHdriVaultCategoryItem>>& Categories, const FString& Name) -> TSharedPtr<FHdriVaultCategoryItem>
	{
		for (const auto& Category : Categories)
		{
			if (Category->CategoryName == Name)
			{
				return Category;
			}
			
			TSharedPtr<FHdriVaultCategoryItem> FoundInChildren = FindCategory(Category->Children, Name);
			if (FoundInChildren.IsValid())
			{
				return FoundInChildren;
			}
		}
		return nullptr;
	};

	TSharedPtr<FHdriVaultCategoryItem> FoundCategory = FindCategory(RootCategories, CategoryName);
	if (FoundCategory.IsValid())
	{
		if (CategoryTreeView.IsValid())
		{
			// Ensure parents are expanded
			TSharedPtr<FHdriVaultCategoryItem> Parent = FoundCategory->Parent;
			while (Parent.IsValid())
			{
				CategoryTreeView->SetItemExpansion(Parent, true);
				Parent = Parent->Parent;
			}
			
			CategoryTreeView->SetSelection(FoundCategory);
			CategoryTreeView->RequestScrollIntoView(FoundCategory);
		}
	}
}

void SHdriVaultCategoriesPanel::SetSelectedTag(const FString& TagName)
{
	if (TagName.IsEmpty())
	{
		return;
	}

	TSharedPtr<FString> FoundTag;
	for (const auto& TagPtr : AllTags)
	{
		if (*TagPtr == TagName)
		{
			FoundTag = TagPtr;
			break;
		}
	}

	if (FoundTag.IsValid() && TagsListView.IsValid())
	{
		TagsListView->SetSelection(FoundTag);
		TagsListView->RequestScrollIntoView(FoundTag);
	}
}

TSharedRef<ITableRow> SHdriVaultCategoriesPanel::OnGenerateCategoryWidget(TSharedPtr<FHdriVaultCategoryItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SHdriVaultCategoryTreeItem, OwnerTable)
		.CategoryItem(Item);
}

void SHdriVaultCategoriesPanel::OnCategorySelectionChanged(TSharedPtr<FHdriVaultCategoryItem> SelectedItem, ESelectInfo::Type SelectInfo)
{
	SelectedCategory = SelectedItem;
	
	// Clear tag selection
	if (SelectedItem.IsValid() && TagsListView.IsValid())
	{
		TagsListView->ClearSelection();
	}

	OnCategorySelected.ExecuteIfBound(SelectedItem);
}

void SHdriVaultCategoriesPanel::OnCategoryExpansionChanged(TSharedPtr<FHdriVaultCategoryItem> Item, bool bIsExpanded)
{
	if (Item.IsValid())
	{
		Item->bIsExpanded = bIsExpanded;
	}
}

void SHdriVaultCategoriesPanel::OnGetCategoryChildren(TSharedPtr<FHdriVaultCategoryItem> Item, TArray<TSharedPtr<FHdriVaultCategoryItem>>& OutChildren)
{
	if (Item.IsValid())
	{
		OutChildren = Item->Children;
	}
}

TSharedPtr<SWidget> SHdriVaultCategoriesPanel::OnCategoryContextMenuOpening()
{
	if (!SelectedCategory.IsValid())
	{
		return nullptr;
	}

	if (SelectedCategory->CategoryName == TEXT("Uncategorized") || SelectedCategory->CategoryName == TEXT("All"))
	{
		return nullptr;
	}

	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("DeleteCategory", "Delete Category"),
		LOCTEXT("DeleteCategoryTooltip", "Delete this category and move assets to Uncategorized"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Delete"),
		FUIAction(FExecuteAction::CreateSP(this, &SHdriVaultCategoriesPanel::OnDeleteCategory, SelectedCategory))
	);

	return MenuBuilder.MakeWidget();
}

void SHdriVaultCategoriesPanel::OnFilterTextChanged(const FText& FilterText)
{
	CurrentFilterText = FilterText.ToString();
	ApplyFilter();
	
	if (CategoryTreeView.IsValid())
	{
		CategoryTreeView->RequestTreeRefresh();
	}
}

void SHdriVaultCategoriesPanel::BuildCategoryStructure()
{
	RootCategories.Empty();
	
	if (!HdriVaultManager) return;

	// Create "All" category
	TSharedPtr<FHdriVaultCategoryItem> AllCategory = MakeShared<FHdriVaultCategoryItem>(TEXT("All"));
	RootCategories.Add(AllCategory);
	
	// Create "Uncategorized" category
	TSharedPtr<FHdriVaultCategoryItem> UncategorizedCategory = MakeShared<FHdriVaultCategoryItem>(TEXT("Uncategorized"));
	
	for (const auto& MaterialPair : HdriVaultManager->MaterialMap)
	{
		const TSharedPtr<FHdriVaultMaterialItem>& Material = MaterialPair.Value;
		if (Material.IsValid())
		{
			// Add to "All"
			AllCategory->Materials.Add(Material);
			
			FString Category = Material->Metadata.Category;
			if (Category.IsEmpty())
			{
				UncategorizedCategory->Materials.Add(Material);
			}
			else
			{
				AddMaterialToCategory(Material, Category);
			}
		}
	}
	
	// Add Uncategorized if it has items
	if (UncategorizedCategory->Materials.Num() > 0)
	{
		RootCategories.Add(UncategorizedCategory);
	}
	
	// Sort categories
	RootCategories.Sort([](const TSharedPtr<FHdriVaultCategoryItem>& A, const TSharedPtr<FHdriVaultCategoryItem>& B) {
		// Keep "All" at top
		if (A->CategoryName == TEXT("All")) return true;
		if (B->CategoryName == TEXT("All")) return false;
		
		// Keep "Uncategorized" at bottom
		if (A->CategoryName == TEXT("Uncategorized")) return false;
		if (B->CategoryName == TEXT("Uncategorized")) return true;
		
		return A->CategoryName < B->CategoryName;
	});
}

TSharedPtr<FHdriVaultCategoryItem> SHdriVaultCategoriesPanel::GetOrCreateCategory(const FString& CategoryName)
{
	// Simple flat structure for now, can be enhanced to support nested categories (e.g. "Nature/Forest")
	
	for (const auto& Category : RootCategories)
	{
		if (Category->CategoryName == CategoryName)
		{
			return Category;
		}
	}
	
	TSharedPtr<FHdriVaultCategoryItem> NewCategory = MakeShared<FHdriVaultCategoryItem>(CategoryName);
	RootCategories.Add(NewCategory);
	return NewCategory;
}

void SHdriVaultCategoriesPanel::AddMaterialToCategory(TSharedPtr<FHdriVaultMaterialItem> Material, const FString& CategoryName)
{
	// Handle nested categories separated by / or |
	// For now, just simple single level
	
	TSharedPtr<FHdriVaultCategoryItem> Category = GetOrCreateCategory(CategoryName);
	Category->Materials.Add(Material);
}

void SHdriVaultCategoriesPanel::OnDeleteCategory(TSharedPtr<FHdriVaultCategoryItem> CategoryToDelete)
{
	if (!CategoryToDelete.IsValid()) return;
	
	DeleteCategoryRecursive(CategoryToDelete);
	
	// Refresh
	if (HdriVaultManager)
	{
		HdriVaultManager->RefreshMaterialDatabase();
	}
}

void SHdriVaultCategoriesPanel::DeleteCategoryRecursive(TSharedPtr<FHdriVaultCategoryItem> Category)
{
	// Move all materials to Uncategorized (empty category string)
	for (const auto& Material : Category->Materials)
	{
		if (Material.IsValid())
		{
			Material->Metadata.Category.Empty();
			if (HdriVaultManager)
			{
				HdriVaultManager->SaveMaterialMetadata(Material);
			}
		}
	}
	
	// Process children if any
	for (const auto& Child : Category->Children)
	{
		DeleteCategoryRecursive(Child);
	}
}

TSharedRef<SWidget> SHdriVaultCategoriesPanel::CreateTagsPanel()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(0)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("TagsLabel", "Tags"))
				.Font(FAppStyle::GetFontStyle("HeadingExtraSmall"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SAssignNew(TagsListView, SListView<TSharedPtr<FString>>)
				.ListItemsSource(&FilteredTags)
				.OnGenerateRow(this, &SHdriVaultCategoriesPanel::OnGenerateTagWidget)
				.OnSelectionChanged(this, &SHdriVaultCategoriesPanel::OnTagSelectionChanged)
				.OnContextMenuOpening(this, &SHdriVaultCategoriesPanel::OnTagContextMenuOpening)
			]
		];
}

TSharedRef<ITableRow> SHdriVaultCategoriesPanel::OnGenerateTagWidget(TSharedPtr<FString> TagItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
		[
			SNew(STextBlock)
			.Text(FText::FromString(*TagItem))
			.Margin(FMargin(4.0f, 2.0f))
		];
}

void SHdriVaultCategoriesPanel::OnTagSelectionChanged(TSharedPtr<FString> SelectedTag, ESelectInfo::Type SelectInfo)
{
	if (SelectedTag.IsValid())
	{
		// Clear category selection
		if (CategoryTreeView.IsValid())
		{
			CategoryTreeView->ClearSelection();
		}
		
		OnTagSelected.ExecuteIfBound(*SelectedTag);
	}
}

TSharedPtr<SWidget> SHdriVaultCategoriesPanel::OnTagContextMenuOpening()
{
	TArray<TSharedPtr<FString>> SelectedTags = TagsListView->GetSelectedItems();
	if (SelectedTags.Num() == 0)
	{
		return nullptr;
	}

	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("DeleteTag", "Delete Tag"),
		LOCTEXT("DeleteTagTooltip", "Remove this tag from all assets"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Delete"),
		FUIAction(FExecuteAction::CreateSP(this, &SHdriVaultCategoriesPanel::OnDeleteTag, SelectedTags[0]))
	);

	return MenuBuilder.MakeWidget();
}

void SHdriVaultCategoriesPanel::OnDeleteTag(TSharedPtr<FString> TagToDelete)
{
	if (!TagToDelete.IsValid() || !HdriVaultManager) return;
	
	FString TagName = *TagToDelete;
	
	// Remove tag from all materials
	for (const auto& MaterialPair : HdriVaultManager->MaterialMap)
	{
		const TSharedPtr<FHdriVaultMaterialItem>& Material = MaterialPair.Value;
		if (Material.IsValid() && Material->Metadata.Tags.Contains(TagName))
		{
			Material->Metadata.Tags.Remove(TagName);
			HdriVaultManager->SaveMaterialMetadata(Material);
		}
	}
	
	// Refresh
	HdriVaultManager->RefreshMaterialDatabase();
}

void SHdriVaultCategoriesPanel::ApplyFilter()
{
	FilteredCategories.Empty();
	
	if (CurrentFilterText.IsEmpty())
	{
		FilteredCategories = RootCategories;
		return;
	}
	
	// Filter recursively
	for (const auto& Category : RootCategories)
	{
		if (DoesCategoryPassFilter(Category))
		{
			FilteredCategories.Add(Category);
		}
	}
}

bool SHdriVaultCategoriesPanel::DoesCategoryPassFilter(TSharedPtr<FHdriVaultCategoryItem> Category) const
{
	if (!Category.IsValid()) return false;
	
	bool bMatches = Category->CategoryName.Contains(CurrentFilterText);
	bool bHasMatchingChildren = HasFilteredChildren(Category);
	
	return bMatches || bHasMatchingChildren;
}

bool SHdriVaultCategoriesPanel::HasFilteredChildren(TSharedPtr<FHdriVaultCategoryItem> Category) const
{
	if (!Category.IsValid()) return false;
	
	for (const auto& Child : Category->Children)
	{
		if (DoesCategoryPassFilter(Child))
		{
			return true;
		}
	}
	
	return false;
}

#undef LOCTEXT_NAMESPACE