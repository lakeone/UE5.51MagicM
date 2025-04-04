// Copyright Epic Games, Inc. All Rights Reserved.

#include "LiveLinkHubMainTabController.h"

#include "UI/Window/LiveLinkHubWindowController.h"
#include "SLiveLinkHubMainTabView.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "LiveLinkHubUI"

static const FName LiveLinkHubTabID = "LiveLinkHubMainTab";

void FLiveLinkHubMainTabController::Init(const FLiveLinkHubComponentInitParams& Params)
{
	FGlobalTabmanager::Get()->RegisterTabSpawner(LiveLinkHubTabID, FOnSpawnTab::CreateRaw(this, &FLiveLinkHubMainTabController::SpawnMainTab, Params.Window))
		.SetDisplayName(LOCTEXT("LiveLinkHubTabLabel", "Live Link Hub"))
		.SetIcon(FSlateIcon("LiveLinkStyle", "LiveLinkHub.Icon.Small"))
		.SetTooltipText(LOCTEXT("MainTabTooltipText", "Live Link Hub Main Tab"));
	
	Params.MainStack->AddTab(LiveLinkHubTabID, ETabState::OpenedTab);
}

void FLiveLinkHubMainTabController::OpenTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(LiveLinkHubTabID);
}

TSharedRef<SDockTab> FLiveLinkHubMainTabController::SpawnMainTab(const FSpawnTabArgs& Args, TSharedRef<SWindow> RootWindow)
{
	const TSharedRef<SDockTab> DockTab = SNew(SDockTab)
		.Label(LOCTEXT("LiveLinkHubTitle", "Live Link Hub"))
		.TabRole(MajorTab)
		.OnCanCloseTab_Lambda([]()
		{
			return false;
		})
		.CanEverClose(false);

	DockTab->SetContent(
		SAssignNew(MainTabView, SLiveLinkHubMainTabView)
			.ConstructUnderMajorTab(DockTab)
			.ConstructUnderWindow(RootWindow)
	);

	return DockTab;
}

#undef LOCTEXT_NAMESPACE
