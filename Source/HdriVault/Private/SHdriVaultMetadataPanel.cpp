// Copyright Pyre Labs. All Rights Reserved.

#include "SHdriVaultMetadataPanel.h"
#include "HdriVaultManager.h"
#include "Engine/Engine.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Images/SThrobber.h"
#include "AssetThumbnail.h"
#include "Engine/Texture2D.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "ToolMenus.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "HAL/PlatformApplicationMisc.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Misc/FileHelper.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "HdriVaultMetadataPanel"

void SHdriVaultTagEditor::Construct(const FArguments& InArgs)
{
	TagsPtr = InArgs._Tags;

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(0, 0, 0, 4)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(4)
			[
							SAssignNew(TagListView, SListView<TSharedPtr<FString>>)
			.ListItemsSource(&TagItems)
			.OnGenerateRow(this, &SHdriVaultTagEditor::OnGenerateTagWidget)
			.SelectionMode(ESelectionMode::None)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(0, 0, 4, 0)
			[
				SAssignNew(NewTagTextBox, SEditableTextBox)
				.HintText(LOCTEXT("AddTagHint", "Enter new tag..."))
				.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type CommitType)
				{
					if (CommitType == ETextCommit::OnEnter)
					{
						OnAddTag();
					}
				})
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
				.OnClicked(this, &SHdriVaultTagEditor::OnAddTag)
				.ToolTipText(LOCTEXT("AddTagTooltip", "Add tag"))
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("Icons.Plus"))
				]
			]
		]
	];

	RefreshTagList();
}

TSharedRef<ITableRow> SHdriVaultTagEditor::OnGenerateTagWidget(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
		.Padding(FMargin(2))
		[
					SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(FMargin(4, 2))
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(*Item))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(4, 0, 0, 0)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "FlatButton.Danger")
					.ContentPadding(FMargin(2))
					.OnClicked_Lambda([this, Item]()
					{
						OnRemoveTag(Item);
						return FReply::Handled();
					})
					.ToolTipText(LOCTEXT("RemoveTagTooltip", "Remove tag"))
					[
						SNew(SImage)
						.Image(FAppStyle::GetBrush("Icons.X"))
						.ColorAndOpacity(FSlateColor::UseForeground())
					]
				]
			]
		];
}

FReply SHdriVaultTagEditor::OnAddTag()
{
	FString NewTag = NewTagTextBox->GetText().ToString().TrimStartAndEnd();
	if (!NewTag.IsEmpty() && TagsPtr && !TagsPtr->Contains(NewTag))
	{
		TagsPtr->Add(NewTag);
		RefreshTagList();
		NotifyTagsChanged();
		NewTagTextBox->SetText(FText::GetEmpty());
	}
	return FReply::Handled();
}

void SHdriVaultTagEditor::OnRemoveTag(TSharedPtr<FString> TagToRemove)
{
	if (TagToRemove.IsValid() && TagsPtr)
	{
		TagsPtr->Remove(*TagToRemove);
		RefreshTagList();
		NotifyTagsChanged();
	}
}

void SHdriVaultTagEditor::RefreshTagList()
{
	TagItems.Empty();
	if (TagsPtr)
	{
			for (const FString& ItemTag : *TagsPtr)
	{
		TagItems.Add(MakeShareable(new FString(ItemTag)));
	}
	}
	
	if (TagListView.IsValid())
	{
		TagListView->RequestListRefresh();
	}
}

void SHdriVaultTagEditor::SetTags(TArray<FString>* InTags)
{
	TagsPtr = InTags;
	RefreshTagList();
}

void SHdriVaultTagEditor::NotifyTagsChanged()
{
	if (TagsPtr)
	{
		OnTagsChanged.ExecuteIfBound(*TagsPtr);
	}
}

void SHdriVaultTextureItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	TextureItem = InArgs._TextureItem;

	// Create asset thumbnail
	if (TextureItem.IsValid() && !TextureItem->Texture.IsNull())
	{
		FAssetData TextureAssetData = FAssetData(TextureItem->Texture.Get());
		AssetThumbnail = MakeShareable(new FAssetThumbnail(TextureAssetData, 32, 32, UThumbnailManager::Get().GetSharedThumbnailPool()));
	}

	STableRow<TSharedPtr<FHdriVaultTextureItem>>::Construct(
		STableRow::FArguments()
		.Style(FAppStyle::Get(), "ContentBrowser.AssetListView.ColumnListTableRow")
		.Padding(FMargin(0, 1, 0, 1))
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
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(this, &SHdriVaultTextureItem::GetTextureName)
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
					.ToolTipText(this, &SHdriVaultTextureItem::GetTextureTooltip)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(this, &SHdriVaultTextureItem::GetTextureInfo)
					.Font(FAppStyle::GetFontStyle("PropertyWindow.SmallFont"))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]
		],
		InOwnerTableView
	);
}

FReply SHdriVaultTextureItem::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && TextureItem.IsValid())
	{
		OnTextureDoubleClicked.ExecuteIfBound(TextureItem->Texture);
		return FReply::Handled();
	}

	return STableRow<TSharedPtr<FHdriVaultTextureItem>>::OnMouseButtonDoubleClick(InMyGeometry, InMouseEvent);
}

FText SHdriVaultTextureItem::GetTextureName() const
{
	if (TextureItem.IsValid() && !TextureItem->Texture.IsNull())
	{
		return FText::FromString(TextureItem->Texture.GetAssetName());
	}
	return LOCTEXT("InvalidTexture", "Invalid Texture");
}

FText SHdriVaultTextureItem::GetTextureInfo() const
{
	if (TextureItem.IsValid() && !TextureItem->Texture.IsNull())
	{
		UTexture2D* LoadedTexture = TextureItem->Texture.LoadSynchronous();
		if (LoadedTexture)
		{
			return FText::Format(LOCTEXT("TextureInfoFormat", "{0}x{1}"), 
				FText::AsNumber(LoadedTexture->GetSizeX()), 
				FText::AsNumber(LoadedTexture->GetSizeY()));
		}
	}
	return FText::GetEmpty();
}

FText SHdriVaultTextureItem::GetTextureTooltip() const
{
	if (TextureItem.IsValid() && !TextureItem->Texture.IsNull())
	{
		FString TooltipText = FString::Printf(TEXT("Texture: %s\nPath: %s"),
			*TextureItem->Texture.GetAssetName(),
			*TextureItem->Texture.ToString()
		);
		return FText::FromString(TooltipText);
	}
	return FText::GetEmpty();
}

void SHdriVaultTextureDependencies::Construct(const FArguments& InArgs)
{
	MaterialItem = InArgs._MaterialItem;
	HdriVaultManager = GEditor->GetEditorSubsystem<UHdriVaultManager>();

	ChildSlot
	[
			SAssignNew(TextureListView, SListView<TSharedPtr<FHdriVaultTextureItem>>)
	.ListItemsSource(&TextureDependencies)
	.OnGenerateRow(this, &SHdriVaultTextureDependencies::OnGenerateTextureWidget)
	.SelectionMode(ESelectionMode::None)
	];

	RefreshTextureDependencies();
}

void SHdriVaultTextureDependencies::SetMaterialItem(TSharedPtr<FHdriVaultMaterialItem> InMaterialItem)
{
	MaterialItem = InMaterialItem;
	RefreshTextureDependencies();
}

TSharedRef<ITableRow> SHdriVaultTextureDependencies::OnGenerateTextureWidget(TSharedPtr<FHdriVaultTextureItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	TSharedRef<SHdriVaultTextureItem> TextureWidget = SNew(SHdriVaultTextureItem, OwnerTable)
		.TextureItem(Item);

	TextureWidget->OnTextureDoubleClicked.BindSP(this, &SHdriVaultTextureDependencies::OnTextureDoubleClicked);

	return TextureWidget;
}

void SHdriVaultTextureDependencies::RefreshTextureDependencies()
{
	TextureDependencies.Empty();

	if (MaterialItem.IsValid() && HdriVaultManager)
	{
		// Load dependencies if not already loaded
		if (MaterialItem->TextureDependencies.Num() == 0)
		{
			HdriVaultManager->LoadMaterialDependencies(MaterialItem);
		}

		// Convert to wrapper items
		for (const auto& TexturePtr : MaterialItem->TextureDependencies)
		{
			TextureDependencies.Add(MakeShareable(new FHdriVaultTextureItem(TexturePtr)));
		}
	}

	if (TextureListView.IsValid())
	{
		TextureListView->RequestListRefresh();
	}
}

void SHdriVaultTextureDependencies::OnTextureDoubleClicked(TSoftObjectPtr<UTexture2D> Texture)
{
	if (!Texture.IsNull())
	{
		// Browse to texture in Content Browser
		FAssetData AssetData = FAssetData(Texture.Get());
		TArray<FAssetData> AssetDataArray;
		AssetDataArray.Add(AssetData);

		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets(AssetDataArray);
	}
}

void SHdriVaultMetadataPanel::Construct(const FArguments& InArgs)
{
	HdriVaultManager = GEditor->GetEditorSubsystem<UHdriVaultManager>();
	bHasUnsavedChanges = false;

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
				.Text(LOCTEXT("MetadataTitle", "Metadata"))
				.Font(FAppStyle::GetFontStyle("ContentBrowser.SourceTitleFont"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					// No selection message
					SNew(SBox)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Visibility(this, &SHdriVaultMetadataPanel::GetNoSelectionVisibility)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("NoMaterialSelected", "Select a material to view its metadata"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						.Justification(ETextJustify::Center)
					]
				]
				+ SOverlay::Slot()
				[
					// Content
					SAssignNew(ContentScrollBox, SScrollBox)
					.Visibility(this, &SHdriVaultMetadataPanel::GetContentVisibility)
					+ SScrollBox::Slot()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 8)
						[
							CreateMaterialPreview()
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 8)
						[
							CreateBasicProperties()
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 8)
						[
							CreateTagsSection()
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 8)
						[
							CreateNotesSection()
						]
						// Texture dependencies not needed for HdriVault
						/*
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 8)
						[
							CreateTextureDependenciesSection()
						]
						*/
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							CreateActionButtons()
						]
					]
				]
			]
		]
	];

	UpdatePreviewWidget();
}

void SHdriVaultMetadataPanel::SetMaterialItem(TSharedPtr<FHdriVaultMaterialItem> InMaterialItem)
{
	// Save current changes if any
	if (bHasUnsavedChanges && MaterialItem.IsValid())
	{
		SaveMetadata();
	}

	MaterialItem = InMaterialItem;
	PreviewThumbnail.Reset();
	CustomPreviewBrush.Reset();
	CurrentPreviewTexture.Reset();
	
	if (MaterialItem.IsValid())
	{
		// Load metadata if not already loaded
		if (HdriVaultManager)
		{
			HdriVaultManager->LoadMaterialMetadata(MaterialItem);
		}
		
		OriginalMetadata = MaterialItem->Metadata;
		bHasUnsavedChanges = false;
	}

	RefreshCustomPreviewBrush();
	UpdateUI();
}

void SHdriVaultMetadataPanel::RefreshMetadata()
{
	if (MaterialItem.IsValid() && HdriVaultManager)
	{
		HdriVaultManager->LoadMaterialMetadata(MaterialItem);
		OriginalMetadata = MaterialItem->Metadata;
		bHasUnsavedChanges = false;
		UpdateUI();
	}
}

void SHdriVaultMetadataPanel::SaveMetadata()
{
	if (MaterialItem.IsValid() && HdriVaultManager && bHasUnsavedChanges)
	{
		// Check if material name changed and offer to rename asset
		FString NewName = MaterialItem->Metadata.MaterialName;
		FString CurrentAssetName = MaterialItem->AssetData.AssetName.ToString();
		
		if (!NewName.IsEmpty() && NewName != CurrentAssetName)
		{
			// Store original name in metadata before renaming
			if (OriginalMetadata.MaterialName.IsEmpty())
			{
				MaterialItem->Metadata.MaterialName = CurrentAssetName;
			}
			
			// Try to rename the actual asset
			if (RenameAsset(NewName))
			{
				// If successful, keep the new name in metadata
				MaterialItem->Metadata.MaterialName = NewName;
			}
			else
			{
				// If failed, revert to original asset name but keep user's desired name in metadata
				MaterialItem->Metadata.MaterialName = NewName;
			}
		}
		
		HdriVaultManager->SaveMaterialMetadata(MaterialItem);
		OriginalMetadata = MaterialItem->Metadata;
		bHasUnsavedChanges = false;
		OnMetadataChanged.ExecuteIfBound(MaterialItem);
	}
}

bool SHdriVaultMetadataPanel::HasUnsavedChanges() const
{
	return bHasUnsavedChanges;
}

TSharedRef<SWidget> SHdriVaultMetadataPanel::CreateMaterialPreview()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(8)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0, 0, 0, 8)
			[
				SNew(SBox)
				.WidthOverride(PreviewImageSize.X)
				.HeightOverride(PreviewImageSize.Y)
				[
					SAssignNew(PreviewImageContainer, SBorder)
					.BorderImage(FAppStyle::GetBrush("ContentBrowser.AssetTileItem.DropShadow"))
					.OnMouseButtonUp(FPointerEventHandler::CreateLambda([this](const FGeometry& Geometry, const FPointerEvent& MouseEvent) -> FReply
					{
						if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
						{
							TSharedPtr<SWidget> ContextMenu = OnMaterialPreviewContextMenuOpening();
							if (ContextMenu.IsValid())
							{
								FSlateApplication::Get().PushMenu(
									AsShared(),
									FWidgetPath(),
									ContextMenu.ToSharedRef(),
									MouseEvent.GetScreenSpacePosition(),
									FPopupTransitionEffect::ContextMenu
								);
								return FReply::Handled();
							}
						}
						return FReply::Unhandled();
					}))
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						CreatePreviewPlaceholder()
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0, 0, 8, 0)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "FlatButton")
					.OnClicked(this, &SHdriVaultMetadataPanel::OnBrowseToMaterialClicked)
					.ToolTipText(LOCTEXT("BrowseToMaterialTooltip", "Browse to material in Content Browser"))
					[
						SNew(SImage)
						.Image(FAppStyle::GetBrush("Icons.FolderOpen"))
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "FlatButton")
					.OnClicked(this, &SHdriVaultMetadataPanel::OnOpenMaterialEditorClicked)
					.ToolTipText(LOCTEXT("OpenMaterialEditorTooltip", "Open material in Material Editor"))
					[
						SNew(SImage)
						.Image(FAppStyle::GetBrush("Icons.Edit"))
					]
				]
			]
		];
}

TSharedRef<SWidget> SHdriVaultMetadataPanel::CreateBasicProperties()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(8)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 4)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("BasicPropertiesTitle", "Properties"))
				.Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SUniformGridPanel)
				.SlotPadding(FMargin(0, 2))
				+ SUniformGridPanel::Slot(0, 0)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("MaterialNameLabel", "Name:"))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				]
				+ SUniformGridPanel::Slot(1, 0)
				[
					SAssignNew(MaterialNameTextBox, SEditableTextBox)
					.IsEnabled(this, &SHdriVaultMetadataPanel::IsEnabled)
					.OnTextChanged(this, &SHdriVaultMetadataPanel::OnMaterialNameChanged)
					.ToolTipText(LOCTEXT("MaterialNameTooltip", "Display name for this material (metadata only, does not rename the actual asset)"))
				]
				+ SUniformGridPanel::Slot(0, 1)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SizeLabel", "Size:"))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				]
				+ SUniformGridPanel::Slot(1, 1)
				[
					SAssignNew(SizeTextBlock, STextBlock)
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
				+ SUniformGridPanel::Slot(0, 2)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("LocationLabel", "Location:"))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				]
				+ SUniformGridPanel::Slot(1, 2)
				[
					SAssignNew(LocationTextBlock, STextBlock)
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
				+ SUniformGridPanel::Slot(0, 3)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("AuthorLabel", "Author:"))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				]
				+ SUniformGridPanel::Slot(1, 3)
				[
					SAssignNew(AuthorTextBox, SEditableTextBox)
					.IsEnabled(this, &SHdriVaultMetadataPanel::IsEnabled)
					.OnTextChanged(this, &SHdriVaultMetadataPanel::OnAuthorChanged)
				]
				+ SUniformGridPanel::Slot(0, 4)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("CategoryLabel", "Category:"))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				]
				+ SUniformGridPanel::Slot(1, 4)
				[
					SAssignNew(CategoryTextBox, SEditableTextBox)
					.IsEnabled(this, &SHdriVaultMetadataPanel::IsEnabled)
					.OnTextChanged(this, &SHdriVaultMetadataPanel::OnCategoryChanged)
				]
				+ SUniformGridPanel::Slot(0, 5)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("LastModifiedLabel", "Modified:"))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				]
				+ SUniformGridPanel::Slot(1, 5)
				[
					SAssignNew(LastModifiedTextBlock, STextBlock)
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]
		];
}

TSharedRef<SWidget> SHdriVaultMetadataPanel::CreateTagsSection()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(8)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 4)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("TagsTitle", "Tags"))
				.Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SBox)
				.HeightOverride(120)
				[
					SAssignNew(TagEditor, SHdriVaultTagEditor)
					.Tags(MaterialItem.IsValid() ? &MaterialItem->Metadata.Tags : nullptr)
				]
			]
		];
}

TSharedRef<SWidget> SHdriVaultMetadataPanel::CreateNotesSection()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(8)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 4)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("NotesTitle", "Notes"))
				.Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SBox)
				.HeightOverride(80)
				[
					SAssignNew(NotesTextBox, SMultiLineEditableTextBox)
					.IsEnabled(this, &SHdriVaultMetadataPanel::IsEnabled)
					.OnTextChanged(this, &SHdriVaultMetadataPanel::OnNotesChanged)
					.WrapTextAt(0)
				]
			]
		];
}

TSharedRef<SWidget> SHdriVaultMetadataPanel::CreateTextureDependenciesSection()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(8)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 4)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("TextureDependenciesTitle", "Texture Dependencies"))
				.Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SBox)
				.HeightOverride(120)
				[
					SAssignNew(TextureDependencies, SHdriVaultTextureDependencies)
					.MaterialItem(MaterialItem)
				]
			]
		];
}

TSharedRef<SWidget> SHdriVaultMetadataPanel::CreateActionButtons()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(8)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNullWidget::NullWidget
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 4, 0)
			[
				SAssignNew(RevertButton, SButton)
				.ButtonStyle(FAppStyle::Get(), "FlatButton")
				.OnClicked(this, &SHdriVaultMetadataPanel::OnRevertClicked)
				.IsEnabled_Lambda([this]() { return bHasUnsavedChanges; })
				.ToolTipText(LOCTEXT("RevertTooltip", "Revert changes"))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("RevertButton", "Revert"))
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(SaveButton, SButton)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
				.OnClicked(this, &SHdriVaultMetadataPanel::OnSaveClicked)
				.Visibility(this, &SHdriVaultMetadataPanel::GetSaveButtonVisibility)
				.ToolTipText(LOCTEXT("SaveTooltip", "Save metadata changes"))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SaveButton", "Save"))
					.ColorAndOpacity(this, &SHdriVaultMetadataPanel::GetSaveButtonColor)
				]
			]
		];
}

void SHdriVaultMetadataPanel::OnMaterialNameChanged(const FText& NewText)
{
	if (MaterialItem.IsValid())
	{
		// Just update metadata, don't rename asset until Save is clicked
		MaterialItem->Metadata.MaterialName = NewText.ToString();
		MarkAsChanged();
	}
}

void SHdriVaultMetadataPanel::OnAuthorChanged(const FText& NewText)
{
	if (MaterialItem.IsValid())
	{
		MaterialItem->Metadata.Author = NewText.ToString();
		MarkAsChanged();
	}
}

void SHdriVaultMetadataPanel::OnCategoryChanged(const FText& NewText)
{
	if (MaterialItem.IsValid())
	{
		MaterialItem->Metadata.Category = NewText.ToString();
		MarkAsChanged();
	}
}

void SHdriVaultMetadataPanel::OnNotesChanged(const FText& NewText)
{
	if (MaterialItem.IsValid())
	{
		MaterialItem->Metadata.Notes = NewText.ToString();
		MarkAsChanged();
	}
}

void SHdriVaultMetadataPanel::OnTagsChanged(const TArray<FString>& NewTags)
{
	if (MaterialItem.IsValid())
	{
		MaterialItem->Metadata.Tags = NewTags;
		MarkAsChanged();
	}
}

FReply SHdriVaultMetadataPanel::OnSaveClicked()
{
	SaveMetadata();
	return FReply::Handled();
}

FReply SHdriVaultMetadataPanel::OnRevertClicked()
{
	if (MaterialItem.IsValid())
	{
		MaterialItem->Metadata = OriginalMetadata;
		bHasUnsavedChanges = false;
		UpdateUI();
	}
	return FReply::Handled();
}

FReply SHdriVaultMetadataPanel::OnBrowseToMaterialClicked()
{
	if (MaterialItem.IsValid())
	{
		TArray<FAssetData> AssetDataArray;
		AssetDataArray.Add(MaterialItem->AssetData);

		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets(AssetDataArray);
	}
	return FReply::Handled();
}

FReply SHdriVaultMetadataPanel::OnOpenMaterialEditorClicked()
{
	if (MaterialItem.IsValid())
	{
		UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		if (AssetEditorSubsystem)
		{
			UObject* MaterialObject = MaterialItem->AssetData.GetAsset();
			if (MaterialObject)
			{
				AssetEditorSubsystem->OpenEditorForAsset(MaterialObject);
			}
		}
	}
	return FReply::Handled();
}

void SHdriVaultMetadataPanel::UpdateUI()
{
	if (MaterialItem.IsValid())
	{
		// Update text boxes
		if (MaterialNameTextBox.IsValid())
		{
			MaterialNameTextBox->SetText(FText::FromString(MaterialItem->Metadata.MaterialName));
		}
		
		if (SizeTextBlock.IsValid())
		{
			SizeTextBlock->SetText(GetMaterialSizeText());
		}

		if (LocationTextBlock.IsValid())
		{
			LocationTextBlock->SetText(FText::FromString(MaterialItem->Metadata.Location));
		}
		
		if (AuthorTextBox.IsValid())
		{
			AuthorTextBox->SetText(FText::FromString(MaterialItem->Metadata.Author));
		}
		
		if (CategoryTextBox.IsValid())
		{
			CategoryTextBox->SetText(FText::FromString(MaterialItem->Metadata.Category));
		}
		
		if (LastModifiedTextBlock.IsValid())
		{
			LastModifiedTextBlock->SetText(FText::FromString(MaterialItem->Metadata.LastModified.ToString()));
		}
		
		if (NotesTextBox.IsValid())
		{
			NotesTextBox->SetText(FText::FromString(MaterialItem->Metadata.Notes));
		}

		// Update tag editor
		if (TagEditor.IsValid())
		{
			TagEditor->SetTags(&MaterialItem->Metadata.Tags);
			TagEditor->OnTagsChanged.BindSP(this, &SHdriVaultMetadataPanel::OnTagsChanged);
		}

		// Update texture dependencies
		if (TextureDependencies.IsValid())
		{
			TextureDependencies->SetMaterialItem(MaterialItem);
		}
	}

	UpdatePreviewWidget();
}

void SHdriVaultMetadataPanel::MarkAsChanged()
{
	if (!bHasUnsavedChanges)
	{
		bHasUnsavedChanges = true;
		if (MaterialItem.IsValid())
		{
			MaterialItem->Metadata.LastModified = FDateTime::Now();
		}
	}
}

void SHdriVaultMetadataPanel::UpdatePreviewWidget()
{
	if (!PreviewImageContainer.IsValid())
	{
		return;
	}

	TSharedPtr<SWidget> ContentWidget;

	if ((!CustomPreviewBrush.IsValid() || !CurrentPreviewTexture.IsValid()) && MaterialItem.IsValid() && !MaterialItem->Metadata.CustomThumbnailPath.IsEmpty())
	{
		RefreshCustomPreviewBrush();
	}

	if (CustomPreviewBrush.IsValid() && CurrentPreviewTexture.IsValid())
	{
		ContentWidget = SNew(SImage)
			.Image(CustomPreviewBrush.Get());
	}
	else if (MaterialItem.IsValid())
	{
		PreviewThumbnail = MakeShareable(new FAssetThumbnail(MaterialItem->AssetData, PreviewImageSize.X, PreviewImageSize.Y, UThumbnailManager::Get().GetSharedThumbnailPool()));
		ContentWidget = PreviewThumbnail->MakeThumbnailWidget();
	}
	else
	{
		CustomPreviewBrush.Reset();
		CurrentPreviewTexture.Reset();
		ContentWidget = CreatePreviewPlaceholder();
	}

	if (ContentWidget.IsValid())
	{
		PreviewImageContainer->SetContent(ContentWidget.ToSharedRef());
	}
	else
	{
		PreviewImageContainer->SetContent(SNullWidget::NullWidget);
	}
}

TSharedRef<SWidget> SHdriVaultMetadataPanel::CreatePreviewPlaceholder() const
{
	return SNew(STextBlock)
		.Text(LOCTEXT("MaterialPreview", "HDRI Preview\n(Right-click to change thumbnail)"))
		.Justification(ETextJustify::Center);
}

void SHdriVaultMetadataPanel::RefreshCustomPreviewBrush()
{
	CustomPreviewBrush.Reset();
	CurrentPreviewTexture.Reset();

	if (MaterialItem.IsValid() && !MaterialItem->Metadata.CustomThumbnailPath.IsEmpty())
	{
		if (UTexture2D* CustomTexture = LoadObject<UTexture2D>(nullptr, *MaterialItem->Metadata.CustomThumbnailPath))
		{
			CurrentPreviewTexture = CustomTexture;
			CustomPreviewBrush = MakeShareable(new FSlateDynamicImageBrush(CustomTexture, PreviewImageSize, CustomTexture->GetFName()));
		}
	}
}

void SHdriVaultMetadataPanel::MarkAsClean()
{
	bHasUnsavedChanges = false;
}

bool SHdriVaultMetadataPanel::IsEnabled() const
{
	return MaterialItem.IsValid();
}

FText SHdriVaultMetadataPanel::GetMaterialTypeText() const
{
	if (MaterialItem.IsValid())
	{
		return FText::FromString(MaterialItem->AssetData.AssetClassPath.GetAssetName().ToString());
	}
	return FText::GetEmpty();
}

FText SHdriVaultMetadataPanel::GetMaterialSizeText() const
{
	if (!MaterialItem.IsValid())
	{
		return LOCTEXT("SizeUnknown", "Unknown");
	}

	FString Dimensions = TEXT("Unknown");
	MaterialItem->AssetData.GetTagValue(TEXT("Dimensions"), Dimensions);
	
	int64 ResourceSizeBytes = 0;
	MaterialItem->AssetData.GetTagValue(TEXT("ResourceSize"), ResourceSizeBytes);
	
	// Format: 2048x1024 (4.0 MB)
	if (ResourceSizeBytes > 0)
	{
		return FText::Format(LOCTEXT("SizeFormat", "{0} ({1})"), 
			FText::FromString(Dimensions), 
			FText::AsMemory(ResourceSizeBytes));
	}
	
	return FText::FromString(Dimensions);
}

EVisibility SHdriVaultMetadataPanel::GetNoSelectionVisibility() const
{
	return MaterialItem.IsValid() ? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility SHdriVaultMetadataPanel::GetContentVisibility() const
{
	return MaterialItem.IsValid() ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SHdriVaultMetadataPanel::GetSaveButtonVisibility() const
{
	return bHasUnsavedChanges ? EVisibility::Visible : EVisibility::Collapsed;
}

FSlateColor SHdriVaultMetadataPanel::GetSaveButtonColor() const
{
	return bHasUnsavedChanges ? FSlateColor(FLinearColor::White) : FSlateColor::UseSubduedForeground();
}

TSharedPtr<SWidget> SHdriVaultMetadataPanel::OnMaterialPreviewContextMenuOpening()
{
	if (!MaterialItem.IsValid())
	{
		return SNullWidget::NullWidget;
	}
	
	FMenuBuilder MenuBuilder(true, nullptr);
	
	MenuBuilder.AddMenuEntry(
		FUIAction(FExecuteAction::CreateSP(this, &SHdriVaultMetadataPanel::OnChangeThumbnail)),
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0, 0, 4, 0)
		[
			SNew(SImage)
			.Image(FAppStyle::GetBrush("EditorStyle.PaletteIcon"))
		]
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("UploadSwatch", "Upload Custom Swatch Preview"))
		],
		NAME_None,
		LOCTEXT("ChangeThumbnailTooltip", "Select a custom preview image for this material")
	);
	
	return MenuBuilder.MakeWidget();
}

void SHdriVaultMetadataPanel::OnChangeThumbnail()
{
	if (!MaterialItem.IsValid() || !HdriVaultManager)
	{
		return;
	}

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return;
	}

	void* ParentWindowHandle = nullptr;
	if (FSlateApplication::IsInitialized())
	{
		const TSharedPtr<SWindow> ActiveWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
		ParentWindowHandle = ActiveWindow.IsValid() ? ActiveWindow->GetNativeWindow()->GetOSWindowHandle() : nullptr;
	}

	TArray<FString> SelectedFiles;
	const bool bOpened = DesktopPlatform->OpenFileDialog(
		ParentWindowHandle,
		TEXT("Select Swatch Image"),
		FPaths::ProjectContentDir(),
		TEXT(""),
		TEXT("Image Files|*.png;*.jpg;*.jpeg;*.bmp"),
		EFileDialogFlags::None,
		SelectedFiles
	);

	if (!bOpened || SelectedFiles.Num() == 0)
	{
		return;
	}

	const FString SourceFile = SelectedFiles[0];
	if (!PlatformFile.FileExists(*SourceFile))
	{
		return;
	}

	if (UTexture2D* ImportedTexture = HdriVaultManager->ImportCustomThumbnail(MaterialItem, SourceFile))
	{
		PreviewThumbnail.Reset();
		MaterialItem->Metadata.CustomThumbnailPath = ImportedTexture->GetPathName();
		CustomPreviewBrush = MakeShareable(new FSlateDynamicImageBrush(ImportedTexture, PreviewImageSize, ImportedTexture->GetFName()));
		CurrentPreviewTexture = ImportedTexture;
		UpdatePreviewWidget();
		MarkAsChanged();
	}
}

bool SHdriVaultMetadataPanel::RenameAsset(const FString& NewName)
{
	if (!MaterialItem.IsValid())
	{
		return false;
	}

	// Validate new name
	if (NewName.IsEmpty())
	{
		return false;
	}

	// Just update the metadata name (no actual asset renaming)
	FString OldName = MaterialItem->Metadata.MaterialName;
	MaterialItem->Metadata.MaterialName = NewName;
	MaterialItem->DisplayName = NewName;
	
	// Mark as changed for saving
	MarkAsChanged();

	return true;
}

#undef LOCTEXT_NAMESPACE 