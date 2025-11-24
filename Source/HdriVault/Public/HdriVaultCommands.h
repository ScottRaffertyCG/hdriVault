// Copyright Pyre Labs. All Rights Reserved.

#pragma once

#include "Framework/Commands/Commands.h"
#include "HdriVaultStyle.h"

class FHdriVaultCommands : public TCommands<FHdriVaultCommands>
{
public:

	FHdriVaultCommands()
		: TCommands<FHdriVaultCommands>(TEXT("HdriVault"), NSLOCTEXT("Contexts", "HdriVault", "HdriVault Plugin"), NAME_None, FHdriVaultStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PluginAction;
};
