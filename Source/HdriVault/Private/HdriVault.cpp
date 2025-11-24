// Copyright Pyre Labs 2025. All Rights Reserved.

#include "HdriVault.h"
#include "HdriVaultCommands.h"
#include "HdriVaultStyle.h"
#include "HdriVaultManager.h"
#include "SHdriVaultWidget.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "EditorStyleSet.h"

const FName FHdriVaultModule::HdriVaultTabName("HdriVault");

#define LOCTEXT_NAMESPACE "FHdriVaultModule"

void FHdriVaultModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	// Initialize style system for custom 16:9 icon
	FHdriVaultStyle::Initialize();

	FHdriVaultCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FHdriVaultCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FHdriVaultModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FHdriVaultModule::RegisterMenus));
	
	// Register tab spawner
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(HdriVaultTabName, FOnSpawnTab::CreateRaw(this, &FHdriVaultModule::OnSpawnHdriVaultTab))
		.SetDisplayName(LOCTEXT("HdriVaultTabTitle", "Hdri Vault"))
		.SetTooltipText(LOCTEXT("HdriVaultTooltipText", "Launch the Hdri Vault library"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())
		.SetIcon(FSlateIcon(FHdriVaultStyle::GetStyleSetName(), "HdriVault.PluginAction"));
	
	// Initialize manager
	HdriVaultManager = nullptr;
	bIsTabOpen = false;
}

void FHdriVaultModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	// Shutdown style system
	FHdriVaultStyle::Shutdown();

	FHdriVaultCommands::Unregister();
	
	// Unregister tab spawner
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(HdriVaultTabName);
	
	// Clean up
	HdriVaultWidget.Reset();
	HdriVaultManager = nullptr;
}

void FHdriVaultModule::PluginButtonClicked()
{
	OpenHdriVaultTab();
}

void FHdriVaultModule::OpenHdriVaultTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(HdriVaultTabName);
}

TSharedRef<SDockTab> FHdriVaultModule::OnSpawnHdriVaultTab(const FSpawnTabArgs& Args)
{
	// Get or create the HdriVault manager
	if (!HdriVaultManager)
	{
		HdriVaultManager = GEditor->GetEditorSubsystem<UHdriVaultManager>();
		if (!HdriVaultManager)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to get HdriVault manager"));
		}
	}
	
	// Create the main widget
	HdriVaultWidget = SNew(SHdriVaultWidget);
	
	// Create the tab
	TSharedRef<SDockTab> NewTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("HdriVaultTabTitle", "Hdri Vault"))
		.ToolTipText(LOCTEXT("HdriVaultTooltipText", "Launch the Hdri Vault library"))
		.OnTabClosed(SDockTab::FOnTabClosedCallback::CreateRaw(this, &FHdriVaultModule::OnHdriVaultTabClosed))
		[
			HdriVaultWidget.ToSharedRef()
		];
	
	bIsTabOpen = true;
	return NewTab;
}

void FHdriVaultModule::OnHdriVaultTabClosed(TSharedRef<SDockTab> Tab)
{
	HdriVaultWidget.Reset();
	bIsTabOpen = false;
}

void FHdriVaultModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FHdriVaultCommands::Get().PluginAction, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FHdriVaultCommands::Get().PluginAction));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FHdriVaultModule, HdriVault)