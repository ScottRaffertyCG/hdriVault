// Copyright Pyre Labs 2025. All Rights Reserved.

#include "HdriVaultStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"
#include "Styling/SlateTypes.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FHdriVaultStyle::StyleInstance = nullptr;

void FHdriVaultStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FHdriVaultStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FHdriVaultStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("HdriVaultStyle"));
	return StyleSetName;
}

TSharedRef< FSlateStyleSet > FHdriVaultStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("HdriVaultStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("HdriVault")->GetBaseDir() / TEXT("Resources"));

	// HdriVault toolbar button icon
	Style->Set("HdriVault.PluginAction", new IMAGE_BRUSH(TEXT("Icon128"), FVector2D(40.0f, 40.0f)));

	return Style;
}

const ISlateStyle& FHdriVaultStyle::Get()
{
	return *StyleInstance;
} 