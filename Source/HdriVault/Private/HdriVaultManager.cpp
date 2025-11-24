// Copyright Pyre Labs. All Rights Reserved.

#include "HdriVaultManager.h"
#include "HdriVaultThumbnailManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Engine/Engine.h"
#include "Editor.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureCube.h"
#include "Components/SkyLightComponent.h"
#include "Engine/SkyLight.h"
#include "Misc/DateTime.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EditorActorFolders.h"
#include "Selection.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "ScopedTransaction.h"
#include "EngineUtils.h"
#include "UObject/UnrealType.h"
#include "SHdriVaultImportOptions.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"
#include "Factories/TextureFactory.h"
#include "AutomatedAssetImportData.h"
#include "Misc/FileHelper.h"
#include "HdriVaultImageUtils.h"

#define LOCTEXT_NAMESPACE "HdriVaultManager"

UHdriVaultManager::UHdriVaultManager()
	: AssetRegistryModule(nullptr)
	, bIsInitialized(false)
{
}

void UHdriVaultManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	// Initialize asset registry
	AssetRegistryModule = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	if (AssetRegistryModule)
	{
		IAssetRegistry& AssetRegistry = AssetRegistryModule->Get();
		AssetRegistry.OnAssetAdded().AddUObject(this, &UHdriVaultManager::OnAssetAdded);
		AssetRegistry.OnAssetRemoved().AddUObject(this, &UHdriVaultManager::OnAssetRemoved);
		AssetRegistry.OnAssetRenamed().AddUObject(this, &UHdriVaultManager::OnAssetRenamed);
		AssetRegistry.OnAssetUpdated().AddUObject(this, &UHdriVaultManager::OnAssetUpdated);
	}
	
	// Initialize thumbnail manager
	ThumbnailManager = MakeShared<FHdriVaultThumbnailManager>();
	ThumbnailManager->Initialize();
	
	// Initialize root folder
	RootFolderNode = MakeShared<FHdriVaultFolderNode>(TEXT("Root"), Settings.RootFolder);
	FolderMap.Add(Settings.RootFolder, RootFolderNode);
	
	// Load initial data
	RefreshMaterialDatabase();
	
	bIsInitialized = true;
}

void UHdriVaultManager::Deinitialize()
{
	if (AssetRegistryModule)
	{
		IAssetRegistry& AssetRegistry = AssetRegistryModule->Get();
		AssetRegistry.OnAssetAdded().RemoveAll(this);
		AssetRegistry.OnAssetRemoved().RemoveAll(this);
		AssetRegistry.OnAssetRenamed().RemoveAll(this);
		AssetRegistry.OnAssetUpdated().RemoveAll(this);
	}
	
	if (ThumbnailManager.IsValid())
	{
		ThumbnailManager->Shutdown();
		ThumbnailManager.Reset();
	}
	
	// Clean up data
	FolderMap.Empty();
	MaterialMap.Empty();
	MetadataCache.Empty();
	RootFolderNode.Reset();
	
	bIsInitialized = false;
	
	Super::Deinitialize();
}

void UHdriVaultManager::RefreshMaterialDatabase()
{
	if (!bIsInitialized)
	{
		return;
	}
	
	// Clear existing data
	MaterialMap.Empty();
	if (RootFolderNode.IsValid())
	{
		RootFolderNode->Materials.Empty();
		RootFolderNode->Children.Empty();
	}
	
	// Get all HDRI assets
	TArray<FAssetData> HdriAssets;
	if (AssetRegistryModule)
	{
		IAssetRegistry& AssetRegistry = AssetRegistryModule->Get();
		
		// Get TextureCube assets only
		AssetRegistry.GetAssetsByClass(UTextureCube::StaticClass()->GetClassPathName(), HdriAssets);
	}
	
	// Process each asset
	for (const FAssetData& AssetData : HdriAssets)
	{
		ProcessMaterialAsset(AssetData);
	}
	
	// Build folder structure
	BuildFolderStructure();
	
	// Broadcast refresh complete
	OnRefreshRequested.Broadcast();
}

void UHdriVaultManager::BuildFolderStructure()
{
	if (!RootFolderNode.IsValid())
	{
		return;
	}
	
	// Clear existing structure
	RootFolderNode->Children.Empty();
	FolderMap.Empty();
	FolderMap.Add(Settings.RootFolder, RootFolderNode);
	
	// Create main category folders
	TSharedPtr<FHdriVaultFolderNode> ContentFolder = CreateFolderNode(TEXT("/Game"));
	ContentFolder->FolderName = TEXT("Content");
	TSharedPtr<FHdriVaultFolderNode> EngineFolder = CreateFolderNode(TEXT("/Engine"));
	EngineFolder->FolderName = TEXT("Engine");
	TSharedPtr<FHdriVaultFolderNode> PluginFolder = CreateFolderNode(TEXT("/Plugins"));
	PluginFolder->FolderName = TEXT("Plugins");
	
	if (RootFolderNode.IsValid())
	{
		ContentFolder->Parent = RootFolderNode;
		EngineFolder->Parent = RootFolderNode;
		PluginFolder->Parent = RootFolderNode;
		RootFolderNode->Children.Add(ContentFolder);
		RootFolderNode->Children.Add(EngineFolder);
		RootFolderNode->Children.Add(PluginFolder);
	}
	
	FolderMap.Add(TEXT("/Game"), ContentFolder);
	FolderMap.Add(TEXT("/Engine"), EngineFolder);
	FolderMap.Add(TEXT("/Plugins"), PluginFolder);
	
	// Build structure from materials
	for (const auto& MaterialPair : MaterialMap)
	{
		const TSharedPtr<FHdriVaultMaterialItem>& MaterialItem = MaterialPair.Value;
		if (MaterialItem.IsValid())
		{
			FString PackagePath = MaterialItem->AssetData.PackagePath.ToString();
			FString OrganizedPath = OrganizePackagePath(PackagePath);
			
			// Create folder nodes for this path
			TSharedPtr<FHdriVaultFolderNode> FolderNode = GetOrCreateFolderNode(OrganizedPath);
			if (FolderNode.IsValid())
			{
				FolderNode->Materials.Add(MaterialItem);
			}
		}
	}
}

void UHdriVaultManager::LoadMaterialsFromFolder(const FString& FolderPath)
{
	TSharedPtr<FHdriVaultFolderNode> FolderNode = FindFolder(FolderPath);
	if (FolderNode.IsValid())
	{
		// Load thumbnails for materials in this folder
		for (const auto& MaterialItem : FolderNode->Materials)
		{
			if (MaterialItem.IsValid() && ThumbnailManager.IsValid())
			{
				LoadMaterialThumbnail(MaterialItem);
			}
		}
	}
}

TSharedPtr<FHdriVaultFolderNode> UHdriVaultManager::FindFolder(const FString& FolderPath) const
{
	return FolderMap.FindRef(FolderPath);
}

TArray<TSharedPtr<FHdriVaultFolderNode>> UHdriVaultManager::GetChildFolders(const FString& FolderPath) const
{
	TSharedPtr<FHdriVaultFolderNode> FolderNode = FindFolder(FolderPath);
	if (FolderNode.IsValid())
	{
		return FolderNode->Children;
	}
	return TArray<TSharedPtr<FHdriVaultFolderNode>>();
}

TArray<TSharedPtr<FHdriVaultMaterialItem>> UHdriVaultManager::GetMaterialsInFolder(const FString& FolderPath) const
{
	TSharedPtr<FHdriVaultFolderNode> FolderNode = FindFolder(FolderPath);
	if (FolderNode.IsValid())
	{
		TArray<TSharedPtr<FHdriVaultMaterialItem>> CollectedMaterials;

		TFunction<void(const TSharedPtr<FHdriVaultFolderNode>&)> GatherMaterialsRecursive;
		GatherMaterialsRecursive = [&CollectedMaterials, &GatherMaterialsRecursive](const TSharedPtr<FHdriVaultFolderNode>& Node)
		{
			if (!Node.IsValid())
			{
				return;
			}

			CollectedMaterials.Append(Node->Materials);

			for (const TSharedPtr<FHdriVaultFolderNode>& Child : Node->Children)
			{
				GatherMaterialsRecursive(Child);
			}
		};

		GatherMaterialsRecursive(FolderNode);

		SortMaterials(CollectedMaterials);
		return CollectedMaterials;
	}
	return TArray<TSharedPtr<FHdriVaultMaterialItem>>();
}

TSharedPtr<FHdriVaultMaterialItem> UHdriVaultManager::GetMaterialByPath(const FString& AssetPath) const
{
	return MaterialMap.FindRef(AssetPath);
}

void UHdriVaultManager::LoadMaterialThumbnail(TSharedPtr<FHdriVaultMaterialItem> MaterialItem)
{
	if (MaterialItem.IsValid() && ThumbnailManager.IsValid())
	{
		ThumbnailManager->LoadThumbnailAsync(MaterialItem, (int32)Settings.ThumbnailSize);
	}
}

void UHdriVaultManager::LoadMaterialDependencies(TSharedPtr<FHdriVaultMaterialItem> MaterialItem)
{
	if (!MaterialItem.IsValid())
	{
		return;
	}
	
	// Load the asset
	UObject* Asset = MaterialItem->MaterialPtr.LoadSynchronous();
	UMaterialInterface* Material = Cast<UMaterialInterface>(Asset);
	
	if (!Material)
	{
		// Not a material, likely an HDRI texture. No dependencies to load.
		return;
	}
	
	// Get texture dependencies
	TArray<UTexture*> ReferencedTextures;
	Material->GetUsedTextures(ReferencedTextures, EMaterialQualityLevel::Num, true, ERHIFeatureLevel::Num, true);
	
	MaterialItem->TextureDependencies.Empty();
	for (UTexture* Texture : ReferencedTextures)
	{
		if (UTexture2D* Texture2D = Cast<UTexture2D>(Texture))
		{
			MaterialItem->TextureDependencies.Add(Texture2D);
		}
	}
}

void UHdriVaultManager::ApplyMaterialToSelection(TSharedPtr<FHdriVaultMaterialItem> MaterialItem)
{
	if (!MaterialItem.IsValid())
	{
		return;
	}
	
	// Try to load as TextureCube first
	UTextureCube* HdriTexture = Cast<UTextureCube>(MaterialItem->MaterialPtr.LoadSynchronous());
	
	if (!HdriTexture)
	{
		// Show error notification
		FNotificationInfo Info(LOCTEXT("HdriLoadFailed", "Failed to load HDRI asset for application"));
		Info.ExpireDuration = 3.0f;
		Info.bFireAndForget = true;
		Info.Image = FAppStyle::GetBrush("Icons.ErrorWithColor");
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}
	
	// Start transaction for undo/redo
	FScopedTransaction Transaction(LOCTEXT("ApplyHdri", "Apply HDRI to Skylight"));
	
	int32 ComponentsModified = 0;
	int32 BackdropsModified = 0;
	
	// Find Skylight Actors in the world
	if (GEditor && GEditor->GetEditorWorldContext().World())
	{
		UWorld* World = GEditor->GetEditorWorldContext().World();
		
		// 1. Apply to Skylights
		for (TActorIterator<ASkyLight> It(World); It; ++It)
		{
			ASkyLight* SkyLight = *It;
			if (SkyLight && SkyLight->GetLightComponent())
			{
				USkyLightComponent* Component = SkyLight->GetLightComponent();
				Component->Modify();
				
				if (HdriTexture)
				{
					Component->SourceType = SLS_SpecifiedCubemap;
					Component->Cubemap = HdriTexture;
				}
				
				// Recapture skylight
				Component->SetCaptureIsDirty();
				ComponentsModified++;
			}
		}

		// 2. Apply to HDRI Backdrop actors
		// Try to load the specific HDRIBackdrop class
		UClass* BackdropClass = nullptr;
		FSoftClassPath BackdropClassPath(TEXT("/HDRIBackdrop/Blueprints/HDRIBackdrop.HDRIBackdrop_C"));
		BackdropClass = BackdropClassPath.TryLoadClass<AActor>();

		if (BackdropClass)
		{
			for (TActorIterator<AActor> It(World, BackdropClass); It; ++It)
			{
				AActor* Actor = *It;
				if (!Actor) continue;

				// Check for Cubemap property
				FProperty* Property = Actor->GetClass()->FindPropertyByName(FName("Cubemap"));
				FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property);
				
				if (ObjectProperty && ObjectProperty->PropertyClass && ObjectProperty->PropertyClass->IsChildOf(UTextureCube::StaticClass()))
				{
					Actor->Modify();
					
					if (HdriTexture)
					{
						ObjectProperty->SetObjectPropertyValue_InContainer(Actor, HdriTexture);
					}
					
					// Notify of property change to trigger Construction Script
					FPropertyChangedEvent PropertyEvent(Property, EPropertyChangeType::ValueSet);
					Actor->PostEditChangeProperty(PropertyEvent);
					
					BackdropsModified++;
				}
			}
		}
		else
		{
			// Fallback: Iterate all actors if class not found (e.g. if path changed or user made a copy)
			// but only check actors with "HDRI" in name to be slightly more efficient than checking everything
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				AActor* Actor = *It;
				if (!Actor) continue;

				if (Actor->GetName().Contains(TEXT("HDRI"), ESearchCase::IgnoreCase) || 
					Actor->GetClass()->GetName().Contains(TEXT("HDRI"), ESearchCase::IgnoreCase))
				{
					FProperty* Property = Actor->GetClass()->FindPropertyByName(FName("Cubemap"));
					FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property);
					
					if (ObjectProperty && ObjectProperty->PropertyClass && ObjectProperty->PropertyClass->IsChildOf(UTextureCube::StaticClass()))
					{
						Actor->Modify();
						
						if (HdriTexture)
						{
							ObjectProperty->SetObjectPropertyValue_InContainer(Actor, HdriTexture);
						}
						
						FPropertyChangedEvent PropertyEvent(Property, EPropertyChangeType::ValueSet);
						Actor->PostEditChangeProperty(PropertyEvent);
						
						BackdropsModified++;
					}
				}
			}
		}
	}
	
	if (ComponentsModified > 0 || BackdropsModified > 0)
	{
		// Mark level as modified
		if (GEditor && GEditor->GetEditorWorldContext().World())
		{
			GEditor->GetEditorWorldContext().World()->MarkPackageDirty();
		}
		
		// Show success notification
		FText Message;
		if (ComponentsModified > 0 && BackdropsModified > 0)
		{
			Message = FText::Format(LOCTEXT("HdriAppliedBoth", "Applied HDRI '{0}' to {1} Skylight(s) and {2} Backdrop(s)"), 
				FText::FromString(MaterialItem->DisplayName), FText::AsNumber(ComponentsModified), FText::AsNumber(BackdropsModified));
		}
		else if (BackdropsModified > 0)
		{
			Message = FText::Format(LOCTEXT("HdriAppliedBackdrop", "Applied HDRI '{0}' to {1} Backdrop(s)"), 
				FText::FromString(MaterialItem->DisplayName), FText::AsNumber(BackdropsModified));
		}
		else
		{
			Message = FText::Format(LOCTEXT("HdriApplied", "Applied HDRI '{0}' to {1} Skylight(s)"), 
				FText::FromString(MaterialItem->DisplayName), FText::AsNumber(ComponentsModified));
		}

		FNotificationInfo Info(Message);
		Info.ExpireDuration = 3.0f;
		Info.bFireAndForget = true;
		Info.Image = FAppStyle::GetBrush("Icons.SuccessWithColor");
		FSlateNotificationManager::Get().AddNotification(Info);
	}
	else
	{
		// Show warning notification
		FNotificationInfo Info(LOCTEXT("NoTargetFound", "No Skylight or HDRI Backdrop found in the current level"));
		Info.ExpireDuration = 3.0f;
		Info.bFireAndForget = true;
		Info.Image = FAppStyle::GetBrush("Icons.Warning");
		FSlateNotificationManager::Get().AddNotification(Info);
	}
}

void UHdriVaultManager::RegenerateMaterialThumbnail(TSharedPtr<FHdriVaultMaterialItem> MaterialItem, int32 ThumbnailSize)
{
	if (!MaterialItem.IsValid() || !ThumbnailManager.IsValid())
	{
		return;
	}

	UObject* Asset = MaterialItem->MaterialPtr.LoadSynchronous();
	if (!Asset)
	{
		return;
	}

	const FString MaterialPath = MaterialItem->AssetData.GetObjectPathString();
	ThumbnailManager->ClearThumbnailForMaterial(MaterialPath);

	if (UTexture2D* GeneratedThumbnail = ThumbnailManager->GenerateMaterialThumbnail(Asset, ThumbnailSize, true))
	{
		ThumbnailManager->UpdateCacheWithThumbnail(MaterialPath, GeneratedThumbnail, ThumbnailSize);
		OnRefreshRequested.Broadcast();
	}
}

UTexture2D* UHdriVaultManager::ImportCustomThumbnail(TSharedPtr<FHdriVaultMaterialItem> MaterialItem, const FString& SourceFile, int32 ThumbnailSize)
{
	if (!MaterialItem.IsValid() || !ThumbnailManager.IsValid() || SourceFile.IsEmpty())
	{
		return nullptr;
	}

	UObject* Asset = MaterialItem->MaterialPtr.LoadSynchronous();
	if (!Asset)
	{
		return nullptr;
	}

	if (UTexture2D* ImportedThumbnail = ThumbnailManager->ImportThumbnailFromImage(Asset, SourceFile, ThumbnailSize))
	{
		const FString MaterialPath = MaterialItem->AssetData.GetObjectPathString();
		ThumbnailManager->UpdateCacheWithThumbnail(MaterialPath, ImportedThumbnail, ThumbnailSize);
		OnRefreshRequested.Broadcast();
		return ImportedThumbnail;
	}

	return nullptr;
}

void UHdriVaultManager::SaveMaterialMetadata(TSharedPtr<FHdriVaultMaterialItem> MaterialItem)
{
	if (!MaterialItem.IsValid())
	{
		return;
	}
	
	// Update cache
	MetadataCache.Add(MaterialItem->AssetData.GetObjectPathString(), MaterialItem->Metadata);
	
	// Save to file
	FString MetadataPath = GetMetadataFilePath(MaterialItem->AssetData);
	
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("MaterialName"), MaterialItem->Metadata.MaterialName);
	JsonObject->SetStringField(TEXT("Location"), MaterialItem->Metadata.Location);
	JsonObject->SetStringField(TEXT("Author"), MaterialItem->Metadata.Author);
	JsonObject->SetStringField(TEXT("LastModified"), MaterialItem->Metadata.LastModified.ToString());
	JsonObject->SetStringField(TEXT("Notes"), MaterialItem->Metadata.Notes);
	JsonObject->SetStringField(TEXT("Category"), MaterialItem->Metadata.Category);
	JsonObject->SetStringField(TEXT("CustomThumbnailPath"), MaterialItem->Metadata.CustomThumbnailPath);
	
	TArray<TSharedPtr<FJsonValue>> TagsArray;
	for (const FString& Tag : MaterialItem->Metadata.Tags)
	{
		TagsArray.Add(MakeShareable(new FJsonValueString(Tag)));
	}
	JsonObject->SetArrayField(TEXT("Tags"), TagsArray);
	
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
	
	FFileHelper::SaveStringToFile(OutputString, *MetadataPath);
}

void UHdriVaultManager::LoadMaterialMetadata(TSharedPtr<FHdriVaultMaterialItem> MaterialItem)
{
	if (!MaterialItem.IsValid())
	{
		return;
	}
	
	// Check cache first
	FString ObjectPath = MaterialItem->AssetData.GetObjectPathString();
	if (MetadataCache.Contains(ObjectPath))
	{
		MaterialItem->Metadata = MetadataCache[ObjectPath];
		return;
	}
	
	// Load from file
	FString MetadataPath = GetMetadataFilePath(MaterialItem->AssetData);
	
	FString FileContents;
	if (FFileHelper::LoadFileToString(FileContents, *MetadataPath))
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContents);
		
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			MaterialItem->Metadata.MaterialName = JsonObject->GetStringField(TEXT("MaterialName"));
			MaterialItem->Metadata.Location = JsonObject->GetStringField(TEXT("Location"));
			MaterialItem->Metadata.Author = JsonObject->GetStringField(TEXT("Author"));
			MaterialItem->Metadata.Notes = JsonObject->GetStringField(TEXT("Notes"));
			MaterialItem->Metadata.Category = JsonObject->GetStringField(TEXT("Category"));
			
			FString DateString = JsonObject->GetStringField(TEXT("LastModified"));
			FDateTime::Parse(DateString, MaterialItem->Metadata.LastModified);
			
			const TArray<TSharedPtr<FJsonValue>>* TagsArray;
			if (JsonObject->TryGetArrayField(TEXT("Tags"), TagsArray))
			{
				MaterialItem->Metadata.Tags.Empty();
				for (const auto& TagValue : *TagsArray)
				{
					MaterialItem->Metadata.Tags.Add(TagValue->AsString());
				}
			}

			if (JsonObject->HasTypedField<EJson::String>(TEXT("CustomThumbnailPath")))
			{
				MaterialItem->Metadata.CustomThumbnailPath = JsonObject->GetStringField(TEXT("CustomThumbnailPath"));
			}
			else
			{
				MaterialItem->Metadata.CustomThumbnailPath.Empty();
			}
			
			// Cache the loaded metadata
			MetadataCache.Add(ObjectPath, MaterialItem->Metadata);
		}
	}
}

void UHdriVaultManager::SetSettings(const FHdriVaultSettings& NewSettings)
{
	Settings = NewSettings;
	OnSettingsChanged.Broadcast(Settings);
}

TArray<TSharedPtr<FHdriVaultMaterialItem>> UHdriVaultManager::SearchMaterials(const FString& SearchTerm) const
{
	TArray<TSharedPtr<FHdriVaultMaterialItem>> Results;
	
	if (SearchTerm.IsEmpty())
	{
		return Results;
	}
	
	FString LowerSearchTerm = SearchTerm.ToLower();
	
	for (const auto& MaterialPair : MaterialMap)
	{
		const TSharedPtr<FHdriVaultMaterialItem>& MaterialItem = MaterialPair.Value;
		if (MaterialItem.IsValid())
		{
			FString MaterialName = MaterialItem->DisplayName.ToLower();
			FString MaterialPath = MaterialItem->AssetData.PackagePath.ToString().ToLower();
			
			if (MaterialName.Contains(LowerSearchTerm) || MaterialPath.Contains(LowerSearchTerm))
			{
				Results.Add(MaterialItem);
			}
		}
	}
	
	SortMaterials(Results);
	return Results;
}

TArray<TSharedPtr<FHdriVaultMaterialItem>> UHdriVaultManager::FilterMaterialsByTag(const FString& Tag) const
{
	TArray<TSharedPtr<FHdriVaultMaterialItem>> Results;
	
	for (const auto& MaterialPair : MaterialMap)
	{
		const TSharedPtr<FHdriVaultMaterialItem>& MaterialItem = MaterialPair.Value;
		if (MaterialItem.IsValid() && MaterialItem->Metadata.Tags.Contains(Tag))
		{
			Results.Add(MaterialItem);
		}
	}
	
	SortMaterials(Results);
	return Results;
}

void UHdriVaultManager::OnAssetAdded(const FAssetData& AssetData)
{
	if (AssetData.AssetClassPath == UTextureCube::StaticClass()->GetClassPathName())
	{
		ProcessMaterialAsset(AssetData);
		BuildFolderStructure();
	}
}

void UHdriVaultManager::OnAssetRemoved(const FAssetData& AssetData)
{
	RemoveMaterialAsset(AssetData);
	BuildFolderStructure();
}

void UHdriVaultManager::OnAssetRenamed(const FAssetData& AssetData, const FString& OldObjectPath)
{
	RemoveMaterialAsset(AssetData);
	ProcessMaterialAsset(AssetData);
	BuildFolderStructure();
}

void UHdriVaultManager::OnAssetUpdated(const FAssetData& AssetData)
{
	if (AssetData.AssetClassPath == UTextureCube::StaticClass()->GetClassPathName())
	{
		ProcessMaterialAsset(AssetData);
	}
}

void UHdriVaultManager::ProcessMaterialAsset(const FAssetData& AssetData)
{
	FString ObjectPath = AssetData.GetObjectPathString();
	
	// Create or update material item
	TSharedPtr<FHdriVaultMaterialItem> MaterialItem = MaterialMap.FindRef(ObjectPath);
	if (!MaterialItem.IsValid())
	{
		MaterialItem = MakeShared<FHdriVaultMaterialItem>(AssetData);
		MaterialMap.Add(ObjectPath, MaterialItem);
	}
	else
	{
		// Update existing item
		MaterialItem->AssetData = AssetData;
		MaterialItem->MaterialPtr = AssetData.ToSoftObjectPath();
		MaterialItem->DisplayName = AssetData.AssetName.ToString();
	}
	
	// Load metadata
	LoadMaterialMetadata(MaterialItem);
}

void UHdriVaultManager::RemoveMaterialAsset(const FAssetData& AssetData)
{
	FString ObjectPath = AssetData.GetObjectPathString();
	MaterialMap.Remove(ObjectPath);
	MetadataCache.Remove(ObjectPath);
}

TSharedPtr<FHdriVaultFolderNode> UHdriVaultManager::CreateFolderNode(const FString& FolderPath)
{
	if (FolderPath.IsEmpty())
	{
		return nullptr;
	}
	
	FString FolderName = FPaths::GetCleanFilename(FolderPath);
	if (FolderName.IsEmpty())
	{
		FolderName = TEXT("Root");
	}
	
	return MakeShared<FHdriVaultFolderNode>(FolderName, FolderPath);
}

TSharedPtr<FHdriVaultFolderNode> UHdriVaultManager::GetOrCreateFolderNode(const FString& FolderPath)
{
	// Check if folder already exists
	if (FolderMap.Contains(FolderPath))
	{
		return FolderMap[FolderPath];
	}
	
	// Create new folder
	TSharedPtr<FHdriVaultFolderNode> NewFolder = CreateFolderNode(FolderPath);
	if (!NewFolder.IsValid())
	{
		return nullptr;
	}
	
	// Add to map
	FolderMap.Add(FolderPath, NewFolder);
	
	// Find parent folder
	FString ParentPath = FPaths::GetPath(FolderPath);
	if (!ParentPath.IsEmpty() && ParentPath != FolderPath)
	{
		TSharedPtr<FHdriVaultFolderNode> ParentFolder = GetOrCreateFolderNode(ParentPath);
		if (ParentFolder.IsValid())
		{
			NewFolder->Parent = ParentFolder;
			ParentFolder->Children.Add(NewFolder);
		}
	}
	else
	{
		// This is a root level folder
		if (RootFolderNode.IsValid())
		{
			NewFolder->Parent = RootFolderNode;
			RootFolderNode->Children.Add(NewFolder);
		}
	}
	
	return NewFolder;
}

void UHdriVaultManager::SortMaterials(TArray<TSharedPtr<FHdriVaultMaterialItem>>& Materials) const
{
	switch (Settings.SortMode)
	{
		case EHdriVaultSortMode::Name:
			Materials.Sort([](const TSharedPtr<FHdriVaultMaterialItem>& A, const TSharedPtr<FHdriVaultMaterialItem>& B)
			{
				return A->DisplayName < B->DisplayName;
			});
			break;
		case EHdriVaultSortMode::DateModified:
			Materials.Sort([](const TSharedPtr<FHdriVaultMaterialItem>& A, const TSharedPtr<FHdriVaultMaterialItem>& B)
			{
				return A->Metadata.LastModified > B->Metadata.LastModified;
			});
			break;
		case EHdriVaultSortMode::Type:
			Materials.Sort([](const TSharedPtr<FHdriVaultMaterialItem>& A, const TSharedPtr<FHdriVaultMaterialItem>& B)
			{
				return A->AssetData.AssetClassPath.ToString() < B->AssetData.AssetClassPath.ToString();
			});
			break;
		default:
			break;
	}
}

FString UHdriVaultManager::GetMetadataFilePath(const FAssetData& AssetData) const
{
	FString ProjectDir = FPaths::ProjectDir();
	FString MetadataDir = FPaths::Combine(ProjectDir, TEXT("Saved"), TEXT("HdriVault"), TEXT("Metadata"));
	
	FString AssetPath = AssetData.PackageName.ToString();
	AssetPath.RemoveFromStart(TEXT("/Game/"));
	AssetPath.ReplaceInline(TEXT("/"), TEXT("_"));
	
	FString MetadataFileName = FString::Printf(TEXT("%s_%s.json"), *AssetPath, *AssetData.AssetName.ToString());
	
	return FPaths::Combine(MetadataDir, MetadataFileName);
}

FString UHdriVaultManager::OrganizePackagePath(const FString& PackagePath) const
{
	// Organize package paths into Engine/Content/Plugins structure like Content Browser
	if (PackagePath.StartsWith(TEXT("/Game")))
	{
		// Game content goes to Content folder
		return PackagePath;
	}
	else if (PackagePath.StartsWith(TEXT("/Engine")))
	{
		// Engine content stays in Engine folder
		return PackagePath;
	}
	else
	{
		// Check for plugin patterns - plugins typically start with plugin name
		TArray<FString> PathComponents;
		PackagePath.ParseIntoArray(PathComponents, TEXT("/"), true);
		
		if (PathComponents.Num() > 0)
		{
			FString FirstComponent = PathComponents[0];
			
			// Check if this looks like a plugin (not Engine, not Game, not Script)
			if (!FirstComponent.Equals(TEXT("Engine"), ESearchCase::IgnoreCase) &&
				!FirstComponent.Equals(TEXT("Game"), ESearchCase::IgnoreCase) &&
				!FirstComponent.Equals(TEXT("Script"), ESearchCase::IgnoreCase) &&
				!FirstComponent.Equals(TEXT("Temp"), ESearchCase::IgnoreCase) &&
				!FirstComponent.Equals(TEXT("Memory"), ESearchCase::IgnoreCase))
			{
				// This is likely a plugin
				return FString::Printf(TEXT("/Plugins%s"), *PackagePath);
			}
		}
		
		// Check if it starts with known engine patterns
		if (PackagePath.StartsWith(TEXT("/Script")) || 
			PackagePath.StartsWith(TEXT("/Temp")) ||
			PackagePath.StartsWith(TEXT("/Memory")) ||
			PackagePath.Contains(TEXT("Engine")))
		{
			return FString::Printf(TEXT("/Engine%s"), *PackagePath);
		}
		
		// Unknown content, put in Content by default
		return FString::Printf(TEXT("/Game%s"), *PackagePath);
	}
}

void UHdriVaultManager::ImportHdriFiles(const TArray<FString>& Files)
{
	if (Files.Num() == 0) return;

	// Create and show dialog
	TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
	TSharedPtr<SWindow> ImportWindow = SNew(SWindow)
		.Title(LOCTEXT("ImportHdriTitle", "Import HDRIs"))
		.ClientSize(FVector2D(500, 600))
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.IsTopmostWindow(true);

	TSharedPtr<SHdriVaultImportDialog> ImportDialog;
	ImportWindow->SetContent(
		SAssignNew(ImportDialog, SHdriVaultImportDialog)
		.Files(Files)
		.ParentWindow(ImportWindow)
	);

	FSlateApplication::Get().AddModalWindow(ImportWindow.ToSharedRef(), ParentWindow);

	if (ImportDialog->ShouldImport())
	{
		const FHdriVaultImportOptions& Options = ImportDialog->GetImportOptions();
		
		TArray<FString> FilesToImport;
		int32 ConversionCount = 0;

		// Pre-process files: Convert EXR to HDR
		for (const FString& File : Files)
		{
			FString Ext = FPaths::GetExtension(File).ToLower();
			if (Ext == TEXT("exr"))
			{
				FString HdrFile = FPaths::ChangeExtension(File, TEXT("hdr"));
				FString Error;

				if (FHdriVaultImageUtils::ConvertExrToHdr(File, HdrFile, Error))
				{
					FilesToImport.Add(HdrFile);
					ConversionCount++;
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("HdriVault: Failed to convert %s: %s"), *File, *Error);
					// Fallback to original file if conversion fails
					FilesToImport.Add(File);
				}
			}
			else
			{
				FilesToImport.Add(File);
			}
		}

		if (ConversionCount > 0)
		{
			FNotificationInfo Info(FText::Format(LOCTEXT("ConversionComplete", "Converted {0} EXR files to HDR"), FText::AsNumber(ConversionCount)));
			Info.ExpireDuration = 3.0f;
			FSlateNotificationManager::Get().AddNotification(Info);
		}
		
		// Perform import using AssetTools
		FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
		
		// Create a TextureFactory and set import options for HDRIs
		UTextureFactory* TextureFactory = NewObject<UTextureFactory>();
		TextureFactory->HDRImportShouldBeLongLatCubeMap = EAppReturnType::YesAll; // Hint to import as cubemap
		TextureFactory->AddToRoot(); // Prevent GC

		// Add automated import data to try and suppress dialogs
		UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
		ImportData->bReplaceExisting = true;
		TextureFactory->AutomatedImportData = ImportData;

		TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssets(FilesToImport, Options.DestinationPath, TextureFactory);

		TextureFactory->RemoveFromRoot(); // Clean up factory

		int32 CubemapCount = 0;
		int32 Texture2DCount = 0;

		if (ImportedAssets.Num() > 0)
		{
			// Ensure Asset Registry is up to date before we scan
			if (AssetRegistryModule)
			{
				AssetRegistryModule->Get().ScanPathsSynchronous({ Options.DestinationPath }, true);
			}

			// Force refresh to ensure we have the new assets in our system
			RefreshMaterialDatabase();

			// Apply metadata
			for (UObject* Asset : ImportedAssets)
			{
				if (UTextureCube* Texture = Cast<UTextureCube>(Asset))
				{
					CubemapCount++;
					TSharedPtr<FHdriVaultMaterialItem> MaterialItem = GetMaterialByPath(Asset->GetPathName());
					if (MaterialItem.IsValid())
					{
						// Apply common metadata
						if (!Options.Category.IsEmpty()) MaterialItem->Metadata.Category = Options.Category;
						if (!Options.Author.IsEmpty()) MaterialItem->Metadata.Author = Options.Author;
						if (!Options.Notes.IsEmpty()) MaterialItem->Metadata.Notes = Options.Notes;
						
						// Merge tags
						for (const FString& Tag : Options.Tags)
						{
							MaterialItem->Metadata.Tags.AddUnique(Tag);
						}

						SaveMaterialMetadata(MaterialItem);
					}
				}
				else if (Asset->IsA(UTexture2D::StaticClass()))
				{
					Texture2DCount++;
				}
			}

			// Final refresh to show metadata
			RefreshMaterialDatabase();
			
			FNotificationInfo Info(FText::Format(LOCTEXT("ImportSuccess", "Successfully imported {0} HDRIs"), FText::AsNumber(CubemapCount)));
			
			if (Texture2DCount > 0)
			{
				Info.Text = FText::Format(LOCTEXT("ImportPartialSuccess", "Imported {0} Cubemaps and {1} Texture2Ds.\nTexture2Ds must be converted to Cubemaps to appear in the Vault."), 
					FText::AsNumber(CubemapCount), FText::AsNumber(Texture2DCount));
				Info.Image = FAppStyle::GetBrush("Icons.Warning");
			}
			else
			{
				Info.Image = FAppStyle::GetBrush("Icons.SuccessWithColor");
			}
			
			Info.ExpireDuration = 5.0f;
			Info.bFireAndForget = true;
			FSlateNotificationManager::Get().AddNotification(Info);
		}
	}
}

#undef LOCTEXT_NAMESPACE 