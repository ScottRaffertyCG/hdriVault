// Copyright Pyre Labs. All Rights Reserved.

#include "SHdriVaultImportOptions.h"
#include "SHdriVaultMetadataPanel.h" // For SHdriVaultTagEditor
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "HdriVaultImportDialog"

void SHdriVaultImportDialog::Construct(const FArguments& InArgs)
{
	Options.Files = InArgs._Files;
	Options.DestinationPath = TEXT("/Game/HDRIs"); // Default path
	bShouldImport = false;
	ParentWindow = InArgs._ParentWindow;

	// Populate file list
	for (const FString& File : Options.Files)
	{
		FileList.Add(MakeShareable(new FString(FPaths::GetCleanFilename(File))));
	}

	// Initialize transient settings object for Property Editor
	ImportSettingsObject = NewObject<UHdriVaultImportSettings>();
	ImportSettingsObject->DestinationPath.Path = Options.DestinationPath;
	ImportSettingsObject->AddToRoot(); // Prevent GC

	// Create Property Editor
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	// DetailsViewArgs.bShowPropertyMatrix = false; // Removed as it's deprecated/removed in UE 5.6
	DetailsViewArgs.bShowObjectLabel = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	
	DestinationPathDetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DestinationPathDetailsView->SetObject(ImportSettingsObject);

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(16)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 16)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ImportTitle", "Import HDRIs"))
				.Font(FAppStyle::GetFontStyle("HeadingExtraSmall"))
			]
			
			// File List
			+ SVerticalBox::Slot()
			.FillHeight(0.3f)
			.Padding(0, 0, 0, 16)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder")) // Changed from Brushes.Recess to avoid checkerboard
				.Padding(4)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 0, 0, 4)
					[
						SNew(STextBlock)
						.Text(FText::Format(LOCTEXT("FilesToImport", "Files to Import ({0}):"), FText::AsNumber(Options.Files.Num())))
						.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SAssignNew(FileListView, SListView<TSharedPtr<FString>>)
						.ListItemsSource(&FileList)
						.OnGenerateRow(this, &SHdriVaultImportDialog::OnGenerateFileRow)
						.SelectionMode(ESelectionMode::None)
					]
				]
			]

			// Options
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 16)
			[
				SNew(SVerticalBox)
				// Destination Path (Using Details View) - Full Width
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 4)
				[
					SNew(SBox)
					.MinDesiredWidth(300.0f)
					[
						DestinationPathDetailsView.ToSharedRef()
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SUniformGridPanel)
					.SlotPadding(FMargin(0, 4))

					// Category
					+ SUniformGridPanel::Slot(0, 0)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("CategoryLabel", "Category:"))
					]
					+ SUniformGridPanel::Slot(1, 0)
					[
						SAssignNew(CategoryBox, SEditableTextBox)
					]

					// Author
					+ SUniformGridPanel::Slot(0, 1)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("AuthorLabel", "Author:"))
					]
					+ SUniformGridPanel::Slot(1, 1)
					[
						SAssignNew(AuthorBox, SEditableTextBox)
					]
				]
			]

			// Tags
			+ SVerticalBox::Slot()
			.FillHeight(0.2f)
			.Padding(0, 0, 0, 16)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 4)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("TagsLabel", "Tags (applied to all):"))
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SAssignNew(TagEditor, SHdriVaultTagEditor)
					.Tags(&Options.Tags)
				]
			]

			// Notes
			+ SVerticalBox::Slot()
			.FillHeight(0.2f)
			.Padding(0, 0, 0, 16)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 4)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("NotesLabel", "Notes (applied to all):"))
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SAssignNew(NotesBox, SMultiLineEditableTextBox)
				]
			]

			// Buttons
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0, 0, 8, 0)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "FlatButton")
					.OnClicked(this, &SHdriVaultImportDialog::OnCancelClicked)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("CancelButton", "Cancel"))
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
					.OnClicked(this, &SHdriVaultImportDialog::OnImportClicked)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ImportButton", "Import"))
					]
				]
			]
		]
	];
}

SHdriVaultImportDialog::~SHdriVaultImportDialog()
{
	if (ImportSettingsObject)
	{
		ImportSettingsObject->RemoveFromRoot();
		ImportSettingsObject = nullptr;
	}
}

TSharedRef<ITableRow> SHdriVaultImportDialog::OnGenerateFileRow(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
		[
			SNew(STextBlock)
			.Text(FText::FromString(*Item))
			.Margin(FMargin(4, 2))
		];
}

FReply SHdriVaultImportDialog::OnImportClicked()
{
	UpdateOptionsFromUI();
	bShouldImport = true;
	
	if (ParentWindow.IsValid())
	{
		ParentWindow->RequestDestroyWindow();
	}
	
	return FReply::Handled();
}

FReply SHdriVaultImportDialog::OnCancelClicked()
{
	bShouldImport = false;
	
	if (ParentWindow.IsValid())
	{
		ParentWindow->RequestDestroyWindow();
	}
	
	return FReply::Handled();
}

void SHdriVaultImportDialog::UpdateOptionsFromUI()
{
	if (ImportSettingsObject)
	{
		Options.DestinationPath = ImportSettingsObject->DestinationPath.Path;
	}
	if (CategoryBox.IsValid())
	{
		Options.Category = CategoryBox->GetText().ToString();
	}
	if (AuthorBox.IsValid())
	{
		Options.Author = AuthorBox->GetText().ToString();
	}
	if (NotesBox.IsValid())
	{
		Options.Notes = NotesBox->GetText().ToString();
	}
	// Tags are updated in real-time via pointer
}

#undef LOCTEXT_NAMESPACE
