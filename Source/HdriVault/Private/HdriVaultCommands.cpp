// Copyright Pyre Labs. All Rights Reserved.

#include "HdriVaultCommands.h"

#define LOCTEXT_NAMESPACE "FHdriVaultModule"

void FHdriVaultCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "Hdri Vault", "Launch the Hdri Vault library", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
