// Copyright Pyre Labs 2025. All Rights Reserved.

#include "HdriVaultThumbnailManager.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialRenderProxy.h"
#include "RenderingThread.h"
#include "Engine/Engine.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "Slate/SlateTextures.h"
#include "TextureResource.h"
#include "Async/Async.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Factories/TextureFactory.h"
#include "ObjectTools.h"
#include "Misc/PackageName.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace HdriVaultThumbnailUtils
{
	static FString SanitizeName(const FString& InName)
	{
		if (InName.IsEmpty())
		{
			return TEXT("Material");
		}

		FString Result = InName;
		for (TCHAR& Char : Result)
		{
			if (!FChar::IsAlnum(Char))
			{
				Char = TEXT('_');
			}
		}

		return Result;
	}

	static FString GetGeneratedThumbnailRoot()
	{
		return TEXT("/HdriVault/Generated/Thumbnails");
	}

	static FString BuildThumbnailAssetName(UObject* Asset, int32 ThumbnailSize)
	{
		const FString AssetPath = Asset ? Asset->GetPathName() : FString();
		const uint32 PathHash = GetTypeHash(AssetPath);
		const FString BaseName = Asset ? Asset->GetName() : TEXT("Asset");
		const FString SanitizedName = SanitizeName(BaseName);
		return FString::Printf(TEXT("MV_%s_%u_%d"), *SanitizedName, PathHash, ThumbnailSize);
	}
}

FHdriVaultThumbnailManager::FHdriVaultThumbnailManager()
	: DefaultThumbnailSize(128)
	, MaxCacheSize(1000)
	, DefaultMaterialTexture(nullptr)
	, ErrorTexture(nullptr)
	, bIsInitialized(false)
{
}

FHdriVaultThumbnailManager::~FHdriVaultThumbnailManager()
{
	if (bIsInitialized)
	{
		Shutdown();
	}
}

void FHdriVaultThumbnailManager::Initialize()
{
	if (bIsInitialized)
	{
		return;
	}
	
	// Initialize default textures
	DefaultMaterialTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EditorMaterials/DefaultMaterial"));
	ErrorTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EditorMaterials/DefaultDiffuse"));
	
	bIsInitialized = true;
}

void FHdriVaultThumbnailManager::Shutdown()
{
	if (!bIsInitialized)
	{
		return;
	}
	
	// Clear cache
	ClearThumbnailCache();
	PendingThumbnails.Empty();
	
	DefaultMaterialTexture = nullptr;
	ErrorTexture = nullptr;
	
	bIsInitialized = false;
}

TSharedPtr<FSlateBrush> FHdriVaultThumbnailManager::GetMaterialThumbnail(TSharedPtr<FHdriVaultMaterialItem> MaterialItem, int32 ThumbnailSize)
{
	if (!MaterialItem.IsValid() || !bIsInitialized)
	{
		return nullptr;
	}
	
	FString CacheKey = GetCacheKey(MaterialItem->AssetData.GetObjectPathString(), ThumbnailSize);
	
	// Check cache first
	if (ThumbnailCache.Contains(CacheKey))
	{
		FThumbnailCacheEntry& Entry = ThumbnailCache[CacheKey];
		Entry.LastAccessTime = FDateTime::Now();
		return Entry.Brush;
	}
	
	// Generate thumbnail if not cached
	RequestThumbnail(MaterialItem, ThumbnailSize);
	
	// Return default thumbnail while generating
	if (DefaultMaterialTexture)
	{
		return CreateBrushFromTexture(DefaultMaterialTexture, ThumbnailSize);
	}
	
	return nullptr;
}

void FHdriVaultThumbnailManager::RequestThumbnail(TSharedPtr<FHdriVaultMaterialItem> MaterialItem, int32 ThumbnailSize)
{
	if (!MaterialItem.IsValid() || !bIsInitialized)
	{
		return;
	}
	
	FString MaterialPath = MaterialItem->AssetData.GetObjectPathString();
	
	// Check if already pending
	if (PendingThumbnails.Contains(MaterialPath))
	{
		return;
	}
	
	// Add to pending list
	PendingThumbnails.Add(MaterialPath, MaterialItem);
	
	// Generate thumbnail asynchronously
	LoadThumbnailAsync(MaterialItem, ThumbnailSize);
}

void FHdriVaultThumbnailManager::ClearThumbnailCache()
{
	ThumbnailCache.Empty();
}

void FHdriVaultThumbnailManager::ClearThumbnailForMaterial(const FString& MaterialPath)
{
	for (auto It = ThumbnailCache.CreateIterator(); It; ++It)
	{
		if (It.Key().Contains(MaterialPath))
		{
			It.RemoveCurrent();
		}
	}
}

UTexture2D* FHdriVaultThumbnailManager::GenerateMaterialThumbnail(UObject* Asset, int32 ThumbnailSize, bool bForceRegenerate)
{
	if (!Asset || !bIsInitialized)
	{
		return GetDefaultMaterialThumbnail();
	}

	UMaterialInterface* Material = Cast<UMaterialInterface>(Asset);
	// If it's not a material, we currently don't support custom thumbnail generation logic (rendering to texture).
	// We rely on standard asset thumbnails for other types in the UI.
	// But if called, we should probably return nullptr or default unless we implement texture rendering.
	if (!Material)
	{
		return nullptr; 
	}

	const FString PackageRoot = HdriVaultThumbnailUtils::GetGeneratedThumbnailRoot();
	const FString AssetName = HdriVaultThumbnailUtils::BuildThumbnailAssetName(Asset, ThumbnailSize);
	const FString PackageName = PackageRoot + TEXT("/") + AssetName;
	const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackageName, *AssetName);

	UTexture2D* ThumbnailAsset = FindObject<UTexture2D>(nullptr, *ObjectPath);
	UPackage* Package = ThumbnailAsset ? ThumbnailAsset->GetOutermost() : nullptr;

	if (ThumbnailAsset && !bForceRegenerate)
	{
		return ThumbnailAsset;
	}

	const bool bIsNewAsset = ThumbnailAsset == nullptr;
	if (bIsNewAsset)
	{
		Package = CreatePackage(*PackageName);
		if (!Package)
		{
			return GetDefaultMaterialThumbnail();
		}
		Package->FullyLoad();
		Package->SetFlags(RF_Public | RF_Standalone);

		ThumbnailAsset = NewObject<UTexture2D>(Package, *AssetName, RF_Public | RF_Standalone);
	}
	else
	{
		Package->FullyLoad();
	}

	ThumbnailAsset->MipGenSettings = TMGS_NoMipmaps;
	ThumbnailAsset->CompressionSettings = TC_Default;
	ThumbnailAsset->SRGB = true;

	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), NAME_None, RF_Transient);
	RenderTarget->InitCustomFormat(ThumbnailSize, ThumbnailSize, PF_B8G8R8A8, true);
	RenderTarget->ClearColor = FLinearColor::Transparent;
	RenderTarget->TargetGamma = 2.2f;
	RenderTarget->UpdateResourceImmediate(true);

	FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
	FCanvas Canvas(
		RenderTargetResource,
		nullptr,
		nullptr,
		static_cast<ERHIFeatureLevel::Type>(GMaxRHIFeatureLevel));
	Canvas.Clear(FLinearColor::Transparent);

	FCanvasTileItem TileItem(FVector2D::ZeroVector, Material->GetRenderProxy(), FVector2D(ThumbnailSize, ThumbnailSize));
	TileItem.BlendMode = SE_BLEND_Opaque;
	Canvas.DrawItem(TileItem);
	Canvas.Flush_GameThread();
	FlushRenderingCommands();

	TArray<FColor> SurfaceData;
	if (!RenderTargetResource->ReadPixels(SurfaceData))
	{
		return GetDefaultMaterialThumbnail();
	}

	const int32 ExpectedPixels = ThumbnailSize * ThumbnailSize;
	if (SurfaceData.Num() != ExpectedPixels)
	{
		return GetDefaultMaterialThumbnail();
	}

	ThumbnailAsset->Source.Init(ThumbnailSize, ThumbnailSize, 1, 1, ETextureSourceFormat::TSF_BGRA8, reinterpret_cast<const uint8*>(SurfaceData.GetData()));
	ThumbnailAsset->MarkPackageDirty();
	Package->MarkPackageDirty();
	ThumbnailAsset->PostEditChange();

	if (bIsNewAsset)
	{
		FAssetRegistryModule::AssetCreated(ThumbnailAsset);
	}

	FString PackageFilename;
	if (FPackageName::TryConvertLongPackageNameToFilename(PackageName, PackageFilename, FPackageName::GetAssetPackageExtension()))
	{
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(PackageFilename), true);

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.Error = GWarn;
		UPackage::SavePackage(Package, ThumbnailAsset, *PackageFilename, SaveArgs);
	}

	return ThumbnailAsset;
}

UTexture2D* FHdriVaultThumbnailManager::ImportThumbnailFromImage(UObject* Asset, const FString& SourceFile, int32 ThumbnailSize)
{
	if (!Asset || !bIsInitialized || SourceFile.IsEmpty())
	{
		return nullptr;
	}

	const FString PackageRoot = HdriVaultThumbnailUtils::GetGeneratedThumbnailRoot();
	const FString AssetName = HdriVaultThumbnailUtils::BuildThumbnailAssetName(Asset, ThumbnailSize);
	const FString PackageName = PackageRoot + TEXT("/") + AssetName;
	const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackageName, *AssetName);

	if (UObject* ExistingObject = StaticFindObject(UTexture2D::StaticClass(), nullptr, *ObjectPath))
	{
		TArray<UObject*> ObjectsToDelete;
		ObjectsToDelete.Add(ExistingObject);
		ObjectTools::DeleteObjectsUnchecked(ObjectsToDelete);
	}

	TArray<uint8> FileData;
	if (!FFileHelper::LoadFileToArray(FileData, *SourceFile) || FileData.Num() == 0)
	{
		return nullptr;
	}

	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		return nullptr;
	}
	Package->FullyLoad();
	Package->SetFlags(RF_Public | RF_Standalone);

	UTextureFactory* TextureFactory = NewObject<UTextureFactory>();
	TextureFactory->AddToRoot();
	TextureFactory->SuppressImportOverwriteDialog();
	TextureFactory->bCreateMaterial = false;

	const FString Extension = FPaths::GetExtension(SourceFile, /*bIncludeDot*/ false);

	const uint8* BufferStart = FileData.GetData();
	const uint8* BufferEnd = BufferStart + FileData.Num();

	UObject* NewObject = TextureFactory->FactoryCreateBinary(
		UTexture2D::StaticClass(),
		Package,
		*AssetName,
		RF_Public | RF_Standalone,
		nullptr,
		Extension.Len() > 0 ? *Extension : TEXT("png"),
		BufferStart,
		BufferEnd,
		GWarn);

	TextureFactory->RemoveFromRoot();

	UTexture2D* ImportedTexture = Cast<UTexture2D>(NewObject);
	if (!ImportedTexture)
	{
		return nullptr;
	}

	ImportedTexture->MipGenSettings = TMGS_NoMipmaps;
	ImportedTexture->CompressionSettings = TC_Default;
	ImportedTexture->SRGB = true;
	ImportedTexture->MarkPackageDirty();
	Package->MarkPackageDirty();
	ImportedTexture->PostEditChange();

	FAssetRegistryModule::AssetCreated(ImportedTexture);

	FString PackageFilename;
	if (FPackageName::TryConvertLongPackageNameToFilename(PackageName, PackageFilename, FPackageName::GetAssetPackageExtension()))
	{
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(PackageFilename), true);

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.Error = GWarn;
		UPackage::SavePackage(Package, ImportedTexture, *PackageFilename, SaveArgs);
	}

	return ImportedTexture;
}

TSharedPtr<FSlateDynamicImageBrush> FHdriVaultThumbnailManager::CreateBrushFromTexture(UTexture2D* Texture, int32 ThumbnailSize)
{
	if (!Texture)
	{
		return nullptr;
	}
	
	return MakeShareable(new FSlateDynamicImageBrush(
		Texture,
		FVector2D(ThumbnailSize, ThumbnailSize),
		FName(*FString::Printf(TEXT("MaterialThumbnail_%d"), GetTypeHash(Texture)))
	));
}

void FHdriVaultThumbnailManager::LoadThumbnailAsync(TSharedPtr<FHdriVaultMaterialItem> MaterialItem, int32 ThumbnailSize)
{
	if (!MaterialItem.IsValid())
	{
		return;
	}
	
	FString MaterialPath = MaterialItem->AssetData.GetObjectPathString();
	
	// Load material asynchronously
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, MaterialItem, ThumbnailSize, MaterialPath]()
	{
		UObject* Asset = MaterialItem->MaterialPtr.LoadSynchronous();
		TWeakObjectPtr<UObject> WeakAsset(Asset);

		AsyncTask(ENamedThreads::GameThread, [this, WeakAsset, ThumbnailSize, MaterialPath]()
		{
			if (UObject* StrongAsset = WeakAsset.Get())
			{
				if (UTexture2D* Thumbnail = GenerateMaterialThumbnail(StrongAsset, ThumbnailSize))
				{
					UpdateCacheWithThumbnail(MaterialPath, Thumbnail, ThumbnailSize);
				}
			}

			PendingThumbnails.Remove(MaterialPath);
		});
	});
}

void FHdriVaultThumbnailManager::SetThumbnailSize(int32 NewSize)
{
	DefaultThumbnailSize = FMath::Clamp(NewSize, 32, 512);
}

void FHdriVaultThumbnailManager::TrimCache()
{
	if (ThumbnailCache.Num() <= MaxCacheSize)
	{
		return;
	}
	
	// Sort by last access time and remove oldest entries
	TArray<TPair<FString, FDateTime>> SortedEntries;
	for (const auto& Entry : ThumbnailCache)
	{
		SortedEntries.Add(TPair<FString, FDateTime>(Entry.Key, Entry.Value.LastAccessTime));
	}
	
	SortedEntries.Sort([](const TPair<FString, FDateTime>& A, const TPair<FString, FDateTime>& B)
	{
		return A.Value < B.Value;
	});
	
	// Remove oldest entries
	int32 EntriesToRemove = ThumbnailCache.Num() - MaxCacheSize;
	for (int32 i = 0; i < EntriesToRemove; ++i)
	{
		ThumbnailCache.Remove(SortedEntries[i].Key);
	}
}

FString FHdriVaultThumbnailManager::GetCacheKey(const FString& MaterialPath, int32 ThumbnailSize) const
{
	return FString::Printf(TEXT("%s_%d"), *MaterialPath, ThumbnailSize);
}

void FHdriVaultThumbnailManager::UpdateCacheWithThumbnail(const FString& MaterialPath, UTexture2D* Thumbnail, int32 ThumbnailSize)
{
	OnThumbnailGenerated(MaterialPath, Thumbnail, ThumbnailSize);
}

void FHdriVaultThumbnailManager::OnThumbnailGenerated(const FString& MaterialPath, UTexture2D* Thumbnail, int32 ThumbnailSize)
{
	if (!Thumbnail)
	{
		return;
	}
	
	FString CacheKey = GetCacheKey(MaterialPath, ThumbnailSize);
	
	FThumbnailCacheEntry Entry;
	Entry.Texture = Thumbnail;
	Entry.Brush = CreateBrushFromTexture(Thumbnail, ThumbnailSize);
	Entry.ThumbnailSize = ThumbnailSize;
	Entry.LastAccessTime = FDateTime::Now();
	
	ThumbnailCache.Add(CacheKey, Entry);
	
	// Trim cache if needed
	TrimCache();
}

UTexture2D* FHdriVaultThumbnailManager::GetDefaultMaterialThumbnail() const
{
	if (DefaultMaterialTexture)
	{
		return DefaultMaterialTexture;
	}
	
	if (ErrorTexture)
	{
		return ErrorTexture;
	}
	
	return nullptr;
} 