// Copyright Pyre Labs 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "AssetRegistry/AssetData.h"
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "UObject/SoftObjectPath.h"
#include "HdriVaultTypes.generated.h"

USTRUCT()
struct HDRIVAULT_API FHdriVaultMetadata
{
	GENERATED_BODY()

	UPROPERTY()
	FString MaterialName;

	UPROPERTY()
	FString Location;

	UPROPERTY()
	FString Author;

	UPROPERTY()
	FDateTime LastModified;

	UPROPERTY()
	FString Notes;

	UPROPERTY()
	TArray<FString> Tags;

	UPROPERTY()
	FString Category;

	UPROPERTY()
	FString CustomThumbnailPath;

	FHdriVaultMetadata()
		: MaterialName(TEXT(""))
		, Location(TEXT(""))
		, Author(TEXT(""))
		, LastModified(FDateTime::Now())
		, Notes(TEXT(""))
		, Category(TEXT(""))
		, CustomThumbnailPath(TEXT(""))
	{
	}
};

USTRUCT()
struct HDRIVAULT_API FHdriVaultMaterialItem
{
	GENERATED_BODY()

	// Asset data
	UPROPERTY()
	FAssetData AssetData;

	// Soft reference to the asset (Hdri, Texture, Material)
	UPROPERTY()
	TSoftObjectPtr<UObject> MaterialPtr;

	// Cached thumbnail
	TSharedPtr<struct FSlateBrush> ThumbnailBrush;

	// Metadata
	UPROPERTY()
	FHdriVaultMetadata Metadata;

	// Texture dependencies
	UPROPERTY()
	TArray<TSoftObjectPtr<UTexture2D>> TextureDependencies;

	// Display name
	FString DisplayName;

	// Whether thumbnail is loaded
	bool bThumbnailLoaded = false;

	FHdriVaultMaterialItem()
		: MaterialPtr(nullptr)
		, ThumbnailBrush(nullptr)
		, bThumbnailLoaded(false)
	{
	}

	FHdriVaultMaterialItem(const FAssetData& InAssetData)
		: AssetData(InAssetData)
		, MaterialPtr(InAssetData.ToSoftObjectPath())
		, ThumbnailBrush(nullptr)
		, bThumbnailLoaded(false)
	{
		DisplayName = AssetData.AssetName.ToString();
		Metadata.MaterialName = DisplayName;
		Metadata.Location = AssetData.PackageName.ToString();
	}
};

USTRUCT()
struct HDRIVAULT_API FHdriVaultFolderNode
{
	GENERATED_BODY()

	// Folder name
	FString FolderName;

	// Full folder path
	FString FolderPath;

	// Parent folder
	TSharedPtr<FHdriVaultFolderNode> Parent;

	// Child folders
	TArray<TSharedPtr<FHdriVaultFolderNode>> Children;

	// Materials in this folder
	TArray<TSharedPtr<FHdriVaultMaterialItem>> Materials;

	// Whether this folder is expanded in the tree
	bool bIsExpanded = false;

	FHdriVaultFolderNode()
		: FolderName(TEXT(""))
		, FolderPath(TEXT(""))
		, Parent(nullptr)
		, bIsExpanded(false)
	{
	}

	FHdriVaultFolderNode(const FString& InFolderName, const FString& InFolderPath)
		: FolderName(InFolderName)
		, FolderPath(InFolderPath)
		, Parent(nullptr)
		, bIsExpanded(false)
	{
	}
};

UENUM()
enum class EHdriVaultViewMode : uint8
{
	Grid,
	List
};

UENUM()
enum class EHdriVaultSortMode : uint8
{
	Name,
	DateModified,
	Size,
	Type
};

USTRUCT()
struct HDRIVAULT_API FHdriVaultSettings
{
	GENERATED_BODY()

	UPROPERTY()
	EHdriVaultViewMode ViewMode = EHdriVaultViewMode::Grid;

	UPROPERTY()
	EHdriVaultSortMode SortMode = EHdriVaultSortMode::Name;

	UPROPERTY()
	float ThumbnailSize = 128.0f;

	UPROPERTY()
	bool bShowMetadata = true;

	UPROPERTY()
	bool bShowFolderTree = true;

	UPROPERTY()
	FString RootFolder = TEXT("/Game");

	UPROPERTY()
	bool bAutoRefresh = true;

	UPROPERTY()
	float RefreshInterval = 5.0f;
};

// Delegate declarations
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHdriVaultFolderSelected, TSharedPtr<FHdriVaultFolderNode>);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHdriVaultMaterialSelected, TSharedPtr<FHdriVaultMaterialItem>);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHdriVaultMaterialDoubleClicked, TSharedPtr<FHdriVaultMaterialItem>);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHdriVaultSettingsChanged, const FHdriVaultSettings&);
DECLARE_MULTICAST_DELEGATE(FOnHdriVaultRefreshRequested); 