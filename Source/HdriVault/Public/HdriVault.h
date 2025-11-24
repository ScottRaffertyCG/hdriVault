// Copyright Pyre Labs 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "HdriVaultTypes.h"

class FToolBarBuilder;
class FMenuBuilder;
class SHdriVaultWidget;
class UHdriVaultManager;

class FHdriVaultModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** Gets the HdriVault manager instance */
	UHdriVaultManager* GetHdriVaultManager() const { return HdriVaultManager; }
	
	/** Opens the HdriVault tab */
	void OpenHdriVaultTab();
	
	/** This function will be bound to Command. */
	void PluginButtonClicked();
	
	/** Gets the singleton instance of this module */
	static FHdriVaultModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FHdriVaultModule>("HdriVault");
	}
	
	/** Checks if the module is loaded and ready */
	static bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("HdriVault");
	}

private:
	/** Registers menus and toolbars */
	void RegisterMenus();
	
	/** Tab management */
	TSharedRef<SDockTab> OnSpawnHdriVaultTab(const FSpawnTabArgs& Args);
	void OnHdriVaultTabClosed(TSharedRef<SDockTab> Tab);
	
	/** UI Commands */
	TSharedPtr<class FUICommandList> PluginCommands;
	
	/** Main widget instance */
	TSharedPtr<SHdriVaultWidget> HdriVaultWidget;
	
	/** Manager instance */
	UHdriVaultManager* HdriVaultManager;
	
	/** Tab identifier */
	static const FName HdriVaultTabName;
	
	/** Whether the tab is currently open */
	bool bIsTabOpen;
};
