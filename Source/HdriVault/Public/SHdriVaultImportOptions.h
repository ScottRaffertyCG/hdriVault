// Copyright Pyre Labs. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Views/SListView.h"
#include "UObject/NoExportTypes.h"
#include "SHdriVaultImportOptions.generated.h"

class SHdriVaultTagEditor;
class IDetailsView;

// UObject wrapper to allow utilizing the Property Editor with ContentDir metadata
UCLASS()
class UHdriVaultImportSettings : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Import", meta = (ContentDir, DisplayName = "Destination Path"))
	FDirectoryPath DestinationPath;
};

struct FHdriVaultImportOptions
{
	TArray<FString> Files;
	FString DestinationPath;
	FString Category;
	FString Author;
	TArray<FString> Tags;
	FString Notes;
};

class SHdriVaultImportDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SHdriVaultImportDialog) {}
		SLATE_ARGUMENT(TArray<FString>, Files)
		SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SHdriVaultImportDialog();

	bool ShouldImport() const { return bShouldImport; }
	const FHdriVaultImportOptions& GetImportOptions() const { return Options; }

private:
	FHdriVaultImportOptions Options;
	bool bShouldImport;
	TSharedPtr<SWindow> ParentWindow;

	// Data object for property editor
	UHdriVaultImportSettings* ImportSettingsObject;

	// UI Components
	TArray<TSharedPtr<FString>> FileList;
	TSharedPtr<SListView<TSharedPtr<FString>>> FileListView;
	TSharedPtr<IDetailsView> DestinationPathDetailsView;
	TSharedPtr<SEditableTextBox> CategoryBox;
	TSharedPtr<SEditableTextBox> AuthorBox;
	TSharedPtr<SHdriVaultTagEditor> TagEditor;
	TSharedPtr<SMultiLineEditableTextBox> NotesBox;

	// Callbacks
	TSharedRef<ITableRow> OnGenerateFileRow(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable);
	FReply OnImportClicked();
	FReply OnCancelClicked();

	// Helper to update options struct from UI
	void UpdateOptionsFromUI();
};
