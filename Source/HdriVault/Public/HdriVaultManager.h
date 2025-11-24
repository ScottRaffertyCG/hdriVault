// Copyright Pyre Labs. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInterface.h"
#include "HdriVaultTypes.h"
#include "EditorSubsystem.h"
#include "HdriVaultManager.generated.h"

UCLASS()
class HDRIVAULT_API UHdriVaultManager : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	UHdriVaultManager();

	// USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Main functionality
	void RefreshMaterialDatabase();
	void BuildFolderStructure();
	void LoadMaterialsFromFolder(const FString& FolderPath);
	
	// Folder operations
	TSharedPtr<FHdriVaultFolderNode> GetRootFolder() const { return RootFolderNode; }
	TSharedPtr<FHdriVaultFolderNode> FindFolder(const FString& FolderPath) const;
	TArray<TSharedPtr<FHdriVaultFolderNode>> GetChildFolders(const FString& FolderPath) const;
	
	// Material operations
	TArray<TSharedPtr<FHdriVaultMaterialItem>> GetMaterialsInFolder(const FString& FolderPath) const;
	TSharedPtr<FHdriVaultMaterialItem> GetMaterialByPath(const FString& AssetPath) const;
	void LoadMaterialThumbnail(TSharedPtr<FHdriVaultMaterialItem> MaterialItem);
	void LoadMaterialDependencies(TSharedPtr<FHdriVaultMaterialItem> MaterialItem);
	void ApplyMaterialToSelection(TSharedPtr<FHdriVaultMaterialItem> MaterialItem);
	
	// Metadata operations
	void SaveMaterialMetadata(TSharedPtr<FHdriVaultMaterialItem> MaterialItem);
	void LoadMaterialMetadata(TSharedPtr<FHdriVaultMaterialItem> MaterialItem);
	void RegenerateMaterialThumbnail(TSharedPtr<FHdriVaultMaterialItem> MaterialItem, int32 ThumbnailSize = 512);
	UTexture2D* ImportCustomThumbnail(TSharedPtr<FHdriVaultMaterialItem> MaterialItem, const FString& SourceFile, int32 ThumbnailSize = 512);
	
	// Import operations
	void ImportHdriFiles(const TArray<FString>& Files);

	// Settings
	const FHdriVaultSettings& GetSettings() const { return Settings; }
	void SetSettings(const FHdriVaultSettings& NewSettings);
	
	// Search and filtering
	TArray<TSharedPtr<FHdriVaultMaterialItem>> SearchMaterials(const FString& SearchTerm) const;
	TArray<TSharedPtr<FHdriVaultMaterialItem>> FilterMaterialsByTag(const FString& Tag) const;
	
	// Delegates
	FOnHdriVaultFolderSelected OnFolderSelected;
	FOnHdriVaultMaterialSelected OnMaterialSelected;
	FOnHdriVaultMaterialDoubleClicked OnMaterialDoubleClicked;
	FOnHdriVaultSettingsChanged OnSettingsChanged;
	FOnHdriVaultRefreshRequested OnRefreshRequested;

	// Needs to be public for SHdriVaultCategoriesPanel to access efficiently
	TMap<FString, TSharedPtr<FHdriVaultMaterialItem>> MaterialMap;

private:
	// Asset registry callbacks
	void OnAssetAdded(const FAssetData& AssetData);
	void OnAssetRemoved(const FAssetData& AssetData);
	void OnAssetRenamed(const FAssetData& AssetData, const FString& OldObjectPath);
	void OnAssetUpdated(const FAssetData& AssetData);
	
	// Internal helpers
	void ProcessMaterialAsset(const FAssetData& AssetData);
	void RemoveMaterialAsset(const FAssetData& AssetData);
	TSharedPtr<FHdriVaultFolderNode> CreateFolderNode(const FString& FolderPath);
	TSharedPtr<FHdriVaultFolderNode> GetOrCreateFolderNode(const FString& FolderPath);
	void SortMaterials(TArray<TSharedPtr<FHdriVaultMaterialItem>>& Materials) const;
	FString GetMetadataFilePath(const FAssetData& AssetData) const;
	FString OrganizePackagePath(const FString& PackagePath) const;
	
	// Data members
	TSharedPtr<FHdriVaultFolderNode> RootFolderNode;
	TMap<FString, TSharedPtr<FHdriVaultFolderNode>> FolderMap;
	
	FHdriVaultSettings Settings;
	
	// Asset registry
	FAssetRegistryModule* AssetRegistryModule;
	
	// Thumbnail manager
	TSharedPtr<class FHdriVaultThumbnailManager> ThumbnailManager;
	
	// Metadata cache
	TMap<FString, FHdriVaultMetadata> MetadataCache;
	
	bool bIsInitialized = false;
}; 