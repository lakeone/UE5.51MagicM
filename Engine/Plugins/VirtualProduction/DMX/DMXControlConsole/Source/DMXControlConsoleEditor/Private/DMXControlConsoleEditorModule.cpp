// Copyright Epic Games, Inc. All Rights Reserved.

#include "DMXControlConsoleEditorModule.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Commands/DMXControlConsoleEditorCommands.h"
#include "DMXControlConsole.h"
#include "DMXControlConsoleEditorFromLegacyUpgradeHandler.h"
#include "DMXEditorModule.h"
#include "DMXEditorSettings.h"
#include "Factories/DMXControlConsoleFactory.h"
#include "Framework/Docking/TabManager.h"
#include "LevelEditor.h"
#include "Misc/CoreDelegates.h"
#include "Models/DMXControlConsoleCompactEditorModel.h"
#include "Style/DMXControlConsoleEditorStyle.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Views/SDMXControlConsoleCompactEditorView.h"
#include "Widgets/Docking/SDockTab.h"


#define LOCTEXT_NAMESPACE "DMXControlConsoleEditorModule"

const FName FDMXControlConsoleEditorModule::ControlConsoleEditorTabName("ControlConsoleTabName");
const FName FDMXControlConsoleEditorModule::ControlConsoleEditorAppIdentifier(TEXT("ControlConsoleApp"));
EAssetTypeCategories::Type FDMXControlConsoleEditorModule::DMXEditorAssetCategory;
const FName FDMXControlConsoleEditorModule::CompactEditorTabId = TEXT("ControlConsoleCompactEditor");

FDMXControlConsoleEditorModule::FDMXControlConsoleEditorModule()
	: ControlConsoleCategory(LOCTEXT("ControlConsoleAssetTypeCategory", "DMX"))
{}

void FDMXControlConsoleEditorModule::StartupModule()
{
	FDMXControlConsoleEditorCommands::Register();
	RegisterLevelEditorCommands();

	// Register Asset Tools for control console
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	DMXEditorAssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("DMX")), LOCTEXT("DmxCategory", "DMX"));

	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FDMXControlConsoleEditorModule::OnPostEnginInit);

	RegisterCompactEditorTabSpawner();
}

void FDMXControlConsoleEditorModule::ShutdownModule()
{
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);

	// It is required to explicitly let the widget go, before further shutting down this module.
	CompactEditorTab.Reset();
}

void FDMXControlConsoleEditorModule::OpenControlConsole()
{
	UDMXEditorSettings* DMXEditorSettings = GetMutableDefault<UDMXEditorSettings>();
	if (!DMXEditorSettings)
	{
		return;
	}

	const FString& LastOpenedConsolePath = DMXEditorSettings->LastOpenedControlConsolePath;

	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	const FAssetData& ControlConsoleAsset = AssetRegistryModule.Get().GetAssetByObjectPath(LastOpenedConsolePath);
	if (ControlConsoleAsset.IsValid())
	{
		// focus last opened asset editor
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(ControlConsoleAsset.GetAsset());
	}
	else
	{
		// Create unique path for the new control console asset
		const FString AssetPath = TEXT("/Game");
		const FString AssetName = TEXT("NewDMXControlConsole");
		
		FString UniquePackageName;
		FString UniqueAssetName;
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		AssetToolsModule.Get().CreateUniqueAssetName(AssetPath / AssetName, TEXT(""), UniquePackageName, UniqueAssetName);

		const UDMXControlConsoleFactory* ControlConsoleFactory = NewObject<UDMXControlConsoleFactory>();
		UObject* DMXControlConsoleObject = ControlConsoleFactory->CreateConsoleAssetFromData(AssetPath, UniqueAssetName, nullptr);
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(DMXControlConsoleObject);
	}

	const FDMXControlConsoleEditorModule& ThisModule = FModuleManager::GetModuleChecked<FDMXControlConsoleEditorModule>(TEXT("DMXControlConsoleEditor"));
	const bool bFloatingWindow = ThisModule.CompactEditorTab.IsValid() && ThisModule.CompactEditorTab->GetParentWindow().IsValid();
	if (bFloatingWindow)
	{
		// Close the compact editor tab if it is not docked
		ThisModule.CompactEditorTab->RequestCloseTab();
	}
}

void FDMXControlConsoleEditorModule::RegisterLevelEditorCommands()
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::Get().LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	const TSharedRef<FUICommandList> CommandList = LevelEditorModule.GetGlobalLevelEditorActions();

	CommandList->MapAction
	(
		FDMXControlConsoleEditorCommands::Get().OpenControlConsole,
		FExecuteAction::CreateStatic(&FDMXControlConsoleEditorModule::OpenControlConsole)
	);
}

void FDMXControlConsoleEditorModule::RegisterDMXMenuExtender()
{
	FDMXEditorModule& DMXEditorModule = FModuleManager::GetModuleChecked<FDMXEditorModule>(TEXT("DMXEditor"));
	const TSharedPtr<FExtender> LevelEditorToolbarDMXMenuExtender = DMXEditorModule.GetLevelEditorToolbarDMXMenuExtender();
	
	FLevelEditorModule& LevelEditorModule = FModuleManager::Get().LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedRef<FUICommandList> CommandList = LevelEditorModule.GetGlobalLevelEditorActions();

	LevelEditorToolbarDMXMenuExtender->AddMenuExtension("ConflictMonitor", EExtensionHook::Position::After, CommandList, FMenuExtensionDelegate::CreateStatic(&FDMXControlConsoleEditorModule::ExtendDMXMenu));
}

void FDMXControlConsoleEditorModule::ExtendDMXMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(FDMXControlConsoleEditorCommands::Get().OpenControlConsole,
		NAME_None,
		LOCTEXT("DMXControlConsoleMenuLabel", "Control Console"),
		LOCTEXT("DMXControlConsoleMenuTooltip", "Opens a control console asset that can send DMX locally or over the network"),
		FSlateIcon(FDMXControlConsoleEditorStyle::Get().GetStyleSetName(), "DMXControlConsole.TabIcon")
	);
}

void FDMXControlConsoleEditorModule::RegisterCompactEditorTabSpawner()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		CompactEditorTabId,
		FOnSpawnTab::CreateStatic(&FDMXControlConsoleEditorModule::OnSpawnCompactEditorTab))
		.SetMenuType(ETabSpawnerMenuType::Hidden)
		.SetIcon(FSlateIcon(FDMXControlConsoleEditorStyle::Get().GetStyleSetName(), "DMXControlConsole.TabIcon"));
}

TSharedRef<SDockTab> FDMXControlConsoleEditorModule::OnSpawnCompactEditorTab(const FSpawnTabArgs& InSpawnTabArgs)
{
	using namespace UE::DMX::Private;

	FDMXControlConsoleEditorModule& ThisModule = FModuleManager::GetModuleChecked<FDMXControlConsoleEditorModule>(TEXT("DMXControlConsoleEditor"));
	ThisModule.CompactEditorTab = SNew(SDockTab)
		.Label(LOCTEXT("CompactEditorTabTitle", "Compact DMX Control Console"))
		.TabRole(ETabRole::NomadTab)
		.OnTabClosed(SDockTab::FOnTabClosedCallback::CreateStatic(&FDMXControlConsoleEditorModule::OnCompactEditorTabClosed))
		[
			SNew(SDMXControlConsoleCompactEditorView)
		];

	return ThisModule.CompactEditorTab.ToSharedRef();
}

void FDMXControlConsoleEditorModule::OnCompactEditorTabClosed(TSharedRef<SDockTab> Tab)
{
	UDMXControlConsoleCompactEditorModel* CompactEditorModel = GetMutableDefault<UDMXControlConsoleCompactEditorModel>();
	CompactEditorModel->StopPlayingDMX();

	FDMXControlConsoleEditorModule& ThisModule = FModuleManager::GetModuleChecked<FDMXControlConsoleEditorModule>(TEXT("DMXControlConsoleEditor"));
	ThisModule.CompactEditorTab.Reset();
}

void FDMXControlConsoleEditorModule::OnPostEnginInit()
{
	RegisterDMXMenuExtender();

	// Try UpgradePath if configurations settings have data from Output Consoles, the Console that was used before 5.2.
	FDMXControlConsoleEditorFromLegacyUpgradeHandler::TryUpgradePathFromLegacy();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDMXControlConsoleEditorModule, DMXControlConsoleEditor)
