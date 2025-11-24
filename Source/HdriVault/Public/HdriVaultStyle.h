// Copyright Pyre Labs. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FSlateStyleSet;

/**
 * Minimal style system for HdriVault - only handles custom icon registration
 */
class FHdriVaultStyle
{
public:
	static void Initialize();
	static void Shutdown();
	static FName GetStyleSetName();
	static const class ISlateStyle& Get();

private:
	static TSharedRef< FSlateStyleSet > Create();
	static TSharedPtr< FSlateStyleSet > StyleInstance;
}; 